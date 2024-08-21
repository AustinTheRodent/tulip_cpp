
#include <cmath>
#include <tgmath.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <chrono>
#include <vector>
#include <iostream>
#include "footpedal_gpio.h"

using namespace std::chrono;
using namespace std;

footpedal_gpio::footpedal_gpio()
{
  // Constructor
  pagesize = sysconf(_SC_PAGE_SIZE);
  offset = FOOTPEDAL_GPIO_ADDRESS;
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

footpedal_gpio::~footpedal_gpio()
{
  // destructor
  munmap((void *)map_base, pagesize);
  close(mem_fd);
}

uint32_t footpedal_gpio::read_gpio(void)
{
  const uint32_t reg_address = 0;
  void *virt_addr;
  uint32_t reg_val;
  virt_addr = (char *)map_base + reg_address;
  reg_val = *(volatile uint32_t*)virt_addr;
  return reg_val;
}

void footpedal_gpio::delay_ms(uint64_t delay_time_ms)
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
