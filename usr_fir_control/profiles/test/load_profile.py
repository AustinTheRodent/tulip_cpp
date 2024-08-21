#!/bin/python3

import os
import sys

if __name__ == "__main__":
  script_directory = os.path.dirname(os.path.abspath(sys.argv[0])) + "/"
  usr_fir_exe = "/home/ubuntu/cpp_workspace/usr_fir_control/bin/main.exe"

  import scipy
  import numpy as np

  numtaps = 129
  bands = np.array([0, 0.1, 0.15, 1.0], dtype=float)
  desired = np.array([1, 0], dtype=float)

  fir_taps = scipy.signal.remez(\
    numtaps, \
    bands, \
    desired,
    fs=2)

  test_tap_f = open(script_directory+"fir_taps.txt", "w")
  for tap in fir_taps:
    test_tap_f.write("%i\n" % int(tap*2**15))
  test_tap_f.close()

  command = ""
  command += "sudo %s " % usr_fir_exe
  command += "-fir " + script_directory + "fir_taps.txt"

  os.system(command)
