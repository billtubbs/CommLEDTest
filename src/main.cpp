/*

    File: main.cpp
    Author: Bill Tubbs
    Date: 2025-06-21
    
    Description:
    C++ code for Teensy 3.1 microcontrollers on the 1593 LED 
    irregular light array.
 
    Uses the OctoWS2811 library by Paul Stoffregen:
    http://www.pjrc.com/teensy/td_libs_OctoWS2811.html

    This scipt listens for commands coming in on the serial port 
    (e.g. from a Raspberry Pi) and updates the LED display.
    
    The 1593 LED irregular light array contains two Teensy 3.1
    microcontrollers mounted on OctoWS2811 Adaptor boards for 
    communication with the 16 led strips (8 on each Teensy) 
    containing 98 to 100 LEDs per strip.

    Required Connections
    --------------------
      pin 2:  LED Strip #1    OctoWS2811 drives 8 LED Strips.
      pin 14: LED strip #2    All 8 are the same length.
      pin 7:  LED strip #3
      pin 8:  LED strip #4    A 100 ohm resistor should used
      pin 6:  LED strip #5    between each Teensy pin and the
      pin 20: LED strip #6    wire to the LED strip, to minimize
      pin 21: LED strip #7    high frequency ringing & noise.
      pin 5:  LED strip #8
      pin 15 & 16 - Connect together, but do not use
      pin 4 - Do not use
      pin 3 - Do not use as PWM.  Normal use is ok.

*/

// CHANGE THE FOLLOWING LINE WHEN COMPILING AND UPLOADING THIS 
// CODE TO EITHER TEENSY.
// Set which Teensy controller the code is for (1 or 2)
//  - TEENSY1 is usually on usb port 1275401
//  - TEENSY2 is usually on usb port 6862001
#define TEENSY2

#include <OctoWS2811.h>
#include <avr/pgmspace.h>
#include <stdio.h>
#include <SerialComm.h>

#ifdef TEENSY1
#define MY_NAME "TEENSY1"
// The following data is specific to the LEDS connected to Teensy #1
// Strip arrangement | 0  2  4  6 |
//   (upper half)    | 1  3  5  7 |
const unsigned short numLeds = 798;
const unsigned short numberOfStrips = 8;
// Old version is wrong:
// const unsigned short ledsPerStrip[] = {100, 98, 100, 100, 100, 100, 100, 100};
// const unsigned short firstLedOfStrip[] = {0, 100, 198, 298, 398, 498, 598, 698, 798};
// Based on visually testing LED arrangement:
const unsigned short ledsPerStrip[] = {100, 100, 98, 100, 100, 100, 100, 100};
const unsigned short firstLedOfStrip[] = {0, 100, 200, 298, 398, 498, 598, 698, 798};
const unsigned short maxLedsPerStrip = 100;
#endif

#ifdef TEENSY2
#define MY_NAME "TEENSY2"
// The following data is specific to the LEDS connected to Teensy #2
// Strip arrangement | 0  2  4  6 |
//   (lower half)    | 1  3  5  7 |
const unsigned short numLeds = 795;
const unsigned short numberOfStrips = 8;
// Old version is wrong:
// const unsigned short ledsPerStrip[] = {99, 99, 100, 100, 99, 100, 100, 98};
// const unsigned short firstLedOfStrip[] = {0,  99, 198, 298, 398, 497, 597, 697, 795};
// Based on visually testing LED arrangement:
const unsigned short ledsPerStrip[] = {99, 99, 99, 100, 100, 100, 100, 98};
const unsigned short firstLedOfStrip[] = {0, 99, 198, 297, 397, 497, 597, 697, 795};
const unsigned short maxLedsPerStrip = 100;
#endif


// Pin number for on-board LED
#define BOARDLED 13


DMAMEM int displayMemory[maxLedsPerStrip*6];
int drawingMemory[maxLedsPerStrip*6];

// LED strip configuration
const int config = WS2811_RGB | WS2811_800kHz;

OctoWS2811 leds(maxLedsPerStrip, displayMemory, drawingMemory, config);


// Function prototypes
void flashBoardLed();
void processData();

void setup()
{
  pinMode(BOARDLED, OUTPUT);

  // Note: Serial.begin(BAUD_RATE) is not needed for Teensy

  leds.begin();
  leds.show();

  // The board LED will flash until a connection is established.
  digitalWrite(LED_BUILTIN, HIGH);
}

void loop()
{
  if (Serial)
  {
    if (!connEstablished)
    {
      newConnection(MY_NAME);
      connEstablished = true;
    }
    getSerialData();
    processData();
  }
  else
  {
    connEstablished = false;
    flashBoardLed();
  }
}

void flashBoardLed()
{
  #ifdef TEENSY1
  if ((millis() % 1000) > 100)
  #endif
  #ifdef TEENSY2
  if ((millis() % 500) > 100)
  #endif
  {
    digitalWrite(LED_BUILTIN, LOW);
  }
  else
  {
    digitalWrite(LED_BUILTIN, HIGH);
  }
}

uint8_t checkByteDataLength(uint16_t dataRecvCount, uint16_t expectedCount) {
  if (dataRecvCount != expectedCount) {
    snprintf(msg_buffer, MSG_BUFFER_SIZE, "%d bytes received, %d expected", dataRecvCount, expectedCount);
    debugToPC(msg_buffer);
    return 1;
  }
  return 0;
}


