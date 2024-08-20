
#include <string>
#include <string.h>
#include <stdio.h>
#include "tulip_dsp.h"
#include "KR260_TULIP_REGISTERS.h"

#include <iostream>
#include <fstream>
using namespace std;

int main (int argc, char *argv[])
{

  ifstream fir_profile;
  uint32_t fir_profile_len = 0;

  bool fir_defined = false;
  string fir_fname = "";

  bool bypass = false;

  for(int i = 0 ; i < argc ; i++)
  {
    if (strcmp(argv[i], "-fir") == 0)
    {
      fir_defined = true;
      fir_fname = argv[i+1];
    }
    else if (strcmp(argv[i], "-bypass") == 0)
    {
      bypass = true;
    }
  }

  if (bypass == true)
  {
    // call constructor:
    cout << "bypass User FIR\n";
    tulip_dsp dsp_module;

    dsp_module.write_tulip_reg_set_bits(TULIP_DSP_CONTROL, TULIP_DSP_CONTROL_BYPASS_USR_FIR);

    return 0;
  }

  if (fir_defined == false)
  {
    cerr << "FIR file name undefined\n";
    return 0;
  }

  //////////////////////////////////////////////////////////////////////////
  // Read FIR Inputs from File
  //////////////////////////////////////////////////////////////////////////

  fir_profile.open(fir_fname);

  if (!fir_profile)
  {
      // Print an error and exit
      cerr << fir_fname << " could not be opened for reading!\n";
      return 1;
  }

  while (fir_profile)
  {
    string strInput;
    fir_profile >> strInput;
    if (strInput.size() > 0)
    {
      fir_profile_len++;
    }
  }

  fir_profile.clear();
  fir_profile.seekg(0);
  vector<int32_t> fir_taps(fir_profile_len);

  for (int i = 0 ; i < fir_profile_len ; i++)
  {
    string strInput;
    fir_profile >> strInput;
    fir_taps[i] = (int32_t)stoi(strInput);
  }

  fir_profile.close();



  // call constructor:
  tulip_dsp dsp_module;

  dsp_module.write_tulip_reg_set_bits(CONTROL, CONTROL_DSP_ENABLE);
  dsp_module.write_tulip_reg_clear_bits(TULIP_DSP_CONTROL, TULIP_DSP_CONTROL_BYPASS);

  dsp_module.write_tulip_reg_clear_bits(TULIP_DSP_CONTROL, TULIP_DSP_CONTROL_SW_RESETN_USR_FIR);
  dsp_module.write_tulip_reg_set_bits(TULIP_DSP_CONTROL, TULIP_DSP_CONTROL_SW_RESETN_USR_FIR);
  dsp_module.write_tulip_reg_clear_bits(TULIP_DSP_CONTROL, TULIP_DSP_CONTROL_BYPASS_USR_FIR);

  dsp_module.delay_ms(1);

  if (dsp_module.program_FIR_filter(fir_taps) == false)
  {
    cerr << "Issue with FIR program interface\n";
    dsp_module.write_tulip_reg_clear_bits(TULIP_DSP_CONTROL, TULIP_DSP_CONTROL_SW_RESETN_USR_FIR);
    dsp_module.write_tulip_reg_set_bits(TULIP_DSP_CONTROL, TULIP_DSP_CONTROL_BYPASS_USR_FIR);
    return 0;
  }

  cout << "Tulip User FIR Programming Finished\n";

  return 1;
}

