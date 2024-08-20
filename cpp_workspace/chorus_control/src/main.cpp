
#include <string>
#include <string.h>
#include <stdio.h>
#include "tulip_dsp.h"
#include "KR260_TULIP_REGISTERS.h"

#include <iostream>
#include <fstream>
using namespace std;

bool read_text_file_long_int(uint32_t expected_length, string file_name, vector<int64_t>& return_array);

int main (int argc, char *argv[])
{

  string gain_fname;
  vector<uint32_t> avg_delay(1);
  vector<uint32_t> lfo_depth(1);
  vector<int32_t> lfo_freq(1);
  bool gain_fname_defined = false;
  bool avg_delay_defined  = false;
  bool lfo_depth_defined  = false;
  bool lfo_freq_defined   = false;

  bool bypass = false;
  uint8_t return_code = 0;

  for(int i = 0 ; i < argc ; i++)
  {
    if (strcmp(argv[i], "-gain") == 0)
    {
      gain_fname = argv[i+1];
      gain_fname_defined = true;
    }
    else if (strcmp(argv[i], "-avg_delay") == 0)
    {
      avg_delay[0] = atoi(argv[i+1]);
      avg_delay_defined = true;
    }
    else if (strcmp(argv[i], "-lfo_depth") == 0)
    {
      lfo_depth[0] = atoi(argv[i+1]);
      lfo_depth_defined = true;
    }
    else if (strcmp(argv[i], "-lfo_freq") == 0)
    {
      lfo_freq[0] = atoi(argv[i+1]);
      lfo_freq_defined = true;
    }
    else if (strcmp(argv[i], "-bypass") == 0)
    {
      bypass = true;
    }
  }

  if (bypass == true)
  {
    // call constructor:
    cout << "Bypass Chorus\n";
    tulip_dsp dsp_module;

    dsp_module.write_tulip_reg_set_bits(TULIP_DSP_CONTROL, TULIP_DSP_CONTROL_BYPASS_CHORUS);

    return 0;
  }

  if (gain_fname_defined == false)
  {
    cerr << "gain file name undefined\n";
    return_code = 1;
  }

  if (avg_delay_defined == false)
  {
    cerr << "average delay undefined\n";
    return_code = 1;
  }

  if (lfo_depth_defined == false)
  {
    cerr << "LFO depth undefined\n";
    return_code = 1;
  }

  if (lfo_freq_defined == false)
  {
    cerr << "LFO frequency undefined\n";
    return_code = 1;
  }

  if (return_code > 0)
  {
    return return_code;
  }

  //////////////////////////////////////////////////////////////////////////
  // Read Inputs from Files
  //////////////////////////////////////////////////////////////////////////

  vector<int64_t> gain_array_long(2);
  vector<uint32_t> gain_array(2);

  if(read_text_file_long_int(2, gain_fname, gain_array_long) == false){return 1;};

  for (int i = 0 ; i < 2 ; i++)
  {
    gain_array[i] = (uint32_t)gain_array_long[i];
  }

  // call constructor:
  tulip_dsp dsp_module;

  dsp_module.write_tulip_reg_set_bits(CONTROL, CONTROL_DSP_ENABLE);
  dsp_module.write_tulip_reg_clear_bits(TULIP_DSP_CONTROL, TULIP_DSP_CONTROL_BYPASS);

  dsp_module.write_tulip_reg_clear_bits(TULIP_DSP_CONTROL, TULIP_DSP_CONTROL_SW_RESETN_CHORUS);
  dsp_module.write_tulip_reg_set_bits(TULIP_DSP_CONTROL, TULIP_DSP_CONTROL_SW_RESETN_CHORUS);
  dsp_module.write_tulip_reg_clear_bits(TULIP_DSP_CONTROL, TULIP_DSP_CONTROL_BYPASS_CHORUS);

  dsp_module.delay_ms(1);

  cout << "program Chorus gain:\n";
  dsp_module.program_chorus_gain(gain_array);

  cout << "program Chorus average delay:\n";
  dsp_module.program_chorus_avg_delay(avg_delay);

  cout << "program Chorus LFO depth:\n";
  dsp_module.program_chorus_lfo_depth(lfo_depth);

  cout << "program Chorus LFO frequency:\n";
  dsp_module.program_chorus_lfo_freq(lfo_freq);

  cout << "Chorus Programming Done\n";

  return 0;
}

bool read_text_file_long_int(uint32_t expected_length, string file_name, vector<int64_t>& return_array)
{
  ifstream file_profile;
  uint8_t file_profile_len = 0;

  file_profile.open(file_name);

  if (!file_profile)
  {
    cerr << file_name << "could not be opened for reading!\n";
    return false;
  }

  while (file_profile)
  {
    string strInput;
    file_profile >> strInput;
    if (strInput.size() > 0)
    {
      file_profile_len++;
    }
  }

  if (expected_length > 0)
  {
    if (expected_length != file_profile_len)
    {
      cerr << file_name << "file length is not equal to expected length\n";
      return false;
    }
  }

  file_profile.clear();
  file_profile.seekg(0);

  for (int i = 0 ; i < file_profile_len ; i++)
  {
    string strInput;
    file_profile >> strInput;
    return_array[i] = (int64_t)stol(strInput);
  }

  file_profile.close();

  return true;

}


