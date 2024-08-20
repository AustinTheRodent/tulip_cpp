/******************************************************************************
  Example_08_I2S_Passthrough.ino
  Demonstrates reading I2S audio from the ADC, and passing that back to the DAC.
  
  This example sets up analog audio input (on INPUT1s), ADC/DAC enabled as I2S 
  peripheral, sets volume control, and Headphone output on the WM8960 Codec.

  Audio should be connected to both the left and right "INPUT1" inputs, 
  they are labeled "RIN1" and "LIN1" on the board.

  This example will pass your audio source through the mixers and gain stages of 
  the codec into the ADC. Read the audio from the ADC via I2S. Then send audio 
  immediately back to the DAC via I2S. Then send the output of the DAC to the 
  headphone outs.

  Development platform used:
  SparkFun ESP32 IoT RedBoard v10

  HARDWARE CONNECTIONS

  **********************
  ESP32 ------- CODEC
  **********************
  QWIIC ------- QWIIC       *Note this connects GND/3.3V/SDA/SCL
  GND --------- GND         *optional, but not a bad idea
  5V ---------- VIN         *needed to power codec's onboard AVDD (3.3V vreg)
  4 ----------- DDT         *aka DAC_DATA/I2S_SDO/"serial data out", this carries the I2S audio data from ESP32 to codec DAC
  16 ---------- BCK         *aka BCLK/I2S_SCK/"bit clock", this is the clock for I2S audio, can be controlled via controller or peripheral.
  17 ---------- ADAT        *aka ADC_DATA/I2S_SD/"serial data in", this carries the I2S audio data from codec's ADC to ESP32 I2S bus.
  25 ---------- DLRC        *aka I2S_WS/LRC/"word select"/"left-right-channel", this toggles for left or right channel data.
  25 ---------- ALR         *for this example WS is shared for both the ADC WS (ALR) and the DAC WS (DLRC)

  **********************
  CODEC ------- AUDIO IN
  **********************
  GND --------- TRS INPUT SLEEVE        *ground for line level input
  LINPUT1 ----- TRS INPUT TIP           *left audio
  RINPUT1 ----- TRS INPUT RING1         *right audio

  **********************
  CODEC -------- AUDIO OUT
  **********************
  OUT3 --------- TRS OUTPUT SLEEVE          *buffered "vmid" (aka "HP GND")
  HPL ---------- TRS OUTPUT TIP             *left HP output
  HPR ---------- TRS OUTPUT RING1           *right HP output

  You can now control the volume of the codecs built in headphone amp using this 
  fuction:

  codec.setHeadphoneVolumeDB(6.00); Valid inputs are -74.00 (MUTE) up to +6.00, 
  (1.00dB steps).

  Pete Lewis @ SparkFun Electronics
  October 14th, 2022
  https://github.com/sparkfun/SparkFun_WM8960_Arduino_Library
  
  This code was created using some code by Mike Grusin at SparkFun Electronics
  Included with the LilyPad MP3 example code found here:
  Revision history: version 1.0 2012/07/24 MDG Initial release
  https://github.com/sparkfun/LilyPad_MP3_Player

  This code was created using some modified code from DroneBot Workshop.
  Specifically, the I2S configuration setup was super helpful to get I2S working.
  This example has a similar I2S config to what we are using here: Microphone to 
  serial plotter example. Although, here we are doing a full duplex I2S port, in 
  order to do reads and writes. To see the original Drone Workshop code and 
  learn more about I2S in general, please visit:
  https://dronebotworkshop.com/esp32-i2s/

  Do you like this library? Help support SparkFun. Buy a board!

    SparkFun Audio Codec Breakout - WM8960 (QWIIC)
    https://www.sparkfun.com/products/21250
	
	All functions return 1 if the read/write was successful, and 0
	if there was a communications failure. You can ignore the return value
	if you just don't care anymore.

	For information on the data sent to and received from the CODEC,
	refer to the WM8960 datasheet at:
	https://github.com/sparkfun/SparkFun_Audio_Codec_Breakout_WM8960/blob/main/Documents/WM8960_datasheet_v4.2.pdf
  This code is released under the [MIT License](http://opensource.org/licenses/MIT).
  Please review the LICENSE.md file included with this example. If you have any questions 
  or concerns with licensing, please contact techsupport@sparkfun.com.
  Distributed as-is; no warranty is given.
******************************************************************************/

//#include <Wire.h>
#include <stdio.h>
#include "SparkFun_WM8960_Arduino_Library.h"
// Click here to get the library: http://librarymanager/All#SparkFun_WM8960

// Include I2S driver
//#include <driver/i2s.h>

void codec_setup(WM8960& codec);

int main(void)
{
  WM8960 codec;
  printf("Example 8 - I2S Passthough\n");

  if (codec.begin() == false) //Begin communication over I2C
  {
    printf("The device did not respond. Please check wiring.\n");
    while (1); // Freeze
  }
  printf("Device is connected properly.\n");

  codec_setup(codec);

  return 0;
}


