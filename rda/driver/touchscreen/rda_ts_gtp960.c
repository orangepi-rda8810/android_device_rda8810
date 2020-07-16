#include <linux/errno.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/delay.h>
#include <linux/hrtimer.h>
#include <linux/i2c.h>
#include <linux/input.h>
#include <linux/interrupt.h>
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
#include "rda_ts_gtp960.h"


static u8 config[GTP_CONFIG_LENGTH + GTP_ADDR_LENGTH]
                = {GTP_REG_CONFIG_DATA >> 8, GTP_REG_CONFIG_DATA & 0xff};
static void rda_gtp868_init_gpio(void);
static void rda_gtp868_init(struct i2c_client *client);
static void rda_gtp868_startup_chip(struct i2c_client *client);
static void rda_gtp868_reset_chip(struct i2c_client *client);
static int rda_gtp868_get_pos(struct i2c_client *client, struct rda_ts_pos_data * pos_data);
static int rda_gtp868_suspend(struct i2c_client *client, pm_message_t mesg);
static int rda_gtp868_resume(struct i2c_client *client);

//Log define
#define GTP_INFO(fmt,arg...)           printk("<<-GTP-INFO->>[%d]"fmt"\n", __LINE__, ##arg)
#define GTP_ERROR(fmt,arg...)          printk("<<-GTP-ERROR->>[%d]"fmt"\n",__LINE__, ##arg)
#define GTP_DEBUG(fmt,arg...)          do{\
                                         if(GTP_DEBUG_ON)\
                                         printk("<<-GTP-DEBUG->>[%d]"fmt"\n",__LINE__, ##arg);\
                                       }while(0)
#define GTP_DEBUG_ARRAY(array, num)    do{\
                                         s32 i;\
                                         u8* a = array;\
                                         if(GTP_DEBUG_ARRAY_ON)\
                                         {\
                                            printk("<<-GTP-DEBUG-ARRAY->>\n");\
                                            for (i = 0; i < (num); i++)\
                                            {\
                                                printk("%02x   ", (a)[i]);\
                                                if ((i + 1 ) %10 == 0)\
                                                {\
                                                    printk("\n");\
                                                }\
                                            }\
                                            printk("\n");\
                                        }\
                                       }while(0)
#define GTP_DEBUG_FUNC()               do{\
                                         if(GTP_DEBUG_FUNC_ON)\
                                         printk("<<-GTP-FUNC->>[%d]Func:%s\n", __LINE__, __func__);\
                                       }while(0)

#if 0
static void rda_gtp868_reset_gpio(void)
{
	gpio_set_value(GPIO_TOUCH_RESET, 0);
	mdelay(100);
	gpio_set_value(GPIO_TOUCH_RESET, 1);
	mdelay(100);		// Delay 10 ms
	gpio_set_value(GPIO_TOUCH_RESET, 0);
	mdelay(100);		// Delay 10 ms
	gpio_set_value(GPIO_TOUCH_RESET, 1);
	mdelay(100);		// Delay 10 ms
}
#endif
static void rda_gtp868_init_gpio(void)
{
	gpio_request(GPIO_TOUCH_RESET, "touch screen reset");
	gpio_request(GPIO_TOUCH_IRQ, "touch screen interrupt");

	gpio_direction_output(GPIO_TOUCH_RESET, 0);
	mdelay(2);		// Delay 10ms
	gpio_direction_output(GPIO_TOUCH_IRQ, 0);
	mdelay(3);
	gpio_direction_output(GPIO_TOUCH_RESET, 1);
	mdelay(10);
	gpio_direction_input(GPIO_TOUCH_IRQ);

	//rda_gtp868_reset_gpio();
}

s32 gtp_i2c_read(struct i2c_client *client, u8 *buf, s32 len)
{
    struct i2c_msg msgs[2];
    s32 ret=-1;
    s32 retries = 0;

    GTP_DEBUG_FUNC();

    msgs[0].flags = !I2C_M_RD;
    msgs[0].addr  = client->addr;
    msgs[0].len   = GTP_ADDR_LENGTH;
    msgs[0].buf   = &buf[0];

    msgs[1].flags = I2C_M_RD;
    msgs[1].addr  = client->addr;
    msgs[1].len   = len - GTP_ADDR_LENGTH;
    msgs[1].buf   = &buf[GTP_ADDR_LENGTH];

    while(retries < 5)
    {
        ret = i2c_transfer(client->adapter, msgs, 2);
        if (ret == 2)break;
        retries++;
    }
    return ret;
}

