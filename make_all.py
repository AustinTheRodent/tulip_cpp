#!/bin/python3

import os
import sys

script_dir = os.path.dirname(os.path.realpath(sys.argv[0]))
os.chdir(script_dir)

directory_list = []
for item in os.listdir():
  if os.path.isdir(item):
    directory_list.append(item)

onlyclean = False
for arg in sys.argv:
  if arg == "clean":
    onlyclean = True
  if arg == "-clean":
    onlyclean = True

for directory in directory_list:
  os.chdir(script_dir+"/"+directory)
  for item in os.listdir(script_dir+"/"+directory):
    if item == "Makefile":
      print(script_dir+"/"+directory+":")
      os.system("make clean")
      if onlyclean == False:
        os.system("make")
      print("")
      break
