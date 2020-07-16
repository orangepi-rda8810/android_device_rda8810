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


#ifdef GSL168X_MODEL_35_HVGA_1680
#include "gsl168x_model_35_hvga_1680.h"
#elif defined(GSL168X_MODEL_35_HVGA_1688E)
#include "gsl168x_model_35_hvga_1688e.h"
#elif defined(GSL168X_MODEL_35_HVGA_1688E_KRTR118)
#include "gsl168x_model_35_hvga_1688e_KRTR118.h"
#elif defined(GSL168X_MODEL_40_WVGA)
#include "gsl168x_model_40_wvga.h"
#elif defined(GSL168X_MODEL_70_WVGAL)
#include "gsl168x_model_70_wvgal.h"
#elif defined(GSL168X_MODEL_DUOCAI_1680E)
#include "gsl168x_model_duocai_1688e_131010.h"
#elif defined(GSL168X_MODEL_70_WVGAL_B)
#include "gsl168x_model_70_wvgal_b.h"
#elif defined(GSL168X_MODEL_70_WVGAL_CMW)
#include "gsl168x_model_70_wvgal_cmw.h"
#elif defined(GSL168X_MODEL_70_WVGAL_BP786)
#include "gsl168x_model_70_wvgal_BP786.h"
#elif defined(GSL168X_MODEL_70_WVGAL_GLS2439)
#include "gsl168x_model_70_wvgal_gls2439.h"
#elif defined(GSL168X_MODEL_70_WVGAL_D70E)
#include "gsl168x_model_70_wvgal_d70e.h"
#elif defined(GSL168X_MODEL_70_WVGAL_D706E)
#include "gsl168x_model_70_wvgal_d706e.h"
#elif defined(GSL168X_MODEL_WVGA_X3)
#include "gsl168x_model_wvga_x3.h"
#elif defined(GSL168X_MODEL_WVGA_X3V4)
#include "gsl168x_model_wvga_x3v4.h"
#elif defined(GSL168X_MODEL_70_WVGAL_BP605)
#include "gsl168x_model_70_wvgal_BP605.h"
#elif defined(GSL168X_MODEL_70_WSVGAL_BP605)
#include "gsl168x_model_70_wsvgal_BP605.h"
#elif defined(GSL168X_MODEL_40_WVGA_ORANGEPI)
#include "gsl168x_model_OrangePi_2G-IOT.h"
#else
#error "Undefined GSL168X Model"
#endif
#define GSL_NOID_VERSION
#define SMBUS_TRANS_LEN	0x08
#define RDA_TS_EVENT_LENGTH 24

static int g_168x_id = -1;
//#define GSL_ALG_ID
//#define GSL_DRV_WIRE_IDT_TP
//#define GSL168X_INVERT_XY
static void gsl_start_core(struct i2c_client *client)
{
	//u8 tmp = 0x00;
	u8 buf[4] = {0};
#if 0
	buf[0]=0;
	buf[1]=0x10;
	buf[2]=0xfe;
	buf[3]=0x1;
	gsl_write_interface(client,0xf0,buf,4);
	buf[0]=0xf;
	buf[1]=0;
	buf[2]=0;
	buf[3]=0;
	gsl_write_interface(client,0x4,buf,4);
	msleep(20);
#endif
	buf[0]=0;
	rda_ts_i2c_write(client,0xe0,buf,4);
#ifdef GSL_ALG_ID
	{
#ifdef GSL_DRV_WIRE_IDT_TP
	gsl_DataInit(gsl_cfg_table[gsl_cfg_index].data_id);
#else
	gsl_DataInit(gsl_config_data_id);
#endif
	}
#endif	
}

static void rda_gsl168x_reset_gpio(void)
{
	gpio_set_value(GPIO_TOUCH_RESET, 1);
	mdelay(10);		// Delay 10 ms
	gpio_set_value(GPIO_TOUCH_RESET, 0);
	mdelay(10);		// Delay 10 ms
	gpio_set_value(GPIO_TOUCH_RESET, 1);
	mdelay(10);		// Delay 10 ms
}

