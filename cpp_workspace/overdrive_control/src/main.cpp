
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

  //ifstream fir_profile;
  //uint32_t fir_profile_len = 0;
  ifstream lut_profile;
  uint32_t lut_profile_len = 0;
  ifstream poly1_profile;
  uint32_t poly1_profile_len = 0;

  bool gain_input_lin_defined = false;
  bool gain_output_lin_defined = false;
  bool gain_input_db_defined = false;
  bool gain_output_db_defined = false;

  float gain_input_lin;
  float gain_output_lin;
  float gain_input_db;
  float gain_output_db;

  //bool fir_defined = false;
  bool lut_defined = false;
  //string fir_fname = "";
  string lut_fname = "";

  bool bypass = false;
  bool symmetric_mode = true;

  for(int i = 0 ; i < argc ; i++)
  {
    //if (strcmp(argv[i], "-fir") == 0)
    //{
    //  fir_defined = true;
    //  fir_fname = argv[i+1];
    //}
    if (strcmp(argv[i], "-lut") == 0)
    {
      lut_defined = true;
      lut_fname = argv[i+1];
    }
    else if (strcmp(argv[i], "-gi_lin") == 0)
    {
      gain_input_lin = stof(argv[i+1]);
      gain_input_lin_defined = true;
    }
    else if (strcmp(argv[i], "-go_lin") == 0)
    {
      gain_output_lin = stof(argv[i+1]);
      gain_output_lin_defined = true;
    }
    else if (strcmp(argv[i], "-gi_db") == 0)
    {
      gain_input_db = stof(argv[i+1]);
      gain_input_db_defined = true;
    }
    else if (strcmp(argv[i], "-go_db") == 0)
    {
      gain_output_db = stof(argv[i+1]);
      gain_output_db_defined = true;
    }
    else if (strcmp(argv[i], "-bypass") == 0)
    {
      bypass = true;
    }
    else if (strcmp(argv[i], "-nonsymmetric") == 0)
    {
      symmetric_mode = false;
    }
  }

  if (bypass == true)
  {
    // call constructor:
    cout << "bypass Overdrive\n";
    tulip_dsp dsp_module;

    dsp_module.write_tulip_reg_set_bits(TULIP_DSP_CONTROL, TULIP_DSP_CONTROL_BYPASS_LUT_TF);
    //dsp_module.write_tulip_reg_set_bits(TULIP_DSP_CONTROL, TULIP_DSP_CONTROL_BYPASS_USR_FIR);

    return 0;
  }

  //if (fir_defined == false)
  //{
  //  cerr << "FIR file name undefined\n";
  //  return 0;
  //}

  if (lut_defined == false)
  {
    cerr << "LUT file name undefined\n";
    return 0;
  }

  //cout << "fir_fname        :" << fir_fname       << "\n";
  cout << "lut_fname        :" << lut_fname       << "\n";
  //cout << "gain_input_lin   :" << gain_input_lin  << "\n";
  //cout << "gain_output_lin  :" << gain_output_lin << "\n";
  //cout << "gain_input_db    :" << gain_input_db   << "\n";
  //cout << "gain_output_db   :" << gain_output_db  << "\n";

