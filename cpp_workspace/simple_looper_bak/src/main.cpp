#include <pthread.h>
#include <string>
#include <string.h>
#include <stdio.h>
#include "KR260_TULIP_REGISTERS.h"
#include "footpedal_gpio.h"
#include "tulip_dsp.h"

#include <iostream>
#include <fstream>
using namespace std;

#define I2S_2_PS_FIFO_FILL_FLAG 256
#define PS_2_I2S_FIFO_FILL_FLAG 256
#define TRANSITION_WINDOW_START 32
#define TRANSITION_WINDOW_END 32
#define MAX_SAMPS (12500000-2048)
#define TEMPO_COUNTER_MAX 8

bool file_exists(char* fname);
uint32_t get_file_size_bytes(char* fname);

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
  char default_capture_fname[256] = "/home/ubuntu/captures/capture.bin";
  char adjacent_capture_fname[256] = "/home/ubuntu/captures/capture_adjacent.bin";
  FILE* capture_file_ptr;
  FILE* capture_file_ptr2;
  FILE* adjacent_file_ptr;

  float gain = 1.0;

  for(int i = 0 ; i < argc ; i++)
  {
    if (strcmp(argv[i], "-help") == 0)
    {
      cout << "I need to add more print helps in general\n";
    }
    else if (strcmp(argv[i], "-g") == 0)
    {
      gain = atof(argv[i+1]);
    }
  }

  (void) pthread_create(&tId, 0, userInput_thread, 0);

  uint8_t pedal_0_prev_val = 0;
  uint8_t pedal_0_val = 0;
  uint8_t pedal_1_prev_val = 0;
  uint8_t pedal_1_val = 0;
  uint32_t footpedal_reg;
  footpedal_gpio footpedal_input;
  tulip_dsp tulip_dsp_obj;

  bool capture_exists = file_exists(default_capture_fname);
  bool capture_append = false;
  bool capture_append_start = false;
  bool capture_file_open = false;
  bool capture2_file_open = false;
  bool adjacent_file_open = false;
  bool adjacent_file_waiting = false;

  uint16_t tempo_counter = 0;

  uint32_t i2s_2_ps_fifo_fill;
  uint32_t ps_2_i2s_fifo_avail;
  uint32_t i2s_2_ps_lr_chan[2*I2S_2_PS_FIFO_FILL_FLAG];
  uint32_t ps_2_i2s_lr_chan[2*PS_2_I2S_FIFO_FILL_FLAG];
  int32_t current_lr_chan[2*I2S_2_PS_FIFO_FILL_FLAG];
  uint32_t sample_counter = 0;
  uint32_t capture_size_samples;
  uint32_t samples_read = 0;
  uint32_t adj_samples_read = 0;
  uint32_t samples_to_read = 0;
  uint32_t adj_samples_to_read = 0;

  float multiplier_array[PS_2_I2S_FIFO_FILL_FLAG];

  if (capture_exists == true)
  {
    capture_size_samples = get_file_size_bytes(default_capture_fname)/8; // 8 bytes per sample for L+R channel
    if (capture_size_samples == 0)
    {
      capture_exists = false;
    }
    else
    {
      capture_file_ptr = fopen(default_capture_fname, "rb");
      capture_file_open = true;
    }
  }

  while (keep_running == true)
  {
    footpedal_reg = footpedal_input.read_gpio();
    pedal_0_val = footpedal_reg & 0b1;
    pedal_1_val = ((footpedal_reg & 0b10) >> 1) & 0b1;

    if (pedal_1_val == 1 && pedal_1_prev_val == 0)
    {
      cout << "rising edge 1\n";
      if (capture_append == false)
      {
        cout << "reset capture\n";
        tulip_dsp_obj.write_tulip_reg_clear_bits(CONTROL, CONTROL_I2S_2_PS_ENABLE | CONTROL_PS_2_I2S_ENABLE);

        if (capture_file_open == true)
        {
          fclose(capture_file_ptr);
        }

        capture_file_ptr = fopen(default_capture_fname, "wb");
        fclose(capture_file_ptr);

        samples_read = 0;
        capture_file_open = false;
        capture_exists = false;
      }
    }

    if (pedal_0_val == 1 && pedal_0_prev_val == 0)
    {
      cout << "rising edge 0\n";
      if (capture_append == true)
      {
        capture_append = false;
        capture_append_start = false;
        if (capture_exists == false)
        {

          i2s_2_ps_fifo_fill = tulip_dsp_obj.read_tulip_reg(I2S_2_PS_FIFO_COUNT);
          if (i2s_2_ps_fifo_fill > I2S_2_PS_FIFO_FILL_FLAG)
          {
            i2s_2_ps_fifo_fill = I2S_2_PS_FIFO_FILL_FLAG;
          }
          for (int i = 0 ; i < i2s_2_ps_fifo_fill ; i++)
          {
            i2s_2_ps_lr_chan[2*i]   = tulip_dsp_obj.read_tulip_reg(I2S_2_PS_FIFO_READ_L);
            i2s_2_ps_lr_chan[2*i+1] = tulip_dsp_obj.read_tulip_reg(I2S_2_PS_FIFO_READ_R);
          }
          fwrite(i2s_2_ps_lr_chan, sizeof(uint32_t), 2*i2s_2_ps_fifo_fill, capture_file_ptr);
          if (capture_file_open == true)
          {
            fclose(capture_file_ptr);
          }
          tulip_dsp_obj.write_tulip_reg_clear_bits(CONTROL, CONTROL_I2S_2_PS_ENABLE);
          capture_exists = true;
          tulip_dsp_obj.write_tulip_reg_set_bits(CONTROL, CONTROL_PS_2_I2S_ENABLE);

          capture_size_samples = get_file_size_bytes(default_capture_fname)/8; // 8 bytes per sample for L+R channel
          capture_file_ptr = fopen(default_capture_fname, "rb");
          capture_file_open = true;
        }
        else
        {
          capture_append = false;
          capture_append_start = false;
          if (adjacent_file_waiting == true)
          {
            adjacent_file_waiting = false;
          }
          else
          {
            uint32_t zero = 0;
            for (int i = 0 ; i < 2*(capture_size_samples - adj_samples_read) ; i++)
            {
              fwrite(&zero, sizeof(uint32_t), 1, adjacent_file_ptr);
            }

            if (adjacent_file_open == true)
            {
              fclose(adjacent_file_ptr);
            }
            adjacent_file_open = false;

            if (capture2_file_open == true)
            {
              fclose(capture_file_ptr2);
            }
            capture2_file_open = false;

            tulip_dsp_obj.write_tulip_reg_clear_bits(CONTROL, CONTROL_I2S_2_PS_ENABLE);
          }
          if (capture_file_open == true)
          {
            fclose(capture_file_ptr);
          }
          ifstream ifs(adjacent_capture_fname, ios::in | ios::binary);
          ofstream ofs(default_capture_fname, ios::out | ios::binary);
          ofs << ifs.rdbuf();
          ifs.close();
          ofs.close();
          capture_file_ptr = fopen(default_capture_fname, "rb");
          capture_file_open = true;
        }
      }
      else
      {
        sample_counter = 0;
        capture_append = true;
        if (capture_exists == false && capture_file_open == false)
        {
          cout << "Start Capture\n";
          capture_file_ptr = fopen(default_capture_fname, "wb");
          capture_file_open = true;
        }
        tulip_dsp_obj.write_tulip_reg_set_bits(CONTROL, CONTROL_I2S_2_PS_ENABLE);
      }
    }

    if (capture_exists == false && capture_append == true)
    {
      // write to new file
      i2s_2_ps_fifo_fill = tulip_dsp_obj.read_tulip_reg(I2S_2_PS_FIFO_COUNT);
      if (i2s_2_ps_fifo_fill >= I2S_2_PS_FIFO_FILL_FLAG)
      {
        for (int i = 0 ; i < I2S_2_PS_FIFO_FILL_FLAG ; i++)
        {
          i2s_2_ps_lr_chan[2*i]   = tulip_dsp_obj.read_tulip_reg(I2S_2_PS_FIFO_READ_L);
          i2s_2_ps_lr_chan[2*i+1] = tulip_dsp_obj.read_tulip_reg(I2S_2_PS_FIFO_READ_R);
        }
        fwrite(i2s_2_ps_lr_chan, sizeof(uint32_t), 2*I2S_2_PS_FIFO_FILL_FLAG, capture_file_ptr);
        sample_counter = sample_counter + I2S_2_PS_FIFO_FILL_FLAG;
        if (sample_counter >= MAX_SAMPS)
        {
          cout << "woah there cowboy, that's a mighty large file you've aquired\n";
        }
      }
    }
    else if (capture_exists == true)
    {

      if (capture_append == true && capture_append_start == true)
      {
        // capture audio in adjacent file
        i2s_2_ps_fifo_fill = tulip_dsp_obj.read_tulip_reg(I2S_2_PS_FIFO_COUNT);
        if (i2s_2_ps_fifo_fill >= I2S_2_PS_FIFO_FILL_FLAG)
        {
          if (capture_size_samples - adj_samples_read <= I2S_2_PS_FIFO_FILL_FLAG)
          {
            adj_samples_to_read = capture_size_samples - adj_samples_read;
            fread(current_lr_chan, sizeof(uint32_t), 2*adj_samples_to_read, capture_file_ptr2);

            if (capture2_file_open == true)
            {
              fclose(capture_file_ptr2);
            }
            capture2_file_open = false;
            adj_samples_read = 0;
            capture_append = false;
            capture_append_start = false;
            adjacent_file_waiting = true;
          }
          else
          {
            adj_samples_to_read = I2S_2_PS_FIFO_FILL_FLAG;
            adj_samples_read = adj_samples_read + adj_samples_to_read;
            fread(current_lr_chan, sizeof(uint32_t), 2*adj_samples_to_read, capture_file_ptr2);
          }

          for (int i = 0 ; i < adj_samples_to_read ; i++)
          {
            i2s_2_ps_lr_chan[2*i]   = (uint32_t)((int32_t)tulip_dsp_obj.read_tulip_reg(I2S_2_PS_FIFO_READ_L) + current_lr_chan[2*i]  );
            i2s_2_ps_lr_chan[2*i+1] = (uint32_t)((int32_t)tulip_dsp_obj.read_tulip_reg(I2S_2_PS_FIFO_READ_R) + current_lr_chan[2*i+1]);
          }
          fwrite(i2s_2_ps_lr_chan, sizeof(uint32_t), 2*adj_samples_to_read, adjacent_file_ptr);

          if (capture_append == false)
          {
            if (adjacent_file_open == true)
            {
              fclose(adjacent_file_ptr);
            }
            adjacent_file_open = false;
            tulip_dsp_obj.write_tulip_reg_clear_bits(CONTROL, CONTROL_I2S_2_PS_ENABLE);
          }

        }
      }

      ps_2_i2s_fifo_avail = tulip_dsp_obj.read_tulip_reg(PS_2_I2S_FIFO_COUNT);
      if (ps_2_i2s_fifo_avail >= PS_2_I2S_FIFO_FILL_FLAG)
      {

        if (tempo_counter*capture_size_samples < samples_read*TEMPO_COUNTER_MAX)
        {
          tempo_counter++;
          cout << "\t" << tempo_counter << "\r";
          fflush(stdout);
        }

        if (capture_size_samples - samples_read <= PS_2_I2S_FIFO_FILL_FLAG)
        {

          samples_to_read = capture_size_samples - samples_read;

          for (int i = 0 ; i < samples_to_read ; i++)
          {
            if ((samples_to_read - i) < TRANSITION_WINDOW_END)
            {
              multiplier_array[i] = gain * (float)(samples_to_read - i) / (float)TRANSITION_WINDOW_END;
            }
            else
            {
              multiplier_array[i] = gain;
            }
          }

          fread(ps_2_i2s_lr_chan, sizeof(uint32_t), 2*samples_to_read, capture_file_ptr);
          rewind(capture_file_ptr);
          tempo_counter = 0;
          samples_read = 0;

          if (capture_append == true && capture_append_start == false)
          {
            capture_append_start = true;
            adj_samples_read = 0;

            if (capture2_file_open == false)
            {
              capture_file_ptr2 = fopen(default_capture_fname, "rb");
            }
            capture2_file_open = true;

            if (adjacent_file_open == false)
            {
              cout << "Start Overlay Capture\n";
              adjacent_file_ptr = fopen(adjacent_capture_fname, "wb");
            }
            adjacent_file_open = true;
            tulip_dsp_obj.write_tulip_reg_set_bits(CONTROL, CONTROL_I2S_2_PS_ENABLE);
          }

          if (adjacent_file_waiting == true)
          {
            if (capture_file_open == true)
            {
              fclose(capture_file_ptr);
            }
            ifstream ifs(adjacent_capture_fname, ios::in | ios::binary);
            ofstream ofs(default_capture_fname, ios::out | ios::binary);
            ofs << ifs.rdbuf();
            ifs.close();
            ofs.close();
            capture_file_ptr = fopen(default_capture_fname, "rb");
            capture_file_open = true;
            adjacent_file_waiting = false;
          }

        }
        else
        {
          samples_to_read = PS_2_I2S_FIFO_FILL_FLAG;

          for (int i = 0 ; i < samples_to_read ; i++)
          {
            if ((samples_read + i) < TRANSITION_WINDOW_START)
            {
              multiplier_array[i] = gain * (float)(samples_read + i) / (float)TRANSITION_WINDOW_START;
            }
            else
            {
              multiplier_array[i] = gain;
            }
          }

          samples_read = samples_read + samples_to_read;
          fread(ps_2_i2s_lr_chan, sizeof(uint32_t), 2*samples_to_read, capture_file_ptr);
        }

        for (int i = 0 ; i < samples_to_read ; i++)
        {
          //tulip_dsp_obj.write_tulip_reg(PS_2_I2S_FIFO_WRITE_L, ps_2_i2s_lr_chan[2*i]);
          //tulip_dsp_obj.write_tulip_reg(PS_2_I2S_FIFO_WRITE_R, ps_2_i2s_lr_chan[2*i+1]);

          tulip_dsp_obj.write_tulip_reg(PS_2_I2S_FIFO_WRITE_L, (uint32_t)((int32_t)((float)((int32_t)ps_2_i2s_lr_chan[2*i]  )*multiplier_array[i]))  );
          tulip_dsp_obj.write_tulip_reg(PS_2_I2S_FIFO_WRITE_R, (uint32_t)((int32_t)((float)((int32_t)ps_2_i2s_lr_chan[2*i+1])*multiplier_array[i]))  );
        }
      }
    }

    pedal_0_prev_val = pedal_0_val;
    pedal_1_prev_val = pedal_1_val;
  }

  (void) pthread_join(tId, NULL);

  if (capture_file_open == true)
  {
    fclose(capture_file_ptr);
  }

  if (capture2_file_open == true)
  {
    fclose(capture_file_ptr2);
  }

  if (adjacent_file_open == true)
  {
    fclose(adjacent_file_ptr);
  }

  return 1;
}

bool file_exists(char* fname)
{
  FILE* fid;
  fid = fopen(fname, "rb");

  if (fid == NULL)
  {
    return false;
  }

  fclose(fid);
  return true;
}

uint32_t get_file_size_bytes(char* fname)
{
  uint32_t capture_size_bytes;
  FILE* ptr;
  ptr = fopen(fname,"rb");
  if (ptr == NULL)
  {
    cout << "get_file_size_bytes could not open " << fname << "\n";
    return 0;
  }
  fseek(ptr, 0L, SEEK_END);
  capture_size_bytes = ftell(ptr);
  fclose(ptr);
  return capture_size_bytes;
}