static void rda_gsl168x_init_gpio(void)
{
	gpio_request(GPIO_TOUCH_RESET, "touch screen reset");
	gpio_request(GPIO_TOUCH_IRQ, "touch screen interrupt");

	gpio_direction_output(GPIO_TOUCH_RESET, 0);
	mdelay(1);		// Delay 10ms
	gpio_direction_input(GPIO_TOUCH_IRQ);
	mdelay(1);

	rda_gsl168x_reset_gpio();
}

static void rda_gsl168x_startup_chip(struct i2c_client *client)
{
	u8 tmp = 0x00;

#ifdef GSL_NOID_VERSION
	gsl_DataInit(gsl_config_data_id);
#else
#if 0
	u8 buf[4] = {0x00};
	buf[3] = 0x01;
	buf[2] = 0xfe;
	buf[1] = 0x10;
	buf[0] = 0x00;	
	rda_ts_i2c_write(client, 0xf0, buf, sizeof(buf));
	buf[3] = 0x00;
	buf[2] = 0x00;
	buf[1] = 0x00;
	buf[0] = 0x0f;	
	rda_ts_i2c_write(client, 0x04, buf, sizeof(buf));
	msleep(20);	
#endif
#endif

	rda_ts_i2c_write(client, 0xe0, &tmp, 1);
}

static void rda_gsl168x_clr_reg(struct i2c_client *client)
{
	u8 buf[4] = {0x00};
	buf[0] = 0x88;
	rda_ts_i2c_write(client, 0xe0, buf, 4);
	mdelay(20);

	buf[0]  = 0x03;
	rda_ts_i2c_write(client, 0x80, buf, 4);
	mdelay(10);

	buf[0]  = 0x04;
	rda_ts_i2c_write(client, 0xe4, buf, 4);
	mdelay(10);

	buf[0] = 0;
	rda_ts_i2c_write(client, 0xe0, buf, 4);
	mdelay(10);
}

static void rda_gsl168x_reset_chip(struct i2c_client *client)
{
	u8 buf[4] = {0x00};
	buf[0] = 0x88;
	rda_ts_i2c_write(client, 0xe0, buf, 4);
	mdelay(20);

	buf[0]  = 0x04;
	rda_ts_i2c_write(client, 0xe4, buf, 4);
	mdelay(10);

	buf[0] = 0;
	rda_ts_i2c_write(client, 0xbc, buf, 4);
	mdelay(10);
}

static unsigned int fw_len = 0;
static void *fw_data = NULL;

static void rda_gsl168x_load_fw(struct i2c_client *client)
{
	s8 buf[SMBUS_TRANS_LEN*4] = {0};
	s8 reg = 0, send_flag = 1, cur = 0;
	int ret;
	unsigned int i = 0;
	struct fw_data *gsl168x_fw = (struct fw_data *)fw_data;

	BUG_ON(!fw_len || !fw_data);
	
	if(0 == g_168x_id)
	{
		return;
	}

	for (i = 0; i < fw_len; i++) {
		/* init page trans, set the page val */
		if (0xf0 == gsl168x_fw[i].offset) {
			buf[0] = (char)(gsl168x_fw[i].val & 0x000000ff);
			ret = rda_ts_i2c_write(client, 0xf0, &buf[0],1);
			if(ret<0) {
				printk("rda_gsl168x_load_fw 0xf0  err \n");
				break;
			}
			send_flag = 1;
		} else {
			if (1 == send_flag % (SMBUS_TRANS_LEN < 0x08 ? SMBUS_TRANS_LEN : 0x08))
				reg = gsl168x_fw[i].offset;

			buf[cur + 0] = (char)(gsl168x_fw[i].val & 0x000000ff);
			buf[cur + 1] = (char)((gsl168x_fw[i].val & 0x0000ff00) >> 8);
			buf[cur + 2] = (char)((gsl168x_fw[i].val & 0x00ff0000) >> 16);
			buf[cur + 3] = (char)((gsl168x_fw[i].val & 0xff000000) >> 24);
			cur += 4;

			if (0 == send_flag % (SMBUS_TRANS_LEN < 0x08 ? SMBUS_TRANS_LEN : 0x08))	{
				ret = rda_ts_i2c_write(client, reg ,buf,SMBUS_TRANS_LEN*4);
				if(ret<0) {
					printk("rda_gsl168x_load_fw array  err \n");
					break;
				}
				cur = 0;
			}
			send_flag++;
		}
	}
}

