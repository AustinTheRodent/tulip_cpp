#!/bin/python3

import os
import sys
import numpy as np

feedforward_gain = 0.25
feedback_gain = 0.1

script_dir = os.path.dirname(os.path.realpath(sys.argv[0]))
reverb_dir = script_dir[0:script_dir.rfind("reverb_control")] + "reverb_control"

command = \
  "sudo " + reverb_dir + "/bin/main.exe " + \
  "-fir " +  script_dir + "/fir_profile.txt " + \
  "-ff_gain " + str(feedforward_gain) + " " + \
  "-fb_gain " + str(feedback_gain)

os.system(command)