s32 gtp_i2c_write(struct i2c_client *client,u8 *buf,s32 len)
{
    struct i2c_msg msg;
    s32 ret=-1;
    s32 retries = 0;

    GTP_DEBUG_FUNC();

    msg.flags = !I2C_M_RD;
    msg.addr  = client->addr;
    msg.len   = len;
    msg.buf   = buf;

    while(retries < 5)
    {
        ret = i2c_transfer(client->adapter, &msg, 1);
        if (ret == 1)break;
        retries++;
    }
    return ret;
}

s32 gtp_i2c_end_cmd(struct i2c_client *client)
{
    s32 ret = -1;
    u8 end_cmd_data[2]={0x80, 0x00}; 
    
    GTP_DEBUG_FUNC();

    ret = gtp_i2c_write(client, end_cmd_data, 2);

    return ret;
}

s32 gtp_send_cfg(struct i2c_client *client)
{
    s32 ret = -1;
#if GTP_DRIVER_SEND_CFG
    s32 retry = 0;

    for (retry = 0; retry < 5; retry++)
    {
        ret = gtp_i2c_write(client, config , GTP_CONFIG_LENGTH + GTP_ADDR_LENGTH);        
        gtp_i2c_end_cmd(client);

        if (ret > 0)
        {
            break;
        }
    }
#endif

    return ret;
}

#if 0
static s8 gtp_i2c_test(struct i2c_client *client)
{
    u8 retry = 0;
    s8 ret = -1;
  
    GTP_DEBUG_FUNC();
  
    while(retry++ < 5)
    {
        ret = gtp_i2c_end_cmd(client);
        if (ret > 0)
        {
            return ret;
        }
        GTP_ERROR("GTP i2c test failed time %d.",retry);
        msleep(10);
    }
    return ret;
}
#endif

static s32 gtp_init_panel(struct i2c_client *client)
{
    s32 ret = -1;
  
#if GTP_DRIVER_SEND_CFG
	u8 sensor_id = 0;

    u8 cfg_info_group1[] = CTP_CFG_GROUP1;
    u8 cfg_info_group2[] = CTP_CFG_GROUP2;
    u8 cfg_info_group3[] = CTP_CFG_GROUP3;
    u8 *send_cfg_buf[3] = {cfg_info_group1, cfg_info_group2, cfg_info_group3};
    u8 cfg_info_len[3] = {sizeof(cfg_info_group1)/sizeof(cfg_info_group1[0]), 
                          sizeof(cfg_info_group2)/sizeof(cfg_info_group2[0]),
                          sizeof(cfg_info_group3)/sizeof(cfg_info_group3[0])};
    GTP_DEBUG("len1= %d,len2= %d,len3= %d",cfg_info_len[0],cfg_info_len[1],cfg_info_len[2]);
    if ((!cfg_info_len[1]) && (!cfg_info_len[2]))
    {
        sensor_id = 0;
    }
    if (sensor_id > 2)
    {
        GTP_ERROR("Invalid Sensor ID: %d, use default value: 0", sensor_id);
        sensor_id = 0;
    }
    GTP_DEBUG("SENSOR ID: %d", sensor_id);
    memcpy(&config[GTP_ADDR_LENGTH], send_cfg_buf[sensor_id], GTP_CONFIG_LENGTH);

#if (GTP_CUSTOM_CFG)
    config[RESOLUTION_LOC]     = (u8)(GTP_MAX_WIDTH);
    config[RESOLUTION_LOC + 1] = (u8)(GTP_MAX_WIDTH>>8);
    config[RESOLUTION_LOC + 2] = (u8)GTP_MAX_HEIGHT;
    config[RESOLUTION_LOC + 3] = (u8)(GTP_MAX_HEIGHT>>8);
    if (GTP_INT_TRIGGER == 0)  //FALLING
    {
        config[TRIGGER_LOC] &= 0xf7; 
    }
    else if (GTP_INT_TRIGGER == 1)  //RISING
    {
        config[TRIGGER_LOC] |= 0x08;
    }
#endif  //endif GTP_CUSTOM_CFG

#else //else DRIVER NEED NOT SEND CONFIG

    ret = gtp_i2c_read(client, config, GTP_CONFIG_LENGTH + GTP_ADDR_LENGTH);
    gtp_i2c_end_cmd(client);
#endif //endif GTP_DRIVER_SEND_CFG

    ret = gtp_send_cfg(client);
    if (ret < 0)
    {
        GTP_ERROR("Send config error. ret = %d", ret);
    }
    msleep(10);

    return 0;
}

