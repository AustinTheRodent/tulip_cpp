#!/bin/python3

import os
import sys

if __name__ == "__main__":
  script_directory = os.path.dirname(os.path.abspath(sys.argv[0])) + "/"
  overdrive_exe = "/home/ubuntu/cpp_workspace/overdrive_control/bin/main.exe"

  input_gain_lin = 1.0
  output_gain_lin = 1.0
  compress_factor = 1.0
  start_location = 0.0
  end_location = 1.0

  for i in range(len(sys.argv)):
    if sys.argv[i] == "-i":
      input_gain_lin = float(sys.argv[i+1])
    elif sys.argv[i] == "-o":
      output_gain_lin = float(sys.argv[i+1])
    elif sys.argv[i] == "-c":
      compress_factor = float(sys.argv[i+1])
    elif sys.argv[i] == "-s":
      start_location = float(sys.argv[i+1])
    elif sys.argv[i] == "-e":
      end_location = float(sys.argv[i+1])

  import numpy as np

  N = 1024.0
  M = (end_location-start_location)/1024.0

  x = np.arange(start_location, end_location, M, dtype=float)
  lut_din = np.sin(x*(np.pi/2.0))
  lut_din = ((lut_din-lut_din[0]) / (lut_din[int(N)-1]-lut_din[0])) * ((N-1.0)/N)

  if compress_factor != 1:
    lut_din = lut_din**compress_factor

  lut_din = np.round(lut_din*(2**24-1))

  f = open(script_directory + "lut_din.txt", "w")
  for i in range(len(lut_din)):
    f.write("%i\n" % lut_din[i])
  f.close()

  command = ""
  command += "sudo %s " % overdrive_exe
  command += "-lut " + script_directory + "lut_din.txt "
  command += "-gi_lin %f " % input_gain_lin
  command += "-go_lin %f " % output_gain_lin

  os.system(command)
