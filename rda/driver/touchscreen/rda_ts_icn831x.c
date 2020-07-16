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


#define TPD_X_REVERSE
//#define TPD_Y_REVERSE


#define POINT_NUM 5
#define POINT_SIZE 7

#define VIR_KEY_BACK_X          20
#define VIR_KEY_BACK_Y          498
#define VIR_KEY_BACK_DX         60
#define VIR_KEY_BACK_DY         30

#define VIR_KEY_HOMEPAGE_X      140
#define VIR_KEY_HOMEPAGE_Y      498
#define VIR_KEY_HOMEPAGE_DX     60
#define VIR_KEY_HOMEPAGE_DY     30

#define VIR_KEY_MENU_X          300
#define VIR_KEY_MENU_Y          498
#define VIR_KEY_MENU_DX         60
#define VIR_KEY_MENU_DY         30

static void rda_icn831x_reset_gpio(void)
{
	gpio_set_value(GPIO_TOUCH_RESET, 1);
	mdelay(20);		// Delay 10 ms
	gpio_set_value(GPIO_TOUCH_RESET, 0);
	mdelay(20);		// Delay 10 ms
	gpio_set_value(GPIO_TOUCH_RESET, 1);
	mdelay(100);	// Delay 10 ms
}

static void rda_icn831x_init_gpio(void)
{
	gpio_request(GPIO_TOUCH_RESET, "touch screen reset");
	gpio_request(GPIO_TOUCH_IRQ, "touch screen interrupt");

	gpio_direction_output(GPIO_TOUCH_RESET, 0);
	mdelay(1);		// Delay 10ms
	gpio_direction_input(GPIO_TOUCH_IRQ);
	mdelay(1);

	rda_icn831x_reset_gpio();
}

static void rda_icn831x_startup_chip(struct i2c_client *client)
{
	//rda_icn831x_reset_gpio();
}

static void rda_icn831x_reset_chip(struct i2c_client *client)
{
	rda_icn831x_reset_gpio();
}

static void rda_icn831x_load_fw(struct i2c_client *client)
{
	return;
}

static void rda_icn831x_init_chip(struct i2c_client *client)
{
	//rda_icn831x_reset_chip(client);
	rda_icn831x_load_fw(client);
}

static void rda_icn831x_check_mem_data(struct i2c_client *client)
{
	return;
}

static void rda_icn831x_check_chip_ver(struct i2c_client *client)
{
	int err = 0;
	u8 buf[1] = {0};

	err = rda_ts_i2c_read(client,0x0A,buf,1);
	if (err < 0)
	{
		rda_dbg_ts("icn831x_iic_test  failed.\n");
	}
	else
	{
		rda_dbg_ts("icn831x_iic communication ok\n");
	}
	
	if(buf[0] == 0x83)
	{
		rda_dbg_ts("icn831x_iic read chipid ok\n");
	}
	else
	{
		rda_dbg_ts("rda_icn831x_check_chip_ver: buf[0] =%x\n", buf[0]);
	}

}

static void rda_icn831x_init(struct i2c_client *client)
{
	rda_icn831x_reset_gpio();
	rda_icn831x_check_chip_ver(client);
	rda_icn831x_init_chip(client);
	rda_icn831x_check_mem_data(client);
}

static inline u16 rda_icn831x_join_bytes(u8 a, u8 b)
{
	u16 ab = 0;
	ab = ab | a;
	ab = ab << 8 | b;
	return ab;
}

static void  rda_icn831x_process_data(u8 * read_data,
			struct rda_ts_pos_data * pos_data)
{
	u8 id, touches;
	u16 x, y;
	int i = 0;
	//unsigned char finger_current[POINT_NUM + 1] = { 0 };

	pos_data->point_num = 0;

	touches = read_data[1];

	for(i=0;i<(touches > MAX_FINGERS ? MAX_FINGERS : touches);i++)
	{
		id = read_data[POINT_SIZE * i + 2];
		x = rda_icn831x_join_bytes(read_data[POINT_SIZE * i + 3],read_data[POINT_SIZE * i + 4]);
		y = rda_icn831x_join_bytes(read_data[POINT_SIZE * i + 5],read_data[POINT_SIZE * i + 6]);

#ifdef TPD_X_REVERSE
		x = ICN831X_RES_X - x;
#endif

#ifdef TPD_Y_REVERSE
		y = ICN831X_RES_Y - y;
#endif

		rda_dbg_ts("get touch point: id= %d, x= %d, y= %d \n", id, x, y);

		if(0 <= id && id < MAX_TOUCH_POINTS)
		{
			pos_data->ts_position[pos_data->point_num].x = x;
			pos_data->ts_position[pos_data->point_num].y = y;
			pos_data->point_num += 1;
		}
	}

	rda_dbg_ts(" touches: is %d \n",touches);

	return;

}
static int rda_icn831x_get_pos(struct i2c_client *client,
			       struct rda_ts_pos_data *pos_data)
{
	int ret;
	//u8 read_buf[4] = {0};
	u8 read_data[POINT_NUM*POINT_SIZE+3]= {0};

	/* read data from DATA_REG */
	ret = rda_ts_i2c_read(client, 0x10, read_data, (POINT_NUM*POINT_SIZE+2));
	if (ret < 0)
	{
		rda_dbg_ts("rda_icn831x_get_pos: icn831x_iic_test  failed.\n");
		return TS_ERROR;
	}

	if(read_data[1]==0)
	{
		rda_dbg_ts("*******rda_icn831x_get_pos:no_touch\n");
		pos_data->point_num = 0;
              return TS_NOTOUCH;
	}
	else
       {
              rda_icn831x_process_data(read_data, pos_data);
       }

	return TS_OK;
}

static int rda_icn831x_suspend(struct i2c_client *client, pm_message_t mesg)
{

	/* Pull down the reset pin */
	gpio_set_value(GPIO_TOUCH_RESET, 0);
	mdelay(10);

	return 0;
}

static int rda_icn831x_resume(struct i2c_client *client)
{
	gpio_set_value(GPIO_TOUCH_RESET, 1);
	mdelay(10);

	rda_icn831x_reset_chip(client);
	rda_icn831x_startup_chip(client);
	rda_icn831x_check_mem_data(client);
	mdelay(30);

	return 0;
}

struct rda_ts_panel_info icn831x_info = {
	/* struct rda_lcd_ops ops; */
	.ops = {
		.ts_init_gpio = rda_icn831x_init_gpio,
		.ts_init_chip = rda_icn831x_init,
		.ts_start_chip = rda_icn831x_startup_chip,
		.ts_reset_chip = rda_icn831x_reset_chip,
		.ts_get_parse_data = rda_icn831x_get_pos,
		.ts_suspend = rda_icn831x_suspend,
		.ts_resume = rda_icn831x_resume,
		},
	/* struct lcd_scr_info  src_info; */
	.ts_para = {
		.name = "icn831x",
		.i2c_addr = _DEF_I2C_ADDR_TS_ICN831X,
		.x_max = ICN831X_RES_X,
		.y_max = ICN831X_RES_Y,
		.gpio_irq = GPIO_TOUCH_IRQ,
		.vir_key_num = 3,
		.irqTriggerMode = IRQF_TRIGGER_RISING,
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

int rda_ts_add_panel_icn831x(void)
{
	int ret = 0;

	ret = rda_ts_register_driver(&icn831x_info);
	if (ret)
		pr_err("rda_ts_add_panel icn831x failed \n");

	return ret;
}

