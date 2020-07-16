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
#include <linux/workqueue.h>

#ifdef CONFIG_HAS_EARLYSUSPEND
#include <linux/earlysuspend.h>
#endif /* CONFIG_HAS_EARLYSUSPEND */

#include <linux/platform_device.h>

#include <mach/hardware.h>
#include <plat/devices.h>
#include <mach/board.h>
#include <linux/gpio.h>
#include "rda_ts.h"
#include "rda_elan.h"

int recover = 0;
static struct workqueue_struct *init_elan_ic_wq = NULL;
static struct delayed_work init_work;
static unsigned long delay = 2*HZ;
static struct i2c_client * ts_client = NULL;

static void rda_elan_reset_gpio(void)
{
	rda_dbg_ts(" rda_elan_reset_gpio\n");

	gpio_set_value(GPIO_TOUCH_RESET, 1);
	msleep(10);		// Delay 10 ms
	gpio_set_value(GPIO_TOUCH_RESET, 0);
	msleep(10);		// Delay 10 ms
	gpio_set_value(GPIO_TOUCH_RESET, 1);
	msleep(10);		// Delay 200 ms
}

static void rda_elan_init_gpio(void)
{
	rda_dbg_ts(" rda_elan_init_gpio\n");

	gpio_request(GPIO_TOUCH_RESET, "touch screen reset");
	gpio_request(GPIO_TOUCH_IRQ, "touch screen interrupt");
	gpio_direction_output(GPIO_TOUCH_RESET, 0);
	mdelay(1);		// Delay 10ms
	gpio_direction_input(GPIO_TOUCH_IRQ);
	mdelay(1);
	rda_elan_reset_gpio();
}

static void rda_elan_startup_chip(struct i2c_client *client)
{
	rda_dbg_ts(" rda_elan_startup_chip\n");
}
static void rda_elan_reset_chip(struct i2c_client *client)
{
	rda_dbg_ts(" rda_elan_reset_chip\n");

	rda_elan_reset_gpio();
}

static int elan_ts_poll(int retry)
{
	int status = 0;

	while(retry--){
		status = gpio_get_value(GPIO_TOUCH_IRQ);
		printk("[elan]: %s: status = %d\n", __func__, status);

		if(status == 0)
			break;
		msleep(40);
	}

	printk( "[elan]%s: poll interrupt status %s\n", __func__, status == 1 ? "high" : "low");

	return status == 0 ? 0 : -ETIMEDOUT;
}

static int elan_ts_get_data(struct i2c_client *client, uint8_t *cmd, uint8_t *buf, int size)
{
	int rc;

	if (buf == NULL)
		return -EINVAL;


	if (i2c_master_send(client, cmd, size) != size)
		return -EINVAL;

	mdelay(2);

	rc = elan_ts_poll(1);
	if (rc < 0)
		return -EINVAL;
	else {
		rc = i2c_master_recv(client, buf, size);
		printk("[elan] %s: respone packet %2x:%2X:%2x:%2x\n", __func__, buf[0], buf[1], buf[2], buf[3]);
		if(buf[0] != CMD_S_PKT || rc != size){
			printk("[elan error]%s: cmd respone error\n", __func__);
			return -EINVAL;
		}
	}

	return 0;
}

