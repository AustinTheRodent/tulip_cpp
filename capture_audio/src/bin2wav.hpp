
#ifndef BIN2WAV_HPP
#define BIN2WAV_HPP

#include <stdint.h>
#include <stdbool.h>
#include <string>
#include <vector>

using namespace std;

int bin2wav
(
  bool is_fixed_point,
  string input_fname,
  string output_fname
);

#endif
