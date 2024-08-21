#!/bin/python3

import os
import sys

if __name__ == "__main__":
  script_directory = os.path.dirname(os.path.abspath(sys.argv[0])) + "/"
  usr_fir_exe = "/home/ubuntu/cpp_workspace/usr_fir_control/bin/main.exe"

  f_pass = None
  f_stop = None

  import scipy
  import numpy as np

  fs = 48000

  numtaps = 129
  #bands = np.array([0, f_pass, f_stop, float(fs)/2.0], dtype=float)
  #desired = np.array([1, 0], dtype=float)

  freq_resolution = 4096
  FR = freq_resolution

  freq = np.arange(0, FR+1, 1, dtype=float)*(float(fs)/2.0)/float(FR)
  gain = np.zeros(FR+1, dtype=float)
  for i in range(len(freq)):
    if freq[i] < 50:
      gain[i] = 0.0
    elif freq[i] < 100:
      gain[i] = 0.0
    elif freq[i] < 2000:
      gain[i] = 1.0
    elif freq[i] < 5000:
      gain[i] = 1.0/(1.0+(float(freq[i]-2000)/100.0))
    else:
      gain[i] = 0.0

  fir_taps = scipy.signal.firwin2(\
    numtaps, \
    freq, \
    gain, \
    fs=fs)

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
