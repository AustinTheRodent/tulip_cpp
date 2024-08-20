
#include <cmath>
#include <tgmath.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <chrono>
#include <vector>
#include <iostream>
#include "tulip_dsp.h"
#include "KR260_TULIP_REGISTERS.h"

using namespace std::chrono;
using namespace std;

tulip_dsp::tulip_dsp()
{
  // Constructor
  pagesize = sysconf(_SC_PAGE_SIZE);
  offset = TULIP_ADDRESS;
  page_base = (offset / pagesize) * pagesize;
  page_offset = offset - page_base;

  if ((mem_fd = open("/dev/mem", O_RDWR | O_SYNC)) == -1) {
    perror("Error opening /dev/mem");
    return;
  }

  map_base = mmap(NULL,
    pagesize,
    PROT_READ | PROT_WRITE,
    MAP_SHARED,
    mem_fd,
    page_base);	// phys_addr should be page-aligned.

  if (map_base == MAP_FAILED) {
    perror("Error mapping memory");
    close(mem_fd);
    return;
  }
}

tulip_dsp::~tulip_dsp()
{
  // destructor
  munmap((void *)map_base, pagesize);
  close(mem_fd);
}

void tulip_dsp::reset(void)
{
  tulip_dsp::write_tulip_reg_clear_bits(CONTROL, CONTROL_DSP_ENABLE);
  tulip_dsp::write_tulip_reg(TULIP_DSP_CONTROL, 0);
  tulip_dsp::program_input_gain_linear(1.0);
  tulip_dsp::program_output_gain_linear(1.0);
}

void tulip_dsp::bypass(void)
{
  tulip_dsp::write_tulip_reg_set_bits(
    TULIP_DSP_CONTROL,
    TULIP_DSP_CONTROL_BYPASS |
    TULIP_DSP_CONTROL_BYPASS_REVERB |
    TULIP_DSP_CONTROL_BYPASS_LUT_TF |
    TULIP_DSP_CONTROL_BYPASS_USR_FIR |
    TULIP_DSP_CONTROL_BYPASS_VIBRATO |
    TULIP_DSP_CONTROL_BYPASS_CHORUS);
}

bool tulip_dsp::program_LUT_transfer_function(vector<uint32_t>& lut_entries)
{

  return tulip_dsp::program_tulip_uint32(lut_entries,
                                         TULIP_DSP_CONTROL,
                                         TULIP_DSP_CONTROL_SW_RESETN_LUT_TF,
                                         TULIP_DSP_LUT_PROG,
                                         TULIP_DSP_STATUS,
                                         TULIP_DSP_STATUS_LUT_PROG_DONE,
                                         TULIP_DSP_STATUS_LUT_PROG_READY,
                                         "LUT transfer function program interface",
                                         100);

}


bool tulip_dsp::program_FIR_filter(vector<int32_t>& FIR_taps)
{

  return tulip_dsp::program_tulip_int32(FIR_taps,
                                        TULIP_DSP_CONTROL,
                                        TULIP_DSP_CONTROL_SW_RESETN_USR_FIR,
                                        TULIP_DSP_USR_FIR_PROG,
                                        TULIP_DSP_STATUS,
                                        TULIP_DSP_STATUS_FIR_TAP_DONE,
                                        TULIP_DSP_STATUS_FIR_TAP_READY,
                                        "User FIR program interface",
                                        100);

}

bool tulip_dsp::program_reverb_delay_profile(vector<int32_t>& FIR_taps)
{

  return tulip_dsp::program_tulip_int32(FIR_taps,
                                        TULIP_DSP_CONTROL,
                                        TULIP_DSP_CONTROL_SW_RESETN_REVERB,
                                        TULIP_DSP_REVERB_PROG,
                                        TULIP_DSP_STATUS,
                                        TULIP_DSP_STATUS_REVERB_PROG_DONE,
                                        TULIP_DSP_STATUS_REVERB_PROG_READY,
                                        "Reverb FIR program interface",
                                        100);

}

