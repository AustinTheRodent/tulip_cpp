#include <iostream>
#include <fstream>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <sys/mman.h>
#include <cstring>
#include "axi_register_driver.h"


#define MM2S_CONTROL_REGISTER 0x00
#define MM2S_STATUS_REGISTER 0x04
#define MM2S_START_ADDRESS 0x18
#define MM2S_START_ADDRESS_MSB 0x1C
#define MM2S_LENGTH 0x28

#define S2MM_CONTROL_REGISTER 0x30
#define S2MM_STATUS_REGISTER 0x34
#define S2MM_DESTINATION_ADDRESS 0x48
#define S2MM_DESTINATION_ADDRESS_MSB 0x4C
#define S2MM_LENGTH 0x58

#define DMA_BASEADDR 0x80020000

uint32_t dma_set(uint32_t* dma_virtual_address, int offset, uint32_t value);
uint32_t dma_get(uint32_t* dma_virtual_address, int offset);
int dma_mm2s_sync(uint32_t* dma_virtual_address);
int dma_s2mm_sync(uint32_t* dma_virtual_address);
void dma_s2mm_status(uint32_t* dma_virtual_address);
void dma_mm2s_status(uint32_t* dma_virtual_address);
void memdump(void* dma_address, int byte_count);
uint64_t get_physcial_address(void *dma_address);

