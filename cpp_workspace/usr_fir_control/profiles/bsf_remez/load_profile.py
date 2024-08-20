#!/bin/python3

import os
import sys

if __name__ == "__main__":
  script_directory = os.path.dirname(os.path.abspath(sys.argv[0])) + "/"
  usr_fir_exe = "/home/ubuntu/cpp_workspace/usr_fir_control/bin/main.exe"

  f_pass0 = None
  f_stop0 = None
  f_pass1 = None
  f_stop1 = None
  weight = [1, 1, 1]

  for i in range(len(sys.argv)):
    if sys.argv[i] == "-fpass0":
      f_pass0 = float(sys.argv[i+1])
    elif sys.argv[i] == "-fstop0":
      f_stop0 = float(sys.argv[i+1])
    elif sys.argv[i] == "-fpass1":
      f_pass1 = float(sys.argv[i+1])
    elif sys.argv[i] == "-fstop1":
      f_stop1 = float(sys.argv[i+1])
    elif sys.argv[i] == "-w":
      weight[0] = float(sys.argv[i+1])
      weight[1] = float(sys.argv[i+2])
      weight[2] = float(sys.argv[i+3])

  if f_pass0 == None:
    print("Passband0 Undefined")
    quit()
  if f_stop0 == None:
    print("Stopband0 Undefined")
    quit()
  if f_pass1 == None:
    print("Passband1 Undefined")
    quit()
  if f_stop1 == None:
    print("Stopband1 Undefined")
    quit()

  import scipy
  import numpy as np

  fs = 48000

  numtaps = 129
  bands = np.array([0, f_pass0, f_stop0, f_stop1, f_pass1, float(fs)/2.0], dtype=float)
  desired = np.array([1, 0, 1], dtype=float)
  weight = np.array(weight, dtype=float)

  fir_taps = scipy.signal.remez(\
    numtaps, \
    bands, \
    desired, \
    weight=weight, \
    fs=fs)

  freq_resolution = 4096
  FR = freq_resolution

  tap_f = open(script_directory+"fir_taps.txt", "w")
  for tap in fir_taps:
    tap_f.write("%i\n" % int(tap*2**15))
  tap_f.close()

  command = ""
  command += "sudo %s " % usr_fir_exe
  command += "-fir " + script_directory + "fir_taps.txt"

  os.system(command)

  taps_zp = np.append(np.array(fir_taps), np.zeros(FR-numtaps))

  freq_resp = 20*np.log10(np.abs(np.array(scipy.fft.fft(taps_zp))))
  w = np.arange(0, 1, 2/FR) * float(fs)/2.0

  freq_f = open(script_directory+"freq_response.csv", "w")
  freq_f.write("Frequency (Hz), Magnitude (dB)\n")
  for i in range(int(FR/2)):
    freq_f.write("%f, %f\n" % (w[i], freq_resp[i]))
  freq_f.close()

  fpass0_max = None
  fpass0_min = None
  fpass1_max = None
  fpass1_min = None
  fstop_max = None

  for i in range(len(w)):
    if w[i] <= f_pass0:
      if fpass0_max == None:
        fpass0_max = freq_resp[i]
      elif freq_resp[i] > fpass0_max:
        fpass0_max = freq_resp[i]

      if fpass0_min == None:
        fpass0_min = freq_resp[i]
      elif freq_resp[i] < fpass0_min:
        fpass0_min = freq_resp[i]

    if w[i] >= f_pass1:
      if fpass1_max == None:
        fpass1_max = freq_resp[i]
      elif freq_resp[i] > fpass1_max:
        fpass1_max = freq_resp[i]

      if fpass1_min == None:
        fpass1_min = freq_resp[i]
      elif freq_resp[i] < fpass1_min:
        fpass1_min = freq_resp[i]

    if w[i] >= f_stop0 and w[i] <= f_stop1:
      if fstop_max == None:
        fstop_max = freq_resp[i]
      elif freq_resp[i] > fstop_max:
        fstop_max = freq_resp[i]

print("Passband 0 Ripple: %f dB" % (fpass0_max-fpass0_min))
print("Stopband Attenuation: %f dB" % fstop_max)
print("Passband 1 Ripple: %f dB" % (fpass1_max-fpass1_min))
