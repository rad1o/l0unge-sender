/*
 *
 */


#include <Conceptinetics.h>
// Thanks to conflicting Serial Interrupts with the DMX lib,
// we are using Software Serial for now
#include <SoftwareSerial.h>

#include "l0unge_dmx.h"
#include "l0unge_air.h"

// Set up DMX Master Type
//#define DMX_MF078
#define DMX_L0UNGE
// Setup up DMX Address of the Board
// \todo Do this by read 3 input Pins
#define DMX_ADDRESS 1

#ifdef DMX_MF078
  #define DMX_SLAVE_CHANNELS 8
#endif
#ifdef DMX_L0UNGE
  #define DMX_SLAVE_CHANNELS 29
#endif

// l0unge Message
l0unge_msg_type_t msgType = AIRMSGTYPE_ERR;
rgb LEDs[MAX_NUM_LEDS];
rgb DisplayColor;
uint8_t AnimationId = 0;

// DMX
DMX_Slave dmx_slave ( DMX_SLAVE_CHANNELS );
unsigned long       lastFrameReceivedTime;
const unsigned long dmxTimeoutMillis = 10000UL;

// serial Communication to the sender
SoftwareSerial softSerial(10, 11); // RX, TX
#ifdef DEBUG_OUTPUT
SoftwareSerial softDebug(0, 1);
#endif

void setup() {
  // Enable DMX slave interface and start recording
  // DMX data
  dmx_slave.enable ();
  dmx_slave.setStartAddress (DMX_ADDRESS);

  // Register on frame complete event to determine signal timeout
  dmx_slave.onReceiveComplete (OnFrameReceiveComplete);

  // Connection to the sender
  softSerial.begin(115200);
#ifdef DEBUG_OUTPUT
  softDebug.begin(115200);
#endif

  // Clear LED Array
  memset(&LEDs, 0, MAX_NUM_LEDS*sizeof(rgb));
  memset(&DisplayColor, 0, sizeof(rgb));
}

void loop()
{
  // Get current time
  unsigned long now = millis();

  // If we didn't receive a DMX frame within the timeout period
  // clear all dmx channels
  if (now - lastFrameReceivedTime > dmxTimeoutMillis)
  {
    dmx_slave.getBuffer().clear();
  }
}

void OnFrameReceiveComplete (unsigned short channelsReceived)
{
  if (channelsReceived == DMX_SLAVE_CHANNELS)
  {
    // All slave channels have been received
    handleDMXData();
  }
  else
  {
    // We have received a frame but not all channels we where
    // waiting for, master might have transmitted less
    // channels
  }

  // Update receive time to determine signal timeout
  lastFrameReceivedTime = millis ();
}

void handleDMXData (void)
{

#ifdef DMX_L0UNGE
  // Check LED Channel Setting
  l0unge_led_mode_t ledMode =
    (l0unge_led_mode_t)dmx_slave.getChannelValue(LED_MODE);

  switch(ledMode)
  {
    case LED_OFF:
      msgType = AIRMSGTYPE_ALL_OFF;
      break;
    case LED_LIKE0:
      msgType = AIRMSGTYPE_ALL_SAME;
      LEDs[0].x[0] = dmx_slave.getChannelValue(LED_0);
      LEDs[0].x[1] = dmx_slave.getChannelValue(LED_0+1);
      LEDs[0].x[2] = dmx_slave.getChannelValue(LED_0+2);
      break;
    case LED_INDIVIDUAL:
      msgType = AIRMSGTYPE_LED_ALL;
      for (int led=0; led < MAX_NUM_LEDS; led++)
      {
        LEDs[led].x[0] = dmx_slave.getChannelValue(LED_0+(led*RGB_SIZE));
        LEDs[led].x[1] = dmx_slave.getChannelValue(LED_0+(led*RGB_SIZE)+1);
        LEDs[led].x[2] = dmx_slave.getChannelValue(LED_0+(led*RGB_SIZE)+2);
      }
      break;
    case LED_MODE_ERR:
      break;
    default:
      msgType = AIRMSGTYPE_LED_ANI;
      AnimationId = ledMode - LED_ANIMATION;
      break;
  }
  if(LED_MODE_ERR != ledMode)
  {
    sendPacket();
  }

  // Check Display Channel Setting
  l0unge_display_mode_t displayMode =
    (l0unge_display_mode_t)dmx_slave.getChannelValue(DISPLAY_MODE);
  switch(displayMode)
  {
    case DISP_OFF:
      msgType = AIRMSGTYPE_DISP_COLOR;
      memset(&DisplayColor, 0, sizeof(rgb));
      break;
    case DISP_LIKE0:
      msgType = AIRMSGTYPE_DISP_COLOR;
      DisplayColor.x[0] = dmx_slave.getChannelValue(LED_0);
      DisplayColor.x[1] = dmx_slave.getChannelValue(LED_0+1);
      DisplayColor.x[2] = dmx_slave.getChannelValue(LED_0+2);
      break;
    case DISP_INDIVIDUAL:
      msgType = AIRMSGTYPE_DISP_COLOR;
      DisplayColor.x[0] = dmx_slave.getChannelValue(DISPLAY_COLOR);
      DisplayColor.x[1] = dmx_slave.getChannelValue(DISPLAY_COLOR+1);
      DisplayColor.x[2] = dmx_slave.getChannelValue(DISPLAY_COLOR+2);
      break;
    case DISP_MODE_ERR:
      break;
    default:
      msgType = AIRMSGTYPE_DISP_ANI;
      AnimationId = displayMode - DISP_ANIMATION;
      break;
  }
  if(DISP_MODE_ERR != displayMode)
  {
    if(LED_LIKE0 != ledMode) 
    // In this mode, the Display is the same as LED0,
    // so we do not need to send a Disp Msg
    {
      sendPacket();
    }
  }

#endif // DMX_L0UNGE
#ifdef DMX_MF078

  uint8_t chan3 = dmx_slave.getChannelValue(3);
  if(0 <= chan3
      && chan3 <= 8)
  { msgType = AIRMSGTYPE_ALL_OFF; }
  else
  { msgType = AIRMSGTYPE_ALL_SAME; }

  LEDs[0].x[0] = dmx_slave.getChannelValue(4);
  LEDs[0].x[1] = dmx_slave.getChannelValue(5);
  LEDs[0].x[2] = dmx_slave.getChannelValue(6);

  sendPacket();

#endif // DMX_MF078

}

