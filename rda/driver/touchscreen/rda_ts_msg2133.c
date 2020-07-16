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
#endif /* CONFIG_HAS_EARLYSUSPEND */

#include <linux/platform_device.h>

#include <mach/hardware.h>
#include <plat/devices.h>
#include <plat/rda_debug.h>
#include <mach/board.h>
#include <linux/gpio.h>
#include "rda_ts.h"

#ifdef TS_FIRMWARE_UPDATE
#include "msg2133_auto_update_firmware.h"
#endif

/* for most scenes of psensor : distance range such as [0-1] means near, other means far
 * when we want to pass 'near', just set distance to be a value in range [min - max]
 * when we want to pass 'far', just set distance to be a value not.
 * same as range defined in like "hardware/rda/rda8810/libsensors/STKProximitySensor.cpp" sensor list
*/
#define PSENSOR_NEAR_RANGE_MIN 0
#define PSENSOR_NEAR_RANGE_MAX 1

enum {
	TP_YUANSHENG = 0,
	TP_HUANGZE,
	TP_TONGXINCHENG,
};

static u8 g_ts_vendor = TP_TONGXINCHENG;


//#define TPD_XY_INVERT

#if 0
#ifdef rda_dbg_ts
#undef rda_dbg_ts
#define rda_dbg_ts printk
#endif
#endif
#if defined(VIR_WVGA_VER)
#define VIR_KEY_BACK_X          280
#define VIR_KEY_BACK_Y         	854
#define VIR_KEY_BACK_DX         60
#define VIR_KEY_BACK_DY         20

#define VIR_KEY_HOMEPAGE_X      200
#define VIR_KEY_HOMEPAGE_Y      854
#define VIR_KEY_HOMEPAGE_DX     60
#define VIR_KEY_HOMEPAGE_DY     20

#define VIR_KEY_MENU_X          120
#define VIR_KEY_MENU_Y          854
#define VIR_KEY_MENU_DX         60
#define VIR_KEY_MENU_DY         20

#define VIR_KEY_SEARCH_X        40
#define VIR_KEY_SEARCH_Y        854
#define VIR_KEY_SEARCH_DX       60
#define VIR_KEY_SEARCH_DY       20

#else
#define VIR_KEY_BACK_X          280
#define VIR_KEY_BACK_Y          490
#define VIR_KEY_BACK_DX         60
#define VIR_KEY_BACK_DY         20

#define VIR_KEY_HOMEPAGE_X      200
#define VIR_KEY_HOMEPAGE_Y      490
#define VIR_KEY_HOMEPAGE_DX     60
#define VIR_KEY_HOMEPAGE_DY     20

#define VIR_KEY_MENU_X          120
#define VIR_KEY_MENU_Y          490
#define VIR_KEY_MENU_DX         60
#define VIR_KEY_MENU_DY         20

#define VIR_KEY_SEARCH_X        40
#define VIR_KEY_SEARCH_Y        490
#define VIR_KEY_SEARCH_DX       60
#define VIR_KEY_SEARCH_DY       20

#endif
#define RDA_TS_EVENT_LENGTH 8

static void rda_msg2133_reset_gpio(void)
{
	gpio_set_value(GPIO_TOUCH_RESET, 1);
	mdelay(5);		// Delay 5 ms
	gpio_set_value(GPIO_TOUCH_RESET, 0);
	mdelay(80);		// Delay 80 ms
	gpio_set_value(GPIO_TOUCH_RESET, 1);
	mdelay(80);// Delay 80 ms
}

static void rda_msg2133_init_gpio(void)
{
	gpio_request(GPIO_TOUCH_RESET, "touch screen reset");
	gpio_request(GPIO_TOUCH_IRQ, "touch screen interrupt");

	gpio_direction_output(GPIO_TOUCH_RESET, 0);
	mdelay(1);		// Delay 10ms
	gpio_direction_input(GPIO_TOUCH_IRQ);
	mdelay(1);

	rda_msg2133_reset_gpio();
}

static void rda_msg2133_startup_chip(struct i2c_client *client)
{
	//rda_msg2133_reset_gpio();
}

static void rda_msg2133_reset_chip(struct i2c_client *client)
{
	rda_msg2133_reset_gpio();
}

static void rda_msg2133_load_fw(struct i2c_client *client)
{
	return;
}

static void rda_msg2133_init_chip(struct i2c_client *client)
{
	rda_msg2133_reset_chip(client);
	rda_msg2133_load_fw(client);
	rda_msg2133_startup_chip(client);
	rda_msg2133_reset_chip(client);
	rda_msg2133_reset_gpio();
	rda_msg2133_reset_chip(client);
	rda_msg2133_startup_chip(client);
}

