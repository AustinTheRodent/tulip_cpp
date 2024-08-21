#!/bin/python3

import os
import sys

if __name__ == "__main__":

  script_directory = os.path.dirname(os.path.abspath(sys.argv[0])) + "/"
  chorus_exe = "/home/ubuntu/cpp_workspace/chorus_control/bin/main.exe"

  DWIDTH = 24
  LFO_PROG_DWIDTH = 32
  samp_rate = 48000.0 # [Hz]
  fs = samp_rate

  gain_array = [
    0.5,
    0.5
  ]

  lfo_depth = 100
  lfo_freq_hz = 0.2

  avg_delay = lfo_depth+1

  gain_f = open(script_directory+"gain.txt", "w")
  for gain in gain_array:
    gain_f.write("%i\n" % (gain*2**(DWIDTH-2)))
  gain_f.close()

  lfo_freq_norm = float(lfo_freq_hz)/(fs/2.0)
  lfo_freq = lfo_freq_norm*2**(LFO_PROG_DWIDTH-1)

  print("avg_delay: %X" % int(avg_delay))
  print("lfo_depth: %X" % int(lfo_depth))
  print("lfo_freq: %X" % int(lfo_freq))

  command = "sudo "+chorus_exe + " " + \
    "-gain "+script_directory+"gain.txt " + \
    "-avg_delay %i -lfo_depth %i -lfo_freq %i" % (int(avg_delay), int(lfo_depth), int(lfo_freq))

  os.system(command)