void codec_setup(WM8960& codec)
{
  // General setup needed
  printf("general setup:\n");
  codec.enableVREF();
  codec.enableVMID();

  // Setup signal flow to the ADC
  printf("signal flow:\n");
  codec.enableLMIC();
  codec.enableRMIC();

  // Connect from INPUT1 to "n" (aka inverting) inputs of PGAs.
  printf("connect from INPUT1 to \"n\" (aka inverting) inputs of PGAs:\n");
  codec.connectLMN1();
  codec.connectRMN1();

  // Disable mutes on PGA inputs (aka INTPUT1)
  printf("Disable mutes on PGA inputs (aka INTPUT1)\n");
  codec.disableLINMUTE();
  codec.disableRINMUTE();

  // Set pga volumes
  printf("Set pga volumes\n");
  codec.setLINVOLDB(0.00); // Valid options are -17.25dB to +30dB (0.75dB steps)
  codec.setRINVOLDB(0.00); // Valid options are -17.25dB to +30dB (0.75dB steps)

  // Set input boosts to get inputs 1 to the boost mixers
  printf("Set input boosts to get inputs 1 to the boost mixers\n");
  codec.setLMICBOOST(WM8960_MIC_BOOST_GAIN_0DB);
  codec.setRMICBOOST(WM8960_MIC_BOOST_GAIN_0DB);

  // Connect from MIC inputs (aka pga output) to boost mixers
  printf("Connect from MIC inputs (aka pga output) to boost mixers\n");
  codec.connectLMIC2B();
  codec.connectRMIC2B();

  // Enable boost mixers
  printf("Enable boost mixers\n");
  codec.enableAINL();
  codec.enableAINR();

  // Disconnect LB2LO (booster to output mixer (analog bypass)
  // For this example, we are going to pass audio throught the ADC and DAC
  printf("Disconnect LB2LO (booster to output mixer (analog bypass), For this example, we are going to pass audio throught the ADC and DAC\n");
  codec.disableLB2LO();
  codec.disableRB2RO();

  // Connect from DAC outputs to output mixer
  printf("Connect from DAC outputs to output mixer\n");
  codec.enableLD2LO();
  codec.enableRD2RO();

  // Set gainstage between booster mixer and output mixer
  // For this loopback example, we are going to keep these as low as they go
  printf("Set gainstage between booster mixer and output mixer, For this loopback example, we are going to keep these as low as they go\n");
  codec.setLB2LOVOL(WM8960_OUTPUT_MIXER_GAIN_NEG_21DB); 
  codec.setRB2ROVOL(WM8960_OUTPUT_MIXER_GAIN_NEG_21DB);

  // Enable output mixers
  printf("Enable output mixers\n");
  codec.enableLOMIX();
  codec.enableROMIX();

  // CLOCK STUFF, These settings will get you 48KHz sample rate, and class-d 
  // freq at 705.6kHz
  printf("CLOCK STUFF, These settings will get you 48KHz sample rate\n");
  codec.enablePLL(); // Needed for class-d amp clock
  codec.setPLLPRESCALE(WM8960_PLLPRESCALE_DIV_2);
  codec.setSMD(WM8960_PLL_MODE_FRACTIONAL);
  codec.setCLKSEL(WM8960_CLKSEL_PLL);
  codec.setSYSCLKDIV(WM8960_SYSCLK_DIV_BY_2);
  codec.setBCLKDIV(4);
  codec.setDCLKDIV(WM8960_DCLKDIV_16);

  //44.1kHz:
  //codec.setPLLN(7);
  //codec.setPLLK(0x86, 0xC2, 0x26); // PLLK=86C226h

  //48kHz:
  codec.setPLLN(8);
  codec.setPLLK(0x31, 0x26, 0xE8); // PLLK=3126E8h

  //codec.setADCDIV(0); // Default is 000 (what we need for 44.1KHz/48kHz)
  //codec.setDACDIV(0); // Default is 000 (what we need for 44.1KHz/48kHz)
  //codec.setWL(WM8960_WL_16BIT);
  codec.setWL(WM8960_WL_32BIT);

  //codec.enablePeripheralMode();
  printf("set master mode\n");
  codec.enableMasterMode();
  //codec.setALRCGPIO(); // Note, should not be changed while ADC is enabled.

  // Enable ADCs and DACs
  printf("Enable ADCs and DACs\n");
  codec.enableAdcLeft();
  codec.enableAdcRight();
  codec.enableDacLeft();
  codec.enableDacRight();
  codec.disableDacMute();

  //codec.enableLoopBack(); // Loopback sends ADC data directly into DAC
  printf("disable loopback\n");
  codec.disableLoopBack();

  // Default is "soft mute" on, so we must disable mute to make channels active
  printf("disable dac mute\n");
  codec.disableDacMute(); 

  printf("enable headphones\n");
  codec.enableHeadphones();
  codec.enableOUT3MIX(); // Provides VMID as buffer for headphone ground

  printf("Volume set to +0dB\n");
  codec.setHeadphoneVolumeDB(0.00);
  codec.enableI2S();
  printf("Codec Setup complete. Listen to left/right INPUT1 on Headphone outputs.\n");
}

