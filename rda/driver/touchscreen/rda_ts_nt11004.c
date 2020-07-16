#include <linux/errno.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/delay.h>
#include <linux/hrtimer.h>
#include <linux/i2c.h>
#include <linux/input.h>
#include <linux/interrupt.h>
#include <linux/i2c.h>
#include <linux/init.h>
#include <linux/slab.h>

#ifdef CONFIG_HAS_EARLYSUSPEND
#include <linux/earlysuspend.h>
#endif

#include <linux/platform_device.h>

#include <mach/hardware.h>
#include <plat/devices.h>
#include <plat/rda_debug.h>
#include <mach/board.h>
#include <linux/gpio.h>
#include "rda_ts.h"
#include "nt11004.h"

#ifdef TS_WITH_PS_FUNCTIONS

/* for most scenes of psensor : distance range such as [0-1] means near, other means far
 * when we want to pass 'near', just set distance to be a value in range [min - max]
 * when we want to pass 'far', just set distance to be a value not.
 * same as range defined in like "hardware/rda/rda8810/libsensors/STKProximitySensor.cpp" sensor list
*/
#define PSENSOR_NEAR_RANGE_MIN 0
#define PSENSOR_NEAR_RANGE_MAX 1
static int g_ts_face_det_mode = 0;

/*when backlight off ,only ps data enable --by wyc0207*/
int g_ps_enable_screenoff = 0;

/* [RDA] patch-for-ps-not-work-when-powerkey-press-to-sleep --start--*/
static struct timer_list    ps_timer;
static void nt11004_delay_set_ps_mode(unsigned long ftdata);
enum {
	TP_YUANSHENG = 0,
	TP_HUANGZE,
	TP_TONGXINCHENG,
};

/*static bool g_ts_ps_switch_retry = false;*/
struct nt11004_data{
    struct i2c_client *my_i2c;
    bool    ps_on;

};
static struct nt11004_data my_data;
/*[RDA] patch-for-ps-not-work-when-powerkey-press-to-sleep --end--*/

/*static u8 g_ts_vendor = TP_TONGXINCHENG;*/
#endif


static void rda_nt11004_init_gpio(void);
static void rda_nt11004_init(struct i2c_client *client);
static void rda_nt11004_startup_chip(struct i2c_client *client);
static void rda_nt11004_reset_chip(struct i2c_client *client);
static s32 ntp_read_version(struct i2c_client *client, u16* version);
static int rda_nt11004_get_pos(struct i2c_client *client, struct rda_ts_pos_data * pos_data);
static int rda_nt11004_suspend(struct i2c_client *client, pm_message_t mesg);
static int rda_nt11004_resume(struct i2c_client *client);

static void rda_nt11004_reset_gpio(void)
{
	gpio_set_value(GPIO_TOUCH_RESET, 1);
	mdelay(10);
	gpio_set_value(GPIO_TOUCH_RESET, 0);
	mdelay(60);
	gpio_set_value(GPIO_TOUCH_RESET, 1);
	mdelay(10);		/*Delay 10 ms*/
}

static void rda_nt11004_init_gpio(void)
{
	gpio_request(GPIO_TOUCH_RESET, "touch screen reset");
	gpio_request(GPIO_TOUCH_IRQ, "touch screen interrupt");

	gpio_direction_output(GPIO_TOUCH_RESET, 1);
	mdelay(1);		/*Delay 10ms*/
	gpio_direction_input(GPIO_TOUCH_IRQ);
	mdelay(1);

	rda_nt11004_reset_gpio();
}

static void rda_nt11004_startup_chip(struct i2c_client *client)
{
	/*_init_panel(client);*/
}

static void rda_nt11004_reset_chip(struct i2c_client *client)
{
	rda_nt11004_reset_gpio();
}

static void rda_nt11004_init(struct i2c_client *client)
{
	u16 ID_version = 0;

#ifdef TS_WITH_PS_FUNCTIONS
	 setup_timer(&ps_timer,nt11004_delay_set_ps_mode,(unsigned long)&my_data);
#endif
	rda_dbg_ts("#########rda_nt11004_init#########\n");

	rda_nt11004_reset_chip(client);
	/*_init_panel(client);*/
	ntp_read_version(client, &ID_version);
	rda_dbg_ts("TP IC Version: %x\n", ID_version);
}

