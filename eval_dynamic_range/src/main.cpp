//#include <pthread.h>
#include <iostream>
#include <sys/mman.h>
#include <fcntl.h>
#include <tgmath.h>
#include <unistd.h>
#include <stdio.h>
#include <math.h>

#define MAX_SAMPS 1048576

#define FULLSCALE_MAX (8388608.0-1.0)

int main()
{
  int i;
  float capture_sample_l;
  float capture_sample_r;
  float capture_sample_sqr_sum_l = 0.0;
  float capture_sample_sqr_sum_r = 0.0;
  float capture_sample_sqr_mean_sum_l = 0.0;
  float capture_sample_sqr_mean_sum_r = 0.0;
  float capture_sample_abs_max_l = 0.0;
  float capture_sample_abs_max_r = 0.0;
  uint32_t capture_size_bytes;
  uint32_t capture_size_samples;
  int32_t ps_2_i2s_lr_chan[2];
  FILE *ptr;

  ptr = fopen("/home/ubuntu/captures/capture.bin","rb");
  if (ptr == NULL)
  {
    perror("Failed: could not open /home/ubuntu/captures/capture.bin\n");
    return 0;
  }
  fseek(ptr, 0L, SEEK_END);
  capture_size_bytes = ftell(ptr);
  capture_size_samples = capture_size_bytes/8;
  rewind(ptr);

  if (capture_size_samples > MAX_SAMPS)
  {
    capture_size_samples = MAX_SAMPS;
  }

  for (i = 0 ; i < capture_size_samples ; i++)
  {
    fread(ps_2_i2s_lr_chan, sizeof(uint32_t), 2, ptr);
    capture_sample_l = ((float)ps_2_i2s_lr_chan[0])/FULLSCALE_MAX;
    capture_sample_r = ((float)ps_2_i2s_lr_chan[1])/FULLSCALE_MAX;

    capture_sample_sqr_sum_l += pow(capture_sample_l, 2);
    capture_sample_sqr_sum_r += pow(capture_sample_r, 2);

    if (abs(capture_sample_l) > capture_sample_abs_max_l)
    {
      capture_sample_abs_max_l = abs(capture_sample_l);
    }
    if (abs(capture_sample_r) > capture_sample_abs_max_r)
    {
      capture_sample_abs_max_r = abs(capture_sample_r);
    }
  }

  fclose(ptr);

  capture_sample_sqr_mean_sum_l = capture_sample_sqr_sum_l/((float)capture_size_samples);
  capture_sample_sqr_mean_sum_r = capture_sample_sqr_sum_r/((float)capture_size_samples);

  printf("max dynamic range left : %f%%\n", capture_sample_abs_max_l*100.0);
  printf("max dynamic range right: %f%%\n", capture_sample_abs_max_r*100.0);

  printf("mean power left : %f dBFS\n", 10.0*log10(capture_sample_sqr_mean_sum_l));
  printf("mean power right: %f dBFS\n", 10.0*log10(capture_sample_sqr_mean_sum_r));

  return 0;
}