static void rda_msg2133_check_mem_data(struct i2c_client *client)
{
	return;
}

static void rda_tsl1680_check_chip_ver(struct i2c_client *client)
{
	return;
}

static void rda_msg2133_init(struct i2c_client *client)
{
	rda_tsl1680_check_chip_ver(client);
	rda_msg2133_init_chip(client);
	rda_msg2133_check_mem_data(client);
}

unsigned char check_sum(unsigned char *pval)
{
	int i, sum = 0;

	for (i = 0; i < 7; i++) {
		sum += pval[i];
	}

	return (unsigned char)((-sum) & 0xFF);
}

static int rda_msg2133_switch_ps_mode(struct i2c_client *client,
			       int enable)
{
	int ret = 0;
	u8 data[4] = {0};
	struct i2c_msg msg;

	if(g_ts_vendor == TP_YUANSHENG) {
		data[0] = 0x52;
		data[1] = 0x01;
		data[2] = 0x24;
		if(enable)
			data[3] = 0xA0;
		else
			data[3] = 0xA0;
	}
	else if(g_ts_vendor == TP_HUANGZE) {
		data[0] = 0xFF;
		data[1] = 0x11;
		data[2] = 0xFF;
		if(enable)
			data[3] = 0x01;
		else
			data[3] = 0x00;
	}
	else if(g_ts_vendor == TP_TONGXINCHENG) {
		data[0] = 0x52;
		data[1] = 0x00;
		data[2] = 0x4A;
		if(enable)
			data[3] = 0xA0;
		else
			data[3] = 0xA1;
	}

	msg.addr = _DEF_I2C_ADDR_TS_MSG2133;
	msg.flags = !I2C_M_RD;
	msg.len = 4;
	msg.buf = data;

	ret = i2c_transfer(client->adapter, &msg, 1);

	if (ret < 0) {
		return -1;
	}

	return 0;
}

static int rda_msg2133_get_pos(struct i2c_client *client,
			       struct rda_ts_pos_data *pos_data)
{
	int ret;
	int pos_x, pos_y, dst_x, dst_y;
	int pos_x2, pos_y2;
	int x, y, x2, y2;
	int virKeyCode;
	u32 temp = 0;

	u8 data[RDA_TS_EVENT_LENGTH] = { 0 };


	struct i2c_msg msg;
	msg.addr = _DEF_I2C_ADDR_TS_MSG2133;
	msg.flags = I2C_M_RD;
	msg.len = RDA_TS_EVENT_LENGTH;
	msg.buf = data;

	memset(data, 0, RDA_TS_EVENT_LENGTH);
	ret = i2c_transfer(client->adapter, &msg, 1);
	/* read data from DATA_REG */
	//ret = rda_ts_i2c_read(client, msg.addr, data, sizeof(data));

	temp = check_sum(data);

	if (ret < 0) {
		return TS_ERROR;
	}

	/* ps distance data check */
	if(data[5] == 0x80) {
		pos_data->distance = PSENSOR_NEAR_RANGE_MIN;
		pos_data->data_type = PS_DATA;
		rda_dbg_ts("msg2133 : ps :distance near\n");
		return TS_OK;
	} else if(data[5] == 0x40) {
		pos_data->distance = PSENSOR_NEAR_RANGE_MAX;
		pos_data->data_type = PS_DATA;
		rda_dbg_ts("msg2133 : ps : distance far\n");
		return TS_OK;
	} else {
		pos_data->distance = -1;
		pos_data->data_type = TS_DATA;
	}