//  //////////////////////////////////////////////////////////////////////////
//  // Read FIR Inputs from File
//  //////////////////////////////////////////////////////////////////////////
//
//  fir_profile.open(fir_fname);
//
//  if (!fir_profile)
//  {
//      // Print an error and exit
//      cerr << fir_fname << " could not be opened for reading!\n";
//      return 1;
//  }
//
//  while (fir_profile)
//  {
//    string strInput;
//    fir_profile >> strInput;
//    if (strInput.size() > 0)
//    {
//      fir_profile_len++;
//    }
//  }
//
//  fir_profile.clear();
//  fir_profile.seekg(0);
//  vector<int32_t> fir_taps(fir_profile_len);
//
//  for (int i = 0 ; i < fir_profile_len ; i++)
//  {
//    string strInput;
//    fir_profile >> strInput;
//    fir_taps[i] = (int32_t)stoi(strInput);
//  }
//
//  fir_profile.close();


  //////////////////////////////////////////////////////////////////////////
  // Read LUT Inputs from File
  //////////////////////////////////////////////////////////////////////////

  lut_profile.open(lut_fname);

  if (!lut_profile)
  {
      // Print an error and exit
      cerr << lut_fname << " could not be opened for reading!\n";
      return 1;
  }

  while (lut_profile)
  {
    string strInput;
    lut_profile >> strInput;
    if (strInput.size() > 0)
    {
      lut_profile_len++;
    }
  }

  lut_profile.clear();
  lut_profile.seekg(0);
  vector<uint32_t> lut_entries(lut_profile_len);

  for (int i = 0 ; i < lut_profile_len ; i++)
  {
    string strInput;
    lut_profile >> strInput;
    lut_entries[i] = (uint32_t)stoi(strInput);
  }

  lut_profile.close();

  // call constructor:
  tulip_dsp dsp_module;

  dsp_module.write_tulip_reg_set_bits(CONTROL, CONTROL_DSP_ENABLE);
  dsp_module.write_tulip_reg_clear_bits(TULIP_DSP_CONTROL, TULIP_DSP_CONTROL_BYPASS);

  dsp_module.write_tulip_reg_clear_bits(TULIP_DSP_CONTROL, TULIP_DSP_CONTROL_SW_RESETN_LUT_TF);
  dsp_module.write_tulip_reg_set_bits(TULIP_DSP_CONTROL, TULIP_DSP_CONTROL_SW_RESETN_LUT_TF);
  dsp_module.write_tulip_reg_clear_bits(TULIP_DSP_CONTROL, TULIP_DSP_CONTROL_BYPASS_LUT_TF);

  if (symmetric_mode == true)
  {
    dsp_module.write_tulip_reg_set_bits(TULIP_DSP_CONTROL, TULIP_DSP_CONTROL_SYMMETRIC_MODE);
  }
  else
  {
    dsp_module.write_tulip_reg_clear_bits(TULIP_DSP_CONTROL, TULIP_DSP_CONTROL_SYMMETRIC_MODE);
  }

  dsp_module.delay_ms(1);

  if(dsp_module.program_LUT_transfer_function(lut_entries) == false)
  {
    cerr << "issue with LUT transfer function program interface\n";
    dsp_module.write_tulip_reg_clear_bits(TULIP_DSP_CONTROL, TULIP_DSP_CONTROL_SW_RESETN_LUT_TF);
    dsp_module.write_tulip_reg_set_bits(TULIP_DSP_CONTROL, TULIP_DSP_CONTROL_BYPASS_LUT_TF);
    return 0;
  }

//  if (dsp_module.program_FIR_filter(fir_taps) == false)
//  {
//    cerr << "issue with FIR program interface\n";
//    dsp_module.write_tulip_reg_clear_bits(TULIP_DSP_CONTROL, TULIP_DSP_CONTROL_SW_RESETN_USR_FIR | TULIP_DSP_CONTROL_SW_RESETN_LUT_TF);
//    dsp_module.write_tulip_reg_set_bits(TULIP_DSP_CONTROL, TULIP_DSP_CONTROL_BYPASS_USR_FIR | TULIP_DSP_CONTROL_BYPASS_LUT_TF);
//    return 0;
//  }

  if (gain_input_lin_defined == true)
  {
    dsp_module.program_input_gain_linear(gain_input_lin);
  }
  else if (gain_input_db_defined == true)
  {
    dsp_module.program_input_gain_db(gain_input_db);
  }

  if (gain_output_lin_defined == true)
  {
    dsp_module.program_output_gain_linear(gain_output_lin);
  }
  else if (gain_output_db_defined == true)
  {
    dsp_module.program_output_gain_db(gain_output_db);
  }

  cout << "Tulip Overdrive Programming Finished\n";

  return 1;
}

