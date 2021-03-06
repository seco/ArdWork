#ifndef _DISPLAY_DEVICE_DRIVER_CONSTS_h
#define _DISPLAY_DEVICE_DRIVER_CONSTS_h


#include "Device_Driver_Consts.h"


static unsigned int const DISPLAY_DEVICE_DRIVER_FIRST_MESSAGE = DEVICE_DRIVER_LAST_MESSAGE + 1001000;
static unsigned int const DISPLAY_DRIVER_CLEAR = DISPLAY_DEVICE_DRIVER_FIRST_MESSAGE + 1;
static unsigned int const DISPLAY_DRIVER_FLUSH = DISPLAY_DEVICE_DRIVER_FIRST_MESSAGE + 2;

static unsigned int const DISPLAY_DRIVER_DRAW_CHAR = DISPLAY_DEVICE_DRIVER_FIRST_MESSAGE + 3;
static unsigned int const DISPLAY_DRIVER_DRAW_STRING = DISPLAY_DEVICE_DRIVER_FIRST_MESSAGE + 4;

static unsigned int const DISPLAY_DRIVER_SET_CURSOR = DISPLAY_DEVICE_DRIVER_FIRST_MESSAGE + 5;

static unsigned int const DISPLAY_DRIVER_START_SCROLLING_RIGHT = DISPLAY_DEVICE_DRIVER_FIRST_MESSAGE + 6;
static unsigned int const DISPLAY_DRIVER_START_SCROLLING_LEFT = DISPLAY_DEVICE_DRIVER_FIRST_MESSAGE + 7;
static unsigned int const DISPLAY_DRIVER_STOP_SCROLL = DISPLAY_DEVICE_DRIVER_FIRST_MESSAGE + 8;

static unsigned int const DISPLAY_DRIVER_LAST_MESSAGE = DISPLAY_DRIVER_STOP_SCROLL;

#endif