#ifndef _TGT_AP_GPIO_SETTING_H_
#define _TGT_AP_GPIO_SETTING_H_

#define _TGT_AP_GPIO_TOUCH_RESET        GPO_2
#define _TGT_AP_GPIO_TOUCH_IRQ          GPIO_B3

//#define _TGT_AP_GPIO_CAM_RESET
//#define _TGT_AP_GPIO_CAM_PWDN0
#define _TGT_AP_GPIO_CAM_PWDN1		GPIO_A4
//#define _TGT_AP_GPIO_CAM_FLASH
//#define _TGT_AP_GPIO_CAM_EN
#define _TGT_AP_LED_CAM_FLASH             "red-flash"

//this macro for mmc hotplug 
//#define _TGT_AP_GPIO_MMC_HOTPLUG        GPIO_B3

//this macro for lcd backlight control whit a GPIO
//#define _TGT_AP_GPIO_LCD_PWR            GPIO_A17

//#define _TGT_AP_GPIO_LCD_EN             GPIO_A17
//#define _TGT_AP_GPIO_LCD_BL_EN          GPIO_A6
//#define _TGT_AP_GPIO_LCD_BL_PML         GPO_2
#define _TGT_AP_GPIO_LCD_RESET          GPO_3

#define _TGT_AP_GPIO_HEADSET_DETECT     GPIO_A0

//#define _TGT_AP_GPIO_USB_DETECT         GPIO_B7

#define _TGT_AP_GPIO_VOLUME_UP          GPIO_D6
#define _TGT_AP_GPIO_VOLUME_DOWN        GPIO_D5

#define _TGT_AP_GPIO_WIFI               GPIO_B2
#define _TGT_AP_GPIO_BT_HOST_WAKE       GPIO_B1
#define _TGT_AP_GPIO_GSENSOR_WAKE       GPIO_A3

#define _TGT_AP_GPIO_TP_INTPIN_PULLCTRL GPIO_A1
//#define _TGT_AP_GPIO_USB_ID				GPIO_B5
//#define _TGT_AP_GPIO_USB_VBUS_SWITCH	GPIO_A17

//#define _TGT_AP_GPIO_EXT_AUD_CLASSK_ENABLE	GPIO_A3
//# define _TGT_AP_EXT_AUD_CLASSK_MODE	4

#endif // _TGT_AP_GPIO_SETTING_H_