void tulip_dsp::program_reverb_feedforward_gain_linear(float gain_linear) // (2.0,0.0]
{
  float gain_integer = floor(gain_linear);
  float gain_decimal = gain_linear - gain_integer;
  uint32_t gain_integer_u32 = (uint32_t)gain_integer;
  uint32_t gain_decimal_u32 = gain_decimal*32768.0;
  uint32_t reg_val = ((gain_integer_u32 & 0b1) << 15) | (gain_decimal_u32 & 0x7FFF);
  tulip_dsp::write_tulip_reg(TULIP_DSP_REVERB_FEEDFORWARD_GAIN, reg_val);
}

void tulip_dsp::program_reverb_feedback_gain_linear(float gain_linear) // (2.0,0.0]
{
  float gain_integer = floor(gain_linear);
  float gain_decimal = gain_linear - gain_integer;
  uint32_t gain_integer_u32 = (uint32_t)gain_integer;
  uint32_t gain_decimal_u32 = gain_decimal*32768.0;
  uint32_t reg_val = ((gain_integer_u32 & 0b1) << 15) | (gain_decimal_u32 & 0x7FFF);
  tulip_dsp::modify_tulip_reg(TULIP_DSP_REVERB_SCALE, reg_val, TULIP_DSP_REVERB_SCALE_FEEDBACK_GAIN_MASK);
}

void tulip_dsp::program_reverb_feedback_right_shift(uint8_t right_shift)
{
  uint32_t reg_val = ((uint32_t)right_shift & TULIP_DSP_REVERB_SCALE_FEEDBACK_RIGHT_SHIFT_MASK) << TULIP_DSP_REVERB_SCALE_FEEDBACK_RIGHT_SHIFT_SHIFT;
  tulip_dsp::modify_tulip_reg(
    TULIP_DSP_REVERB_SCALE,
    reg_val,
    TULIP_DSP_REVERB_SCALE_FEEDBACK_RIGHT_SHIFT_MASK << TULIP_DSP_REVERB_SCALE_FEEDBACK_RIGHT_SHIFT_SHIFT);
}

void tulip_dsp::program_input_gain_db(float gain_db)
{
  float gain_linear = pow(10, gain_db/20);
  tulip_dsp::program_input_gain_linear(gain_linear);
}

void tulip_dsp::program_input_gain_linear(float gain_linear)
{
  float gain_integer = floor(gain_linear);
  float gain_decimal = gain_linear - gain_integer;
  uint32_t gain_integer_u32 = (uint32_t)gain_integer;
  uint32_t gain_decimal_u32 = gain_decimal*65536.0;
  uint32_t reg_val = ((gain_integer_u32 & 0xFFFF) << 16) | (gain_decimal_u32 & 0xFFFF);
  tulip_dsp::write_tulip_reg(TULIP_DSP_INPUT_GAIN, reg_val);
}

void tulip_dsp::program_output_gain_db(float gain_db)
{
  float gain_linear = pow(10, gain_db/20);
  tulip_dsp::program_output_gain_linear(gain_linear);
}

void tulip_dsp::program_output_gain_linear(float gain_linear)
{
  float gain_integer = floor(gain_linear);
  float gain_decimal = gain_linear - gain_integer;
  uint32_t gain_integer_u32 = (uint32_t)gain_integer;
  uint32_t gain_decimal_u32 = gain_decimal*65536.0;
  uint32_t reg_val = ((gain_integer_u32 & 0xFFFF) << 16) | (gain_decimal_u32 & 0xFFFF);
  tulip_dsp::write_tulip_reg(TULIP_DSP_OUTPUT_GAIN, reg_val);
}

bool tulip_dsp::program_vibrato_gain(vector<uint32_t>& vibrato_gain_array)
{

  return tulip_dsp::program_tulip_uint32(vibrato_gain_array,
                                         TULIP_DSP_CONTROL,
                                         TULIP_DSP_CONTROL_SW_RESETN_VIBRATO,
                                         TULIP_DSP_VIBRATO_GAIN,
                                         TULIP_DSP_STATUS,
                                         TULIP_DSP_STATUS_VIBRATO_GAIN_PROG_DONE,
                                         TULIP_DSP_STATUS_VIBRATO_GAIN_PROG_READY,
                                         "vibrato program interface",
                                         100);

}