void sendPacket(void)
{
  // Preamble
  softSerial.write(0x21);
  softSerial.write(0x21);
  softSerial.write(msgType);

  switch(msgType)
  {
    case AIRMSGTYPE_ALL_OFF:
      softSerial.write((uint8_t)0);
      // Nothing more to do
      break;
    case AIRMSGTYPE_ALL_SAME:
      softSerial.write((uint8_t)RGB_SIZE);
      softSerial.write(LEDs[0].x[0]);
      softSerial.write(LEDs[0].x[1]);
      softSerial.write(LEDs[0].x[2]);
      break;
    case AIRMSGTYPE_DISP_COLOR:
      softSerial.write((uint8_t)RGB_SIZE);
      softSerial.write(DisplayColor.x[0]);
      softSerial.write(DisplayColor.x[1]);
      softSerial.write(DisplayColor.x[2]);
      break;
    case AIRMSGTYPE_LED_ALL:
      // There us Software Serial .write() can't handle (buf,len) -.-
      softSerial.write((uint8_t)(MAX_NUM_LEDS*RGB_SIZE));
      for(int i=0;i<MAX_NUM_LEDS;i++)
      {
        softSerial.write(LEDs[i].x[0]);
        softSerial.write(LEDs[i].x[1]);
        softSerial.write(LEDs[i].x[2]);
      }
      break;
    case AIRMSGTYPE_LED_ANI:
    case AIRMSGTYPE_DISP_ANI:
      softSerial.write(sizeof(uint8_t));
      softSerial.write(AnimationId);
      break;
    default:
      break;
  }
  softSerial.println("");

#ifdef DEBUG_OUTPUT
  char buf[12];
  softDebug.print("Values: ");
  softDebug.print(itoa(msgType, buf, 10));
  softDebug.print(" ");
  softDebug.print(itoa(LEDs[0].x[0], buf, 10));
  softDebug.print(" ");
  softDebug.print(itoa(LEDs[0].x[1], buf, 10));
  softDebug.print(" ");
  softDebug.print(itoa(LEDs[0].x[2], buf, 10));
  softDebug.print("    ");
  softDebug.print(itoa(LEDs[1].x[0], buf, 10));
  softDebug.print(" ");
  softDebug.print(itoa(LEDs[1].x[1], buf, 10));
  softDebug.print(" ");
  softDebug.print(itoa(LEDs[1].x[2], buf, 10));
  softDebug.print("    ");
  softDebug.print(itoa(LEDs[2].x[0], buf, 10));
  softDebug.print(" ");
  softDebug.print(itoa(LEDs[2].x[1], buf, 10));
  softDebug.print(" ");
  softDebug.print(itoa(LEDs[2].x[2], buf, 10));
  softDebug.print("    ");
  softDebug.print(itoa(DisplayColor.x[0], buf, 10));
  softDebug.print(" ");
  softDebug.print(itoa(DisplayColor.x[1], buf, 10));
  softDebug.print(" ");
  softDebug.println(itoa(DisplayColor.x[2], buf, 10));
#endif // DEBUG_OUTPUT  
}
