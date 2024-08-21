
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
  bool enable_dsp = true;

  for(int i = 0 ; i < argc ; i++)
  {
    if (strcmp(argv[i], "-bypass") == 0)
    {
      enable_dsp = false;
    }
  }

  // call constructor:
  tulip_dsp dsp_module;

  if (enable_dsp == true)
  {
    cout << "enable Tulip DSP\n";
    dsp_module.write_tulip_reg_set_bits(CONTROL, CONTROL_DSP_ENABLE);
    dsp_module.write_tulip_reg_clear_bits(TULIP_DSP_CONTROL, TULIP_DSP_CONTROL_BYPASS);
    return 0;
  }
  else
  {
    cout << "bypass Tulip DSP\n";
    dsp_module.write_tulip_reg_set_bits(TULIP_DSP_CONTROL, TULIP_DSP_CONTROL_BYPASS);
    return 0;
  }

}