static void rda_gsl168x_init_chip(struct i2c_client *client)
{
	rda_gsl168x_reset_gpio();
	rda_gsl168x_clr_reg(client);
	rda_gsl168x_reset_chip(client);
	rda_gsl168x_load_fw(client);
gsl_start_core(client);
	rda_gsl168x_startup_chip(client);
	rda_gsl168x_reset_chip(client);
//	rda_gsl168x_reset_gpio();
//	rda_gsl168x_reset_chip(client);
	rda_gsl168x_startup_chip(client);
}

static void rda_gsl168x_check_mem_data(struct i2c_client *client)
{
	u8 read_buf[4]  = {0};

	//if(gsl_chipType_new == 1)	
	{
		msleep(30);
		rda_ts_i2c_read(client,0xb0, read_buf, sizeof(read_buf));
	
		if (read_buf[3] != 0x5a || read_buf[2] != 0x5a || read_buf[1] != 0x5a || read_buf[0] != 0x5a)
		{	
			rda_gsl168x_reset_gpio();

			rda_gsl168x_init_chip(client);
		}
	}
}

static void rda_tsl168x_check_chip_ver(struct i2c_client *client)
{
	u8 buf[4] = {0};

	rda_ts_i2c_read(client,0xfc,buf,4);
	if(0x82 == buf[2])
	{
		g_168x_id = buf[2];
	}
	else
	{
		g_168x_id = 0;
	}

#ifdef GSL168X_MODEL_35_HVGA_1680
	u8 buf[4] = {0};

	rda_ts_i2c_read(client,0xfc,buf,4);
	if(buf[0] == 0 && buf[1] == 0 && buf[2] == 0x82 && buf[3] == 0xa0)
	{
		fw_data = (void *)gsxl680_e0_fw;
		fw_len = ARRAY_SIZE(gsxl680_e0_fw);
	}
	else
	{
		fw_data = (void *)gsxl680_d0_fw;
		fw_len = ARRAY_SIZE(gsxl680_d0_fw);
	}
#elif defined(GSL168X_MODEL_35_HVGA_1688E)
	fw_data = (void *)gsxl688_hvga_fw;
	fw_len = ARRAY_SIZE(gsxl688_hvga_fw);
#elif defined(GSL168X_MODEL_35_HVGA_1688E_KRTR118)
	fw_data = (void *)gsxl688_hvga_fw;
	fw_len = ARRAY_SIZE(gsxl688_hvga_fw);
#elif defined(GSL168X_MODEL_40_WVGA_ORANGEPI)
	fw_data = (void *)gsxl688_wvga_fw;
	fw_len = ARRAY_SIZE(gsxl688_wvga_fw);
#elif defined(GSL168X_MODEL_40_WVGA)
	fw_data = (void *)gsxl688_wvga_fw;
	fw_len = ARRAY_SIZE(gsxl688_wvga_fw);
#elif defined(GSL168X_MODEL_70_WVGAL)
	fw_data = (void *)gsxl688_wvgal_fw;
	fw_len = ARRAY_SIZE(gsxl688_wvgal_fw);
#elif defined(GSL168X_MODEL_DUOCAI_1680E)
	fw_data = (void *)GSLX680_FW;
	fw_len = ARRAY_SIZE(GSLX680_FW);
#elif defined(GSL168X_MODEL_70_WVGAL_B)
	fw_data = (void *)gsxl688_wvgal_fw;
	fw_len = ARRAY_SIZE(gsxl688_wvgal_fw);
#elif defined(GSL168X_MODEL_70_WVGAL_CMW)
	fw_data = (void *)gsxl688_wvgal_fw;
	fw_len = ARRAY_SIZE(gsxl688_wvgal_fw);
#elif defined(GSL168X_MODEL_70_WVGAL_BP786)
	fw_data = (void *)gsxl688_wvgal_fw;
	fw_len = ARRAY_SIZE(gsxl688_wvgal_fw);
#elif defined(GSL168X_MODEL_70_WVGAL_GLS2439)
	fw_data = (void *)gsxl688_wvgal_fw;
	fw_len = ARRAY_SIZE(gsxl688_wvgal_fw);
#elif defined(GSL168X_MODEL_70_WVGAL_D70E)
	fw_data = (void *)gsxl688_wvgal_fw;
	fw_len = ARRAY_SIZE(gsxl688_wvgal_fw);
#elif defined(GSL168X_MODEL_70_WVGAL_D706E)
	fw_data = (void *)gsxl688_wvgal_fw;
	fw_len = ARRAY_SIZE(gsxl688_wvgal_fw);
#elif defined(GSL168X_MODEL_WVGA_X3)
	fw_data = (void *)gsxl680_e0_fw;
	fw_len = ARRAY_SIZE(gsxl680_e0_fw);
#elif defined(GSL168X_MODEL_WVGA_X3V4)
	fw_data = (void *)GSLX680_FW;
	fw_len = ARRAY_SIZE(GSLX680_FW);
#elif defined(GSL168X_MODEL_70_WVGAL_BP605)
	fw_data = (void *)gsxl688_wvgal_fw;
	fw_len = ARRAY_SIZE(gsxl688_wvgal_fw);
#elif defined(GSL168X_MODEL_70_WSVGAL_BP605)
	fw_data = (void *)gsxl688_wsvgal_fw;
	fw_len = ARRAY_SIZE(gsxl688_wsvgal_fw);
#else
#error "Undefined GSL168X Model"
#endif
}

