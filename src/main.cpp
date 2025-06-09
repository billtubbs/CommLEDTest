#include <Arduino.h>
#include <SerialComm.h>
#include <FastLED.h>

// Device names used for serial communications
#define MY_NAME "MyTeensy1"

// How many leds in your strip?
#define NUM_LEDS 7

// PIN where LEDs are connected
#define DATA_PIN 6

// Define the array of leds
CRGB leds[NUM_LEDS];

// Function prototypes
void flashBoardLed();
void processData();

void setup()
{
  pinMode(LED_BUILTIN, OUTPUT); // onboard LED

  // LED arrangement
  FastLED.addLeds<WS2811, DATA_PIN, RGB>(leds, NUM_LEDS);

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
  if ((millis() % 1000) > 100)
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
    snprintf(msg_buffer, MSG_BUFFER_SIZE, "Command of length %d bytes received", dataRecvCount);
    debugToPC(msg_buffer);
  
    // Identify command
    if (dataRecvd[0] == 'L' && dataRecvd[1] == '1') {
      // Command 'L1' received
      if (checkByteDataLength(dataRecvCount, 7) == 0) {
        ledId = dataRecvd[2] * 256 + dataRecvd[3];
        r = dataRecvd[4];
        g = dataRecvd[5];
        b = dataRecvd[6];
        snprintf(msg_buffer, MSG_BUFFER_SIZE, "Set the colour of LED %d to (%d, %d, %d)", ledId, r, g, b);
        debugToPC(msg_buffer);
        leds[ledId] = CRGB(r, g, b);
      }
    }
    else if (dataRecvd[0] == 'L' && dataRecvd[1] == 'C') {
      // Command 'LN' received
      if (checkByteDataLength(dataRecvCount, 2) == 0) {
        snprintf(msg_buffer, MSG_BUFFER_SIZE, "Clear all LEDs");
        debugToPC(msg_buffer);
        for (i=0; i<NUM_LEDS; i++) {
          leds[i] = CRGB::Black;
        }
      }
    }
    else if (dataRecvd[0] == 'L' && dataRecvd[1] == 'N') {
      nLeds = dataRecvd[2] * 256 + dataRecvd[3];
      // Command 'LN' received
      if (checkByteDataLength(dataRecvCount, 4 + 5 * nLeds) == 0) {
        snprintf(msg_buffer, MSG_BUFFER_SIZE, "Set the colour of %d LEDs", nLeds);
        debugToPC(msg_buffer);
        p = &dataRecvd[4];
        for (i=0; i<nLeds; i++) {
          ledId = (*p++) * 256 + *p++;
          r = *p++;
          g = *p++;
          b = *p++;
          leds[ledId] = CRGB(r, g, b);
        }
      }
    }
    else if (dataRecvd[0] == 'L' && dataRecvd[1] == 'A') {
      // Command 'LA' received
      if (checkByteDataLength(dataRecvCount, 2 + 3 * 7) == 0) {
        snprintf(msg_buffer, MSG_BUFFER_SIZE, "Set the colour of all LEDs");
        debugToPC(msg_buffer);
        p = &dataRecvd[2];
        for (i=0; i<NUM_LEDS; i++) {
          r = *p++;
          g = *p++;
          b = *p++;
          leds[i] = CRGB(r, g, b);
        }
      }
    }
    else if (dataRecvd[0] == 'S' && dataRecvd[1] == 'N') {
      // Command 'SN' received
      if (checkByteDataLength(dataRecvCount, 2) == 0) {
        snprintf(msg_buffer, MSG_BUFFER_SIZE, "Show LED updates now");
        debugToPC(msg_buffer);
        FastLED.show();
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
    snprintf(msg_buffer, MSG_BUFFER_SIZE, "Checksum: %d", dataSum);
    debugToPC(msg_buffer);

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

