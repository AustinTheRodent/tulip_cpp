#include <pthread.h>
#include <string>
#include <string.h>
#include <stdio.h>
#include "fft.c"
#include "tulip_dsp.h"
#include "KR260_TULIP_REGISTERS.h"

#include <iostream>
#include <fstream>
using namespace std;

#define N_PEAKS 4
#define PEAK_ZERO_OUT 500
#define FS 48000
#define N_FFT 32768*2
#define N_SAMP 2048
#define DECIMATION_FACTOR 8

#define LOW_D_STRING 73.415 // Hz
#define LOW_E_STRING 82.41 // Hz
#define A_STRING 110.00 // Hz
#define D_STRING 146.83 // Hz
#define G_STRING 196.00 // Hz
#define B_STRING 246.94 // Hz
#define HIGH_E_STRING 329.63 // Hz

#define FREQ_RES ((float)FS/((float)N_FFT*(float)DECIMATION_FACTOR))

uint32_t get_max(float* input_array, uint32_t array_length);
uint32_t get_min_uint32(uint32_t* input_array, uint32_t array_length);
uint32_t get_min_float(float* input_array, uint32_t array_length);

static volatile bool keep_running = true;

static void* userInput_thread(void*)
{
  while(keep_running)
  {
    if (std::cin.get())
    //if (std::cin.get() == 'q')
    {
      keep_running = false;
    }
  }
  return 0;
}