s32 ntp_i2c_read(struct i2c_client *client, u8 *buf, s32 len)
{
	struct i2c_msg msgs[2];
	s32 ret=-1;
	s32 retries = 0;

	msgs[0].flags = !I2C_M_RD;
	msgs[0].addr  = client->addr;
	msgs[0].len   = NTP_ADDR_LENGTH;
	msgs[0].buf   = &buf[0];

	msgs[1].flags = I2C_M_RD;
	msgs[1].addr  = client->addr;
	msgs[1].len   = len - NTP_ADDR_LENGTH;
	msgs[1].buf   = &buf[NTP_ADDR_LENGTH];

	while(retries < 5) {
		ret = i2c_transfer(client->adapter, msgs, 2);
		if (ret == 2) {
			break;
		}
		retries++;
		if (ret != 2) {
			rda_dbg_ts("#########NT11004 i2c write error: %d\n#########"
					,ret);
		}
	}
	return ret;
}

s32 ntp_i2c_write(struct i2c_client *client,u8 *buf,s32 len)
{
	struct i2c_msg msg;
	s32 ret=-1;
	s32 retries = 0;

	msg.flags = !I2C_M_RD;
	msg.addr  = client->addr;
	msg.len   = len;
	msg.buf   = buf;

	while(retries < 5) {
		ret = i2c_transfer(client->adapter, &msg, 1);
		if (ret == 1) {
			break;
		}
		if (ret != 1) {
			rda_dbg_ts("###NT11004 i2c write error: %d\n###", ret);
		}
		retries++;
	}
	return ret;
}

static  s32 ntp_read_version(struct i2c_client *client, u16* version)
{
	s32 ret = -1;
	u8 buf[8] = {NTP_REG_VERSION};

	ret = ntp_i2c_read(client, buf, 2);

	if (ret < 0) {
		rda_dbg_ts("#########NT11004 read version failed#########\n");
		return ret;
	}

	if (version) {
		*version = buf[1];
	}
	rda_dbg_ts("#########NT11004 IC VERSION:%02x%02x\n",buf[0], buf[1]);

	return ret;
}

/*
nt11004 touch data format
Byte0 bit7-bit3  track_id  bit2  ?   bit1 - bit 0   01 down 02 repeat 03 up
Byte1 bit3-bit0  xH
Byte2 bit3-bit0  yH
Byte3 bit7-bit4  xL  bit3-bit0  yL
Byte4 ?
Byte5 ?
Byte6 bit7-bit3  track_id  bit2  ?   bit1 - bit 0   01 down 02 repeat 03 up
Byte7 bit3-bit0  xH
Byte8 bit3-bit0  yH
Byte9 bit7-bit4  xL  bit3-bit0  yL
ByteA ?
ByteB ?
ByteC for NTP_WAKEUP_SYSTEM_SUPPORT
ByteD bit2 Key3 bit1 Key2 bit0 Key1
*/

static int rda_nt11004_get_pos(struct i2c_client *client,
			struct rda_ts_pos_data * pos_data)
{
	int index,pos, x, y;
	u8  touch_num = 0;
	unsigned char track_id = 0;
	s32 idx = 0;
	unsigned char buf[15] = {0};
	s32 ret = -1;

#ifdef TS_WITH_PS_FUNCTIONS
	static u8 near_far = 0xff;
#endif

	rda_dbg_ts("%s\r\n",__func__);

	ret = ntp_i2c_read(client, buf, 15);
	rda_dbg_ts("ts buf: %x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x\n"
		,buf[0],buf[1],buf[2],buf[3],buf[4],buf[5],buf[6],buf[7]
		,buf[8],buf[9],buf[10],buf[11],buf[12],buf[13],buf[14]);

	if (ret < 0) {
		rda_dbg_ts("#########nt11004 I2C transfer error. errno:%d#########\n"
				,ret);

		if (ret == -EAGAIN) {
			return TS_I2C_RETRY;
		} else {
		    	return TS_I2C_ERROR;
		}
	}

#ifdef TS_WITH_PS_FUNCTIONS
	rda_dbg_ts("mode %d  near_far %x buf[14] %x\n"
			,g_ts_face_det_mode,near_far,buf[14]);

	/* ps distance data check */
	if (g_ts_face_det_mode ) {
		if ((near_far != buf[14]) && (0xf8 == buf[13]) ) {
			near_far = buf[14];

			if (buf[14] == 0x80) {
				printk(KERN_EMERG "%s the min range data \n",
					__func__);
				pos_data->distance = PSENSOR_NEAR_RANGE_MIN;
				pos_data->data_type = PS_DATA;
				rda_dbg_ts("msg2133 : ps :distance near return@@@\n");
				return TS_OK;
			} else if (buf[14] == 0x00) {
				printk(KERN_EMERG "%s the max range data \n",
					__func__);
				pos_data->distance = PSENSOR_NEAR_RANGE_MAX;
				pos_data->data_type = PS_DATA;
				rda_dbg_ts("msg2133 : ps : distance far return@@@\n");
				return TS_OK;
			} else {
				pos_data->distance = -1;
				pos_data->data_type = TS_DATA;
			}
		}
	}

/*when backlight off ,only ps data enable --by wyc0207*/
      if(g_ps_enable_screenoff)
	  	return TS_PACKET_ERROR;
/* --by wyc0207 end*/
#endif	/*  */