int main()
{

  system("sudo insmod /lib/modules/5.15.0-1025-xilinx-zynqmp/build/udmabuf-master/u-dma-buf.ko udmabuf0=8388608 udmabuf1=8388608");

  axi_register_driver axis_sniffer(0x80011000);
  printf("Scratch Pad: 0x%X\n", axis_sniffer.read_reg(0x014));
  axis_sniffer.write_reg(0, 0);
  axis_sniffer.write_reg(0, 1);

  int dh = open("/dev/mem", O_RDWR | O_SYNC); // Open /dev/mem which represents the whole physical memory
  uint32_t* dma_address = (uint32_t*)mmap(NULL, 4096, PROT_READ | PROT_WRITE, MAP_SHARED, dh, DMA_BASEADDR); // Memory map AXI Lite register block

  uint8_t src_dma_word_sz_bytes = 4; 
  int src_dmabuff_fd;
  int dest_dmabuff_fd;
  uint32_t* src_dmabuff;
  uint32_t* dest_dmabuff;
  uint32_t* dest_cachebuff;
  uint32_t buf_size_bytes = 32;
  if ((src_dmabuff_fd  = open("/dev/udmabuf1", O_RDWR | O_SYNC)) == -1)
  {
    printf("Could not open /dev/udmabuf1\n");
    return 0;
  }
  if ((dest_dmabuff_fd  = open("/dev/udmabuf0", O_RDWR | O_SYNC)) == -1)
  {
    printf("Could not open /dev/udmabuf0\n");
    return 0;
  }
  src_dmabuff = (uint32_t*)mmap(NULL, buf_size_bytes, PROT_READ|PROT_WRITE, MAP_SHARED, src_dmabuff_fd, 0);
  dest_dmabuff = (uint32_t*)mmap(NULL, buf_size_bytes, PROT_READ|PROT_WRITE, MAP_SHARED, dest_dmabuff_fd, 0);
  /* Do some read/write access to dest_dmabuff */

  ifstream src_inputFile("/sys/class/u-dma-buf/udmabuf1/phys_addr");
  if (!src_inputFile.is_open())
  { 
    printf("Error opening /sys/class/u-dma-buf/udmabuf1/phys_addr\n");
    return 0; 
  }

  string line;
  uint64_t src_dmabuff_physical_address;
  getline(src_inputFile, line);
  src_dmabuff_physical_address = strtol(line.c_str(), NULL, 16);
  cout << "DMA src Buff Physical Address: " << line << "\n";
  printf("DMA src Buff Physical Address: 0x%016lX\n", src_dmabuff_physical_address);
  src_inputFile.close();


  ifstream inputFile("/sys/class/u-dma-buf/udmabuf0/phys_addr");
  if (!inputFile.is_open())
  { 
    printf("Error opening /sys/class/u-dma-buf/udmabuf0/phys_addr\n");
    return 0; 
  }

  uint64_t dest_dmabuff_physical_address;
  getline(inputFile, line);
  dest_dmabuff_physical_address = strtol(line.c_str(), NULL, 16);
  cout << "DMA dest Buff Physical Address: " << line << "\n";
  printf("DMA dest Buff Physical Address: 0x%016lX\n", dest_dmabuff_physical_address);
  inputFile.close();


  //memset(dest_dmabuff, 0, 32); // Clear destination block
  for (int i = 0 ; i < buf_size_bytes/src_dma_word_sz_bytes ; i++)
  {
    src_dmabuff[i] = i*2;
  }

  dma_set(dma_address, S2MM_STATUS_REGISTER, 0xFFFFFFFF);
  dma_set(dma_address, MM2S_STATUS_REGISTER, 0xFFFFFFFF);


  printf("Resetting DMA\n");
  dma_set(dma_address, S2MM_CONTROL_REGISTER, 4);
  dma_set(dma_address, MM2S_CONTROL_REGISTER, 4);
  dma_s2mm_status(dma_address);
  dma_mm2s_status(dma_address);

  printf("Halting DMA\n");
  dma_set(dma_address, S2MM_CONTROL_REGISTER, 0);
  dma_set(dma_address, MM2S_CONTROL_REGISTER, 0);

  dma_s2mm_status(dma_address);
  dma_mm2s_status(dma_address);

  printf("MM2S_CONTROL_REGISTER: 0x%08X\n", dma_get(dma_address, MM2S_CONTROL_REGISTER));
  printf("S2MM_CONTROL_REGISTER: 0x%08X\n", dma_get(dma_address, S2MM_CONTROL_REGISTER));

  printf("Starting S2MM channel with all interrupts masked...\n");
  dma_set(dma_address, S2MM_CONTROL_REGISTER, 0xf001);
  dma_s2mm_status(dma_address);

  printf("Starting MM2S channel with all interrupts masked...\n");
  dma_set(dma_address, MM2S_CONTROL_REGISTER, 0xf001);
  dma_mm2s_status(dma_address);

  printf("MM2S_CONTROL_REGISTER: 0x%08X\n", dma_get(dma_address, MM2S_CONTROL_REGISTER));
  printf("S2MM_CONTROL_REGISTER: 0x%08X\n", dma_get(dma_address, S2MM_CONTROL_REGISTER));

  printf("Writing source address...\n");
  dma_set(dma_address, MM2S_START_ADDRESS, (uint64_t)(src_dmabuff_physical_address) & 0xFFFFFFFF); // Write source address
  dma_set(dma_address, MM2S_START_ADDRESS_MSB, (uint64_t)(src_dmabuff_physical_address) >> 32); // Write source address
  dma_mm2s_status(dma_address);

  printf("Writing destination address...\n");
  dma_set(dma_address, S2MM_DESTINATION_ADDRESS, (uint64_t)(dest_dmabuff_physical_address) & 0xFFFFFFFF); // Write destination address
  dma_set(dma_address, S2MM_DESTINATION_ADDRESS_MSB, (uint64_t)(dest_dmabuff_physical_address) >> 32); // Write destination address
  dma_mm2s_status(dma_address);

  printf("Reading source address: ");
  printf("0x%08X%08X\n", dma_get(dma_address, MM2S_START_ADDRESS_MSB), dma_get(dma_address, MM2S_START_ADDRESS));
  printf("Reading destination address: ");
  printf("0x%08X%08X\n", dma_get(dma_address, S2MM_DESTINATION_ADDRESS_MSB), dma_get(dma_address, S2MM_DESTINATION_ADDRESS));

  printf("MM2S_CONTROL_REGISTER: 0x%08X\n", dma_get(dma_address, MM2S_CONTROL_REGISTER));
  printf("S2MM_CONTROL_REGISTER: 0x%08X\n", dma_get(dma_address, S2MM_CONTROL_REGISTER));


  printf("Writing S2MM transfer length...\n");
  dma_set(dma_address, S2MM_LENGTH, buf_size_bytes);
  dma_s2mm_status(dma_address);

  printf("Writing MM2S transfer length...\n");
  dma_set(dma_address, MM2S_LENGTH, buf_size_bytes);
  dma_mm2s_status(dma_address);

  printf("Waiting for MM2S synchronization...\n");
  dma_mm2s_sync(dma_address);

  printf("Waiting for S2MM sychronization...\n");
  dma_s2mm_sync(dma_address); // If this locks up make sure all memory ranges are assigned under Address Editor!

  dma_s2mm_status(dma_address);
  dma_mm2s_status(dma_address);

  dest_cachebuff = (uint32_t*)malloc(buf_size_bytes);
  memcpy(dest_cachebuff, dest_dmabuff, buf_size_bytes);
  for (int i = 0 ; i < buf_size_bytes/src_dma_word_sz_bytes ; i++)
  {
    printf("dest_cachebuff[%i]: %i\n", i, dest_cachebuff[i]);
  }
  free(dest_cachebuff);

  printf("Transact Count: %i\n", axis_sniffer.read_reg(0x004));
  printf("First_val: 0x%X\n", axis_sniffer.read_reg(0x008));

  munmap(dma_address, 4096);
  munmap(src_dmabuff, buf_size_bytes);
  munmap(dest_dmabuff, buf_size_bytes);

  close(dh);
  close(src_dmabuff_fd);
  close(dest_dmabuff_fd);

  system("sudo rmmod u-dma-buf");

  return 0;

}



