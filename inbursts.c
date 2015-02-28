#include <errno.h>
#include <getopt.h>
#include <pcap.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netinet/if_ether.h>

#include "inbursts.h"

static const char usage[] =
  "usage: %s [-h] [-i INTERFACE] [-c COUNT] [-o OUTPUT]\n"
  "  -h              : Print help and exit.\n"
  "  -i              : Interface to collect otherwise defaults to first non-loopback interface.\n"
  "  -c              : Number of packets to collect otherwise collects forever.\n"
  "  -o=inbursts.out : Output file for data.\n"
  ;

/* Handlers */
static pcap_t* handle;
static struct bpf_program fp;      /* Compiled filter expression */
static char ip_addr[16];
static FILE *fd;

/* Data */
static struct timeval in_tv;
static uint64_t bytes_in = 0;
static uint64_t pkts_in = 0;
static uint64_t total_packets = 0;
static uint64_t total_pkts_in = 0;
static uint64_t max_bytes_in = 0;
static uint64_t max_pkts_in = 0;

/**
 * record_data
 * Records the data to the file.
 */
void record_data(void)
{
  if (total_pkts_in > 0) {  /* Do not need to record if there wasn't any data. */
    max_bytes_in = max(max_bytes_in, bytes_in);
    max_pkts_in  = max(max_pkts_in, pkts_in);
    fprintf(fd, "%ld%03d,%llu,%llu\n", in_tv.tv_sec, in_tv.tv_usec / 1000, bytes_in, pkts_in);
  }
}

/**
 * cleanup
 * This function handles any kernel signals. We mainly want it to clean up gracefully and then output some statistics.
 */
void cleanup(int signo)
{
  record_data();

  fprintf(stderr, "\nCapture completed\n");
  fprintf(stderr, "Captured %llu packets and %llu incoming packets\n", total_packets, total_pkts_in);
  fprintf(stderr, "Maxed at %llu bytes/ms in and %llu packets/ms in\n", max_bytes_in, max_pkts_in);

  /* Clean up state and exit */
  pcap_freecode(&fp);
  pcap_close(handle);
  fclose(fd);
  fprintf(stderr, "Exiting\n");
  exit(0);
}

/**
 * processor
 * This function is the callback function for pcap_loop to process packets. It will sum up packets and bytes during a
 * millisecond interval and output the following:
 *
 *   timestamp (milliseconds),bytes_in,packets_in
 *
 */
void processor(u_char *args, const struct pcap_pkthdr* header, const u_char* packet)
{
  const struct ethernet_hdr *ethernet;
  const struct ip_hdr *ip;

  ethernet = (struct ethernet_hdr*) (packet);
  ip = (struct ip_hdr*) (packet + SIZE_ETHERNET);
  if (IP_HL(ip) * 4 < 20) {
    /* Invalid IP Header length */
    return;
  }

  if (strcmp(ip_addr, inet_ntoa(ip->ip_dst)) == 0) {  /* Incoming traffic */
    if ((in_tv.tv_sec == header->ts.tv_sec) && ((in_tv.tv_usec / 1000) == (header->ts.tv_usec / 1000))) {
      bytes_in += header->len;
      pkts_in  += 1;
    } else {
      if (in_tv.tv_sec != 0) {
        record_data();
      }
      bytes_in = header->len;
      pkts_in  = 1;
      in_tv.tv_sec  = header->ts.tv_sec;
      in_tv.tv_usec = header->ts.tv_usec;
    }
    total_pkts_in += 1;
  }
  total_packets += 1;
}

/**
 * In Bursts
 * This program will output the network statistics as follows and helps output data to
 * locate bursty traffic.
 *
 *   timestamp (milliseconds),bytes_in,packets_in
 *
 * This program was built using documentation: http://www.tcpdump.org/pcap.html
 */
int main(int argc, char *argv[])
{
  char *dev = NULL, errbuf[PCAP_ERRBUF_SIZE];
  bpf_u_int32 net;            /* IP of our sniffing device */
  bpf_u_int32 mask;           /* Subnet mask */

  pcap_if_t *alldevs, *d;
  pcap_addr_t *a;
  int status;
  bool found = false;

  char filter_exp[] = "ip";   /* Filter expression */
  char *output_file  = "inbursts.out";

  int opt;
  int count = -1;

  while ((opt = getopt(argc, argv, "hi:c:o:")) != -1) {
    switch (opt) {
      case 'h':
        fprintf(stderr, usage, argv[0]);
        return(2);
      case 'i':
        dev = optarg;
        break;
      case 'c':
        count = atoi(optarg);
        break;
      case 'o':
        output_file = optarg;
        break;
      default:
        fprintf(stderr, usage, argv[0]);
        return(2);
      }
  }

  if (dev == NULL) {
    /* No device provided so will look up. */
    dev = pcap_lookupdev(errbuf);
    if (dev == NULL) {
      fprintf(stderr, "Couldn't find default device: %s\n", errbuf);
      return(2);
    }
  }

  pcap_lookupnet(dev, &net, &mask, errbuf);

  /* Look up the IP Address associated with the device */
  status = pcap_findalldevs(&alldevs, errbuf);
  if (status != 0) {
    fprintf(stderr, "Couldn't get devices: %s\n", errbuf);
    return(2);
  }

  for (d = alldevs; d != NULL; d = d->next) {
    if (strcmp(d->name, dev) == 0) {
      for (a = d->addresses; a != NULL; a = a->next) {
        if (a->addr->sa_family == AF_INET) {
          strcpy(ip_addr, inet_ntoa(((struct sockaddr_in*) a->addr)->sin_addr));
          found = true;
          break;
        }
      }

      if (found) {
        break;
      }
    }
  }

  pcap_freealldevs(alldevs);

  if (ip_addr == NULL) {
    fprintf(stderr, "Couldn't obtain interface IP Address\n");
    return(2);
  }

  /* Open the device for sniffing with a 0 timeout to wait for buffered packets */
  handle = pcap_open_live(dev, 100, 1, 0, errbuf);
  if (handle == NULL) {
    fprintf(stderr, "Couldn't open device %s: %s\n", dev, errbuf);
    return(2);
  }

  /* Check if Ethernet headers supported */
  if (pcap_datalink(handle) != DLT_EN10MB) {
    fprintf(stderr, "Device %s doesn't provide Ethernet headers - not supported\n", dev);
    return(2);
  }

  /* Compiled filter expression for IP Packets */
  if (pcap_compile(handle, &fp, filter_exp, 0, net) == -1) {
    fprintf(stderr, "Couldn't parse filter %s: %s\n", filter_exp, pcap_geterr(handle));
    return(2);
  }

  /* Apply the filter expression */
  if (pcap_setfilter(handle, &fp) == -1) {
    fprintf(stderr, "Couldn't install filter %s: %s\n", filter_exp, pcap_geterr(handle));
    return(2);
  }

  /* Add signal handler */
  if (signal(SIGINT, cleanup) == SIG_ERR) {
    fprintf(stderr, "Couldn't set up SIGINT signal handlers\n");
    return(2);
  }

  if (signal(SIGTERM, cleanup) == SIG_ERR) {
    fprintf(stderr, "Couldn't set up SIGTERM signal handlers\n");
    return(2);
  }

  /* Open file to write to */
  fd = fopen(output_file, "w");
  setbuf(fd, NULL);

  fprintf(stderr, "Starting to collect on interface %s\n", dev);
  fprintf(stderr, "Outputting data to %s\n", output_file);
  /* Loop to get the packets with the timeout */
  pcap_loop(handle, count, processor, NULL);
  cleanup(0);
  return(0);
}
