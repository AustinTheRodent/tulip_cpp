
//#include <iostream>
#include <fstream>
//#include <stdlib.h>
#include <stdint.h>
//#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
//#include <termios.h>
#include <sys/mman.h>
//#include <cstring>

#include "axi_register_driver.h"
#include "dma_controller.h"

using namespace std;

dma_controller::dma_controller
(
  uint32_t axi_register_address,
  uint8_t dma_type_, // 0 = TX (PS -> PL), 1 = RX, 2 = DUAL
  uint8_t tx_dma_id,
  uint32_t tx_dma_buff_sz_bytes_,
  uint8_t rx_dma_id,
  uint32_t rx_dma_buff_sz_bytes_,
  bool tx_is_blocking_,
  bool rx_is_blocking_
):
  dma_registers(axi_register_address)
{
  string line;
  tx_is_blocking = tx_is_blocking_;
  rx_is_blocking = rx_is_blocking_;
  constructor_ret = 0;

  dma_type = dma_type_;
  tx_dma_buff_sz_bytes = tx_dma_buff_sz_bytes_;
  rx_dma_buff_sz_bytes = rx_dma_buff_sz_bytes_;

  // todo: format this system call:
  const string gen = "sudo insmod /lib/modules/5.15.0-1025-xilinx-zynqmp/build/udmabuf-master/u-dma-buf.ko ";
  const string tx_gen = "udmabuf" + to_string(tx_dma_id) + '=' + to_string(tx_dma_buff_sz_bytes);
  const string rx_gen = "udmabuf" + to_string(rx_dma_id) + '=' + to_string(rx_dma_buff_sz_bytes);
  if (dma_type == TX_TYPE)
  {
    //system("sudo insmod /lib/modules/5.15.0-1025-xilinx-zynqmp/build/udmabuf-master/u-dma-buf.ko udmabuf0=8388608 udmabuf1=8388608");
    system((gen + tx_gen).c_str());
  }
  else if (dma_type == RX_TYPE)
  {
    system((gen + rx_gen).c_str());
  }
  else
  {
    system((gen + tx_gen + ' ' + rx_gen).c_str());
  }


  if ((dma_type == TX_TYPE) || (dma_type == DUAL_TYPE))
  {
    string tx_dmabuff_fd_name = "/dev/udmabuf" + to_string(tx_dma_id);

    if ((tx_dmabuff_fd  = open(tx_dmabuff_fd_name.c_str(), O_RDWR | O_SYNC)) == -1)
    {
      printf("Could not open %s\n", tx_dmabuff_fd_name.c_str());
      constructor_ret = -1;
    }

    tx_dmabuff = mmap(NULL, tx_dma_buff_sz_bytes, PROT_READ|PROT_WRITE, MAP_SHARED, tx_dmabuff_fd, 0);

    string tx_dmabuff_sys = "/sys/class/u-dma-buf/udmabuf" + to_string(tx_dma_id);

    ifstream tx_inputFile(tx_dmabuff_sys + "/phys_addr");
    if (!tx_inputFile.is_open())
    { 
      printf("Error opening %s\n", (tx_dmabuff_sys + "/phys_addr").c_str());
      constructor_ret = -1; 
    }

    getline(tx_inputFile, line);
    tx_dmabuff_physical_address = strtol(line.c_str(), NULL, 16);
    tx_inputFile.close();

    dma_controller::dma_tx_reset();
  }

  ////////////////////////////////////////////////////////////////

  if ((dma_type == RX_TYPE) || (dma_type == DUAL_TYPE))
  {
    string rx_dmabuff_fd_name = "/dev/udmabuf" + to_string(rx_dma_id);

    if ((rx_dmabuff_fd  = open(rx_dmabuff_fd_name.c_str(), O_RDWR | O_SYNC)) == -1)
    {
      printf("Could not open %s\n", rx_dmabuff_fd_name.c_str());
      constructor_ret = -1;
    }

    rx_dmabuff = mmap(NULL, rx_dma_buff_sz_bytes, PROT_READ|PROT_WRITE, MAP_SHARED, rx_dmabuff_fd, 0);

    string rx_dmabuff_sys = "/sys/class/u-dma-buf/udmabuf" + to_string(rx_dma_id);

    ifstream rx_inputFile(rx_dmabuff_sys + "/phys_addr");
    if (!rx_inputFile.is_open())
    { 
      printf("Error opening %s\n", (rx_dmabuff_sys + "/phys_addr").c_str());
      constructor_ret = -1; 
    }

    getline(rx_inputFile, line);
    rx_dmabuff_physical_address = strtol(line.c_str(), NULL, 16);
    rx_inputFile.close();

    dma_controller::dma_rx_reset();
  }

}

