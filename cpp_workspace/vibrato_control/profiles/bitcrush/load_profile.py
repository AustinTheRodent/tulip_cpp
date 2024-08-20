#!/bin/python3

import os
import sys

if __name__ == "__main__":

  script_directory = os.path.dirname(os.path.abspath(sys.argv[0])) + "/"
  vibrato_exe = "/home/ubuntu/cpp_workspace/vibrato_control/bin/main.exe"

  DWIDTH = 24
  CHIRP_PROG_DWIDTH = 32
  samp_rate = 48000.0 # [Hz]
  fs = samp_rate

  gain_array = [
    0.0,
    3.0
  ]

  prog_chirp_depth_din_array_hz = [ # depth per side
    1000.0
  ]

  prog_freq_deriv_din_array_2pi_radians_ps = [
    2000
  ]

  prog_freq_offset_din_array_hz = [
    0
  ]

  gain_f = open(script_directory+"gain.txt", "w")
  for gain in gain_array:
    gain_f.write("%i\n" % (gain*2**(DWIDTH-2)))
  gain_f.close()

  chirp_depth_f = open(script_directory+"chirp_depth.txt", "w")
  for prog_chirp_depth in prog_chirp_depth_din_array_hz:
    chirp_depth_norm = prog_chirp_depth/(fs/2.0)
    chirp_depth_f.write("%i\n" % (chirp_depth_norm*2**(CHIRP_PROG_DWIDTH)))
  chirp_depth_f.close()

  freq_deriv_f = open(script_directory+"freq_deriv.txt", "w")
  for prog_freq_deriv in prog_freq_deriv_din_array_2pi_radians_ps:
    freq_deriv_norm = prog_freq_deriv/(fs/2.0)
    freq_deriv_f.write("%i\n" % (freq_deriv_norm*2**(CHIRP_PROG_DWIDTH-1)))
  freq_deriv_f.close()

  freq_offset_f = open(script_directory+"freq_offset.txt", "w")
  for prog_freq_offset in prog_freq_offset_din_array_hz:
    freq_offset_norm = prog_freq_offset/(fs/2.0)
    freq_offset_f.write("%i\n" % (freq_offset_norm*2**(CHIRP_PROG_DWIDTH-1)))
  freq_offset_f.close()

  command = "sudo "+vibrato_exe + " " + \
    "-gain "+script_directory+"gain.txt " + \
    "-chirp_depth "+script_directory+"chirp_depth.txt " + \
    "-freq_deriv "+script_directory+"freq_deriv.txt " + \
    "-freq_offset "+script_directory+"freq_offset.txt "

  os.system(command)