bool tulip_dsp::program_vibrato_chirp_depth(vector<uint32_t>& vibrato_chirp_depth_array)
{

  return tulip_dsp::program_tulip_uint32(vibrato_chirp_depth_array,
                                         TULIP_DSP_CONTROL,
                                         TULIP_DSP_CONTROL_SW_RESETN_VIBRATO,
                                         TULIP_DSP_VIBRATO_CHIRP_DEPTH,
                                         TULIP_DSP_STATUS,
                                         TULIP_DSP_STATUS_VIBRATO_CHIRP_DEPTH_PROG_DONE,
                                         TULIP_DSP_STATUS_VIBRATO_CHIRP_DEPTH_PROG_READY,
                                         "vibrato program interface",
                                         100);

}

bool tulip_dsp::program_vibrato_freq_deriv(vector<int32_t>& vibrato_freq_deriv_array)
{

  return tulip_dsp::program_tulip_int32(vibrato_freq_deriv_array,
                                        TULIP_DSP_CONTROL,
                                        TULIP_DSP_CONTROL_SW_RESETN_VIBRATO,
                                        TULIP_DSP_VIBRATO_FREQ_DERIV,
                                        TULIP_DSP_STATUS,
                                        TULIP_DSP_STATUS_VIBRATO_FREQ_DERIV_PROG_DONE,
                                        TULIP_DSP_STATUS_VIBRATO_FREQ_DERIV_PROG_READY,
                                        "vibrato program interface",
                                        100);

}

bool tulip_dsp::program_vibrato_freq_offset(vector<int32_t>& vibrato_freq_offset_array)
{

  return tulip_dsp::program_tulip_int32(vibrato_freq_offset_array,
                                        TULIP_DSP_CONTROL,
                                        TULIP_DSP_CONTROL_SW_RESETN_VIBRATO,
                                        TULIP_DSP_VIBRATO_FREQ_OFFSET,
                                        TULIP_DSP_STATUS,
                                        TULIP_DSP_STATUS_VIBRATO_FREQ_OFFSET_PROG_DONE,
                                        TULIP_DSP_STATUS_VIBRATO_FREQ_OFFSET_PROG_READY,
                                        "vibrato program interface",
                                        100);

}

bool tulip_dsp::program_chorus_gain(vector<uint32_t>& chorus_gain_array)
{

  return tulip_dsp::program_tulip_uint32(chorus_gain_array,
                                         TULIP_DSP_CONTROL,
                                         TULIP_DSP_CONTROL_SW_RESETN_CHORUS,
                                         TULIP_DSP_CHORUS_GAIN,
                                         TULIP_DSP_STATUS,
                                         TULIP_DSP_STATUS_CHORUS_GAIN_PROG_DONE,
                                         TULIP_DSP_STATUS_CHORUS_GAIN_PROG_READY,
                                         "chorus program interface",
                                         100);

}

bool tulip_dsp::program_chorus_avg_delay(vector<uint32_t>& chorus_avg_delay)
{

  return tulip_dsp::program_tulip_uint32(chorus_avg_delay,
                                         TULIP_DSP_CONTROL,
                                         TULIP_DSP_CONTROL_SW_RESETN_CHORUS,
                                         TULIP_DSP_CHORUS_AVG_DELAY,
                                         TULIP_DSP_STATUS,
                                         TULIP_DSP_STATUS_CHORUS_AVG_DELAY_PROG_DONE,
                                         TULIP_DSP_STATUS_CHORUS_AVG_DELAY_PROG_READY,
                                         "chorus program interface",
                                         100);

}

bool tulip_dsp::program_chorus_lfo_depth(vector<uint32_t>& lfo_depth)
{

  return tulip_dsp::program_tulip_uint32(lfo_depth,
                                         TULIP_DSP_CONTROL,
                                         TULIP_DSP_CONTROL_SW_RESETN_CHORUS,
                                         TULIP_DSP_CHORUS_LFO_DEPTH,
                                         TULIP_DSP_STATUS,
                                         TULIP_DSP_STATUS_CHORUS_LFO_DEPTH_PROG_DONE,
                                         TULIP_DSP_STATUS_CHORUS_LFO_DEPTH_PROG_READY,
                                         "chorus program interface",
                                         100);

}

