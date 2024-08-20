#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string>
#include <vector>
#include <iostream>

#include "bin2wav.hpp"

#define ARRAY_SZ 2048
#define ADC_BIT_WIDTH 24

int32_t abs_int32(int32_t input)
{
  if (input < 0)
  {
    return -input;
  }
  return input;
}

int bin2wav
(
  bool is_fixed_point,
  string input_fname,
  string output_fname
)
{
  FILE *fin_ptr;
  FILE *fout_ptr;

  fin_ptr = fopen(input_fname.c_str(),"rb");
  fout_ptr = fopen(output_fname.c_str(),"wb");
  fseek(fin_ptr, 0L, SEEK_END);
  uint32_t fin_sz = ftell(fin_ptr);
  rewind(fin_ptr);


  uint32_t num_samples = fin_sz/8; // left + right channel

  uint32_t RIFF_id          = 1179011410; // Identifier « RIFF » (0x52, 0x49, 0x46, 0x46)
  uint32_t f_sz_m8;                       // Overall file size minus 8 bytes
  uint32_t WAVE_id          = 1163280727; // Format = « WAVE »  (0x57, 0x41, 0x56, 0x45)
  uint32_t FMT_id           = 544501094;  // Identifier « fmt␣ »  (0x66, 0x6D, 0x74, 0x20)
  uint32_t FMT_chunk_sz_m8  = 16;         // Chunk size minus 8 bytes, which is 16 bytes here  (0x10), I think chunk is referring to the fmt section

  uint16_t num_chan         = 2;          // Number of channels
  uint16_t audio_format     = 1;          // Audio format (1: PCM integer, 3: IIEE float)
  if (is_fixed_point == false)
  {
    audio_format = 3;
  }

  uint32_t chan_audio_format = ((uint32_t)num_chan << 16) | (uint32_t)audio_format;

  uint32_t sample_rate = 48000;           // Sample rate (in hertz)

  uint16_t bits_per_sample = 32;
  uint16_t bytes_per_sample = bits_per_sample/8;
  uint16_t bytes_per_bloc = bytes_per_sample*num_chan;

  uint32_t byte_rate = sample_rate*num_chan*bytes_per_sample; // Number of bytes to read per second (Frequence * BytePerBloc).

  uint32_t sample_size = ((uint32_t)bits_per_sample << 16) | (uint32_t)bytes_per_bloc;

  uint32_t DATA_BLOC_id = 1635017060; // Identifier « data »  (0x64, 0x61, 0x74, 0x61)
  uint32_t data_size_bytes = num_samples*bytes_per_bloc;

  f_sz_m8 = data_size_bytes + 44-8;

  uint32_t wav_header[11] =
  {
    RIFF_id,
    f_sz_m8,
    WAVE_id,
    FMT_id,
    FMT_chunk_sz_m8,
    chan_audio_format,
    sample_rate,
    byte_rate,
    sample_size,
    DATA_BLOC_id,
    data_size_bytes
  };

  fwrite(wav_header, sizeof(uint32_t), 11, fout_ptr);

  int32_t data_vector_int[ARRAY_SZ];
  float data_vector_float[ARRAY_SZ];


  uint32_t file_byte_counter = 0;
  int32_t max_val = 0;
  while (file_byte_counter < fin_sz)
  {
    if ((fin_sz-file_byte_counter) < (ARRAY_SZ*4)) // 4 bytes in float and int32
    {//ADC_BIT_WIDTH
      fread(data_vector_int, 4, (fin_sz-file_byte_counter)/4, fin_ptr);
      for (int i=0 ; i < (fin_sz-file_byte_counter)/4 ; i++)
      {
        if (abs_int32(data_vector_int[i]) > max_val)
        {
          max_val = abs_int32(data_vector_int[i]);
        }
      }

      file_byte_counter = file_byte_counter + (fin_sz-file_byte_counter);
    }
    else
    {
      fread(data_vector_int, 4, ARRAY_SZ, fin_ptr);
      for (int i=0 ; i < ARRAY_SZ ; i++)
      {
        if (abs_int32(data_vector_int[i]) > max_val)
        {
          max_val = abs_int32(data_vector_int[i]);
        }
      }

      file_byte_counter = file_byte_counter + ARRAY_SZ*4;
    }

  }

  rewind(fin_ptr);

  uint8_t additional_lshift = 0;
  while ((1<<additional_lshift)*max_val < pow(2,ADC_BIT_WIDTH-1))
  {
    additional_lshift++;
  }
  additional_lshift--;


  //uint32_t file_byte_counter = 0;
  file_byte_counter = 0;
  while (file_byte_counter < fin_sz)
  {
    if ((fin_sz-file_byte_counter) < (ARRAY_SZ*4)) // 4 bytes in float and int32
    {//ADC_BIT_WIDTH
      fread(data_vector_int, 4, (fin_sz-file_byte_counter)/4, fin_ptr);

      if (is_fixed_point == false)
      {
        for (int i=0 ; i < (fin_sz-file_byte_counter)/4 ; i++)
        {
          data_vector_float[i] = (float)(((double)data_vector_int[i])/((double)pow(2,ADC_BIT_WIDTH-additional_lshift-1)));
          //data_vector_float[i] = (float)data_vector_int[i];
        }
        fwrite(data_vector_float, 4, (fin_sz-file_byte_counter)/4, fout_ptr);
      }
      else
      {
        for (int i=0 ; i < (fin_sz-file_byte_counter)/4 ; i++)
        {
          data_vector_int[i] = data_vector_int[i] << (8+additional_lshift);
        }
        fwrite(data_vector_int, 4, (fin_sz-file_byte_counter)/4, fout_ptr);
      }

      file_byte_counter = file_byte_counter + (fin_sz-file_byte_counter);
    }
    else
    {
      fread(data_vector_int, 4, ARRAY_SZ, fin_ptr);

      if (is_fixed_point == false)
      {
        for (int i=0 ; i < ARRAY_SZ ; i++)
        {
          data_vector_float[i] = (float)(((double)data_vector_int[i])/((double)pow(2,ADC_BIT_WIDTH-additional_lshift-1)));
          //data_vector_float[i] = (float)data_vector_int[i];
        }
        fwrite(data_vector_float, 4, ARRAY_SZ, fout_ptr);
      }
      else
      {
        for (int i=0 ; i < ARRAY_SZ ; i++)
        {
          data_vector_int[i] = data_vector_int[i] << (8+additional_lshift);
        }
        fwrite(data_vector_int, 4, ARRAY_SZ, fout_ptr);
      }

      file_byte_counter = file_byte_counter + ARRAY_SZ*4;
    }

  }

  fclose(fin_ptr);
  fclose(fout_ptr);
  return 1;
}


