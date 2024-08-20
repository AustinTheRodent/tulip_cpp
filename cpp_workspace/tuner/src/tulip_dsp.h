#ifndef TULIP_DSP_H
#define TULIP_DSP_H

#include <stdbool.h>
#include <stdint.h>
#include <vector>
#include <string>

using namespace std;

#define TULIP_ADDRESS 0x80010000

class tulip_dsp
{
	public:
		tulip_dsp();
		~tulip_dsp();

		void reset();
    //void enable(bool lut_is_symmetric);
    void bypass(void);
		void delay_ms(uint64_t delay_time_ms);

    bool program_LUT_transfer_function(vector<uint32_t>& lut_entries);
    bool program_FIR_filter(vector<int32_t>& FIR_taps);
    bool program_reverb_delay_profile(vector<int32_t>& FIR_taps);
    void program_reverb_feedforward_gain_linear(float gain_linear); // (2.0,0.0]
    void program_reverb_feedback_gain_linear(float gain_linear); // (2.0,0.0]
    void program_reverb_feedback_right_shift(uint8_t right_shift);
    void program_input_gain_db(float gain_db);
    void program_input_gain_linear(float gain_linear);
    void program_output_gain_db(float gain_db);
    void program_output_gain_linear(float gain_linear);
    bool program_vibrato_gain(vector<uint32_t>& vibrato_gain_array);
    bool program_vibrato_chirp_depth(vector<uint32_t>& vibrato_chirp_depth_array);
    bool program_vibrato_freq_deriv(vector<int32_t>& vibrato_freq_deriv_array);
    bool program_vibrato_freq_offset(vector<int32_t>& vibrato_freq_offset_array);
    bool program_chorus_gain(vector<uint32_t>& chorus_gain_array);
    bool program_chorus_avg_delay(vector<uint32_t>& chorus_avg_delay);
    bool program_chorus_lfo_depth(vector<uint32_t>& lfo_depth);
    bool program_chorus_lfo_freq(vector<int32_t>& chorus_lfo_freq);

    bool program_tulip_int32(vector<int32_t>& input_prog_data,
                             uint32_t dsp_control_reg,
                             uint32_t resetn_bit,
                             uint32_t prog_reg,
                             uint32_t status_reg,
                             uint32_t done_bit,
                             uint32_t ready_bit,
                             string status_text,
                             uint32_t timeout_lim_ms);

    bool program_tulip_uint32(vector<uint32_t>& input_prog_data,
                              uint32_t dsp_control_reg,
                              uint32_t resetn_bit,
                              uint32_t prog_reg,
                              uint32_t status_reg,
                              uint32_t done_bit,
                              uint32_t ready_bit,
                              string status_text,
                              uint32_t timeout_lim_ms);

    uint32_t write_tulip_reg_clear_bits(uint32_t reg_address, uint32_t bits_mask);
    uint32_t write_tulip_reg_set_bits(uint32_t reg_address, uint32_t bits_mask);
    uint32_t write_tulip_reg_bit(uint32_t reg_address, uint8_t bit_val, uint8_t bit_position);
    uint32_t modify_tulip_reg(uint32_t reg_address, uint32_t reg_val, uint32_t reg_mask);
    void write_tulip_reg(uint32_t reg_address, uint32_t reg_val);
    uint32_t read_tulip_reg(uint32_t reg_address);

	private:

    uint64_t offset = 0;
    int pagesize;
    uint64_t page_base;
    uint64_t page_offset;
    int offset_in_page;

    void *map_base;
    int mem_fd;

};
#endif
