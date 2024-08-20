#!/bin/python3

import sys
import os

script_path = os.path.abspath(sys.argv[0])
script_path = script_path[0:script_path.rfind("/")]

if len(sys.argv) < 2:
  print("no volume given, provide volume in dB")
  quit()

vol = float(sys.argv[1])

if vol > 30:
  print("Action aborted: maximum allowed volume is +30dB")
  quit()

if vol < -17.25:
  print("Action aborted: minimum allowed volume is -17.25dB")
  quit()


command = "sudo " + script_path + "/../bin/main.exe -i " + sys.argv[1]

os.system(command)
