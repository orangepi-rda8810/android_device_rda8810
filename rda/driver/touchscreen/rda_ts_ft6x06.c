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
#include <mach/board.h>
#include <linux/gpio.h>
#include "rda_ts.h"
/* for most scenes of psensor : distance range such as [0-1] means near, other means far
* when we want to pass 'near', just set distance to be a value in range [min - max]
* when we want to pass 'far', just set distance to be a value not.
* same as range defined in like "hardware/rda/rda8810/libsensors/STKProximitySensor.cpp" sensor list
*/
#define PSENSOR_NEAR_RANGE_MIN 0
#define PSENSOR_NEAR_RANGE_MAX 1
static bool g_ts_face_det_turnon = false;

#include "ft6x06.h"
static void rda_ft6x06_reset_gpio(void)
{
	rda_dbg_ts(" rda_ft6x06_reset_gpio\n");
	gpio_set_value(GPIO_TOUCH_RESET, 0);
	mdelay(1);
	gpio_set_value(GPIO_TOUCH_RESET, 1);
	mdelay(1);		// Delay 10 ms
	gpio_set_value(GPIO_TOUCH_RESET, 0);
	mdelay(1);		// Delay 10 ms
	gpio_set_value(GPIO_TOUCH_RESET, 1);
	mdelay(25);		// Delay 200 ms
}
static void rda_ft6x06_init_gpio(void)
{
	rda_dbg_ts(" rda_ft6x06_init_gpio\n");
	gpio_request(GPIO_TOUCH_RESET, "touch screen reset");
	gpio_request(GPIO_TOUCH_IRQ, "touch screen interrupt");
	gpio_direction_output(GPIO_TOUCH_RESET, 0);
	mdelay(1);		// Delay 10ms
	gpio_direction_input(GPIO_TOUCH_IRQ);
	mdelay(1);
	rda_ft6x06_reset_gpio();
}
static void rda_ft6x06_startup_chip(struct i2c_client *client)
{
	rda_dbg_ts(" rda_ft6x06_startup_chip\n");
}
static void rda_ft6x06_reset_chip(struct i2c_client *client)
{
	rda_dbg_ts(" rda_ft6x06_reset_chip\n");

	//rda_ft6x06_reset_gpio();
}
static void rda_ft6x06_init(struct i2c_client *client)
{
	rda_dbg_ts(" rda_ft6x06_init\n");
}

static int rda_ft6x06_switch_ps_mode(struct i2c_client *client, int enable)
{
	int ret = 0;
	u8 data[2] = {
		0};
	struct i2c_msg msg;
	printk(KERN_EMERG "%s enable:%d \n", __func__, enable);
	if (enable)
		data[1] = 1;
	else
		data[1] = 0;
	data[0] = 0xb0;
	msg.addr = client->addr;
	msg.flags = !I2C_M_RD;
	msg.len = 2;
	msg.buf = data;
	ret = i2c_transfer(client->adapter, &msg, 1);
	if (ret < 0) {
		return -1;
	}
	g_ts_face_det_turnon = !!enable;
	return 0;
}

static int rda_ft6x06_get_pos(struct i2c_client *client,
		struct rda_ts_pos_data *pos_data)
{
	int ret;
	u8 buf[POINT_READ_BUF] = {0};
	u8 pointid = FT_MAX_ID;
	int i = 0;

	static u8 near_far = 0;

	ret = rda_ts_i2c_read(client, 0x0, buf, POINT_READ_BUF);
	if (ret < 0)
		return TS_ERROR;

	/* ps distance data check */
	if (g_ts_face_det_turnon && (near_far != buf[1])) {
		near_far = buf[1];
		if (buf[1] == 0xc0) {
			printk(KERN_EMERG "%s the min range data \n",
					__func__);
			pos_data->distance = PSENSOR_NEAR_RANGE_MIN;
			pos_data->data_type = PS_DATA;
			rda_dbg_ts("ft6x06 : ps :distance near\n");
			return TS_OK;
		} else if (buf[1] == 0xe0) {
			printk(KERN_EMERG "%s the max range data \n",
					__func__);
			pos_data->distance = PSENSOR_NEAR_RANGE_MAX;
			pos_data->data_type = PS_DATA;
			rda_dbg_ts("ft6x06 : ps : distance far\n");
			return TS_OK;
		} else {
			pos_data->distance = -1;
			pos_data->data_type = TS_DATA;
		}
	} else {
		pos_data->distance = -1;
		pos_data->data_type = TS_DATA;
	}