dma_controller::~dma_controller()
{
  if (dma_type == TX_TYPE)
  {
    munmap(tx_dmabuff, tx_dma_buff_sz_bytes);
    close(tx_dmabuff_fd);
  }
  else if (dma_type == RX_TYPE)
  {
    munmap(rx_dmabuff, rx_dma_buff_sz_bytes);
    close(rx_dmabuff_fd);
  }
  else
  {
    munmap(tx_dmabuff, tx_dma_buff_sz_bytes);
    close(tx_dmabuff_fd);

    munmap(rx_dmabuff, rx_dma_buff_sz_bytes);
    close(rx_dmabuff_fd);
  }

  system("sudo rmmod u-dma-buf");
}

int dma_controller::set_tx_buffer(void* input_buffer, uint32_t bytes_to_transfer)
{
  if (dma_type == RX_TYPE)
  {
    printf("Error: cannot set TX buffer, dma type is RX mode\n");
    return -1;
  }
  if (bytes_to_transfer > tx_dma_buff_sz_bytes)
  {
    printf("Error: cannot set more than %i Bytes to TX buffer\n", tx_dma_buff_sz_bytes);
    return -1;
  }

  memcpy(tx_dmabuff, input_buffer, bytes_to_transfer);
  return 0;
}

int dma_controller::get_rx_buffer(void* return_buffer, uint32_t bytes_to_transfer)
{
  if (dma_type == TX_TYPE)
  {
    printf("Error: cannot get RX buffer, dma type is RX mode\n");
    return -1;
  }
  if (bytes_to_transfer > rx_dma_buff_sz_bytes)
  {
    printf("Error: cannot get more than %i Bytes from RX buffer\n", rx_dma_buff_sz_bytes);
    return -1;
  }

  memcpy(return_buffer, rx_dmabuff, bytes_to_transfer);
  return 0;
}

void dma_controller::set_tx_blocking(bool is_blocking)
{
  tx_is_blocking = is_blocking;
}

void dma_controller::set_rx_blocking(bool is_blocking)
{
  rx_is_blocking = is_blocking;
}

int dma_controller::launch_tx(uint32_t bytes_to_transfer)
{
  if (dma_type == RX_TYPE)
  {
    printf("Error: cannot transmit DMA, dma type is RX mode\n");
    return -1;
  }
  if (bytes_to_transfer > tx_dma_buff_sz_bytes)
  {
    printf("Error: cannot transmit more than %i Bytes\n", tx_dma_buff_sz_bytes);
    return -1;
  }

  dma_registers.write_reg(MM2S_STATUS_REGISTER, 0xFFFFFFFF);

  //Starting MM2S channel with all interrupts masked...
  dma_registers.write_reg(MM2S_CONTROL_REGISTER, 0xf001);

  //dma_registers.write_reg(MM2S_START_ADDRESS, (uint64_t)(tx_dmabuff_physical_address) & 0xFFFFFFFF); // Write source address
  //dma_registers.write_reg(MM2S_START_ADDRESS_MSB, (uint64_t)(tx_dmabuff_physical_address) >> 32); // Write source address
  dma_controller::set_dma_tx_address(tx_dmabuff_physical_address);

  //Writing MM2S transfer length...
  dma_registers.write_reg(MM2S_LENGTH, bytes_to_transfer);

  if (tx_is_blocking == true)
  {
    dma_controller::wait_for_tx_dma_done();
  }
  dma_controller::get_tx_status();
  return 0;

}

int dma_controller::launch_rx(uint32_t bytes_to_transfer)
{
  if (dma_type == TX_TYPE)
  {
    printf("Error: cannot receive DMA, dma type is TX mode\n");
    return -1;
  }
  if (bytes_to_transfer > rx_dma_buff_sz_bytes)
  {
    printf("Error: cannot receive more than %i Bytes\n", rx_dma_buff_sz_bytes);
    return -1;
  }

  dma_registers.write_reg(S2MM_STATUS_REGISTER, 0xFFFFFFFF);

  //Starting S2MM channel with all interrupts masked...
  dma_registers.write_reg(S2MM_CONTROL_REGISTER, 0xf001);

  //dma_registers.write_reg(S2MM_DESTINATION_ADDRESS, (uint64_t)(rx_dmabuff_physical_address) & 0xFFFFFFFF); // Write source address
  //dma_registers.write_reg(S2MM_DESTINATION_ADDRESS_MSB, (uint64_t)(rx_dmabuff_physical_address) >> 32); // Write source address
  dma_controller::set_dma_rx_address(rx_dmabuff_physical_address);

  //Writing S2MM transfer length...
  dma_registers.write_reg(S2MM_LENGTH, bytes_to_transfer);

  if (rx_is_blocking == true)
  {
    dma_controller::wait_for_rx_dma_done();
  }

  return 0;

}

