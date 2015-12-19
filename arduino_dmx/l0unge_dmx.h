
#ifndef L0UNGE_DMX
#define L0UNGE_DMX
// VERSION x.y

enum l0unge_dmx_channels
{
  LED_MODE      = 1,
  DISPLAY_MODE  = 2,
  LED_0         = 3,
  LED_SIZE      = 4
};

typedef enum l0unge_led_mode
{
  LED_OFF        = 0,
  LED_LIKE0      = 1,
  LED_INDIVIDUAL = 2,
  LED_MODE_ERR   = 0xFF
}l0unge_led_mode_t;

typedef enum l0unge_display_mode
{
  DISP_OFF        = 0,
  DISP_LIKE0      = 1,
  DISP_INDIVIDUAL = 2,
  DISP_MODE_ERR   = 0xFF
}l0unge_display_mode_t;

#endif