uint32_t dma_set(uint32_t* dma_virtual_address, int offset, uint32_t value) {
    dma_virtual_address[offset>>2] = value;
    return 0;
}

uint32_t dma_get(uint32_t* dma_virtual_address, int offset) {
    return dma_virtual_address[offset>>2];
}

int dma_mm2s_sync(uint32_t* dma_virtual_address) {
    uint32_t mm2s_status =  dma_get(dma_virtual_address, MM2S_STATUS_REGISTER);
    while(!(mm2s_status & 1<<12) || !(mm2s_status & 1<<1) ){
        dma_s2mm_status(dma_virtual_address);
        dma_mm2s_status(dma_virtual_address);

        mm2s_status =  dma_get(dma_virtual_address, MM2S_STATUS_REGISTER);
    }
    return 0;
}

int dma_s2mm_sync(uint32_t* dma_virtual_address) {
    uint32_t s2mm_status = dma_get(dma_virtual_address, S2MM_STATUS_REGISTER);
    while(!(s2mm_status & 1<<12) || !(s2mm_status & 1<<1)){//IOC_Irq || idle
        dma_s2mm_status(dma_virtual_address);
        dma_mm2s_status(dma_virtual_address);

        s2mm_status = dma_get(dma_virtual_address, S2MM_STATUS_REGISTER);
    }
    return 0;
}

void dma_s2mm_status(uint32_t* dma_virtual_address) {
    uint32_t status = dma_get(dma_virtual_address, S2MM_STATUS_REGISTER);
    printf("Stream to memory-mapped status (0x%08x@0x%02x):", status, S2MM_STATUS_REGISTER);
    if (status & 0x00000001) printf(" halted"); else printf(" running");
    if (status & 0x00000002) printf(" idle");
    if (status & 0x00000008) printf(" SGIncld");
    if (status & 0x00000010) printf(" DMAIntErr");
    if (status & 0x00000020) printf(" DMASlvErr");
    if (status & 0x00000040) printf(" DMADecErr");
    if (status & 0x00000100) printf(" SGIntErr");
    if (status & 0x00000200) printf(" SGSlvErr");
    if (status & 0x00000400) printf(" SGDecErr");
    if (status & 0x00001000) printf(" IOC_Irq");
    if (status & 0x00002000) printf(" Dly_Irq");
    if (status & 0x00004000) printf(" Err_Irq");
    printf("\n");
}

void dma_mm2s_status(uint32_t* dma_virtual_address) {
    uint32_t status = dma_get(dma_virtual_address, MM2S_STATUS_REGISTER);
    printf("Memory-mapped to stream status (0x%08x@0x%02x):", status, MM2S_STATUS_REGISTER);
    if (status & 0x00000001) printf(" halted"); else printf(" running");
    if (status & 0x00000002) printf(" idle");
    if (status & 0x00000008) printf(" SGIncld");
    if (status & 0x00000010) printf(" DMAIntErr");
    if (status & 0x00000020) printf(" DMASlvErr");
    if (status & 0x00000040) printf(" DMADecErr");
    if (status & 0x00000100) printf(" SGIntErr");
    if (status & 0x00000200) printf(" SGSlvErr");
    if (status & 0x00000400) printf(" SGDecErr");
    if (status & 0x00001000) printf(" IOC_Irq");
    if (status & 0x00002000) printf(" Dly_Irq");
    if (status & 0x00004000) printf(" Err_Irq");
    printf("\n");
}

void memdump(void* dma_address, int byte_count) {
    char* p = (char*)dma_address;
    int offset;
    for (offset = 0; offset < byte_count; offset++) {
        printf("%02x", p[offset]);
        if (offset % 4 == 3) { printf(" "); }
    }
    printf("\n");
}

uint64_t get_physcial_address(void *dma_address)
{
    const uint64_t page_size = getpagesize();
    const uint64_t page_length = 8;
    const uint64_t page_shift = 12;
    uint64_t page_offset, page_number;
    int pagemap;

    pagemap = open("/proc/self/pagemap", O_RDONLY);
    if(pagemap < 0)
    {
        return 0;
    }

    page_offset = (((uint64_t)dma_address) / page_size * page_length);
    if(lseek(pagemap, page_offset, SEEK_SET) != page_offset)
    {
        close(pagemap);
        return 0;
    }

    page_number = 0;
    if(read(pagemap, &page_number, sizeof(page_number)) != sizeof(page_number))
    {
        close(pagemap);
        return 0;
    }
    page_number &= 0x7FFFFFFFFFFFFFULL;

    close(pagemap);

    return ((page_number << page_shift) + (((uint64_t)dma_address) % page_size));
}