	touch_num =0;
	for (index = 0; index < MAX_FINGER_NUM; index++) {
		pos = 6*index+1;

		track_id = (buf[pos]>>3) - 1;

		if((track_id < MAX_FINGER_NUM)&&(track_id >= 0)) {
			if(((buf[pos]&0x03) == 0x01)||((buf[pos]&0x03) == 0x02)) {
				touch_num++;
			}
		}
	}

	pos_data->point_num = touch_num;

	if(touch_num==0) {
		pos_data->point_num = 0;
		pos_data->ts_position[0].x = 0;
		pos_data->ts_position[0].y = 0;
		rda_dbg_ts("nt11004 touch end\n");
	}

	if (touch_num) {
		s32 posc = 1;

		for (idx = 0; idx < touch_num; idx++) {
			posc += 6*idx;
			x = (int)((buf[posc+1]<<4) | ((buf[posc+3] & 0xf0)>>4));
			y = (int)((buf[posc+2]<<4) | ((buf[posc+3] & 0x0f)));
		    	//rda_dbg_ts("ts_position[%d].x=%d ts_position[%d].y=%d\n",idx,x,idx,y);
			x = x * LCD_MAX_WIDTH / TPD_MAX_WIDTH;
			y = y * LCD_MAX_HEIGHT / TPD_MAX_HEIGHT;
		    	//rda_dbg_ts("ts_position[%d].x=%d ts_position[%d].y=%d\n",idx,x,idx,y);
			pos_data->ts_position[idx].x = x;
			pos_data->ts_position[idx].y = y;
		}
	} else {
		u8 key1 = 0, key2 = 0, key3 = 0;
		static u8 key1_old = 0, key2_old = 0, key3_old = 0;

		key1 = (buf[14] & 0x01);
		key2 = (buf[14]  & 0x02);
		key3 = (buf[14]  & 0x04);

		if (key1 == 1) {
			pos_data->ts_position[0].x = 60;
			pos_data->ts_position[0].y = 850;
			pos_data->point_num = 1;
		} else if ((key1_old == 1) & (key1 == 0)) {

		}

		if (key2 == 2) {
			pos_data->ts_position[0].x = 180;
			pos_data->ts_position[0].y = 850;
			pos_data->point_num = 1;
		} else if ((key2_old == 2) & (key2 == 0)) {

		}

		if (key3 == 4) {
			pos_data->ts_position[0].x = 420;
			pos_data->ts_position[0].y = 850;
			pos_data->point_num = 1;
		}
		else if ((key3_old == 4) & (key3 == 0)) {

		}

		rda_dbg_ts("key1=%d key2=%d key3=%d\n",key1,key2,key3);

		key1_old = key1;
		key2_old = key2;
		key3_old = key3;
	}
	return 0;
}
/* [RDA] patch-for-ps-not-work-when-powerkey-press-to-sleep --start--*/
#ifdef TS_WITH_PS_FUNCTIONS

static void nt11004_delay_set_ps_mode(unsigned long ftdata)
 {
	int ret = 0;
	u8 data[2] = {0};
	struct i2c_msg msg;
	struct nt11004_data *mydata = (struct nt11004_data *)ftdata;
	struct i2c_client *client = mydata->my_i2c;
	bool    enable = mydata->ps_on;

	printk(KERN_EMERG "%s enable:%d \n", __func__, enable);

	if (enable)
		data[1] = 1;
	else
		data[1] = 0;
	data[0] = 0x88;
	msg.addr = client->addr;
	msg.flags = !I2C_M_RD;
	msg.len = 2;
	msg.buf = data;
	ret = i2c_transfer(client->adapter, &msg, 1);
	if (ret < 0) {
		/*g_ts_ps_switch_retry = true;*/
		printk(KERN_EMERG " delay enable : error \n");
	}
	g_ts_face_det_mode = enable;
	printk(KERN_EMERG"nt11004_delay_set_ps_mode %d \r\n",g_ts_face_det_mode);
	return ;
}

