
#include <string>
#include <string.h>
#include <stdio.h>
#include "tulip_dsp.h"
#include "KR260_TULIP_REGISTERS.h"

#include <iostream>
#include <fstream>
using namespace std;

#define NUM_VIBRATO_CHAN 1

bool read_text_file_long_int(uint32_t expected_length, string file_name, vector<int64_t>& return_array);

int main (int argc, char *argv[])
{

  string gain_fname;
  string chirp_depth_fname;
  string freq_deriv_fname;
  string freq_offset_fname;
  bool gain_fname_defined         = false;
  bool chirp_depth_fname_defined  = false;
  bool freq_deriv_fname_defined   = false;
  bool freq_offset_fname_defined  = false;
  bool bypass = false;
  uint8_t return_code = 0;

  for(int i = 0 ; i < argc ; i++)
  {
    if (strcmp(argv[i], "-gain") == 0)
    {
      gain_fname = argv[i+1];
      gain_fname_defined = true;
    }
    else if (strcmp(argv[i], "-chirp_depth") == 0)
    {
      chirp_depth_fname = argv[i+1];
      chirp_depth_fname_defined = true;
    }
    else if (strcmp(argv[i], "-freq_deriv") == 0)
    {
      freq_deriv_fname = argv[i+1];
      freq_deriv_fname_defined = true;
    }
    else if (strcmp(argv[i], "-freq_offset") == 0)
    {
      freq_offset_fname = argv[i+1];
      freq_offset_fname_defined = true;
    }
    else if (strcmp(argv[i], "-bypass") == 0)
    {
      bypass = true;
    }
  }

  if (bypass == true)
  {
    // call constructor:
    cout << "Bypass Vibrato\n";
    tulip_dsp dsp_module;

    dsp_module.write_tulip_reg_set_bits(TULIP_DSP_CONTROL, TULIP_DSP_CONTROL_BYPASS_VIBRATO);

    return 0;
  }

  if (gain_fname_defined == false)
  {
    cerr << "gain file name undefined\n";
    return_code = 1;
  }

  if (chirp_depth_fname_defined == false)
  {
    cerr << "chirp depth file name undefined\n";
    return_code = 1;
  }

  if (freq_deriv_fname_defined == false)
  {
    cerr << "frequency derivative file name undefined\n";
    return_code = 1;
  }

  if (freq_offset_fname_defined == false)
  {
    cerr << "frequency offset file name undefined\n";
    return_code = 1;
  }

  if (return_code > 0)
  {
    return return_code;
  }



  //////////////////////////////////////////////////////////////////////////
  // Read Inputs from Files
  //////////////////////////////////////////////////////////////////////////

  vector<int64_t> gain_array_long(NUM_VIBRATO_CHAN+1);
  vector<int64_t> chirp_depth_array_long(NUM_VIBRATO_CHAN);
  vector<int64_t> freq_deriv_array_long(NUM_VIBRATO_CHAN);
  vector<int64_t> freq_offset_array_long(NUM_VIBRATO_CHAN);

  vector<uint32_t> gain_array(NUM_VIBRATO_CHAN+1);
  vector<uint32_t> chirp_depth_array(NUM_VIBRATO_CHAN);
  vector<int32_t> freq_deriv_array(NUM_VIBRATO_CHAN);
  vector<int32_t> freq_offset_array(NUM_VIBRATO_CHAN);

  if(read_text_file_long_int(NUM_VIBRATO_CHAN+1 , gain_fname        , gain_array_long        ) == false){return 1;};
  if(read_text_file_long_int(NUM_VIBRATO_CHAN   , chirp_depth_fname , chirp_depth_array_long ) == false){return 1;};
  if(read_text_file_long_int(NUM_VIBRATO_CHAN   , freq_deriv_fname  , freq_deriv_array_long  ) == false){return 1;};
  if(read_text_file_long_int(NUM_VIBRATO_CHAN   , freq_offset_fname , freq_offset_array_long ) == false){return 1;};

  for (int i = 0 ; i < NUM_VIBRATO_CHAN+1 ; i++)
  {
    gain_array[i] = (uint32_t)gain_array_long[i];
  }

  for (int i = 0 ; i < NUM_VIBRATO_CHAN ; i++)
  {
    chirp_depth_array[i] = (uint32_t)chirp_depth_array_long[i];
  }

  for (int i = 0 ; i < NUM_VIBRATO_CHAN ; i++)
  {
    freq_deriv_array[i] = (int32_t)freq_deriv_array_long[i];
  }

  for (int i = 0 ; i < NUM_VIBRATO_CHAN ; i++)
  {
    freq_offset_array[i] = (int32_t)freq_offset_array_long[i];
  }

  // call constructor:
  tulip_dsp dsp_module;

  dsp_module.write_tulip_reg_set_bits(CONTROL, CONTROL_DSP_ENABLE);
  dsp_module.write_tulip_reg_clear_bits(TULIP_DSP_CONTROL, TULIP_DSP_CONTROL_BYPASS);

  dsp_module.write_tulip_reg_clear_bits(TULIP_DSP_CONTROL, TULIP_DSP_CONTROL_SW_RESETN_VIBRATO);
  dsp_module.write_tulip_reg_set_bits(TULIP_DSP_CONTROL, TULIP_DSP_CONTROL_SW_RESETN_VIBRATO);
  dsp_module.write_tulip_reg_clear_bits(TULIP_DSP_CONTROL, TULIP_DSP_CONTROL_BYPASS_VIBRATO);

  dsp_module.delay_ms(1);

  cout << "program vibrato gain:\n";
  dsp_module.program_vibrato_gain(gain_array);

  cout << "program vibrato chirp depth:\n";
  dsp_module.program_vibrato_chirp_depth(chirp_depth_array);

  cout << "program vibrato frequency derivative:\n";
  dsp_module.program_vibrato_freq_deriv(freq_deriv_array);

  cout << "program vibrato frequency offset:\n";
  dsp_module.program_vibrato_freq_offset(freq_offset_array);


  cout << "Vibrato Programming Done\n";

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


