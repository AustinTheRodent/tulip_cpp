
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

  bool fir_defined = false;
  ifstream fir_profile;
  uint32_t fir_profile_len = 0;
  string fir_fname = "";

  bool bypass = false;

  float feedforward_gain = 1.0;
  float feedback_gain = 1.0;
  uint8_t feedback_rs = 16;

  for(int i = 0 ; i < argc ; i++)
  {
    if (strcmp(argv[i], "-fir") == 0)
    {
      fir_defined = true;
      fir_fname = argv[i+1];
    }
    else if (strcmp(argv[i], "-ff_gain") == 0)
    {
      feedforward_gain = stof(argv[i+1]);
    }
    else if (strcmp(argv[i], "-fb_gain") == 0)
    {
      feedback_gain = stof(argv[i+1]);
    }
    else if (strcmp(argv[i], "-fb_rs") == 0)
    {
      feedback_rs = stoi(argv[i+1]);
    }
    else if (strcmp(argv[i], "-bypass") == 0)
    {
      bypass = true;
    }
  }

  if (bypass == true)
  {
    // call constructor:
    cout << "Bypass Reverb\n";
    tulip_dsp dsp_module;

    dsp_module.write_tulip_reg_set_bits(TULIP_DSP_CONTROL, TULIP_DSP_CONTROL_BYPASS_REVERB);

    return 0;
  }

  if ((feedforward_gain >= 2.0) || (feedforward_gain < 0.0))
  {
    cerr << "Reverb feedforward gain out of bounds, values allowed: (2.0,0.0]\n";
  }

  if ((feedback_gain >= 2.0) || (feedback_gain < 0.0))
  {
    cerr << "Reverb feedback gain out of bounds, values allowed: (2.0,0.0]\n";
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

  cout << "fir_profile_len: " << fir_profile_len << "\n";

  // call constructor:
  tulip_dsp dsp_module;

  dsp_module.write_tulip_reg_set_bits(CONTROL, CONTROL_DSP_ENABLE);
  dsp_module.write_tulip_reg_clear_bits(TULIP_DSP_CONTROL, TULIP_DSP_CONTROL_BYPASS);

  dsp_module.write_tulip_reg_clear_bits(TULIP_DSP_CONTROL, TULIP_DSP_CONTROL_SW_RESETN_REVERB);
  dsp_module.write_tulip_reg_set_bits(TULIP_DSP_CONTROL, TULIP_DSP_CONTROL_SW_RESETN_REVERB);
  dsp_module.write_tulip_reg_clear_bits(TULIP_DSP_CONTROL, TULIP_DSP_CONTROL_BYPASS_REVERB);

  dsp_module.delay_ms(1);

  dsp_module.program_reverb_feedforward_gain_linear(feedforward_gain);
  dsp_module.program_reverb_feedback_gain_linear(feedback_gain);
  dsp_module.program_reverb_feedback_right_shift(feedback_rs);

  if (dsp_module.program_reverb_delay_profile(fir_taps) == false)
  {
    cerr << "issue with Reverb FIR program interface\n";
    dsp_module.write_tulip_reg_clear_bits(TULIP_DSP_CONTROL, TULIP_DSP_CONTROL_SW_RESETN_REVERB);
    dsp_module.write_tulip_reg_set_bits(TULIP_DSP_CONTROL, TULIP_DSP_CONTROL_BYPASS_REVERB);
    return 0;
  }

  cout << "Reverb Programming Done\n";

  return 1;
}