bool tulip_dsp::program_chorus_lfo_freq(vector<int32_t>& chorus_lfo_freq)
{

  return tulip_dsp::program_tulip_int32(chorus_lfo_freq,
                                        TULIP_DSP_CONTROL,
                                        TULIP_DSP_CONTROL_SW_RESETN_CHORUS,
                                        TULIP_DSP_CHORUS_LFO_FREQ,
                                        TULIP_DSP_STATUS,
                                        TULIP_DSP_STATUS_CHORUS_LFO_FREQ_PROG_DONE,
                                        TULIP_DSP_STATUS_CHORUS_LFO_FREQ_PROG_READY,
                                        "chorus program interface",
                                        100);

}

bool tulip_dsp::program_tulip_int32(vector<int32_t>& input_prog_data,
                                    uint32_t dsp_control_reg,
                                    uint32_t resetn_bit,
                                    uint32_t prog_reg,
                                    uint32_t status_reg,
                                    uint32_t done_bit,
                                    uint32_t ready_bit,
                                    string status_text,
                                    uint32_t timeout_lim_ms)
{
  bool prog_done;

  union u32_sign_unsign
  {
    int32_t signed_;
    uint32_t unsigned_;
  };

  u32_sign_unsign reg_val_s_u;

  int i = 0;
  int timeout = 0;

  if (tulip_dsp::read_tulip_reg(status_reg) & done_bit)
  {
    tulip_dsp::write_tulip_reg_clear_bits(dsp_control_reg, resetn_bit);
    while(1)
    {
      if ((tulip_dsp::read_tulip_reg(status_reg) & done_bit) == 0)
      {
        tulip_dsp::write_tulip_reg_set_bits(dsp_control_reg, resetn_bit);
        break;
      }
      else
      {
        timeout++;
        tulip_dsp::delay_ms(1);
        if (timeout > timeout_lim_ms)
        {
          cerr << status_text << " did not reset correctly\n";
          return false;
        }
      }
    }
  }

  i = 0;
  timeout = 0;

  while (i < input_prog_data.size())
  {
    if (tulip_dsp::read_tulip_reg(status_reg) & ready_bit)
    {
      reg_val_s_u.signed_ = input_prog_data[i];
      tulip_dsp::write_tulip_reg(prog_reg, reg_val_s_u.unsigned_);
      timeout = 0;
      i++;
    }
    else
    {
      timeout++;
      tulip_dsp::delay_ms(1);
      if (timeout > timeout_lim_ms)
      {
        cerr << status_text << " did not assert \"ready\"\n";
        return false;
      }
    }
  }

  prog_done = tulip_dsp::read_tulip_reg(status_reg) & done_bit;
  for (i = 0 ; i < timeout_lim_ms ; i++)
  {
    if (prog_done > 0)
    {
      return true;
    }
    tulip_dsp::delay_ms(1);
    prog_done = tulip_dsp::read_tulip_reg(status_reg) & done_bit;
  }

  cerr << status_text << " did not assert \"done\"\n";
  return false;
}