static void rda_gsl168x_init(struct i2c_client *client)
{
	rda_tsl168x_check_chip_ver(client);
	rda_gsl168x_init_chip(client);
	rda_gsl168x_check_mem_data(client);

#ifdef GSL_ALG_ID

#ifdef GSL_DRV_WIRE_IDT_TP
	gsl_DataInit(gsl_cfg_table[gsl_cfg_index].data_id);
#else
	gsl_DataInit(gsl_config_data_id);
#endif
#endif


}

static inline u16 join_bytes(u8 a, u8 b)
{
	u16 ab = 0;
	ab = ab | a;
	ab = ab << 8 | b;
	return ab;
}

static void  rda_gsl168x_process_data(struct i2c_client *client, u8 * read_data,
			struct rda_ts_pos_data * pos_data)
{
	u8 id, touches;
	u16 x, y;
	int i = 0;
#ifdef GSL168X_INVERT_XY
	u16 t;
#endif

#ifdef GSL_NOID_VERSION
	u32 tmp1;
	u8 buf[4] = {0};
	struct gsl_touch_info cinfo;
#endif
	pos_data->point_num = 0;

	touches = read_data[0];
#ifdef GSL_NOID_VERSION
	cinfo.finger_num = touches;
	for(i = 0; i < (touches < MAX_FINGERS ? touches : MAX_FINGERS); i ++)
	{
		cinfo.x[i] = join_bytes((read_data[4 * i + 7] & 0xf),read_data[4 * i + 6]);
		cinfo.y[i] = join_bytes(read_data[4 * i + 5],read_data[4 * i +4]);
	}
	cinfo.finger_num=(read_data[3]<<24)|(read_data[2]<<16)|(read_data[1]<<8)|(read_data[0]);
	gsl_alg_id_main(&cinfo);
	tmp1=gsl_mask_tiaoping();
	if(tmp1>0&&tmp1<0xffffffff)
	{
		buf[0]=0xa;buf[1]=0;buf[2]=0;buf[3]=0;
		rda_ts_i2c_write(client, 0xf0, buf, 4);
		buf[0]=(u8)(tmp1 & 0xff);
		buf[1]=(u8)((tmp1>>8) & 0xff);
		buf[2]=(u8)((tmp1>>16) & 0xff);
		buf[3]=(u8)((tmp1>>24) & 0xff);
		printk("tmp1=%08x,buf[0]=%02x,buf[1]=%02x,buf[2]=%02x,buf[3]=%02x\n",
			tmp1,buf[0],buf[1],buf[2],buf[3]);
		rda_ts_i2c_write(client, 0x8, buf, 4);
	}
	touches = cinfo.finger_num;
#endif