//int main (int argc, char *argv[])
//{
//
//  int i;
//  char* input_fname;
//  char* output_fname;
//  FILE* output_ffin_ptr;
//
//  uint8_t arg_count = 0;
//
//  for(i = 0 ; i < argc ; i++)
//  {
//    if(strcmp(argv[i], "-i") == 0)
//    {
//      input_fname = argv[i+1];
//      arg_count++;
//    }
//    else if(strcmp(argv[i], "-o") == 0)
//    {
//      output_fname = argv[i+1];arg_count++;
//    }
//  }
//
//  if(arg_count < 2)
//  {
//    printf("error: not enough args\n");
//    return 0;
//  }
//
//  output_ffin_ptr = fopen(output_fname, "wb");
//
//  uint32_t num_samples = 34375*8;
//
//  uint32_t RIFF_id          = 1179011410; // Identifier « RIFF » (0x52, 0x49, 0x46, 0x46)
//  uint32_t f_sz_m8;                       // Overall file size minus 8 bytes
//  uint32_t WAVE_id          = 1163280727; // Format = « WAVE »  (0x57, 0x41, 0x56, 0x45)
//  uint32_t FMT_id           = 544501094;  // Identifier « fmt␣ »  (0x66, 0x6D, 0x74, 0x20)
//  uint32_t FMT_chunk_sz_m8  = 16;         // Chunk size minus 8 bytes, which is 16 bytes here  (0x10), I think chunk is referring to the fmt section
//
//
//  uint16_t num_chan         = 2;          // Number of channels
//  uint16_t audio_format     = 1;          // Audio format (1: PCM integer, 3: IIEE float)
//  uint32_t chan_audio_format = ((uint32_t)num_chan << 16) | (uint32_t)audio_format;
//
//  uint32_t sample_rate = 48000;           // Sample rate (in hertz)
//
//  uint16_t bits_per_sample = 32;
//  uint16_t bytes_per_sample = bits_per_sample/8;
//  uint16_t bytes_per_bloc = bytes_per_sample*num_chan;
//
//  uint32_t byte_rate = sample_rate*num_chan*bytes_per_sample; // Number of bytes to read per second (Frequence * BytePerBloc).
//
//  uint32_t sample_size = ((uint32_t)bits_per_sample << 16) | (uint32_t)bytes_per_bloc;
//
//  uint32_t DATA_BLOC_id = 1635017060; // Identifier « data »  (0x64, 0x61, 0x74, 0x61)
//  uint32_t data_size_bytes = num_samples*bytes_per_bloc;
//
//  f_sz_m8 = data_size_bytes + 44-8;
//
//  uint32_t wav_header[11] =
//  {
//    RIFF_id,
//    f_sz_m8,
//    WAVE_id,
//    FMT_id,
//    FMT_chunk_sz_m8,
//    chan_audio_format,
//    sample_rate,
//    byte_rate,
//    sample_size,
//    DATA_BLOC_id,
//    data_size_bytes
//  };
//
//  int32_t sine_wave[num_samples*2];
//  for (int i=0 ; i < num_samples ; i++)
//  {
//    sine_wave[i*2]   = (float)(pow(2,31)/4.0)*sin(2.0*M_PI*(float)i/48.0);
//    sine_wave[i*2+1] = (float)(pow(2,31)/4.0)*sin(2.0*M_PI*(float)i/48.0);
//  }
//
//  fwrite(wav_header, sizeof(uint32_t), 11, output_ffin_ptr);
//  fwrite(sine_wave, sizeof(uint32_t), num_samples*2, output_ffin_ptr);
//
//
//  fclose(output_ffin_ptr);
//
//  return 0;
//
//}