	pos_x = ((data[1] & 0xF0) << 4) | data[2];
	pos_y = ((data[1] & 0x0F) << 8) | data[3];
	dst_x = ((data[4] & 0xF0) << 4) | data[5];
	dst_y = ((data[4] & 0x0F) << 8) | data[6];
	pos_data->point_num = 0;
	if (temp != data[7] || data[0] != 0x52) {
		return TS_NOTOUCH;
	} else {
		if (data[1] == 0xff && data[2] == 0xff && data[3] == 0xff &&
			data[4] == 0xff && data[6] == 0xff) {
			x = y = 0;
			//touch end
			if (data[5] == 0 || data[5] == 0xff) {
				pos_data->point_num = 0;
				pos_data->ts_position[0].x = 0;
				pos_data->ts_position[0].y = 0;
			} else {
				virKeyCode = data[5];
				pos_data->point_num = 1;
#if 0
				if (virKeyCode == 1) {
					//search key
					pos_data->ts_position[0].x = 40 * 2048/320;
					pos_data->ts_position[0].y = 490 * 2048/480;
				} else
#endif
				if (virKeyCode == 1) {
				//menu key
					pos_data->ts_position[0].x = VIR_KEY_MENU_X * MSG2133_RES_X / PANEL_XSIZE;
					pos_data->ts_position[0].y = VIR_KEY_MENU_Y * MSG2133_RES_Y / PANEL_YSIZE;
				} else if (virKeyCode == 2) {
					//home key
					pos_data->ts_position[0].x = VIR_KEY_HOMEPAGE_X * MSG2133_RES_X / PANEL_XSIZE;
					pos_data->ts_position[0].y = VIR_KEY_HOMEPAGE_Y * MSG2133_RES_Y / PANEL_YSIZE;
				} else {
					//back key
					pos_data->ts_position[0].x = VIR_KEY_BACK_X * MSG2133_RES_X / PANEL_XSIZE;
					pos_data->ts_position[0].y = VIR_KEY_BACK_Y * MSG2133_RES_Y /PANEL_YSIZE;
				}
			}
		} else {
			if (dst_x == 0 && dst_y == 0) {
				pos_data->point_num = 1;
#ifdef TPD_XY_INVERT
				x = pos_y;
				y = pos_x;
#else
				x = pos_x;
				y = pos_y;
#endif
				pos_data->ts_position[0].x = x;
				pos_data->ts_position[0].y = y;
			} else {
				if (dst_x > 2048)
					dst_x -= 4096;
				if (dst_y > 2048)
					dst_y -= 4096;

				pos_x2 = pos_x + dst_x;
				pos_y2 = pos_y + dst_y;

#ifdef TPD_XY_INVERT
				x = pos_y;
				y = pos_x;
				x2 = pos_y2;
				y2 = pos_x2;
#else
				x = pos_x;
				y = pos_y;
				x2 = pos_x2;
				y2 = pos_y2;
#endif

				pos_data->point_num = 2;
				pos_data->ts_position[0].x = x;
				pos_data->ts_position[0].y = y;
				pos_data->ts_position[1].x = x2;
				pos_data->ts_position[1].y = y2;
			}
		}
		return TS_OK;
	}

}

static int rda_msg2133_suspend(struct i2c_client *client, pm_message_t mesg)
{

	/* Pull down the reset pin */
	gpio_set_value(GPIO_TOUCH_RESET, 0);
	mdelay(10);

	return 0;
}

static int rda_msg2133_resume(struct i2c_client *client)
{
	gpio_set_value(GPIO_TOUCH_RESET, 1);
	mdelay(1);

	rda_msg2133_reset_chip(client);
	rda_msg2133_startup_chip(client);
	rda_msg2133_check_mem_data(client);
	mdelay(30);

	return 0;
}

#ifdef TS_FIRMWARE_UPDATE
static int msg2133_register_utils_funcs(struct i2c_client *client)
{
	msg2133_create_sysfs(client);
	return 0;
}
static int msg2133_unregister_utils_funcs(struct i2c_client *client)
{
	return 0;
}
#endif

struct rda_ts_panel_info msg2133_info = {
	/* struct rda_lcd_ops ops; */
	.ops = {
		.ts_init_gpio = rda_msg2133_init_gpio,
		.ts_init_chip = rda_msg2133_init,
		.ts_start_chip = rda_msg2133_startup_chip,
		.ts_reset_chip = rda_msg2133_reset_chip,
		.ts_get_parse_data = rda_msg2133_get_pos,
		.ts_switch_ps_mode = rda_msg2133_switch_ps_mode,
#ifdef TS_FIRMWARE_UPDATE
		.ts_register_utils_funcs = msg2133_register_utils_funcs,
		.ts_unregister_utils_funcs = msg2133_unregister_utils_funcs,
#endif
		.ts_suspend = rda_msg2133_suspend,
		.ts_resume = rda_msg2133_resume,
		},
	/* struct lcd_scr_info  src_info; */
	.ts_para = {
		.name = "msg2133",
		.i2c_addr = _DEF_I2C_ADDR_TS_MSG2133,
		.x_max = MSG2133_RES_X,
		.y_max = MSG2133_RES_Y,
		.gpio_irq = GPIO_TOUCH_IRQ,
		.vir_key_num = 4,
		.irqTriggerMode = IRQF_TRIGGER_FALLING,
		.vir_key = {
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
		{
			KEY_SEARCH,
			VIR_KEY_SEARCH_X,
			VIR_KEY_SEARCH_Y,
			VIR_KEY_SEARCH_DX,
			VIR_KEY_SEARCH_DY,
		},
		}
	}
};

int rda_ts_add_panel_msg2133(void)
{
	int ret = 0;

	ret = rda_ts_register_driver(&msg2133_info);
	if (ret)
		pr_err("rda_ts_add_panel GSL11680 failed \n");

	return ret;
}

EXPORT_SYMBOL(rda_ts_add_panel_msg2133);