static int __fw_packet_handler(struct i2c_client *client)
{
	int rc;
	int major, minor;
	int fw_ver;
	int fw_id;
	int fw_bcd;
	int x_resolution;
	int y_resolution;
	uint8_t cmd[]           = {0x53, 0x00, 0x00, 0x01};/* Get Firmware Version*/

	uint8_t cmd_id[]        = {0x53, 0xf0, 0x00, 0x01}; /*Get firmware ID*/
	uint8_t cmd_bc[]        = {0x53, 0x10, 0x00, 0x01};/* Get BootCode Version*/
#if 0
	int x, y;
	uint8_t cmd_getinfo[] = {0x5B, 0x00, 0x00, 0x00, 0x00, 0x00};
	uint8_t adcinfo_buf[17]={0};
#else
	uint8_t cmd_x[]         = {0x53, 0x60, 0x00, 0x00}; /*Get x resolution*/
	uint8_t cmd_y[]         = {0x53, 0x63, 0x00, 0x00}; /*Get y resolution*/
#endif
	uint8_t buf_recv[4]     = {0};

	// Firmware version
	rc = elan_ts_get_data(client, cmd, buf_recv, 4);
	if (rc < 0)
		return rc;
	major = ((buf_recv[1] & 0x0f) << 4) | ((buf_recv[2] & 0xf0) >> 4);
	minor = ((buf_recv[2] & 0x0f) << 4) | ((buf_recv[3] & 0xf0) >> 4);
	fw_ver = major << 8 | minor;

	// Firmware ID
	rc = elan_ts_get_data(client, cmd_id, buf_recv, 4);
	if (rc < 0)
		return rc;
	major = ((buf_recv[1] & 0x0f) << 4) | ((buf_recv[2] & 0xf0) >> 4);
	minor = ((buf_recv[2] & 0x0f) << 4) | ((buf_recv[3] & 0xf0) >> 4);
	fw_id = major << 8 | minor;
#if 1
	// X Resolution
	rc = elan_ts_get_data(client, cmd_x, buf_recv, 4);
	if (rc < 0)
		return rc;
	minor = ((buf_recv[2])) | ((buf_recv[3] & 0xf0) << 4);
#ifdef SWAP_X_Y_RESOLUTION
	y_resolution = minor;
#else
	x_resolution = minor;
#endif

	// Y Resolution
	rc = elan_ts_get_data(client, cmd_y, buf_recv, 4);
	if (rc < 0)
	return rc;
	minor = ((buf_recv[2])) | ((buf_recv[3] & 0xf0) << 4);
#ifdef SWAP_X_Y_RESOLUTION
	x_resolution = minor;
#else
	y_resolution = minor;
#endif
#else
	elan_i2c_send(client, cmd_getinfo, sizeof(cmd_getinfo));
	msleep(10);
	elan_i2c_recv(client, adcinfo_buf, 17);
	x  = adcinfo_buf[2]+adcinfo_buf[6]+adcinfo_buf[10]+adcinfo_buf[14];
	y  = adcinfo_buf[3]+adcinfo_buf[7]+adcinfo_buf[11]+adcinfo_buf[15];

	printk( "[elan] %s: x= %d, y=%d\n",__func__,x,y);

	ts->x_resolution=(x-1)*64;
	ts->y_resolution=(y-1)*64;
#endif

	// Firmware BC
	rc = elan_ts_get_data(client, cmd_bc, buf_recv, 4);
	if (rc < 0)
		return rc;

	major = ((buf_recv[1] & 0x0f) << 4) | ((buf_recv[2] & 0xf0) >> 4);
	minor = ((buf_recv[2] & 0x0f) << 4) | ((buf_recv[3] & 0xf0) >> 4);
	fw_bcd = major << 8 | minor;

	printk( "[elan] %s: firmware version: 0x%4.4x\n",__func__, fw_ver);
	printk( "[elan] %s: firmware ID: 0x%4.4x\n",__func__, fw_id);
	printk( "[elan] %s: firmware BC: 0x%4.4x\n",__func__, fw_bcd);
	printk( "[elan] %s: x resolution: %d, y resolution: %d\n",__func__, x_resolution, y_resolution);

	return 0;
}

static void elan_ic_init_work(struct work_struct * work)
{
	int rc = 0;
	int retry_cnt = 0;

	if (recover == 0){
		disable_irq(gpio_to_irq(GPIO_TOUCH_IRQ));
		for(retry_cnt = 0; retry_cnt < 3; retry_cnt++){
			rc = __fw_packet_handler(ts_client);
			if (rc < 0)
				printk("[elan error] %s, fw_packet_handler fail, rc = %d \n", __func__, rc);
			else
				break;
		}

		enable_irq(gpio_to_irq(GPIO_TOUCH_IRQ));
	}
}

static void rda_elan_init(struct i2c_client *client)
{
	rda_dbg_ts(" rda_elan_init\n");

	printk("recover = %d \n", recover);
	INIT_DELAYED_WORK(&init_work, elan_ic_init_work);
	init_elan_ic_wq = create_singlethread_workqueue("init_elan_ic_wq");
	if (!init_elan_ic_wq)
		return;
	ts_client = client;

	queue_delayed_work(init_elan_ic_wq, &init_work, delay);
}

