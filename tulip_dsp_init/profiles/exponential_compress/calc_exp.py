#!/bin/python3

import os
import sys
import numpy as np

script_directory = os.path.dirname(os.path.abspath(sys.argv[0])) + "/"

compress_factor = float(sys.argv[1])
print(compress_factor)

N = 1024.0
M = 1.0/1024.0

x = np.arange(0, N*M, M, dtype=float)
lut_din = 1.0 - np.exp(-compress_factor*x)
lut_din = np.round((2**24-1) * lut_din / np.max(lut_din))

f = open(script_directory + "lut_din.txt", "w")
for i in range(len(lut_din)):
  f.write("%i\n" % lut_din[i])
f.close()
