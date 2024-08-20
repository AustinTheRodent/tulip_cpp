#ifndef FOOTPEDAL_GPIO_H
#define FOOTPEDAL_GPIO_H

#include <stdbool.h>
#include <stdint.h>
#include <vector>

using namespace std;

#define FOOTPEDAL_GPIO_ADDRESS 0x80000000

class footpedal_gpio
{
	public:
		footpedal_gpio();
		~footpedal_gpio();

    uint32_t read_gpio(void);
    void delay_ms(uint64_t delay_time_ms);

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