void processData()
{
  // processes the data that is in dataRecvd[]
  size_t i, ledId, nLeds;
  uint8_t r, g, b;
  byte* p;

  if (allReceived)
  {
    // snprintf(msg_buffer, MSG_BUFFER_SIZE, "Command of length %d bytes received", dataRecvCount);
    // debugToPC(msg_buffer);
  
    // Identify command
    if (dataRecvd[0] == 'L' && dataRecvd[1] == '1') {
      // Command 'L1' received
      if (checkByteDataLength(dataRecvCount, 7) == 0) {
        ledId = dataRecvd[2] * 256 + dataRecvd[3];
        r = dataRecvd[4];
        g = dataRecvd[5];
        b = dataRecvd[6];
        // snprintf(msg_buffer, MSG_BUFFER_SIZE, "Set the colour of LED %d to (%d, %d, %d)", ledId, r, g, b);
        // debugToPC(msg_buffer);
        leds.setPixel(ledId, r, g, b);
      }
    }
    else if (dataRecvd[0] == 'L' && dataRecvd[1] == 'C') {
      // Command 'LC' received
      if (checkByteDataLength(dataRecvCount, 2) == 0) {
        // snprintf(msg_buffer, MSG_BUFFER_SIZE, "Clear all LEDs");
        // debugToPC(msg_buffer);
        for (i=0; i<numberOfStrips * maxLedsPerStrip; i++) {
          leds.setPixel(i, 0, 0, 0);
        }
      }
    }
    else if (dataRecvd[0] == 'L' && dataRecvd[1] == 'N') {
      // Command 'LN' received
      nLeds = dataRecvd[2] * 256 + dataRecvd[3];
      if (checkByteDataLength(dataRecvCount, 4 + 5 * nLeds) == 0) {
        // snprintf(msg_buffer, MSG_BUFFER_SIZE, "Set the colour of %d LEDs", nLeds);
        // debugToPC(msg_buffer);
        p = &dataRecvd[4];
        for (i=0; i<nLeds; i++) {
          ledId = *p++;
          ledId = ledId * 256 + *p++;
          r = *p++;
          g = *p++;
          b = *p++;
          leds.setPixel(ledId, r, g, b);
        }
      }
    }
    else if (dataRecvd[0] == 'L' && dataRecvd[1] == 'A') {
      // Command 'LA' received
      if (checkByteDataLength(dataRecvCount, 2 + 3 * numberOfStrips * maxLedsPerStrip) == 0) {
        // snprintf(msg_buffer, MSG_BUFFER_SIZE, "Set the colour of all LEDs");
        // debugToPC(msg_buffer);
        p = &dataRecvd[2];
        for (i=0; i<numberOfStrips*maxLedsPerStrip; i++) {
          r = *p++;
          g = *p++;
          b = *p++;
          leds.setPixel(i, r, g, b);
        }
      }
    }
    else if (dataRecvd[0] == 'C' && dataRecvd[1] == 'N') {
      // Command 'CN' received
      nLeds = dataRecvd[2] * 256 + dataRecvd[3];
      r = dataRecvd[4];
      g = dataRecvd[5];
      b = dataRecvd[6];
      if (checkByteDataLength(dataRecvCount, 7 + 2 * nLeds) == 0) {
        // snprintf(msg_buffer, MSG_BUFFER_SIZE, "Set %d LEDs to (%zu, %zu, %zu)", nLeds, r, g, b);
        // debugToPC(msg_buffer);
        p = &dataRecvd[7];
        for (i=0; i<nLeds; i++) {
          ledId = *p++;
          ledId = ledId * 256 + *p++;
          leds.setPixel(ledId, r, g, b);
        }
      }
    }
    else if (dataRecvd[0] == 'L' && dataRecvd[1] == 'C') {
      // Command 'CA' received
      r = dataRecvd[2];
      g = dataRecvd[3];
      b = dataRecvd[4];
      if (checkByteDataLength(dataRecvCount, 5) == 0) {
        // snprintf(msg_buffer, MSG_BUFFER_SIZE, "Set all LEDs to (%zu, %zu, %zu)", r, g, b);
        // debugToPC(msg_buffer);
        for (i=0; i<numberOfStrips * maxLedsPerStrip; i++) {
          leds.setPixel(i, r, g, b);
        }
      }
    }
    else if (dataRecvd[0] == 'S' && dataRecvd[1] == 'N') {
      // Command 'SN' received
      if (checkByteDataLength(dataRecvCount, 2) == 0) {
        // snprintf(msg_buffer, MSG_BUFFER_SIZE, "Show LED updates now");
        // debugToPC(msg_buffer);
        leds.show();
      }
    }
    else {
      // Report unrecognised command
      snprintf(msg_buffer, MSG_BUFFER_SIZE, "Invalid command '%c%c'", dataRecvd[0], dataRecvd[1]);
      debugToPC(msg_buffer);
    }

    // Calculate checksum for data received
    uint32_t dataSum = 0;
    for (uint16_t n = 0; n < dataRecvCount; n++)
    {
      dataSum += (uint32_t) dataRecvd[n];
    }

    // Report number of bytes received and check-sum to host
    dataSendCount = 6;
    dataSend[0] = highByte(dataRecvCount);
    dataSend[1] = lowByte(dataRecvCount);
    dataSend[2] = lowByte(dataSum >> 24);
    dataSend[3] = lowByte(dataSum >> 16);
    dataSend[4] = lowByte(dataSum >> 8);
    dataSend[5] = lowByte(dataSum);
    dataToPC();
    allReceived = false;
  }
}