s32 gtp_read_version(struct i2c_client *client, u16* version)
{
    s32 ret = -1;
    u8 buf[8] = {GTP_REG_VERSION >> 8, GTP_REG_VERSION & 0xff};

    GTP_DEBUG_FUNC();

    ret = gtp_i2c_read(client, buf, 6);
    gtp_i2c_end_cmd(client);
    if (ret < 0)
    {
        GTP_ERROR("GTP read version failed"); 
        return ret;
    }

    if (version)
    {
        *version = (buf[5] << 8) | buf[4];
    }

    GTP_INFO("IC VERSION:%02x%02x_%02x%02x", buf[3], buf[2], buf[5], buf[4]);

    return ret;
}

// general functions
static void rda_gtp868_init(struct i2c_client *client)
{
	//u8 ret = 0;
	u16 ID_version = 0;

	//ret = gtp_i2c_test(client);
    //if (ret < 0)
    //{
     //   GTP_ERROR("I2C communication ERROR! \n");
    //}
	rda_gtp868_reset_chip(client);
	gtp_init_panel(client);
	gtp_read_version(client, &ID_version);
	GTP_INFO("IC Version: %x\n", ID_version); 
}

static void rda_gtp868_startup_chip(struct i2c_client *client)
{
	gtp_init_panel(client);
}

static void rda_gtp868_reset_chip(struct i2c_client *client)
{
	rda_gtp868_init_gpio();
}

static int rda_gtp868_get_pos(struct i2c_client *client,
			struct rda_ts_pos_data * pos_data)
{
	//u8  index_data[3] = {(u8)(GTP_REG_INDEX>>8),(u8)GTP_REG_INDEX,0};
	u8  point_data[2 + 1 + 8 * GTP_MAX_TOUCH + 1] = {GTP_READ_COOR_ADDR >> 8, GTP_READ_COOR_ADDR & 0xFF};
    u8  touch_num = 0;
	u8  finger = 0;
    static u8 pre_touch = 0;
    u8* coor_data = NULL;
    s32 idx = 0;
    s32 ret = -1;
	u8  end_cmd[3] = {GTP_READ_COOR_ADDR >> 8, GTP_READ_COOR_ADDR & 0xFF, 0};
	u8 i=0;
	u8 key_value;
	//u8 pre_key=0;
    
    GTP_DEBUG_FUNC();

