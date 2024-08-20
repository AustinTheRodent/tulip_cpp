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
  bands = np.array([0, f_stop0, f_pass0, f_pass1, f_stop1, float(fs)/2.0], dtype=float)
  desired = np.array([0, 1, 0], dtype=float)
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

  fpass_max = None
  fpass_min = None
  fstop0_max = None
  fstop1_max = None

  for i in range(len(w)):
    if w[i] >= f_pass0 and w[i] <= f_pass1:
      if fpass_max == None:
        fpass_max = freq_resp[i]
      elif freq_resp[i] > fpass_max:
        fpass_max = freq_resp[i]

      if fpass_min == None:
        fpass_min = freq_resp[i]
      elif freq_resp[i] < fpass_min:
        fpass_min = freq_resp[i]

    if w[i] <= f_stop0:
      if fstop0_max == None:
        fstop0_max = freq_resp[i]
      elif freq_resp[i] > fstop0_max:
        fstop0_max = freq_resp[i]

    if w[i] >= f_stop1:
      if fstop1_max == None:
        fstop1_max = freq_resp[i]
      elif freq_resp[i] > fstop1_max:
        fstop1_max = freq_resp[i]

print("Stopband 0 Attenuation: %f dB" % fstop0_max)
print("Passband Ripple: %f dB" % (fpass_max-fpass_min))
print("Stopband 1 Attenuation: %f dB" % fstop1_max)