dma_status_t dma_controller::get_tx_status()
{
    const uint32_t status = dma_registers.read_reg(MM2S_STATUS_REGISTER);
    dma_status_t tx_status;
    tx_status.halted    = status & 0x00000001;
    tx_status.idle      = status & 0x00000002;
    tx_status.SGIncld   = status & 0x00000008;
    tx_status.DMAIntErr = status & 0x00000010;
    tx_status.DMASlvErr = status & 0x00000020;
    tx_status.DMADecErr = status & 0x00000040;
    tx_status.SGIntErr  = status & 0x00000100;
    tx_status.SGSlvErr  = status & 0x00000200;
    tx_status.SGDecErr  = status & 0x00000400;
    tx_status.IOC_Irq   = status & 0x00001000;
    tx_status.Dly_Irq   = status & 0x00002000;
    tx_status.Err_Irq   = status & 0x00004000;
    return tx_status;
}

dma_status_t dma_controller::get_rx_status()
{
    const uint32_t status = dma_registers.read_reg(S2MM_STATUS_REGISTER);
    dma_status_t rx_status;
    rx_status.halted    = status & 0x00000001;
    rx_status.idle      = status & 0x00000002;
    rx_status.SGIncld   = status & 0x00000008;
    rx_status.DMAIntErr = status & 0x00000010;
    rx_status.DMASlvErr = status & 0x00000020;
    rx_status.DMADecErr = status & 0x00000040;
    rx_status.SGIntErr  = status & 0x00000100;
    rx_status.SGSlvErr  = status & 0x00000200;
    rx_status.SGDecErr  = status & 0x00000400;
    rx_status.IOC_Irq   = status & 0x00001000;
    rx_status.Dly_Irq   = status & 0x00002000;
    rx_status.Err_Irq   = status & 0x00004000;
    return rx_status;
}

int dma_controller::wait_for_tx_dma_done(uint32_t max_attempts)
{
    uint32_t attempt_count = 0;
    //dma_status_t tx_status = dma_controller::get_tx_status();
    //while(!(tx_status.IOC_Irq) || !(tx_status.idle))
    while(dma_controller::check_tx_done() == false)
    {
      //tx_status = dma_controller::get_tx_status();
      if (max_attempts > 0)
      {
        if (attempt_count >= max_attempts)
        {
          return -1;
        }
        attempt_count++;
      }
    }
    return 0;
}

int dma_controller::wait_for_rx_dma_done(uint32_t max_attempts)
{
    uint32_t attempt_count = 0;
    //dma_status_t rx_status = dma_controller::get_rx_status();
    //while(!(rx_status.IOC_Irq) || !(rx_status.idle))
    while(dma_controller::check_rx_done() == false)
    {
      //rx_status = dma_controller::get_rx_status();
      if (max_attempts > 0)
      {
        if (attempt_count >= max_attempts)
        {
          return -1;
        }
        attempt_count++;
      }
    }
    return 0;
}

bool dma_controller::check_tx_done()
{
  dma_status_t tx_status = dma_controller::get_tx_status();
  if (tx_status.IOC_Irq && tx_status.idle)
  {
    return true;
  }
  return false;
}

bool dma_controller::check_rx_done()
{
  dma_status_t rx_status = dma_controller::get_rx_status();
  if (rx_status.IOC_Irq && rx_status.idle)
  {
    return true;
  }
  return false;
}

void dma_controller::dma_reset()
{
  dma_controller::dma_tx_reset();
  dma_controller::dma_rx_reset();
}

void dma_controller::dma_tx_reset()
{
  dma_registers.write_reg(MM2S_CONTROL_REGISTER, 4);
  dma_registers.write_reg(MM2S_CONTROL_REGISTER, 0);
}

void dma_controller::dma_rx_reset()
{
  dma_registers.write_reg(MM2S_CONTROL_REGISTER, 4);
  dma_registers.write_reg(MM2S_CONTROL_REGISTER, 0);
  dma_registers.write_reg(S2MM_CONTROL_REGISTER, 4);
  dma_registers.write_reg(S2MM_CONTROL_REGISTER, 0);
}

int dma_controller::set_dma_tx_address(uint64_t physical_address)
{
  dma_registers.write_reg(MM2S_START_ADDRESS, (uint64_t)(physical_address) & 0xFFFFFFFF); // Write source address
  dma_registers.write_reg(MM2S_START_ADDRESS_MSB, (uint64_t)(physical_address) >> 32); // Write source address
  // todo: read back address:
  return 0;
}

int dma_controller::set_dma_rx_address(uint64_t physical_address)
{
  dma_registers.write_reg(S2MM_DESTINATION_ADDRESS, (uint64_t)(physical_address) & 0xFFFFFFFF); // Write source address
  dma_registers.write_reg(S2MM_DESTINATION_ADDRESS_MSB, (uint64_t)(physical_address) >> 32); // Write source address
  // todo: read back address:
  return 0;
}