	ret = gtp_i2c_read(client, point_data, 12);
	if (ret < 0)
	{
	if (ret == -EAGAIN)
		return TS_I2C_RETRY;
	else {
        	GTP_ERROR("I2C transfer error. errno:%d\n ", ret);
        	return TS_I2C_ERROR;
	}
	}
		finger = point_data[GTP_ADDR_LENGTH];
		if ((finger & 0x80) == 0)
	{
		goto exit_work_func;
	}
#ifdef TPD_PROXIMITY
	if (tpd_proximity_flag == 1)
	{
		proximity_status = point_data[GTP_ADDR_LENGTH];
		GTP_DEBUG("REG INDEX[0x814E]:0x%02X\n", proximity_status);
		if (proximity_status & 0x60)				//proximity or large touch detect,enable hwm_sensor.
		{
			tpd_proximity_detect = 0;
		}
		else
		{
			tpd_proximity_detect = 1;
		}
		GTP_DEBUG(" ps change\n");
		GTP_DEBUG("PROXIMITY STATUS:0x%02X\n", tpd_proximity_detect);
		sensor_data.values[0] = tpd_get_ps_value();
		sensor_data.value_divide = 1;
		sensor_data.status = SENSOR_STATUS_ACCURACY_MEDIUM;
		ret = hwmsen_get_interrupt_data(ID_PROXIMITY, &sensor_data);
		if (ret)
		{
			GTP_ERROR("Call hwmsen_get_interrupt_data fail = %d\n", err);
		}
	}
#endif
	touch_num = finger & 0x0f;
	//printk(KERN_DEBUG "******function:%s(),line:%d,touch_num=%d******\n",__FUNCTION__, __LINE__,touch_num);
	if (touch_num > GTP_MAX_TOUCH)
	{
			goto exit_work_func;
	}
	if (touch_num > 1)
	{
			u8 buf[8 * GTP_MAX_TOUCH] = {(GTP_READ_COOR_ADDR + 10) >> 8, (GTP_READ_COOR_ADDR + 10) & 0xff};
			ret = gtp_i2c_read(client, buf, 2 + 8 * (touch_num - 1));
			memcpy(&point_data[12], &buf[2], 8 * (touch_num - 1));
	}
#if GTP_HAVE_TOUCH_KEY
	key_value = point_data[3 + 8 * touch_num];
	//printk(KERN_DEBUG "******function:%s(),line:%d,key_value=%d******\n",__FUNCTION__, __LINE__,key_value);
	if (key_value & (0x01<<0)) {
		pos_data->ts_position[0].x = 70;
        pos_data->ts_position[0].y = 840;
		touch_num = 1;
	}
	else if (key_value & (0x01<<1)) {
		pos_data->ts_position[0].x = 240;
        pos_data->ts_position[0].y = 840;
		touch_num = 1;
	}
	else if (key_value & (0x01<<2)) {
		pos_data->ts_position[0].x = 410;
        pos_data->ts_position[0].y = 840;
		touch_num = 1;
	}
#endif
	//GTP_DEBUG("pre_touch:%02x, finger:%02x,touch_num:%02x.", pre_touch, finger,touch_num);
	pos_data->point_num = touch_num;
	if (touch_num)
	{
			for (i = 0; i < touch_num; i++)
			{
			  #if 0
					coor_data = &point_data[i * 8 + 3];
					id = coor_data[0]&0x0F;
					input_x  = coor_data[1] | coor_data[2] << 8;
					input_y  = coor_data[3] | coor_data[4] << 8;
					input_w  = coor_data[5] | coor_data[6] << 8;
			  		if(Read_IC_Version == GT968_ID)
			  		{
				  		input_x = TPD_WARP_X(abs_x_max, input_x);
			 		 }
					input_y = TPD_WARP_Y(abs_y_max, input_y);
			  		GTP_DEBUG("input_x:%d, input_y:%d,input_w:%d,id:%d", input_x, input_y,input_w,id);				
					tpd_down(input_x, input_y, input_w, id);
			   #endif
					coor_data = &point_data[i * 8 + 3];
					idx = coor_data[0]&0x0F;
					if ((key_value&0x07) == 0) {
						#if 1
						pos_data->ts_position[idx].x = (coor_data[1] | coor_data[2] << 8);
            			pos_data->ts_position[idx].y = (coor_data[3] | coor_data[4] << 8);
						#else
					    pos_data->ts_position[idx].x = coor_data[1] | coor_data[2] << 8;
            			pos_data->ts_position[idx].y =( coor_data[3] | coor_data[4] << 8)*795/800;
						#endif
						//printk("x=%d******y=%d\n",pos_data->ts_position[idx].x,pos_data->ts_position[idx].y);
					}
			}
	}
	else if (pre_touch)
	{
			GTP_DEBUG("Touch Release!");
	}
			pre_touch = touch_num;
	#if 0 //jianghuiyan 
			input_report_key(tpd->dev, BTN_TOUCH, (touch_num || key_value));
    #endif
	#if 0
			if (tpd != NULL && tpd->dev != NULL)
			{
				input_sync(tpd->dev);
			}
	#endif
	exit_work_func:
	ret = gtp_i2c_write(client, end_cmd, 3);
	if (ret < 0)
	{
			GTP_INFO("I2C write end_cmd  error!");
	}
#if 0
    ret = gtp_i2c_read(client, index_data, 3);
    gtp_i2c_end_cmd(client);
    if (ret < 0)
    {
	if (ret == -EAGAIN)
		return TS_I2C_RETRY;
	else {
        	GTP_ERROR("I2C transfer error. errno:%d\n ", ret);
        	return TS_I2C_ERROR;
	}
    }

    if ((index_data[GTP_ADDR_LENGTH] & 0x0f) == 0x0f)
    {
        ret = gtp_send_cfg(client);
        if (ret < 0)
        {
            GTP_DEBUG("Reload config failed!\n");
        }
	if (ret == -EAGAIN)
		return TS_I2C_RETRY;
	else 
        	return TS_I2C_ERROR;
    }
    
    if ((index_data[GTP_ADDR_LENGTH] & 0x30) != 0x20)
    {
        GTP_INFO("Data not ready!");
        return TS_PACKET_ERROR;
    }
    
    touch_num = index_data[GTP_ADDR_LENGTH] & 0x0f;
    if(touch_num > 5)
    {
        touch_num = 5;
    }
    ret = gtp_i2c_read(client, point_data, 2 + 8 * touch_num + 1);
    if(ret < 0)	
    {
        GTP_ERROR("I2C transfer error. Number:%d\n ", ret);
        return TS_I2C_ERROR;
    }
    gtp_i2c_end_cmd(client);

    GTP_DEBUG_ARRAY(index_data, 3);
    GTP_DEBUG("touch num:%x", touch_num);
    GTP_DEBUG_ARRAY(point_data, 2 + 8 * touch_num + 1);

    coor_data = &point_data[3];
	pos_data->point_num = touch_num;

