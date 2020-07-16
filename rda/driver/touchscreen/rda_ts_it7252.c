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
#include "it7252.h"


static void rda_IT7252_reset_gpio(void)
{
	gpio_set_value(GPIO_TOUCH_RESET, 1);
	mdelay(1);
	gpio_set_value(GPIO_TOUCH_RESET, 0);
	mdelay(1);		// Delay 10 ms
	gpio_set_value(GPIO_TOUCH_RESET, 1);
	mdelay(1);		// Delay 10 ms
	gpio_set_value(GPIO_TOUCH_RESET, 0);
	mdelay(5);		// Delay 10 ms
}

static void rda_IT7252_init_gpio(void)
{
	gpio_request(GPIO_TOUCH_RESET, "touch screen reset");
	gpio_request(GPIO_TOUCH_IRQ, "touch screen interrupt");

	gpio_direction_output(GPIO_TOUCH_RESET, 0);
	mdelay(1);		// Delay 10ms
	gpio_direction_input(GPIO_TOUCH_IRQ);
	mdelay(1);

	rda_IT7252_reset_gpio();
}

static int rda_it7252_i2c_read(struct i2c_client *client, u8 addr, u8 * pdata,
			       unsigned int datalen)
{
	struct i2c_msg msgs[] = {
		{
		 .addr = client->addr,
		 .flags = 0,
		 .len = 1,
		 .buf = &addr,
		 },
		{
		 .addr = client->addr,
		 .flags = I2C_M_RD,
		 .len = datalen,
		 .buf = pdata,
		 },
	};

	if (i2c_transfer(client->adapter, msgs, 2) < 0) {
		pr_err("rda_gsensor_i2c_read: transfer error\n");
		return EIO;
	} else
		return 0;
}

static void rda_IT7252_startup_chip(struct i2c_client *client)
{
	rda_dbg_ts("rda_IT7252_startup_chip\n");
}

static void rda_IT7252_reset_chip(struct i2c_client *client)
{
	rda_IT7252_reset_gpio();
}

static void rda_IT7252_init(struct i2c_client *client)
{
	rda_dbg_ts("rda_IT7252_init\n");
}

static int rda_IT7252_get_pos(struct i2c_client *client,
			      struct rda_ts_pos_data *pos_data)
{
	int ret;
	u8 read_buf[1] = { 0 };
	u8 read_data[14] = { 0 };

	ret = rda_it7252_i2c_read(client, 0x80, read_buf, 1);
	rda_dbg_ts("IT7252 rda_IT7252_get_pos read_buf[0]=%d\n", read_buf[0]);
	if (ret == EIO) {
		rda_dbg_ts("IT7252 rda_IT7252_get_pos error\n");
		return TS_ERROR;
	}

	if (read_buf[0] < 0) {
		rda_dbg_ts("IT7252 rda_IT7252_get_pos00 error\n");
		return TS_ERROR;
	} else {
		if (read_buf[0] & 0x80)
			/* read data from DATA_REG */
		{
			pos_data->point_num = 0;
			ret =
			    rda_it7252_i2c_read(client, 0xE0, read_data,
						sizeof(read_data));
			if (ret == EIO) {
				rda_dbg_ts("IT7252 rda_IT7252_get_pos01 \n");
				return TS_ERROR;
			}
			// gesture
			if (read_data[0] & 0xF0) {
				if ((read_data[0] & 0x41) == 0x41) {
					if (read_data[2]) {
						switch (read_data[1]) {
						case 1:
							break;
						case 2:
							break;
						case 3:
							break;
						}
					} else {
						switch (read_data[1]) {
						case 1:
							break;
						case 2:
							break;
						case 3:
							break;
						}
					}
				}

				rda_dbg_ts("IT7252 rda_IT7252_get_pos02 \n");
				//pr_info("(read_data[0] & 0xF0) is true, it's a gesture\n") ;
				//pr_info("read_data[0]=%x\n", read_data[0]);
				return TS_NOTOUCH;
			}
			// palm
			if (read_data[1] & 0x01) {
				rda_dbg_ts("IT7252 rda_IT7252_get_pos03 \n");
				return TS_NOTOUCH;
			}
			// no more data
			if (!(read_data[0] & 0x08)) {
				rda_dbg_ts("IT7252 rda_IT7252_get_pos04 \n");
				pos_data->point_num = 0;
				return TS_NOTOUCH;
			}
			// 3 fingers
			if (read_data[0] & 0x04) {
				rda_dbg_ts("IT7252 rda_IT7252_get_pos05 \n");
				return TS_NOTOUCH;
			}

			if (read_data[0] & 0x01) {

				pos_data->ts_position[pos_data->point_num].x =
				    ((read_data[3] & 0x0F) << 8) + read_data[2];
				pos_data->ts_position[pos_data->point_num].y =
				    ((read_data[3] & 0xF0) << 4) + read_data[4];
				rda_dbg_ts("rda_IT7252_pos finger1 x=%d,y=%d\n",
					   pos_data->ts_position[pos_data->
								 point_num].x,
					   pos_data->ts_position[pos_data->
								 point_num].y);
				pos_data->point_num = 1;
			}

			if (read_data[0] & 0x02) {
				pos_data->ts_position[pos_data->point_num].x =
				    ((read_data[7] & 0x0F) << 8) + read_data[6];
				pos_data->ts_position[pos_data->point_num].y =
				    ((read_data[7] & 0xF0) << 4) + read_data[8];
				rda_dbg_ts("rda_IT7252_pos finger1 x=%d,y=%d\n",
					   pos_data->ts_position[pos_data->
								 point_num].x,
					   pos_data->ts_position[pos_data->
								 point_num].y);
				pos_data->point_num = 2;
			}
		} else
			pos_data->point_num = 0;
	}
	return TS_OK;
}

static int rda_IT7252_suspend(struct i2c_client *client, pm_message_t mesg)
{
	u8 writevalue[3] = { 0x04, 0x00, 0x02 };

	rda_ts_i2c_write(client, 0x20, writevalue, 3);
	return 0;
}

static int rda_IT7252_resume(struct i2c_client *client)
{
	u8 read_buf[1] = { 0 };
	u8 i = 0;

	for (i = 0; i < 5; i++)
		rda_it7252_i2c_read(client, 0x80, read_buf, 1);

	return 0;
}

struct rda_ts_panel_info IT7252_info = {
	/* struct rda_lcd_ops ops; */
	.ops = {
		.ts_init_gpio = rda_IT7252_init_gpio,
		.ts_init_chip = rda_IT7252_init,
		.ts_start_chip = rda_IT7252_startup_chip,
		.ts_reset_chip = rda_IT7252_reset_chip,
		.ts_get_parse_data = rda_IT7252_get_pos,
		.ts_suspend = rda_IT7252_suspend,
		.ts_resume = rda_IT7252_resume,
		},
	/* struct lcd_scr_info  src_info; */
	.ts_para = {
		.name = "IT7252",
		.i2c_addr = _DEF_I2C_ADDR_TS_IT7252,
		.x_max = IT7252_RES_X,
		.y_max = IT7252_RES_Y,
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

int rda_ts_add_panel_it7252(void)
{
	int ret = 0;

	ret = rda_ts_register_driver(&IT7252_info);
	if (ret)
		pr_err("rda_ts_add_panel IT7252 failed \n");

	return ret;
}
