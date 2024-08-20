#!/bin/python3

import os
import sys

if __name__ == "__main__":
  script_directory = os.path.dirname(os.path.abspath(sys.argv[0])) + "/"
  overdrive_exe = "/home/ubuntu/cpp_workspace/overdrive_control/bin/main.exe"

  input_gain_lin = 1.0
  output_gain_lin = 0.25

  if len(sys.argv) < 2:
    print("compress factor undefined")
    quit()

  import numpy as np

  compress_factor = float(sys.argv[1])

  N = 1024.0
  M = 1.0/1024.0

  x = np.arange(0, N*M, M, dtype=float)
  lut_din = x**(1.0/compress_factor)
  lut_din = np.round((2**24-1) * lut_din / np.max(lut_din))

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
