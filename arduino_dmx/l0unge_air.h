#ifndef L0UNGE_AIR
#define L0UNGE_AIR

#define RGB_SIZE 3
#define MAX_NUM_LEDS 8

typedef struct rgb { uint8_t x[RGB_SIZE]; } rgb;

typedef enum l0unge_msg_type
{
  AIRMSGTYPE_ALL_OFF  = 0x10,
  AIRMSGTYPE_ALL_SAME = 0x11,
  AIRMSGTYPE_ALL_DIFF = 0x1D,
  AIRMSGTYPE_ERR      = 0xFF
}l0unge_msg_type_t;

#endif