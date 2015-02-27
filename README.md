# In Bursts

This is a little C program that allows for getting incoming bandwidth data at millisecond granularities. It will create an output CSV file (defaulted to `inbursts.csv`) with the following data:

```
timestamp in ms,incoming bytes,incoming packets
```

> It will skip any millisecond timeslots that did not receive any packets.

## Quick Start

### libpcap

You may need to download the `libpcap` library:

* For `apt-get` based distributions use:

```
sudo apt-get install libpcap-dev
```

* For `rpm` based distributions use:

```
sudo yum install libpcap-devel
```

### Compile and Run

```
make
sudo ./inbursts
```

You can see the options given by typing `./inbursts -h`:

```
usage: ./inbursts [-h] [-i INTERFACE] [-c COUNT] [-o OUTPUT]
  -h              : Print help and exit.
  -i              : Interface to collect otherwise defaults to first non-loopback interface.
  -c              : Number of packets to collect otherwise collects forever.
  -o=inbursts.out : Output file for data.
```
