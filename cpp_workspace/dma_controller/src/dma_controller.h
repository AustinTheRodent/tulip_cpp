#ifndef DMA_CONTROLLER_H
#define DMA_CONTROLLER_H

#include "axi_register_driver.h"

#include <stdbool.h>
#include <stdint.h>
//#include <vector>
#include <cstring>
//#include <string>

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

#define TX_TYPE 0 // PS -> PL
#define RX_TYPE 1 // PL -> PS
#define DUAL_TYPE 2

using namespace std;

typedef struct
{
  bool halted;    //if (status & 0x00000001) printf(" halted"); else printf(" running");
  bool idle;      //if (status & 0x00000002) printf(" idle");       idle
  bool SGIncld;   //if (status & 0x00000008) printf(" SGIncld");    SGIncld
  bool DMAIntErr; //if (status & 0x00000010) printf(" DMAIntErr");  DMAIntErr
  bool DMASlvErr; //if (status & 0x00000020) printf(" DMASlvErr");  DMASlvErr
  bool DMADecErr; //if (status & 0x00000040) printf(" DMADecErr");  DMADecErr
  bool SGIntErr;  //if (status & 0x00000100) printf(" SGIntErr");   SGIntErr
  bool SGSlvErr;  //if (status & 0x00000200) printf(" SGSlvErr");   SGSlvErr
  bool SGDecErr;  //if (status & 0x00000400) printf(" SGDecErr");   SGDecErr
  bool IOC_Irq;   //if (status & 0x00001000) printf(" IOC_Irq");    IOC_Irq
  bool Dly_Irq;   //if (status & 0x00002000) printf(" Dly_Irq");    Dly_Irq
  bool Err_Irq;   //if (status & 0x00004000) printf(" Err_Irq");    Err_Irq
} dma_status_t;

class dma_controller
{
	public:
    dma_controller
    (
      uint32_t axi_register_address,
      uint8_t dma_type, // 0 = TX (PS -> PL), 1 = RX, 2 = DUAL
      uint8_t tx_dma_id,
      uint32_t tx_dma_buff_sz_bytes,
      uint8_t rx_dma_id,
      uint32_t rx_dma_buff_sz_bytes,
      bool tx_is_blocking_=false,
      bool rx_is_blocking_=false
    );
		~dma_controller();
    int set_tx_buffer(void* input_buffer, uint32_t bytes_to_transfer);
    int get_rx_buffer(void* return_buffer, uint32_t bytes_to_transfer);
    void set_tx_blocking(bool is_blocking);
    void set_rx_blocking(bool is_blocking);
    int launch_tx(uint32_t bytes_to_transfer);
    int launch_rx(uint32_t bytes_to_transfer);
    dma_status_t get_tx_status();
    dma_status_t get_rx_status();
    int wait_for_tx_dma_done(uint32_t max_attempts=0);
    int wait_for_rx_dma_done(uint32_t max_attempts=0);
    bool check_tx_done();
    bool check_rx_done();
    void dma_reset();
    void dma_tx_reset();
    void dma_rx_reset();
    int set_dma_tx_address(uint64_t physical_address);
    int set_dma_rx_address(uint64_t physical_address);

	private:

    axi_register_driver dma_registers;

    int constructor_ret;
    uint8_t dma_type;
    int tx_dmabuff_fd;
    int rx_dmabuff_fd;
    void* tx_dmabuff;
    void* rx_dmabuff;
    uint64_t tx_dmabuff_physical_address;
    uint64_t rx_dmabuff_physical_address;
    uint32_t tx_dma_buff_sz_bytes;
    uint32_t rx_dma_buff_sz_bytes;
    bool tx_is_blocking;
    bool rx_is_blocking;

};
#endif