	for(i=0;i<(touches > MAX_FINGERS ? MAX_FINGERS : touches);i++)
	{
#ifdef GSL_NOID_VERSION
		id = cinfo.id[i];
		x =  cinfo.x[i];
		y =  cinfo.y[i];
#else
		id = read_data[4 * i + 7] >> 4;
		x = join_bytes((read_data[4 * i + 7] & 0xf),read_data[4 * i + 6]);
		y = join_bytes(read_data[4 * i + 5],read_data[4 * i +4]);
#endif
		if(id <= MAX_TOUCH_POINTS)
		{
#ifdef GSL168X_INVERT_XY

			t = x;
			x = y;
			y = t;

			x = x - 155;
			y = y - 110;
#endif
#ifdef GSL168X_INVERT_X
			x = GSL168X_RES_X - x;
#endif
#ifdef GSL168X_INVERT_Y
			y = GSL168X_RES_Y - y;
#endif
			pos_data->ts_position[pos_data->point_num].x = x;
			pos_data->ts_position[pos_data->point_num].y = y;

            if (y > 100 && y < 150)
                y = 210 - y;
			pos_data->ts_position[pos_data->point_num].id = id;
			pos_data->point_num += 1;
		}
	}

	return;
}

static int rda_gsl168x_get_pos(struct i2c_client *client,
			struct rda_ts_pos_data * pos_data)
{
	int ret;
	u8 read_buf[4] = {0};
	u8 read_data[RDA_TS_EVENT_LENGTH]= {0};

	/* read data from DATA_REG */
	ret = rda_ts_i2c_read(client, 0x80, read_data, sizeof(read_data));
	if (ret < 0)
	{
		if (ret == -EAGAIN)
			return TS_I2C_RETRY;
		else
			return TS_ERROR;
	}

	if (read_data[0] == 0xff) {
		return TS_ERROR;
	}

	ret = rda_ts_i2c_read(client, 0xbc, read_buf, sizeof(read_buf));
	if (ret < 0)
	{
		if (ret == -EAGAIN)
			return TS_I2C_RETRY;
		else
			return TS_ERROR;
	}

	if (read_buf[3] == 0 && read_buf[2] == 0 && read_buf[1] == 0 && read_buf[0] == 0)
	{
		rda_gsl168x_process_data(client, read_data, pos_data);
	} else {
		pos_data->point_num = 0;
		return TS_NOTOUCH;
	}

