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
#define DMX_MF078
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

// DMX
DMX_Slave dmx_slave ( DMX_SLAVE_CHANNELS );
unsigned long       lastFrameReceivedTime;
const unsigned long dmxTimeoutMillis = 10000UL;

// serial Communication to the sender
SoftwareSerial softSerial(10, 11); // RX, TX

void setup() {
  // Enable DMX slave interface and start recording
  // DMX data
  dmx_slave.enable ();  
  dmx_slave.setStartAddress (DMX_ADDRESS);

  // Register on frame complete event to determine signal timeout
  dmx_slave.onReceiveComplete (OnFrameReceiveComplete);
  
  // Connection to the sender
  softSerial.begin(115200);
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

#ifdef DMX_l0unge
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
      msgType = AIRMSGTYPE_ALL_DIFF;
      for (int led=0, chan=LED_0; led < MAX_NUM_LEDS; led++, chan+RGB_SIZE)
      { 
        LEDs[led].x[0] = dmx_slave.getChannelValue(chan);
        LEDs[led].x[1] = dmx_slave.getChannelValue(chan+1);
        LEDs[led].x[2] = dmx_slave.getChannelValue(chan+2);
      }
      break;
    case LED_MODE_ERR:
      break;
    default:
      // \todo implement LED animations
      break;
  }

// \todo Implement Display Mode
/*
  l0unge_display_mode_t displayMode = 
    (l0unge_display_mode_t)dmx_slave.getChannelValue(DISPLAY_MODE);
  switch(displayMode)
  {
    case DISP_OFF:
      break;
    case DISP_LIKE0:
      break;
    case DISP_INDIVIDUAL:
      break;
    case DISP_MODE_ERR:
      break;
    default:
      // \todo implement display animations
      break;
  }
*/
#endif // DMX_l0unge
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
#endif // DMX_MF078
  sendPacket();
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
    case AIRMSGTYPE_ALL_DIFF:
      // There us Software Serial .write() can't handle (buf,len) -.- 
      softSerial.write((uint8_t)(MAX_NUM_LEDS*RGB_SIZE));  
      for(int i=0;i<MAX_NUM_LEDS;i++)
      {
        softSerial.write(LEDs[i].x[0]);
        softSerial.write(LEDs[i].x[1]);
        softSerial.write(LEDs[i].x[2]);
      } 
      break;
    default:
      break;
  }
  softSerial.println("");

  /*
  char buf[12];
  softSerial.print("Values: ");
  softSerial.print(itoa(msgType, buf, 10));
  softSerial.print(" ");
  softSerial.print(itoa(LEDs[0].x[0], buf, 10));
  softSerial.print(" ");
  softSerial.print(itoa(LEDs[0].x[1], buf, 10));
  softSerial.print(" ");
  softSerial.println(itoa(LEDs[0].x[2], buf, 10));
  */
}