static int rda_nt11004_switch_ps_mode(struct i2c_client *client, int enable)
{
	my_data.my_i2c = client;
	my_data.ps_on = !!enable;

	printk(KERN_EMERG" rda_nt11004_switch_ps_mode %d \r\n",my_data.ps_on);

	mod_timer(&ps_timer,jiffies+HZ/2);
/*
	if(my_data.ps_on)
	{
		this tp will be in use.
		mod_timer(&ps_timer,jiffies+HZ/2);
	}
	else
	{
		del_timer(&ps_timer);
		nt11004_delay_set_ps_mode((unsigned long)&my_data);
	}
*/

	return 0;
}
#endif

/* [RDA] patch-for-ps-not-work-when-powerkey-press-to-sleep --end--*/
static int rda_nt11004_suspend(struct i2c_client *client, pm_message_t mesg)
{
	int ret = 0;
#ifdef NTP_WAKEUP_SYSTEM_SUPPORT
	unsigned char buf[2]={0x88, 0x5A};
	int i = 0;

	while(i++ <10) {
		buf[0] = 0x88;
		buf[1] = 0x5A;
		ntp_i2c_write(client, buf, 2);
		msleep(2);

		buf[0] = 0x98;
		ntp_i2c_read(buf, 2);
		rda_dbg_ts("rda_nt11004_suspend buf[0] = %x\n", buf[0]);

		if(buf[0] == 0xAA) {
			rda_dbg_ts("wakeup cmd OK\n");
			break;
		}
	}
#else
	unsigned char buf[2]={0x88, 0xAA};

	printk(KERN_EMERG"rda_nt11004_suspend \r\n");
	ret =ntp_i2c_write(client, buf, 2);

	/* Pull down the reset pin */
/*	gpio_set_value(GPIO_TOUCH_RESET, 1);
	mdelay(10);
*/
	ret = 0;
#endif

	return ret;
}


static int rda_nt11004_resume(struct i2c_client *client)
{
	rda_nt11004_reset_chip(client);
	rda_nt11004_startup_chip(client);
#ifdef TS_WITH_PS_FUNCTIONS

#if 0
	if ( g_ts_ps_switch_retry )
	{
		msleep(200);
		g_ts_ps_switch_retry = false;
		rda_nt11004_switch_ps_mode(client, my_data.ps_on);
	}
#else
       rda_nt11004_switch_ps_mode(client, my_data.ps_on);
#endif

#endif
	return 0;
}

struct rda_ts_panel_info nt11004_info = {
	/* struct rda_lcd_ops ops; */
	.ops = {
		.ts_init_gpio = rda_nt11004_init_gpio,
		.ts_init_chip = rda_nt11004_init,
		.ts_start_chip = rda_nt11004_startup_chip,
		.ts_reset_chip = rda_nt11004_reset_chip,
		.ts_get_parse_data = rda_nt11004_get_pos,
#ifdef TS_WITH_PS_FUNCTIONS
		.ts_switch_ps_mode = rda_nt11004_switch_ps_mode,
#endif
		.ts_suspend = rda_nt11004_suspend,
		.ts_resume = rda_nt11004_resume,
	},
	/* struct lcd_scr_info  src_info; */
	.ts_para = {
		.name = "nt11004",
		.i2c_addr = _DEF_I2C_ADDR_TS_NT11004,
		.x_max = NT11004_RES_X,
		.y_max = NT11004_RES_Y,
		.gpio_irq = GPIO_TOUCH_IRQ,
		.vir_key_num = 3,
		.irqTriggerMode = IRQF_TRIGGER_FALLING,
		.vir_key =
		{
			{
				KEY_BACK,
				VIR_KEY_BACK_X,
				VIR_KEY_BACK_Y,
				VIR_KEY_BACK_DX,
				VIR_KEY_BACK_DY,
			},
			{
				KEY_HOMEPAGE,
				VIR_KEY_HOMEPAGE_X,
				VIR_KEY_HOMEPAGE_Y,
				VIR_KEY_HOMEPAGE_DX,
				VIR_KEY_HOMEPAGE_DY,
			},
			{
				KEY_MENU,
				VIR_KEY_MENU_X,
				VIR_KEY_MENU_Y,
				VIR_KEY_MENU_DX,
				VIR_KEY_MENU_DY,
			},
		}

	}
};

int rda_ts_add_panel_nt11004(void)
{
	int ret = 0;

	ret = rda_ts_register_driver(&nt11004_info);
	if(ret)
		pr_err("rda_ts_add_panel NT11004 failed \n");

	return ret;
}
#ifdef TS_WITH_PS_FUNCTIONS
EXPORT_SYMBOL(g_ps_enable_screenoff);
#endif
EXPORT_SYMBOL(rda_ts_add_panel_nt11004);
