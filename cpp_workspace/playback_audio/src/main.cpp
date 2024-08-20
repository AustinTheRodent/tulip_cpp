#include <pthread.h>
#include <iostream>
#include <sys/mman.h>
#include <fcntl.h>
#include <tgmath.h>
#include <unistd.h>
#include<stdio.h>
#include "KR260_TULIP_REGISTERS.h"

#define TULIP_ADDRESS 0x80010000
#define FIFO_FILL_FLAG 256
#define MAX_SAMPS (12500000-2048)

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

int main()
{
  int i;
  void *virt_addr;
  uint64_t offset = 0;
  int pagesize;
  uint64_t page_base;
  uint64_t page_offset;
  int offset_in_page;
  uint32_t capture_size_bytes;
  uint32_t capture_size_samples;
  uint32_t samples_read = 0;
  uint32_t samples_to_read;
  uint32_t control_reg_read_result;
  uint32_t ps_2_i2s_fifo_avail;
  uint32_t ps_2_i2s_lr_chan[2*FIFO_FILL_FLAG];
  uint32_t write_value;
  FILE *ptr;

  void *map_base;
  int mem_fd;

  pthread_t tId;

  pagesize = sysconf(_SC_PAGE_SIZE);
  offset = TULIP_ADDRESS;
  page_base = (offset / pagesize) * pagesize;
  page_offset = offset - page_base;

  if ((mem_fd = open("/dev/mem", O_RDWR | O_SYNC)) == -1) {
    perror("Error opening /dev/mem");
    return EXIT_FAILURE;
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
    return EXIT_FAILURE;
  }

  virt_addr = (char *)map_base + CONTROL;
  control_reg_read_result = *(volatile uint32_t*)virt_addr;

  if ( ((control_reg_read_result & CONTROL_SW_RESETN) == 0) || ((control_reg_read_result & CONTROL_I2S_ENABLE) == 0) )
  {
    printf("Tulip I2S is not enabled, did you initialize the codec?\n");
    munmap((void *)map_base, pagesize);
    close(mem_fd);
    return 0;
  }

  write_value = control_reg_read_result | CONTROL_PS_2_I2S_ENABLE;
  *(volatile uint32_t*)virt_addr = write_value;

  ptr = fopen("/home/ubuntu/captures/capture.bin","rb");
  if (ptr == NULL)
  {
    perror("Failed: could not open /home/ubuntu/captures/capture.bin\n");

    munmap((void *)map_base, pagesize);
    close(mem_fd);

    write_value = control_reg_read_result & (~CONTROL_PS_2_I2S_ENABLE);
    *(volatile uint32_t*)virt_addr = write_value;

    return 0;
  }
  fseek(ptr, 0L, SEEK_END);
  capture_size_bytes = ftell(ptr);
  capture_size_samples = capture_size_bytes/8;
  rewind(ptr);

  (void) pthread_create(&tId, 0, userInput_thread, 0);

  printf("capture_size_samples: %u\n", capture_size_samples);

  while (keep_running)
  {
    virt_addr = (char *)map_base + PS_2_I2S_FIFO_COUNT;
    ps_2_i2s_fifo_avail = *(volatile uint32_t*)virt_addr;
    if (ps_2_i2s_fifo_avail >= FIFO_FILL_FLAG)
    {
      if (capture_size_samples - samples_read <= FIFO_FILL_FLAG)
      {
        samples_to_read = capture_size_samples - samples_read;
        keep_running = false;
      }
      else
      {
        samples_to_read = FIFO_FILL_FLAG;
      }
      fread(ps_2_i2s_lr_chan, sizeof(uint32_t), 2*samples_to_read, ptr);
      samples_read = samples_read + samples_to_read;
      for (i = 0 ; i < samples_to_read ; i++)
      {
        virt_addr = (char *)map_base + PS_2_I2S_FIFO_WRITE_L;
        *(volatile uint32_t*)virt_addr = ps_2_i2s_lr_chan[2*i];
        virt_addr = (char *)map_base + PS_2_I2S_FIFO_WRITE_R;
        *(volatile uint32_t*)virt_addr = ps_2_i2s_lr_chan[2*i+1];
      }
    }
  }


  fclose(ptr);
  printf("exiting\n");

  (void) pthread_join(tId, NULL);

  virt_addr = (char *)map_base + CONTROL;
  control_reg_read_result = *(volatile uint32_t*)virt_addr;
  write_value = control_reg_read_result & (~CONTROL_PS_2_I2S_ENABLE);
  *(volatile uint32_t*)virt_addr = write_value;

  munmap((void *)map_base, pagesize);
  close(mem_fd);

  return 0;
}