	if ((buf[2] & 0x0F) == 0) {
		pos_data->point_num = 0;
		rda_dbg_ts(" rda_ft6x06_get_pos,no touch\n");
		return TS_OK;
	} else {
		pos_data->point_num = 0;
	}

	for (i = 0; i < CFG_MAX_TOUCH_POINTS; i++) {
		pointid = (buf[FT_TOUCH_ID_POS + FT_TOUCH_STEP * i]) >> 4;
		if (pointid >= FT_MAX_ID) {

			//pos_data->point_num=0;
			rda_dbg_ts(" rda_ft6x06_get_pos,pointid=%d\n",
					pointid);
			break;
		} else {
			pos_data->point_num++;
		}

		//event->touch_point++;
		pos_data->ts_position[i].x =
			(s16) (buf[FT_TOUCH_X_H_POS + FT_TOUCH_STEP * i] & 0x0F) <<
			8 | (s16) buf[FT_TOUCH_X_L_POS + FT_TOUCH_STEP * i];
		pos_data->ts_position[i].y =
			(s16) (buf[FT_TOUCH_Y_H_POS + FT_TOUCH_STEP * i] & 0x0F) <<
			8 | (s16) buf[FT_TOUCH_Y_L_POS + FT_TOUCH_STEP * i];
		rda_dbg_ts(" rda_ft6x06_get_pos i=%d, x=%d, y=%d\n", i,
				pos_data->ts_position[i].x,
				pos_data->ts_position[i].y);
	}
	return TS_OK;
}

static int rda_ft6x06_suspend(struct i2c_client *client,
		pm_message_t mesg)
{
	u8 wr_data = 3;
	rda_dbg_ts(" rda_ft6x06_suspend\n");
	rda_ts_i2c_write(client, 0xA5, &wr_data, 1);
	return 0;
}

static int rda_ft6x06_resume(struct i2c_client *client)
{
	rda_dbg_ts(" rda_ft6x06_resume\n");
	rda_ft6x06_reset_gpio();
	return 0;
}
static int ft6x0x_get_chip_id(struct i2c_client *client)
{
	u8 uc_tp_fm_ver = 0xff;
	u8 uc_tp_fm_id = 0xff;
	int ret = 0;

	rda_dbg_ts(" ft6x0x_get_chip_id\n");

	ret = rda_ts_i2c_read(client, 0xA6,&uc_tp_fm_ver, 1);
	if(ret < 0)
		return -1;

	ret =  rda_ts_i2c_read(client, 0xA8,&uc_tp_fm_id, 1);
	if (ret < 0)
		return -1;

	printk(KERN_EMERG" %s : uc_tp_fm_ID:%x \n",__func__,uc_tp_fm_id);
	printk(KERN_EMERG " %s : uc_tp_fm_ver:%x \n",__func__,uc_tp_fm_ver);
	if(uc_tp_fm_id != 0xff ||uc_tp_fm_ver != 0xff )
		return 0;

	return -1;
}


struct rda_ts_panel_info ft6x06_info = {
	/* struct rda_lcd_ops ops; */
	.ops = {
		.ts_init_gpio = rda_ft6x06_init_gpio,
		.ts_init_chip = rda_ft6x06_init,
		.ts_start_chip = rda_ft6x06_startup_chip,
		.ts_reset_chip = rda_ft6x06_reset_chip,
		.ts_get_parse_data = rda_ft6x06_get_pos,
		.ts_switch_ps_mode = rda_ft6x06_switch_ps_mode,
		.ts_suspend = rda_ft6x06_suspend,
		.ts_resume = rda_ft6x06_resume,
		.ts_get_chip_id= ft6x0x_get_chip_id,

	},
	/* struct lcd_scr_info  src_info; */
	.ts_para = {
		.name = "ft6x06",
		.i2c_addr = _DEF_I2C_ADDR_TS_FT6x06,
		.x_max = FT6X06_RES_X,
		.y_max = FT6X06_RES_Y,
		.gpio_irq = GPIO_TOUCH_IRQ,
		.vir_key_num = 3,
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
		}
	}
};

int rda_ts_add_panel_ft6x06(void)
{
	int ret = 0;
	ret = rda_ts_register_driver(&ft6x06_info);
	if (ret)
		pr_err("rda_ts_add_panel ft6x06 failed \n");
	return ret;
}


