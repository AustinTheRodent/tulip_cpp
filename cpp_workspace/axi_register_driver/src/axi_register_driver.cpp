
#include <cmath>
#include <tgmath.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <chrono>
#include <vector>
#include <iostream>
#include "axi_register_driver.h"

//using namespace std::chrono;
using namespace std;

axi_register_driver::axi_register_driver(uint32_t axi_register_address)
{
  // Constructor
  pagesize = sysconf(_SC_PAGE_SIZE);
  offset = axi_register_address;
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

axi_register_driver::~axi_register_driver()
{
  // destructor
  munmap((void *)map_base, pagesize);
  close(mem_fd);
}

uint32_t axi_register_driver::write_reg_set_bits(uint32_t reg_address, uint32_t bits_mask)
{
  void *virt_addr;
  uint32_t reg_return;

  reg_return = axi_register_driver::read_reg(reg_address);
  reg_return = reg_return | bits_mask;
  axi_register_driver::write_reg(reg_address, reg_return);
  return reg_return;
}

uint32_t axi_register_driver::write_reg_clear_bits(uint32_t reg_address, uint32_t bits_mask)
{
  void *virt_addr;
  uint32_t reg_return;

  reg_return = axi_register_driver::read_reg(reg_address);
  reg_return = reg_return & ~bits_mask;
  axi_register_driver::write_reg(reg_address, reg_return);
  return reg_return;
}

uint32_t axi_register_driver::write_reg_bit(uint32_t reg_address, uint8_t bit_val, uint8_t bit_position)
{
  void *virt_addr;
  uint32_t reg_return;

  reg_return = axi_register_driver::read_reg(reg_address);
  if (bit_val > 0)
  {
    reg_return = reg_return | (1 << bit_position);
  }
  else
  {
    reg_return = reg_return & ~(1 << bit_position);
  }
  axi_register_driver::write_reg(reg_address, reg_return);
  return reg_return;
}

uint32_t axi_register_driver::modify_reg(uint32_t reg_address, uint32_t reg_val, uint32_t reg_mask)
{
  void *virt_addr;
  uint32_t reg_return;

  reg_return = axi_register_driver::read_reg(reg_address);

  reg_return = reg_return & (~reg_mask);
  reg_return = reg_return | (reg_mask & reg_val);

  axi_register_driver::write_reg(reg_address, reg_return);
  return reg_return;
}

void axi_register_driver::write_reg(uint32_t reg_address, uint32_t reg_val)
{
  void *virt_addr;
  virt_addr = (char *)map_base + reg_address;
  *(volatile uint32_t*)virt_addr = reg_val;
}

uint32_t axi_register_driver::read_reg(uint32_t reg_address)
{
  void *virt_addr;
  uint32_t reg_val;
  virt_addr = (char *)map_base + reg_address;
  reg_val = *(volatile uint32_t*)virt_addr;
  return reg_val;
}
