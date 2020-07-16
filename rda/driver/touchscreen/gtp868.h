#ifndef _LINUX_GOODIX_TOUCH_H
#define	_LINUX_GOODIX_TOUCH_H

#define GTP_CUSTOM_CFG        1
#define GTP_DRIVER_SEND_CFG   1
#define GTP_HAVE_TOUCH_KEY    0
#define GTP_DEBUG_ON          0
#define GTP_DEBUG_ARRAY_ON    0
#define GTP_DEBUG_FUNC_ON     0
#if GTP_CUSTOM_CFG
  #define GTP_MAX_HEIGHT   800			
  #define GTP_MAX_WIDTH    480
  #define GTP_INT_TRIGGER  1    //0:Falling 1:Rising
#else
  #define GTP_MAX_HEIGHT   800
  #define GTP_MAX_WIDTH    480
  #define GTP_INT_TRIGGER  1
#endif
#define GTP_MAX_TOUCH      5
#define GTP_ADDR_LENGTH       2
#define GTP_CONFIG_LENGTH     106
#define GTP_REG_CONFIG_DATA   0x6A2
#define GTP_REG_INDEX         0x712
#define GTP_REG_VERSION       0x715
#define GTP_READ_COOR_ADDR    0x721
#define RESOLUTION_LOC        61
#define TRIGGER_LOC           57

//STEP_1(REQUIRED):Change config table.
/*TODO: puts the config info corresponded to your TP here, the following is just 
a sample config, send this config should cause the chip cannot work normally*/
// default(while leaving group2 & group3 empty) or float
#define CTP_CFG_GROUP1 {\
	0x12,0x10,0x0E,0x0C,0x0A,0x08,0x06,0x04,0x02,0x00,0x04,\
				0x44,0x14,0x44,0x24,0x44,0x34,0x44,0x44,0x44,0x54,0x44,\
				0x64,0x44,0x74,0x44,0x84,0x44,0x94,0x44,0xA4,0x44,0xB4,\
				0x44,0xCA,0xAA,0xD0,0x00,0xE0,0x00,0xF0,0x00,0x3F,0x03,\
				0x80,0x80,0x80,0x2C,0x2C,0x2C,0x0F,0x0E,0x0A,0x40,0x30,\
				0x27,0x03,0x00,0x05,0xE0,0x01,0x20,0x03,0x00,0x00,0x66,\
				0x66,0x60,0x60,0x00,0x00,0x23,0x14,0x03,0x0F,0x80,0x0E,\
				0x0E,0x00,0x00,0x14,0x10,0x00,0x00,0xBF,0x40,0x1B,0x80,\
				0x2A,0x03,0x21,0x30,0x40,0x00,0x0F,0x30,0x20,0x30,0x20,\
				0x00,0x00,0x00,0x00,0x00,0x10,0x01}

//TODO puts your group2 config info here,if need.
// VDDIO
#define CTP_CFG_GROUP2 {\
}
//TODO puts your group3 config info here,if need.
// GND
#define CTP_CFG_GROUP3 {\
    }

#define VIR_KEY_BACK_X          408
#define VIR_KEY_BACK_Y          828
#define VIR_KEY_BACK_DX         60
#define VIR_KEY_BACK_DY         30

#define VIR_KEY_HOMEPAGE_X      216
#define VIR_KEY_HOMEPAGE_Y      828
#define VIR_KEY_HOMEPAGE_DX     60
#define VIR_KEY_HOMEPAGE_DY     30

#define VIR_KEY_MENU_X          72
#define VIR_KEY_MENU_Y          828
#define VIR_KEY_MENU_DX         60
#define VIR_KEY_MENU_DY         30

#endif /* _LINUX_GOODIX_TOUCH_H */