static void elan_ts_report_key(struct rda_ts_pos_data *pos_data, u8 button_data)
{
	switch (button_data) {
		case ELAN_KEY_MENU:
			pos_data->point_num++;
			pos_data->ts_position[0].x = VIR_KEY_MENU_X* ELAN_RES_X / PANEL_XSIZE;
			pos_data->ts_position[0].y = VIR_KEY_MENU_Y*ELAN_RES_Y / PANEL_YSIZE;
			break;
		case ELAN_KEY_HOME:
			pos_data->point_num++;
			pos_data->ts_position[0].x = VIR_KEY_HOMEPAGE_X* ELAN_RES_X / PANEL_XSIZE;
			pos_data->ts_position[0].y = VIR_KEY_HOMEPAGE_Y* ELAN_RES_Y / PANEL_YSIZE;
			break;
		case ELAN_KEY_BACK:
			pos_data->point_num++;
			pos_data->ts_position[0].x = VIR_KEY_BACK_X* ELAN_RES_X / PANEL_XSIZE;
			pos_data->ts_position[0].y = VIR_KEY_BACK_Y* ELAN_RES_Y / PANEL_YSIZE;
			break;
		default:
			pos_data->point_num = 0;
			break;
	}
}

static inline int elan_ts_parse_xy(uint8_t *data, uint16_t *x, uint16_t *y)
{
	*x = *y = 0;

	*x = (data[0] & 0xf0);
	*x <<= 4;
	*x |= data[1];

	*y = (data[0] & 0x0f);
	*y <<= 8;
	*y |= data[2];

	return 0;
}
static void elan_ts_report_data(struct rda_ts_pos_data *pos_data, uint8_t *buf)
{
	uint16_t fbits=0;
	u8 idx;
	int finger_num = FINGERS_NUM;
	int num = 0;
	int reported = 0;
	u16 x = 0;
	u16 y = 0;
	int position = 0;
	u8 button_byte = 0;

#ifdef TWO_FINGERS
	num = buf[7] & 0x03;
	fbits = buf[7] & 0x03;
	idx=1;
	button_byte = buf[PACKET_SIZE-1];
#endif

#ifdef FIVE_FINGERS
	num = buf[1] & 0x07;
	fbits = buf[1] >>3;
	idx=2;
	button_byte = buf[PACKET_SIZE-1];
#endif

#ifdef TEN_FINGERS
	fbits = buf[2] & 0x30;
	fbits = (fbits << 4) | buf[1];
	num = buf[2] &0x0f;
	idx=3;
	button_byte = buf[PACKET_SIZE-1];
#endif

	if (num == 0){
		elan_ts_report_key(pos_data, button_byte);
	} else{
		rda_dbg_ts( "[elan] %d fingers", num);

		for(position = 0; (position < finger_num) && (reported < num); position++){
			if((fbits & 0x01)){
#ifdef SWAP_X_Y_RESOLUTION
				elan_ts_parse_xy(&buf[idx], &y, &x);
#else
				elan_ts_parse_xy(&buf[idx], &x, &y);
#endif

				pos_data->point_num++;
				pos_data->ts_position[reported].x = x;
				pos_data->ts_position[reported].y = y;

				rda_dbg_ts(" elan_ts_report_data i=%d, x=%d, y=%d\n", reported,
							pos_data->ts_position[reported].x,
							pos_data->ts_position[reported].y);

				reported++;
			}

			fbits = fbits >> 1;
			idx += 3;
		}
	}

	return;
}

static void elan_ts_handler_event(uint8_t *buf)
{
	if(buf[0] == 0x55){
		if(buf[2] == 0x55){
			printk("Now you are getting the elan device info\n");
			recover = 0;
		} else if(buf[2] == 0x80){
			recover = 0x80;
			printk("Error : You should update he elan firmware\n");
		}
	}
}

static int rda_etk2527_get_pos(struct i2c_client *client,
		struct rda_ts_pos_data *pos_data)
{
	int ret;
	u8 buf[PACKET_SIZE] = {0};

	ret = i2c_master_recv(client, buf, PACKET_SIZE);

	if (ret < 0)
		return TS_ERROR;

	pos_data->distance = -1;
	pos_data->data_type = TS_DATA;
	pos_data->point_num = 0;

