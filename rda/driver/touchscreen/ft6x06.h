#ifndef __LINUX_FT6X06_TS_H__
#define __LINUX_FT6X06_TS_H__
//yaoby
/* -- dirver configure -- */
#define CFG_MAX_TOUCH_POINTS	2

/*#define VIR_FVGA_VER*/
#define VIR_FWVGA_VER
#define PRESS_MAX	0xFF
#define FT_PRESS	0x7F

#define FT6X06_NAME 	"ft6x06_ts"

#define FT_MAX_ID	0x0F
#define FT_TOUCH_STEP	6
#define FT_TOUCH_X_H_POS		3
#define FT_TOUCH_X_L_POS		4
#define FT_TOUCH_Y_H_POS		5
#define FT_TOUCH_Y_L_POS		6
#define FT_TOUCH_EVENT_POS		3
#define FT_TOUCH_ID_POS			5

#define POINT_READ_BUF	(3 + FT_TOUCH_STEP * CFG_MAX_TOUCH_POINTS)

/*register address*/
#define FT6x06_REG_FW_VER		0xA6
#define FT6x06_REG_POINT_RATE	0x88
#define FT6x06_REG_THGROUP	0x80

#if defined(VIR_FWVGA_VER_4_5)
#define FT6X06_RES_X            480
#define FT6X06_RES_Y            854

#define VIR_KEY_BACK_X          300
#define VIR_KEY_BACK_Y          900
#define VIR_KEY_BACK_DX         60
#define VIR_KEY_BACK_DY         30

#define VIR_KEY_HOMEPAGE_X      180
#define VIR_KEY_HOMEPAGE_Y      900
#define VIR_KEY_HOMEPAGE_DX     60
#define VIR_KEY_HOMEPAGE_DY     30

#define VIR_KEY_MENU_X          60
#define VIR_KEY_MENU_Y          900
#define VIR_KEY_MENU_DX         60
#define VIR_KEY_MENU_DY         30

#elif defined(VIR_FWVGA_VER)

#define FT6X06_RES_X    	480
#define FT6X06_RES_Y    	854

#define VIR_KEY_BACK_X          400
#define VIR_KEY_BACK_Y          900
#define VIR_KEY_BACK_DX         60
#define VIR_KEY_BACK_DY         30

#define VIR_KEY_HOMEPAGE_X      240
#define VIR_KEY_HOMEPAGE_Y      900
#define VIR_KEY_HOMEPAGE_DX     60
#define VIR_KEY_HOMEPAGE_DY     30

#define VIR_KEY_MENU_X          40
#define VIR_KEY_MENU_Y          900
#define VIR_KEY_MENU_DX         60
#define VIR_KEY_MENU_DY         30

#elif defined(VIR_WVGA_VER)

#define FT6X06_RES_X    	480
#define FT6X06_RES_Y    	800

#define VIR_KEY_BACK_X          400
#define VIR_KEY_BACK_Y          900
#define VIR_KEY_BACK_DX         60
#define VIR_KEY_BACK_DY         30

#define VIR_KEY_HOMEPAGE_X      240
#define VIR_KEY_HOMEPAGE_Y      900
#define VIR_KEY_HOMEPAGE_DX     60
#define VIR_KEY_HOMEPAGE_DY     30

#define VIR_KEY_MENU_X          80
#define VIR_KEY_MENU_Y          900
#define VIR_KEY_MENU_DX         60
#define VIR_KEY_MENU_DY         30

#elif defined(FS902_HVGA_B769)

#define FT6X06_RES_X    	320
#define FT6X06_RES_Y    	480

#define VIR_KEY_BACK_X          450
#define VIR_KEY_BACK_Y          1010
#define VIR_KEY_BACK_DX         80
#define VIR_KEY_BACK_DY         30

#define VIR_KEY_HOMEPAGE_X      270
#define VIR_KEY_HOMEPAGE_Y      1010
#define VIR_KEY_HOMEPAGE_DX     80
#define VIR_KEY_HOMEPAGE_DY     30

#define VIR_KEY_MENU_X          90
#define VIR_KEY_MENU_Y          1010
#define VIR_KEY_MENU_DX         80
#define VIR_KEY_MENU_DY         30

#elif defined(FS902_EKT_EK3579)

#define FT6X06_RES_X    	320
#define FT6X06_RES_Y    	480

#define VIR_KEY_BACK_X          240
#define VIR_KEY_BACK_Y          530
#define VIR_KEY_BACK_DX         80
#define VIR_KEY_BACK_DY         30

#define VIR_KEY_HOMEPAGE_X      160
#define VIR_KEY_HOMEPAGE_Y      530
#define VIR_KEY_HOMEPAGE_DX     80
#define VIR_KEY_HOMEPAGE_DY     30

#define VIR_KEY_MENU_X          80
#define VIR_KEY_MENU_Y          530
#define VIR_KEY_MENU_DX         80
#define VIR_KEY_MENU_DY         30

#elif defined(FS801_HVGA_FC)

#define FT6X06_RES_X    	320
#define FT6X06_RES_Y    	480

#define VIR_KEY_BACK_X          200
#define VIR_KEY_BACK_Y          980
#define VIR_KEY_BACK_DX         80
#define VIR_KEY_BACK_DY         30

#define VIR_KEY_HOMEPAGE_X      120
#define VIR_KEY_HOMEPAGE_Y      980
#define VIR_KEY_HOMEPAGE_DX     80
#define VIR_KEY_HOMEPAGE_DY     30

#define VIR_KEY_MENU_X          40
#define VIR_KEY_MENU_Y          980
#define VIR_KEY_MENU_DX         80
#define VIR_KEY_MENU_DY         30

#else

#define FT6X06_RES_X    	320
#define FT6X06_RES_Y    	480

#define VIR_KEY_BACK_X          240
#define VIR_KEY_BACK_Y          535
#define VIR_KEY_BACK_DX         80
#define VIR_KEY_BACK_DY         30

#define VIR_KEY_HOMEPAGE_X      140
#define VIR_KEY_HOMEPAGE_Y      535
#define VIR_KEY_HOMEPAGE_DX     80
#define VIR_KEY_HOMEPAGE_DY     30

#define VIR_KEY_MENU_X          60
#define VIR_KEY_MENU_Y          535
#define VIR_KEY_MENU_DX         80
#define VIR_KEY_MENU_DY         30
#endif

int ft6x06_i2c_Read(struct i2c_client *client, char *writebuf, int writelen,
		    char *readbuf, int readlen);
int ft6x06_i2c_Write(struct i2c_client *client, char *writebuf, int writelen);

/* The platform data for the Focaltech ft5x0x touchscreen driver */
struct ft6x06_platform_data {
	unsigned int x_max;
	unsigned int y_max;
	unsigned long irqflags;
	unsigned int irq;
	unsigned int reset;
};
#endif
