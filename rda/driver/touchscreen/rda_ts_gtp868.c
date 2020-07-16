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

#include "gtp868.h"


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

static void rda_gtp868_reset_gpio(void)
{
	gpio_set_value(GPIO_TOUCH_RESET, 0);
	mdelay(10);
	gpio_set_value(GPIO_TOUCH_RESET, 1);
	mdelay(10);		// Delay 10 ms
	gpio_set_value(GPIO_TOUCH_RESET, 0);
	mdelay(10);		// Delay 10 ms
	gpio_set_value(GPIO_TOUCH_RESET, 1);
}

static void rda_gtp868_init_gpio(void)
{
	gpio_request(GPIO_TOUCH_RESET, "touch screen reset");
	gpio_request(GPIO_TOUCH_IRQ, "touch screen interrupt");

	gpio_direction_output(GPIO_TOUCH_RESET, 0);
	mdelay(10);		// Delay 10ms
	gpio_direction_input(GPIO_TOUCH_IRQ);
	mdelay(10);

	rda_gtp868_reset_gpio();
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
	u8 ret = 0;
	u16 ID_version = 0;

	ret = gtp_i2c_test(client);
    if (ret < 0)
    {
        GTP_ERROR("I2C communication ERROR! \n");
    }
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
	rda_gtp868_reset_gpio();
}

static int rda_gtp868_get_pos(struct i2c_client *client,
			struct rda_ts_pos_data * pos_data)
{
	u8  index_data[3] = {(u8)(GTP_REG_INDEX>>8),(u8)GTP_REG_INDEX,0};
    u8  point_data[2 + 1 + 8 * GTP_MAX_TOUCH] = {GTP_READ_COOR_ADDR>>8, GTP_READ_COOR_ADDR & 0xFF};  
    u8  touch_num = 0;
    static u8 pre_touch = 0;
    u8* coor_data = NULL;
    s32 idx = 0;
    s32 ret = -1;
    
    GTP_DEBUG_FUNC();

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
    else
    {
        u8 key1 = 0, key2 = 0, key3 = 0;
        static u8 key1_old = 0, key2_old = 0, key3_old = 0;

        key1 = (point_data[2] & 0x01);
        key2 = (point_data[2] & 0x02);
        key3 = (point_data[2] & 0x04);
        if (key1 == 1)
        {
	    pos_data->ts_position[0].x = 60;
	    pos_data->ts_position[0].y = 850;
	    pos_data->point_num = 1;
        }
        else if ((key1_old == 1) & (key1 == 0))
        {
            
        }

        if (key2 == 2)
        {
 	    pos_data->ts_position[0].x = 180;
	    pos_data->ts_position[0].y = 850;
	    pos_data->point_num = 1;
        }
        else if ((key2_old == 2) & (key2 == 0))
        {
            
        }

        if (key3 == 4)
        {
 	    pos_data->ts_position[0].x = 420;
	    pos_data->ts_position[0].y = 850;
	    pos_data->point_num = 1;
        }
        else if ((key3_old == 4) & (key3 == 0))
        {
            
        }

        key1_old = key1;
        key2_old = key2;
        key3_old = key3;
    }

    return 0;
}

static int rda_gtp868_suspend(struct i2c_client *client, pm_message_t mesg)
{
	int ret = 0;
    	u8 mode_data[3]={0x06, 0x92, 1}; 

    	ret = gtp_i2c_write(client, mode_data, 3);

        gtp_i2c_end_cmd(client);

	mdelay(50);


	return ret;
}

static int rda_gtp868_resume(struct i2c_client *client)
{
	gpio_set_value(GPIO_TOUCH_RESET, 0);
	mdelay(1);

	gpio_set_value(GPIO_TOUCH_RESET, 1);
	mdelay(1);
	
	gtp_send_cfg(client);

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
		.vir_key = {
			{
				KEY_MENU,
				60,
				850,
				40,
				20,
			},
			{
				KEY_HOMEPAGE,  
				180,
				850,
				40,
				20,
			},
			{
				KEY_BACK,
				420,
				850,
				40,
				20,
			}
		}
	}
};

int rda_ts_add_panel_gtp868(void)
{
	int ret = 0;

	ret = rda_ts_register_driver(&gtp868_info);
	if(ret)
		pr_err("rda_ts_add_panel GTP868 failed \n");

	return ret;
}

