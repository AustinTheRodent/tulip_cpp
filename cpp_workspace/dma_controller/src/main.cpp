//#include <iostream>
//#include <fstream>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
//#include <unistd.h>
//#include <fcntl.h>
//#include <termios.h>
//#include <sys/mman.h>
#include <cstring>
#include "dma_controller.h"

#define DMA_BASEADDR 0x80020000

int main()
{
  int ret;
  const uint8_t tx_dma_id = 0;
  const uint8_t rx_dma_id = 1;
  const uint32_t tx_dma_buff_sz_bytes = 256;
  const uint32_t rx_dma_buff_sz_bytes = 256;
  axi_register_driver axis_sniffer(0x80011000);
  axis_sniffer.write_reg(0, 0);
  axis_sniffer.write_reg(0, 1);
  dma_controller dma_test(DMA_BASEADDR, DUAL_TYPE, tx_dma_id, tx_dma_buff_sz_bytes, rx_dma_id, rx_dma_buff_sz_bytes);

  vector <uint32_t> tx_array(tx_dma_buff_sz_bytes/4);
  vector <uint32_t> rx_array(tx_dma_buff_sz_bytes/4);
  for (int i = 0 ; i < tx_array.size() ; i++)
  {
    tx_array[i] = i;
  }

  dma_test.set_tx_blocking(true);
  dma_test.set_rx_blocking(false);
  dma_test.set_tx_buffer(&tx_array[0], tx_dma_buff_sz_bytes);
  dma_test.launch_rx(tx_dma_buff_sz_bytes);
  dma_test.launch_tx(tx_dma_buff_sz_bytes);
  ret = dma_test.wait_for_rx_dma_done(100);
  printf("wait_for_rx_dma_done: %i\n", ret);
  printf("Test Done\n");


  //test second shot:

  printf("Transact Count: %i\n", axis_sniffer.read_reg(0x004));
  printf("First_val: 0x%X\n\n", axis_sniffer.read_reg(0x008));

  axis_sniffer.write_reg(0, 0);
  axis_sniffer.write_reg(0, 1);
  tx_array[0] = 0x100;

  dma_test.set_tx_buffer(&tx_array[0], tx_dma_buff_sz_bytes);
  dma_test.launch_rx(tx_dma_buff_sz_bytes);
  dma_test.launch_tx(tx_dma_buff_sz_bytes);
  ret = dma_test.wait_for_rx_dma_done(100);
  dma_test.get_rx_buffer(&rx_array[0], tx_dma_buff_sz_bytes);
  printf("wait_for_rx_dma_done: %i\n", ret);
  printf("Test Done\n");

  printf("Transact Count: %i\n", axis_sniffer.read_reg(0x004));
  printf("First_val: 0x%X\n", axis_sniffer.read_reg(0x008));
  printf("rx_array[0]: 0x%X\n", rx_array[0]);

  return 0;

}