	return TS_OK;
}
#if 0
static int runyee_gpio_text(void)
{
	gpio_request(GPIO_TOUCH_B24, "b24");
	gpio_request(GPIO_TOUCH_A0, "A0");
	gpio_request(GPIO_TOUCH_A1, "A1");
	gpio_request(GPIO_TOUCH_A2, "A2");
	gpio_request(GPIO_TOUCH_A3, "A3");
	gpio_request(GPIO_TOUCH_A4, "A4");
	gpio_request(GPIO_TOUCH_A5, "A5");
	gpio_request(GPIO_TOUCH_A6, "A6");

	gpio_direction_output(GPIO_TOUCH_B24, 1);
	gpio_direction_output(GPIO_TOUCH_A0, 0);
	gpio_direction_output(GPIO_TOUCH_A1, 0);
	gpio_direction_output(GPIO_TOUCH_A2, 0);
	gpio_direction_output(GPIO_TOUCH_A3, 0);
	gpio_direction_output(GPIO_TOUCH_A4, 0);
	gpio_direction_output(GPIO_TOUCH_A5, 0);
	gpio_direction_output(GPIO_TOUCH_A6, 0);
	
	while(1)
	{
		gpio_set_value(GPIO_PLUGIN_CTRL, 0);
		mdelay(100);
		gpio_set_value(GPIO_PLUGIN_CTRL, 1);
		mdelay(100);

		gpio_set_value(GPIO_TOUCH_B24, 0);
		mdelay(100);
		gpio_set_value(GPIO_TOUCH_B24, 1);
		mdelay(100);

		gpio_set_value(GPIO_TOUCH_A0, 0);
		mdelay(100);
		gpio_set_value(GPIO_TOUCH_A0, 1);
		mdelay(100);

		gpio_set_value(GPIO_TOUCH_A1, 0);
		mdelay(100);
		gpio_set_value(GPIO_TOUCH_A1, 1);
		mdelay(100);

		gpio_set_value(GPIO_TOUCH_A2, 0);
		mdelay(100);
		gpio_set_value(GPIO_TOUCH_A2, 1);
		mdelay(100);

		gpio_set_value(GPIO_TOUCH_A3, 0);
		mdelay(100);
		gpio_set_value(GPIO_TOUCH_A3, 1);
		mdelay(100);

		gpio_set_value(GPIO_TOUCH_A4, 0);
		mdelay(100);
		gpio_set_value(GPIO_TOUCH_A4, 1);
		mdelay(100);

		gpio_set_value(GPIO_TOUCH_A5, 0);
		mdelay(100);
		gpio_set_value(GPIO_TOUCH_A5, 1);
		mdelay(100);

		gpio_set_value(GPIO_TOUCH_A6, 0);
		mdelay(100);
		gpio_set_value(GPIO_TOUCH_A6, 1);
		mdelay(100);
	}
	return 0;
}
#endif
static int rda_gsl168x_suspend(struct i2c_client *client, pm_message_t mesg)
{

	/* Pull down the reset pin */
	gpio_set_value(GPIO_TOUCH_RESET, 0);
	mdelay(1);
//	runyee_gpio_text();

	return 0;
}

static int rda_gsl168x_resume(struct i2c_client *client)
{
#ifdef TGT_AP_TS_FW_RELOAD
	rda_gsl168x_reset_gpio();
	rda_gsl168x_init(client);
#else
	gpio_set_value(GPIO_TOUCH_RESET, 1);
	mdelay(20);
	
	if(0 == g_168x_id)
	{
		return 0;
	}

	rda_gsl168x_reset_chip(client);
	rda_gsl168x_startup_chip(client);
	mdelay(20);
	rda_gsl168x_check_mem_data(client);
#endif /* TGT_AP_TS_FW_RELOAD */

	return 0;
}

struct rda_ts_panel_info gsl168x_info = {
	/* struct rda_lcd_ops ops; */
	.ops = {
		.ts_init_gpio = rda_gsl168x_init_gpio,
		.ts_init_chip = rda_gsl168x_init,
		.ts_start_chip = rda_gsl168x_startup_chip,
		.ts_reset_gpio = rda_gsl168x_reset_gpio,
		.ts_reset_chip = rda_gsl168x_reset_chip,
		.ts_get_parse_data = rda_gsl168x_get_pos,
		.ts_suspend = rda_gsl168x_suspend,
		.ts_resume = rda_gsl168x_resume,
	},
	/* struct lcd_scr_info  src_info; */
	.ts_para = {
		.name = "gsl168x",
		.i2c_addr = _DEF_I2C_ADDR_TS_GSL168X,
		.x_max = GSL168X_RES_X,
		.y_max = GSL168X_RES_Y,
		.gpio_irq = GPIO_TOUCH_IRQ,
		.vir_key_num = VIR_KEY_NUM,
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

int rda_ts_add_panel_gsl168x(void)
{
	int ret = 0;

	ret = rda_ts_register_driver(&gsl168x_info);
	if(ret)
		pr_err("rda_ts_add_panel GSL1168X failed \n");

	return ret;
}

