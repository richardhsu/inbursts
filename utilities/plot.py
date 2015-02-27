#!/usr/bin/env python

import argparse
import matplotlib as mpl
mpl.use('Agg')
import matplotlib.pyplot as plt

def plot(x, y, title, xlabel, ylabel, filename, color='b-'):
  """
  Plots the data
  :param x: The list of x values to graph.
  :param y: The list of y values to graph.
  :param title: The title of the graph.
  :param xlabel: The x-axis label for the graph.
  :param ylabel: The y-axis label for the graph.
  :param filename: The filename you want to save the graph to.
  :param color: The color of the line graph which defaults to a blue line.
  """
  fig = plt.figure()
  ax = fig.add_subplot(111)
  ax.plot(x, y, color)
  ax.set_title(title)
  ax.set_xlabel(xlabel)
  ax.set_ylabel(ylabel)
  ax.set_ylim(bottom=0)
  fig.savefig(filename)

def keep(timestamps, kbytes_in, pkts_in, *tests):
  """
  Keeps values that satisfy all tests. If any test fails, then the value is rejected from all lists.
  :param timestamps: List of timestamps to be used and processed from start time.
  :param kbytes_in: List of kilobytes in corresponding to timestamps.
  :param pkts_in: List of packets in corresponding to timestamps.
  :param *tests: Multiple functions for tests on whether to keep values.
  :returns: New (timestamps, kbytes_in, pkts_in)
  """
  new_timestamps = []
  new_kbytes_in = []
  new_pkts_in = []

  for (t, b, p) in zip(timestamps, kbytes_in, pkts_in):
    is_good = True
    for test in tests:
      if not test(t):
        is_good = False
        break

    if is_good:
      new_timestamps.append(t)
      new_kbytes_in.append(b)
      new_pkts_in.append(p)

  return (new_timestamps, new_kbytes_in, new_pkts_in)

def run(args):
  """
  Runs the script to collect the data from input file and outputs graphs.
  :param args: The argument list from ArgParser.
  """
  timestamps = []
  kbytes_in  = []
  pkts_in    = []

  start_timestamp = None

  with open(args.input, 'r') as FD:
    for line in FD:
      vals = line.split(",")
      if len(vals) != 3:
        continue

      curr_timestamp = int(vals[0])
      if start_timestamp is None:
        start_timestamp = curr_timestamp

      timestamps.append(curr_timestamp - start_timestamp)
      kbytes_in.append(int(vals[1]) / 1024.0)  # Transform from bytes to kilobytes
      pkts_in.append(int(vals[2]))

  plot(timestamps, kbytes_in, 'Incoming Bandwidth', 'Time Since Start (ms)', 'Bandwidth (kB/ms)', 'bandwidth-cropped.png')
  plot(timestamps, pkts_in, 'Incoming Packets', 'Time Since Start (ms)', 'Packets (pkts/ms)', 'packets-cropped.png')

  tests = []
  if args.min:
    tests.append(lambda x: x >= args.min)
  if args.max:
    tests.append(lambda x: x <= args.max)

  if len(tests) > 0:
    timestamps, kbytes_in, pkts_in = keep(timestamps, kbytes_in, pkts_in, *tests)

    plot(timestamps, kbytes_in, 'Incoming Bandwidth', 'Time Since Start (ms)', 'Bandwidth (kB/ms)', 'bandwidth-limited.png')
    plot(timestamps, pkts_in, 'Incoming Packets', 'Time Since Start (ms)', 'Packets (pkts/ms)', 'packets-limited.png')

if __name__ == "__main__":
  parser = argparse.ArgumentParser(description="Plot In Burst data! Full graph is always generated no matter min or max given.")
  parser.add_argument('--input', '-i', help='Input filename with CSV data.', default='inbursts.out')
  parser.add_argument('--min', type=int, help='Limit minimum elapsed timestamp to graph.')
  parser.add_argument('--max', type=int, help='Limit maximum elapsed timestamp to graph.')
  args = parser.parse_args()
  run(args)