int main (int argc, char *argv[])
{

  pthread_t tId;
  (void) pthread_create(&tId, 0, userInput_thread, 0);

  tulip_dsp tulip_dsp_obj;
  tulip_dsp_obj.write_tulip_reg_clear_bits(CONTROL, CONTROL_I2S_2_PS_ENABLE | CONTROL_PS_2_I2S_ENABLE);

  FILE* time_domain_output_fptr;
  FILE* freq_domain_output_fptr;

  //bool initial_collect_done = false;
  uint32_t tally = 0;
  int32_t fifo_avail;
  int32_t samp_index;
  uint8_t decimation_counter;
  complex fft_data[N_FFT]; 
  complex scratch[N_FFT];
  float mag2[N_FFT/2];
  uint32_t peaks[N_PEAKS];
  float fundamental_frequency;
  float freq_diff[6];
  float full_freq_list[N_FFT/2];
  bool drop_d = false;

  for(int i = 0 ; i < argc ; i++)
  {
    if (strcmp(argv[i], "-dd") == 0)
    {
      drop_d = true;
    }
  }


  while (keep_running == true)
  {

    for (int i = 0 ; i < N_FFT ; i++)
    {
      scratch[i].Re = 0;
      scratch[i].Im = 0;
      fft_data[i].Re = 0;
      fft_data[i].Im = 0;

    }

    tulip_dsp_obj.write_tulip_reg_set_bits(CONTROL, CONTROL_I2S_2_PS_ENABLE);
    samp_index = 0;
    decimation_counter = 0;
    while (samp_index < N_SAMP)
    {
      fifo_avail = tulip_dsp_obj.read_tulip_reg(I2S_2_PS_FIFO_COUNT);
      for(int i=0 ; i < fifo_avail ; i++)
      {
        if (decimation_counter % DECIMATION_FACTOR == 0)
        {
          fft_data[samp_index].Re = (float)((int32_t)tulip_dsp_obj.read_tulip_reg(I2S_2_PS_FIFO_READ_L));
          samp_index++;
        }
        else
        {
          tulip_dsp_obj.read_tulip_reg(I2S_2_PS_FIFO_READ_L);
        }
        decimation_counter++;
        tulip_dsp_obj.read_tulip_reg(I2S_2_PS_FIFO_READ_R);
        if (samp_index >= N_SAMP)
        {
          break;
        }
      }
    }
    tulip_dsp_obj.write_tulip_reg_clear_bits(CONTROL, CONTROL_I2S_2_PS_ENABLE);

    //if (initial_collect_done == false && tally == 2)
    //{
    //  time_domain_output_fptr = fopen("/home/ubuntu/cpp_workspace/tuner/other/time_domain.txt", "w");
    //  for (int i = 0 ; i < N_FFT ; i++)
    //  {
    //    fprintf(time_domain_output_fptr, "%f\n", fft_data[i].Re);
    //  }
    //  fclose(time_domain_output_fptr);
    //
    //  fft(fft_data, N_FFT, scratch);
    //
    //  freq_domain_output_fptr = fopen("/home/ubuntu/cpp_workspace/tuner/other/freq_domain_re.txt", "w");
    //  for (int i = 0 ; i < N_FFT ; i++)
    //  {
    //    fprintf(freq_domain_output_fptr, "%f\n", fft_data[i].Re);
    //  }
    //  fclose(freq_domain_output_fptr);
    //
    //  freq_domain_output_fptr = fopen("/home/ubuntu/cpp_workspace/tuner/other/freq_domain_im.txt", "w");
    //  for (int i = 0 ; i < N_FFT ; i++)
    //  {
    //    fprintf(freq_domain_output_fptr, "%f\n", fft_data[i].Im);
    //  }
    //  fclose(freq_domain_output_fptr);
    //
    //  for (int i = 0 ; i < N_FFT/2 ; i++)
    //  {
    //    mag2[i] = fft_data[i].Re*fft_data[i].Re + fft_data[i].Im*fft_data[i].Im;
    //  }
    //
    //  freq_domain_output_fptr = fopen("/home/ubuntu/cpp_workspace/tuner/other/freq_domain_abs.txt", "w");
    //  for (int i = 0 ; i < N_FFT/2 ; i++)
    //  {
    //    fprintf(freq_domain_output_fptr, "%f\n", mag2[i]);
    //  }
    //  fclose(freq_domain_output_fptr);
    //
    //  initial_collect_done = true;
    //}

    fft(fft_data, N_FFT, scratch);

    for (int i = 0 ; i < N_FFT/2 ; i++)
    {
      mag2[i] = fft_data[i].Re*fft_data[i].Re + fft_data[i].Im*fft_data[i].Im;
    }

    for (int i = 0 ; i < N_PEAKS ; i++)
    {
      peaks[i] = get_max(mag2, N_FFT/2);
      for (int j = peaks[i] - PEAK_ZERO_OUT ; j < peaks[i] + PEAK_ZERO_OUT ; j++)
      {
        if ((j >= 0) && (j < N_FFT/2))
        {
          mag2[j] = 0;
        }
      }
    }
    

    uint32_t fundamental_frequency_index = peaks[get_min_uint32(peaks, N_PEAKS)];
    fundamental_frequency = (float)(fundamental_frequency_index*FS)/(float)(N_FFT*DECIMATION_FACTOR);
    //fundamental_frequency = (float)peaks[get_min_uint32(peaks, N_PEAKS)]/(float)(N_FFT*2)*(FS/2);

    //cout << fundamental_frequency << "\n";

    if (drop_d == false)
    {
      freq_diff[0] = abs(fundamental_frequency - LOW_E_STRING);
    }
    else
    {
      freq_diff[0] = abs(fundamental_frequency - LOW_D_STRING);
    }
    freq_diff[1] = abs(fundamental_frequency - A_STRING);
    freq_diff[2] = abs(fundamental_frequency - D_STRING);
    freq_diff[3] = abs(fundamental_frequency - G_STRING);
    freq_diff[4] = abs(fundamental_frequency - B_STRING);
    freq_diff[5] = abs(fundamental_frequency - HIGH_E_STRING);
    uint32_t freq_diff_min_index = get_min_float(freq_diff, 6);

    if (freq_diff_min_index == 0)
    {
      if (drop_d == false)
      {
        for (int i = 0 ; i < N_FFT/2 ; i++)
        {
          full_freq_list[i] = abs((float)i*FREQ_RES - LOW_E_STRING);
        }
      }
      else
      {
        for (int i = 0 ; i < N_FFT/2 ; i++)
        {
          full_freq_list[i] = abs((float)i*FREQ_RES - LOW_D_STRING);
        }
      }
      uint32_t closest_bin = get_min_float(full_freq_list, N_FFT/2);
      if (drop_d == false)
      {
        cout << "LOW E" << "\t" << fundamental_frequency << "\t" << (int32_t)fundamental_frequency_index - (int32_t)closest_bin << "\n";
      }
      else
      {
        cout << "LOW D" << "\t" << fundamental_frequency << "\t" << (int32_t)fundamental_frequency_index - (int32_t)closest_bin << "\n";
      }
    }
    else if (freq_diff_min_index == 1)
    {
      for (int i = 0 ; i < N_FFT/2 ; i++)
      {
        full_freq_list[i] = abs((float)i*FREQ_RES - A_STRING);
      }
      uint32_t closest_bin = get_min_float(full_freq_list, N_FFT/2);
      cout << "    A" << "\t" << fundamental_frequency << "\t" << (int32_t)fundamental_frequency_index - (int32_t)closest_bin << "\n";
    }
    else if (freq_diff_min_index == 2)
    {
      for (int i = 0 ; i < N_FFT/2 ; i++)
      {
        full_freq_list[i] = abs((float)i*FREQ_RES - D_STRING);
      }
      uint32_t closest_bin = get_min_float(full_freq_list, N_FFT/2);
      cout << "    D" << "\t" << fundamental_frequency << "\t" << (int32_t)fundamental_frequency_index - (int32_t)closest_bin << "\n";
    }
    else if (freq_diff_min_index == 3)
    {
      for (int i = 0 ; i < N_FFT/2 ; i++)
      {
        full_freq_list[i] = abs((float)i*FREQ_RES - G_STRING);
      }
      uint32_t closest_bin = get_min_float(full_freq_list, N_FFT/2);
      cout << "    G" << "\t" << fundamental_frequency << "\t" << (int32_t)fundamental_frequency_index - (int32_t)closest_bin << "\n";
    }
    else if (freq_diff_min_index == 4)
    {
      for (int i = 0 ; i < N_FFT/2 ; i++)
      {
        full_freq_list[i] = abs((float)i*FREQ_RES - B_STRING);
      }
      uint32_t closest_bin = get_min_float(full_freq_list, N_FFT/2);
      cout << "    B" << "\t" << fundamental_frequency << "\t" << (int32_t)fundamental_frequency_index - (int32_t)closest_bin << "\n";
    }
    else if (freq_diff_min_index == 5)
    {
      for (int i = 0 ; i < N_FFT/2 ; i++)
      {
        full_freq_list[i] = abs((float)i*FREQ_RES - HIGH_E_STRING);
      }
      uint32_t closest_bin = get_min_float(full_freq_list, N_FFT/2);
      cout << "HIGH E" << "\t" << fundamental_frequency << "\t" << (int32_t)fundamental_frequency_index - (int32_t)closest_bin << "\n";
    }


    //fflush(stdout);

    tally++;

  }

  (void) pthread_join(tId, NULL);
  return 0;
}

uint32_t get_max(float* input_array, uint32_t array_length)
{
  uint32_t index = 0;
  float max_val = input_array[0];
  for (int i = 1 ; i < array_length ; i++)
  {
    if (input_array[i] > max_val)
    {
      max_val = input_array[i];
      index = i;
    }
  }
  return index;
}

uint32_t get_min_uint32(uint32_t* input_array, uint32_t array_length)
{
  uint32_t index = 0;
  uint32_t min_val = input_array[0];
  for (int i = 1 ; i < array_length ; i++)
  {
    if (input_array[i] < min_val)
    {
      min_val = input_array[i];
      index = i;
    }
  }
  return index;
}

uint32_t get_min_float(float* input_array, uint32_t array_length)
{
  uint32_t index = 0;
  float min_val = input_array[0];
  for (int i = 1 ; i < array_length ; i++)
  {
    if (input_array[i] < min_val)
    {
      min_val = input_array[i];
      index = i;
    }
  }
  return index;
}
