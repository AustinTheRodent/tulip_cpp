
#include "tulip_dsp.h"

#include <iostream>
#include <fstream>
using namespace std;

int main(void)
{
  cout << "bypass DSP\n";
  tulip_dsp dsp_module;
  dsp_module.reset();
  dsp_module.bypass();
  return 0;
}

