#ifndef AXI_REGISTER_DRIVER_H
#define AXI_REGISTER_DRIVER_H

#include <stdbool.h>
#include <stdint.h>
#include <vector>
#include <string>

using namespace std;

class axi_register_driver
{
	public:
		axi_register_driver(uint32_t axi_register_address);
		~axi_register_driver();

    uint32_t write_reg_clear_bits(uint32_t reg_address, uint32_t bits_mask);
    uint32_t write_reg_set_bits(uint32_t reg_address, uint32_t bits_mask);
    uint32_t write_reg_bit(uint32_t reg_address, uint8_t bit_val, uint8_t bit_position);
    uint32_t modify_reg(uint32_t reg_address, uint32_t reg_val, uint32_t reg_mask);
    void write_reg(uint32_t reg_address, uint32_t reg_val);
    uint32_t read_reg(uint32_t reg_address);

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