	if(FINGERS_PKT != buf[0]){
		printk("[elan] other event packet:%02x %02x %02x %02x %02x %02x %02x %02x\n", buf[0],buf[1],buf[2],buf[3],buf[4],buf[5],buf[6],buf[7]);
		elan_ts_handler_event(buf);
		//return TS_PACKET_ERROR;
		return TS_NOTOUCH;
	}

	elan_ts_report_data(pos_data, buf);

	return TS_OK;
}

static int elan_ts_set_power_state(struct i2c_client *client, int state)
{
	uint8_t cmd[] = {CMD_W_PKT, 0x50, 0x00, 0x01};
	int size = sizeof(cmd);

	cmd[1] |= (state << 3);

	if (i2c_master_send(client, cmd, size) != size)
		return -EINVAL;

	return 0;
}

static int rda_elan_suspend(struct i2c_client *client,
		pm_message_t mesg)
{
	rda_dbg_ts(" rda_elan_suspend\n");
	return elan_ts_set_power_state(client, PWR_STATE_DEEP_SLEEP);
}

static int rda_elan_resume(struct i2c_client *client)
{
	rda_dbg_ts(" rda_elan_resume\n");
	rda_elan_reset_gpio();
	return 0;
}

static int __hello_packet_handler(struct i2c_client *client)
{
	int rc;
	uint8_t buf_recv[8] = { 0 };
	uint8_t cmd[] = {0x53, 0x00, 0x00, 0x01};

	rc = elan_ts_poll(20);
	if(rc != 0)
		printk("[elan] %s: Int poll 55 55 55 55 failed!\n", __func__);

	rc = i2c_master_recv(client, buf_recv, sizeof(buf_recv));
	if(rc != sizeof(buf_recv))
		printk("[elan error] __hello_packet_handler recv error\n");

	printk("[elan] %s: hello packet %2x:%2X:%2x:%2x\n", __func__, buf_recv[0], buf_recv[1], buf_recv[2], buf_recv[3]);

	if(buf_recv[0]==0x55 && buf_recv[1]==0x55 && buf_recv[2]==0x80 && buf_recv[3]==0x80){
		printk("[elan] %s: boot code packet %2x:%2X:%2x:%2x\n", __func__, buf_recv[4], buf_recv[5], buf_recv[6], buf_recv[7]);
		rc = 0x80;
	} else if(buf_recv[0]==0x55 && buf_recv[1]==0x55 && buf_recv[2]==0x55 && buf_recv[3]==0x55){
		printk("[elan] __hello_packet_handler recv ok\n");
		rc = 0x0;
	} else{
		if(rc != sizeof(buf_recv)){
			rc = i2c_master_send(client, cmd, sizeof(cmd));
			if(rc != sizeof(cmd)){
				msleep(5);
				rc = i2c_master_recv(client, buf_recv, sizeof(buf_recv));
			}
		}
	}

	return rc;
}

static int rda_elan_get_chip_id(struct i2c_client *client)
{
	int rc = 0;

	msleep(200);
	rc = __hello_packet_handler(client);
	if (rc < 0)
		printk("[elan error] %s, hello_packet_handler fail, rc = %d\n", __func__, rc);

	recover = rc;
	return rc;
}


struct rda_ts_panel_info elan_info = {
	/* struct rda_lcd_ops ops; */
	.ops = {
		.ts_init_gpio = rda_elan_init_gpio,
		.ts_init_chip = rda_elan_init,
		.ts_start_chip = rda_elan_startup_chip,
		.ts_reset_chip = rda_elan_reset_chip,
		.ts_get_parse_data = rda_etk2527_get_pos,
		.ts_suspend = rda_elan_suspend,
		.ts_resume = rda_elan_resume,
		.ts_get_chip_id= rda_elan_get_chip_id,
	},
	/* struct lcd_scr_info  src_info; */
	.ts_para = {
		.name = "elan",
		.i2c_addr = ELAN_7BITS_ADDR,
		.x_max = ELAN_RES_X,
		.y_max = ELAN_RES_Y,
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

int rda_ts_add_panel_elan(void)
{
	int ret = 0;
	ret = rda_ts_register_driver(&elan_info);
	if (ret)
		pr_err("rda_ts_add_panel elan failed \n");
	return ret;
}
