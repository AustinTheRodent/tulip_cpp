#!/bin/python3

import os
import sys
import numpy as np

if __name__ == "__main__":

  feedforward_gain = 1.0
  feedback_gain = 0.5

  script_dir = os.path.dirname(os.path.realpath(sys.argv[0]))
  reverb_dir = script_dir[0:script_dir.rfind("reverb_control")] + "reverb_control"

  fir_profile = open(script_dir + "/fir_profile.txt", "w")
  for i in range(2**16):
    if int(i) == int(48000/8):
      fir_profile.write("%i\n" % int(2**14))
    elif int(i) == int(2*48000/8):
      fir_profile.write("%i\n" % int(2**13))
    elif int(i) == int(3*48000/8):
      fir_profile.write("%i\n" % int(2**12))
    elif int(i) == int(4*48000/8):
      fir_profile.write("%i\n" % int(2**11))
    elif int(i) == int(5*48000/8):
      fir_profile.write("%i\n" % int(2**10))
    elif int(i) == int(6*48000/8):
      fir_profile.write("%i\n" % int(2**9))
    else:
      fir_profile.write("0\n")

  fir_profile.close()

  command = \
    "sudo " + reverb_dir + "/bin/main.exe " + \
    "-fir " +  script_dir + "/fir_profile.txt " + \
    "-ff_gain " + str(feedforward_gain) + " " + \
    "-fb_gain " + str(feedback_gain)

  os.system(command)
