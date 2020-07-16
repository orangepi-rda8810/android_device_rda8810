#ifndef __LINUX_NT11004_H__
#define	__LINUX_NT11004_H__

#define VIR_WVGA_VER

#define NTP_ADDR_LENGTH       1

#define NTP_REG_VERSION       0x86

#define NT11004_NAME 	"nt11004_ts"

#define MAX_FINGER_NUM 2
#define NTP_WITH_BUTTON

//define default resolution of LCM
#define LCD_MAX_WIDTH		480
#define LCD_MAX_HEIGHT 		800

//define default resolution of the touchscreen
#define TPD_MAX_WIDTH 		480
#define TPD_MAX_HEIGHT 		800

#define  NVT_TOUCH_CTRL_DRIVER  1

#if NT11004_MODEL
struct nvt_flash_data {
	rwlock_t lock;
	unsigned char bufferIndex;
	unsigned int length;
	struct i2c_client *client;
};
#endif


#if defined(VIR_HVGA_VER)

#define NT11004_RES_X    	320
#define NT11004_RES_Y    	480

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

#elif defined(VIR_WVGA_VER)

#define NT11004_RES_X    	480
#define NT11004_RES_Y    	800

#define VIR_KEY_BACK_X          420
#define VIR_KEY_BACK_Y          850
#define VIR_KEY_BACK_DX         40
#define VIR_KEY_BACK_DY         20

#define VIR_KEY_HOMEPAGE_X      180
#define VIR_KEY_HOMEPAGE_Y      850
#define VIR_KEY_HOMEPAGE_DX     40
#define VIR_KEY_HOMEPAGE_DY     20

#define VIR_KEY_MENU_X          60
#define VIR_KEY_MENU_Y          850
#define VIR_KEY_MENU_DX         40
#define VIR_KEY_MENU_DY         20

#elif defined(VIR_FWVGA_VER)

#define NT11004_RES_X    	480
#define NT11004_RES_Y    	854

#define VIR_KEY_BACK_X          400
#define VIR_KEY_BACK_Y          960
#define VIR_KEY_BACK_DX         60
#define VIR_KEY_BACK_DY         30

#define VIR_KEY_HOMEPAGE_X      240
#define VIR_KEY_HOMEPAGE_Y      960
#define VIR_KEY_HOMEPAGE_DX     60
#define VIR_KEY_HOMEPAGE_DY     30

#define VIR_KEY_MENU_X          80
#define VIR_KEY_MENU_Y          960
#define VIR_KEY_MENU_DX         60
#define VIR_KEY_MENU_DY         30

#endif

#endif /* __LINUX_NT11004_H__ */


