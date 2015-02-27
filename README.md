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

## Plot Utility

There is a plot utility to plot the data outputted by the C program. These graphs will hopefully make the data more consumable to understand. The utility is at `utilities/plot.py`

### Requirements

1. You'll need some output from the C program!
2. You'll need to have matplotlib installed. You can find instructions [here][matplotlib].

### Plotting

The usage of the program is as follows:

```
usage: plot.py [-h] [--input INPUT] [--min MIN] [--max MAX]

Plot In Burst data!

optional arguments:
  -h, --help            show this help message and exit
  --input INPUT, -i INPUT
                        Input filename with CSV data.
  --min MIN             Limit minimum start timestamp to graph. (Full graph
                        still outputted)
  --max MAX             Limit maximum start timestamp to graph. (Full graph
                        still outputted)
```

If you've run the C program already and have an `inbursts.out` file (or whatever name you have used) then you can run the following in the root directory of this project:

```
python utilities/plot.py
```

### Output Graphs

There are four output graphs that can be produced:

| *Filename* | *Description* |
|:-----------|:--------------|
| `bandwidth-full.png` | This is the graph of all the bandwidth (kB/ms) data in the given CSV file. |
| `packets-full.png` | This is the graph of all the packet (pkts/ms) data in the given CSV file. |
| `bandwidth-cropped.png` | This is the cropped graph of the bandwidth (kB/ms) data depending on the `min` or `max` settings you gave. |
| `packets-cropped.png` | This is the cropped graph of the packet (pkts/ms) data depending on the `min` or `max` settings you gave. |

Please note that cropping is based on the elapsed time in milliseconds and the timestamps must satisfy both `min` and `max` given.

[matplotlib]: http://matplotlib.org/faq/installing_faq.html
