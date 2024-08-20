#!/bin/python3

import os
import time

if __name__ == "__main__":
  os.system("sudo insmod /lib/modules/5.15.0-1025-xilinx-zynqmp/build/udmabuf-master/u-dma-buf.ko udmabuf0=8388608 udmabuf1=8388608")
  os.system("sudo /home/ubuntu/cpp_workspace/dma_test/bin/main.exe")
  os.system("sudo rmmod u-dma-buf")