bool tulip_dsp::program_tulip_uint32(vector<uint32_t>& input_prog_data,
                                     uint32_t dsp_control_reg,
                                     uint32_t resetn_bit,
                                     uint32_t prog_reg,
                                     uint32_t status_reg,
                                     uint32_t done_bit,
                                     uint32_t ready_bit,
                                     string status_text,
                                     uint32_t timeout_lim_ms)
{
  bool prog_done;

  int i = 0;
  int timeout = 0;

  if (tulip_dsp::read_tulip_reg(status_reg) & done_bit)
  {
    tulip_dsp::write_tulip_reg_clear_bits(dsp_control_reg, resetn_bit);
    while(1)
    {
      if ((tulip_dsp::read_tulip_reg(status_reg) & done_bit) == 0)
      {
        tulip_dsp::write_tulip_reg_set_bits(dsp_control_reg, resetn_bit);
        break;
      }
      else
      {
        timeout++;
        tulip_dsp::delay_ms(1);
        if (timeout > timeout_lim_ms)
        {
          cerr << status_text << " did not reset correctly\n";
          return false;
        }
      }
    }
  }

  i = 0;
  timeout = 0;

  while (i < input_prog_data.size())
  {
    if (tulip_dsp::read_tulip_reg(status_reg) & ready_bit)
    {
      tulip_dsp::write_tulip_reg(prog_reg, input_prog_data[i]);
      timeout = 0;
      i++;
    }
    else
    {
      timeout++;
      tulip_dsp::delay_ms(1);
      if (timeout > timeout_lim_ms)
      {
        cerr << status_text << " did not assert \"ready\"\n";
        return false;
      }
    }
  }

  prog_done = tulip_dsp::read_tulip_reg(status_reg) & done_bit;
  for (i = 0 ; i < timeout_lim_ms ; i++)
  {
    if (prog_done > 0)
    {
      return true;
    }
    tulip_dsp::delay_ms(1);
    prog_done = tulip_dsp::read_tulip_reg(status_reg) & done_bit;
  }

  cerr << status_text << " did not assert \"done\"\n";
  return false;
}

uint32_t tulip_dsp::write_tulip_reg_set_bits(uint32_t reg_address, uint32_t bits_mask)
{
  void *virt_addr;
  uint32_t reg_return;

  reg_return = tulip_dsp::read_tulip_reg(reg_address);
  reg_return = reg_return | bits_mask;
  tulip_dsp::write_tulip_reg(reg_address, reg_return);
  return reg_return;
}

uint32_t tulip_dsp::write_tulip_reg_clear_bits(uint32_t reg_address, uint32_t bits_mask)
{
  void *virt_addr;
  uint32_t reg_return;

  reg_return = tulip_dsp::read_tulip_reg(reg_address);
  reg_return = reg_return & ~bits_mask;
  tulip_dsp::write_tulip_reg(reg_address, reg_return);
  return reg_return;
}

uint32_t tulip_dsp::write_tulip_reg_bit(uint32_t reg_address, uint8_t bit_val, uint8_t bit_position)
{
  void *virt_addr;
  uint32_t reg_return;

  reg_return = tulip_dsp::read_tulip_reg(reg_address);
  if (bit_val > 0)
  {
    reg_return = reg_return | (1 << bit_position);
  }
  else
  {
    reg_return = reg_return & ~(1 << bit_position);
  }
  tulip_dsp::write_tulip_reg(reg_address, reg_return);
  return reg_return;
}

uint32_t tulip_dsp::modify_tulip_reg(uint32_t reg_address, uint32_t reg_val, uint32_t reg_mask)
{
  void *virt_addr;
  uint32_t reg_return;

  reg_return = tulip_dsp::read_tulip_reg(reg_address);

  reg_return = reg_return & (~reg_mask);
  reg_return = reg_return | (reg_mask & reg_val);

  tulip_dsp::write_tulip_reg(reg_address, reg_return);
  return reg_return;
}

void tulip_dsp::write_tulip_reg(uint32_t reg_address, uint32_t reg_val)
{
  void *virt_addr;
  virt_addr = (char *)map_base + reg_address;
  *(volatile uint32_t*)virt_addr = reg_val;
}

uint32_t tulip_dsp::read_tulip_reg(uint32_t reg_address)
{
  void *virt_addr;
  uint32_t reg_val;
  virt_addr = (char *)map_base + reg_address;
  reg_val = *(volatile uint32_t*)virt_addr;
  return reg_val;
}

void tulip_dsp::delay_ms(uint64_t delay_time_ms)
{
  uint64_t time_counter;

  milliseconds ms = duration_cast< milliseconds >(
    system_clock::now().time_since_epoch()
  );

  time_counter = (uint64_t)ms.count();
  while(((uint64_t)ms.count() - time_counter) < delay_time_ms)
  {
    ms = duration_cast< milliseconds >(
      system_clock::now().time_since_epoch()
    );
  }

}
