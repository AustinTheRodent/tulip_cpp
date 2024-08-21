#!/bin/python3

import os
import sys

f = open("/home/ubuntu/startup_scripts/data/current_output_volume_db.txt", "r")
cur_vol = int(f.readline())
f.close()

os.system("/home/ubuntu/cpp_workspace/wm8960_codec_set_output_volume/other/set_volume.py -1000")
os.system("/home/ubuntu/cpp_workspace/global_dsp_enable/other/bypass_dsp.sh")

if len(sys.argv) > 1:
  os.system("sudo /home/ubuntu/cpp_workspace/tuner/bin/main.exe -dd")
else:
  os.system("sudo /home/ubuntu/cpp_workspace/tuner/bin/main.exe")

os.system("/home/ubuntu/cpp_workspace/global_dsp_enable/other/enable_dsp.sh")
os.system("/home/ubuntu/cpp_workspace/wm8960_codec_set_output_volume/other/set_volume.py %i" % cur_vol)