    if (pre_touch || touch_num)
    {
        s32 pos = 0;

        for (idx = 0; idx < GTP_MAX_TOUCH; idx++)
        {
            pos_data->ts_position[idx].x = (coor_data[pos + 2] << 8) | coor_data[pos + 1];
            pos_data->ts_position[idx].y = (coor_data[pos + 4] << 8) | coor_data[pos + 3];

            pos += 8;
        }
    }
#endif

	return 0;
}
/*******************************************************
Function:
	Eter sleep function.

Input:
	client:i2c_client.

Output:
	Executive outcomes.0--success,non-0--fail.
*******************************************************/
#if 1
static s8 gtp_enter_sleep(struct i2c_client *client)
{
    s8 ret = -1;
    s8 retry ;
    u8 i2c_control_buf[3] = {(u8)(GTP_REG_SLEEP >> 8), (u8)GTP_REG_SLEEP, 5};
    gpio_set_value(_TGT_AP_GPIO_TOUCH_IRQ, 0);
	msleep(5);
    for (retry = 0; retry < 5; retry++)
    {
         ret = gtp_i2c_write(client, i2c_control_buf, 3);
        gtp_i2c_end_cmd(client);
        if (ret > 0)
        {
            GTP_INFO("GTP enter sleep!");
            return ret;
        }

        msleep(10);
    }
	
    GTP_ERROR("GTP send sleep cmd failed.");
    return ret;
}
#endif
#if 0
/*******************************************************
Function:
	Wakeup from sleep mode Function.

Input:
	client:i2c_client.

Output:
	Executive outcomes.0--success,non-0--fail.
*******************************************************/
static s8 gtp_wakeup_sleep(struct i2c_client *client)
{
    u8 retry = 0;
    s8 ret = -1;


    GTP_INFO("GTP wakeup begin.");


    while (retry++ < 10)
    {
	gpio_set_value(_TGT_AP_GPIO_TOUCH_IRQ, 1);
	mdelay(5);
        ret = gtp_i2c_test(client);

        if (ret >= 0)
        {
            GTP_INFO("GTP wakeup sleep.");
            gtp_int_sync(25);
            return ret;
        }

        gtp_reset_guitar(client, 20);
    }

    GTP_ERROR("GTP wakeup sleep failed.");
    return ret;
}
#endif
/* Function to manage low power suspend */
static int rda_gtp868_suspend(struct i2c_client *client, pm_message_t mesg)
{
	if (client->irq > 0) {
		disable_irq_nosync(client->irq);
	}
	#if 1
	gtp_enter_sleep(client);
	#endif
    printk("****rda_gtp868_suspend******");
	return 0;
}

static int rda_gtp868_resume(struct i2c_client *client)
{
    printk("****rda_gtp960_resume******\n");
	gpio_set_value(_TGT_AP_GPIO_TOUCH_IRQ, 1);
	mdelay(5);
	gtp_send_cfg(client);

	if (client->irq > 0) {
		enable_irq(client->irq);
	}

	return 0;
}

struct rda_ts_panel_info gtp868_info = {
	/* struct rda_lcd_ops ops; */
	.ops = {
		.ts_init_gpio = rda_gtp868_init_gpio,
		.ts_init_chip = rda_gtp868_init,
		.ts_start_chip = rda_gtp868_startup_chip,
		.ts_reset_chip = rda_gtp868_reset_chip,
		.ts_get_parse_data = rda_gtp868_get_pos,
		.ts_suspend = rda_gtp868_suspend,
		.ts_resume = rda_gtp868_resume,
	},

	/* struct lcd_scr_info  src_info; */
	.ts_para = {
		.name = "gtp868",
		.i2c_addr = _DEF_I2C_ADDR_TS_GTP868,
		.x_max = GTP868_RES_X,
		.y_max = GTP868_RES_Y,
		.gpio_irq = GPIO_TOUCH_IRQ,
		.vir_key_num = 3,
		.irqTriggerMode = IRQF_TRIGGER_RISING,
		//.needTimer = true,
		.vir_key = {
			{
				KEY_MENU,
				70,
				840,
				40,
				20,
			},
			{
				KEY_HOMEPAGE,  
				240,
				840,
				40,
				20,
			},
			{
				KEY_BACK,
				410,
				840,
				40,
				20,
			}
		}
	}
};

int rda_ts_add_panel_gtp960(void)
{
	int ret = 0;

	ret = rda_ts_register_driver(&gtp868_info);
	if(ret)
		pr_err("rda_ts_add_panel GTP868 failed \n");

	return ret;
}

