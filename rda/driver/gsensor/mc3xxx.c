/*
 *  mc3xxx.c - Linux kernel modules for 3-Axis Orientation/Motion
 *  Detection Sensor MMA8451/MMA8452/MMA8453
 *
 *  Copyright (C) 2010-2011 Freescale Semiconductor, Inc. All Rights Reserved.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/i2c.h>
#include <linux/pm.h>
#include <linux/mutex.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/hwmon-sysfs.h>
#include <linux/err.h>
#include <linux/hwmon.h>
#include <linux/input-polldev.h>
#include <plat/devices.h>
#include "tgt_ap_board_config.h"

// i2c address
#define MC3XXX_I2C_ADDR		0x4c

// product code
#define MC3XXX_PCODE_3210		0x90
#define MC3XXX_PCODE_3230		0x19
#define MC3XXX_PCODE_3250		0x88
#define MC3XXX_PCODE_3410		0xA8
#define MC3XXX_PCODE_3410N		0xB8
#define MC3XXX_PCODE_3430		0x29
#define MC3XXX_PCODE_3430N		0x39
#define MC3XXX_PCODE_3510B		0x40
#define MC3XXX_PCODE_3530B		0x30
#define MC3XXX_PCODE_3510C		0x10
#define MC3XXX_PCODE_3530C		0x60
#define MC3XXX_PCODE_3433		0x60

// register address define
#define MC3XXX_XOUT_REG			0x00
#define MC3XXX_YOUT_REG			0x01
#define MC3XXX_ZOUT_REG			0x02
#define MC3XXX_TILT_REG			0x03
#define MC3XXX_OPSTAT_REG		0x04
#define MC3XXX_SC_REG			0x05
#define MC3XXX_INTEN_REG		0x06
#define MC3XXX_MODE_REG			0x07
#define MC3XXX_SAMPR_REG		0x08
#define MC3XXX_TAPEN_REG		0x09
#define MC3XXX_TAPP_REG			0x0A
#define MC3XXX_DROP_REG			0x0B
#define MC3XXX_SHDB_REG			0x0C
#define MC3XXX_XOUT_EX_L_REG	0x0D
#define MC3XXX_XOUT_EX_H_REG	0x0E
#define MC3XXX_YOUT_EX_L_REG	0x0F
#define MC3XXX_YOUT_EX_H_REG	0x10
#define MC3XXX_ZOUT_EX_L_REG	0x11
#define MC3XXX_ZOUT_EX_H_REG	0x12

#define MC3XXX_CHIPID_REG		0x18

#define MC3XXX_OUTCFG_REG		0x20
#define MC3XXX_XOFFL_REG		0x21
#define MC3XXX_XOFFH_REG		0x22
#define MC3XXX_YOFFL_REG		0x23
#define MC3XXX_YOFFH_REG		0x24
#define MC3XXX_ZOFFL_REG		0x25
#define MC3XXX_ZOFFH_REG		0x26
#define MC3XXX_XGAIN_REG		0x27
#define MC3XXX_YGAIN_REG		0x28
#define MC3XXX_ZGAIN_REG		0x29

#define MC3XXX_SHAKE_TH_REG		0x2B
#define MC3XXX_UD_Z_TH_REG		0x2C
#define MC3XXX_UD_X_TH_REG		0x2D
#define MC3XXX_RL_Z_TH_REG		0x2E
#define MC3XXX_RL_Y_TH_REG		0x2F
#define MC3XXX_FB_Z_TH_REG		0x30
#define MC3XXX_DROP_TH_REG		0x31
#define MC3XXX_TAP_TH_REG		0x32

#define MC3XXX_PCODE_REG		0x3B

// mode
#define MC3XXX_MODE_AUTO			0
#define MC3XXX_MODE_WAKE			1
#define MC3XXX_MODE_SNIFF			2
#define MC3XXX_MODE_STANDBY		    3

// range
#define MC3XXX_RANGE_2G			    0
#define MC3XXX_RANGE_4G			    1
#define MC3XXX_RANGE_8G_10BIT		2
#define MC3XXX_RANGE_8G_14BIT		3

// bandwidth
#define MC3XXX_BW_512HZ			    0
#define MC3XXX_BW_256HZ			    1
#define MC3XXX_BW_128HZ			    2
#define MC3XXX_BW_64HZ				3
#define MC3XXX_BW_32HZ				4
#define MC3XXX_BW_16HZ				5
#define MC3XXX_BW_8HZ				6

// initial value
#define MC3XXX_RANGE_SET			MC3XXX_RANGE_8G_14BIT  /* +/-8g, 14bit */
#define MC3XXX_BW_SET				MC3XXX_BW_128HZ /* 128HZ  */
#define MC3XXX_MAX_DELAY			200
#define ABSMIN_8G					(-8 * 1024)
#define ABSMAX_8G					(8 * 1024)
#define ABSMIN_1_5G					(-128)
#define ABSMAX_1_5G				        (128)

// 1g constant value
#define GRAVITY_1G_VALUE			1024

#define REMAP_IF_MC3250_READ(nDataX, nDataY) \
            if (MC3XXX_PCODE_3250 == s_bPCODE)          \
            {                                                                         \
                int    _nTemp = 0;                                           \
                                                                                       \
                _nTemp = nDataX;                                         \
                nDataX = nDataY;                                         \
                nDataY = -_nTemp;                                      \
            }

#define REMAP_IF_MC3250_WRITE(nDataX, nDataY) \
            if (MC3XXX_PCODE_3250 == s_bPCODE)            \
            {                                                                           \
                int    _nTemp = 0;                                             \
                                                                                         \
                _nTemp = nDataX;                                           \
                nDataX = -nDataY;                                         \
                nDataY = _nTemp;                                          \
            }

#define REMAP_IF_MC34XX_N(nDataX, nDataY)        \
            if ((MC3XXX_PCODE_3410N == s_bPCODE) ||  \
                 (MC3XXX_PCODE_3430N == s_bPCODE) ||\
              (MC3XXX_PCODE_3433 == s_bPCODE))\
            {                                                                          \
                nDataX = -nDataX;                                        \
                nDataY = -nDataY;                                       \
            }

#define IS_MC35XX()                                                  \
            ((MC3XXX_PCODE_3510B == s_bPCODE) ||   \
             (MC3XXX_PCODE_3510C == s_bPCODE) ||   \
             (MC3XXX_PCODE_3530B == s_bPCODE) ||   \
             (MC3XXX_PCODE_3530C == s_bPCODE))

#define REMAP_IF_MC35XX(nDataX, nDataY)          \
            if (IS_MC35XX())                                            \
            {                                                                       \
                nDataX = -nDataX;                                     \
                nDataY = -nDataY;                                    \
            }


// poll parameter
#define POLL_INTERVAL_MIN	200
#define POLL_INTERVAL_MAX	500
#define POLL_INTERVAL		200 /* msecs */

// if sensor is standby ,set POLL_STOP_TIME to slow down the poll
#define POLL_STOP_TIME		200
#define INPUT_FUZZ			32
#define INPUT_FLAT			32
#define MODE_CHANGE_DELAY_MS	100

#define MC3XXX_BUF_SIZE	6

enum mc3xxx_axis {
	MC3XXX_AXIS_X = 0,
	MC3XXX_AXIS_Y,
	MC3XXX_AXIS_Z,
	MC3XXX_AXIS_NUM
};

enum mc3xxx_orientation {
	MC3XXX_TOP_LEFT_DOWN = 0,
	MC3XXX_TOP_RIGHT_DOWN,
	MC3XXX_TOP_RIGHT_UP,
	MC3XXX_TOP_LEFT_UP,
	MC3XXX_BOTTOM_LEFT_DOWN,
	MC3XXX_BOTTOM_RIGHT_DOWN,
	MC3XXX_BOTTOM_RIGHT_UP,
	MC3XXX_BOTTOM_LEFT_UP
};

/* The sensitivity is represented in counts/g. In 2g mode the
sensitivity is 1024 counts/g. In 4g mode the sensitivity is 512
counts/g and in 8g mode the sensitivity is 256 counts/g.
 */
enum {
	MODE_2G = 0,
	MODE_4G,
	MODE_8G,
};

enum {
	MMA_STANDBY = 0,
	MMA_ACTIVED,
};

struct mc3xxx_data_axis{
	short x;
	short y;
	short z;
};

struct mc3xxx_hwmsen_convert {
	signed char sign[3];
	unsigned char map[3];
};

struct mc3xxx_data{
	struct i2c_client * client;
	struct input_polled_dev *poll_dev;
	struct mutex data_lock;
	int active;
	int mode;
	unsigned short value_1g;
	unsigned char position;
	unsigned char product_code;
};

// Transformation matrix for chip mounting position
static const struct mc3xxx_hwmsen_convert mc3xxx_cvt[] = {
	{{  1,   1,   1}, { MC3XXX_AXIS_X,  MC3XXX_AXIS_Y,  MC3XXX_AXIS_Z}}, // 0: top   , left-down
	{{ -1,   1,   1}, { MC3XXX_AXIS_Y,  MC3XXX_AXIS_X,  MC3XXX_AXIS_Z}}, // 1: top   , right-down
	{{ -1,  -1,   1}, { MC3XXX_AXIS_X,  MC3XXX_AXIS_Y,  MC3XXX_AXIS_Z}}, // 2: top   , right-up
	{{  1,  -1,   1}, { MC3XXX_AXIS_Y,  MC3XXX_AXIS_X,  MC3XXX_AXIS_Z}}, // 3: top   , left-up
	{{ -1,   1,  -1}, { MC3XXX_AXIS_X,  MC3XXX_AXIS_Y,  MC3XXX_AXIS_Z}}, // 4: bottom, left-down
	{{  1,   1,  -1}, { MC3XXX_AXIS_Y,  MC3XXX_AXIS_X,  MC3XXX_AXIS_Z}}, // 5: bottom, right-down
	{{  1,  -1,  -1}, { MC3XXX_AXIS_X,  MC3XXX_AXIS_Y,  MC3XXX_AXIS_Z}}, // 6: bottom, right-up
	{{ -1,  -1,  -1}, { MC3XXX_AXIS_Y,  MC3XXX_AXIS_X,  MC3XXX_AXIS_Z}}, // 7: bottom, left-up
};

// Current soldered placement
static const unsigned char mc3xxx_current_placement = _TGT_AP_GSENSOR_POSITION;
static unsigned char s_bPCODE  = 0x00;
//static unsigned short mc3xxx_i2c_auto_probe_addr[] = { 0x4C, 0x6C, 0x4E, 0x6D, 0x6E, 0x6F };

static bool mc3xxx_is_high_end(unsigned char product_code)
{
	if ((MC3XXX_PCODE_3230 == product_code) || (MC3XXX_PCODE_3430 == product_code) ||
	     (MC3XXX_PCODE_3430N == product_code) || (MC3XXX_PCODE_3530B == product_code) ||
	     (MC3XXX_PCODE_3530C == product_code)||
	     (MC3XXX_PCODE_3433 == product_code))
		return false;
	else
		return true;
}

static bool mc3xxx_validate_pcode(unsigned char bPCode)
{
	if ((MC3XXX_PCODE_3210  == bPCode) || (MC3XXX_PCODE_3230  == bPCode)
	    || (MC3XXX_PCODE_3250  == bPCode)
	    || (MC3XXX_PCODE_3410  == bPCode) || (MC3XXX_PCODE_3430  == bPCode)
	    || (MC3XXX_PCODE_3410N == bPCode) || (MC3XXX_PCODE_3430N == bPCode)
	    || (MC3XXX_PCODE_3510B == bPCode) || (MC3XXX_PCODE_3530B == bPCode)
	    || (MC3XXX_PCODE_3510C == bPCode) || (MC3XXX_PCODE_3530C == bPCode) 
	    ||(MC3XXX_PCODE_3433 == bPCode))
	{
		s_bPCODE = bPCode;
		return true;
	}

	return false;
}

static int mc3xxx_data_convert(struct mc3xxx_data *pdata, struct mc3xxx_data_axis *axis_data)
{
	short data[3];
	unsigned char position = pdata->position;
	unsigned short value_1g = pdata->value_1g;
	const struct mc3xxx_hwmsen_convert *pCvt = NULL;

	if (position > 7 )
		position = 0;

	data[0] = axis_data->x * GRAVITY_1G_VALUE / value_1g;
	data[1] = axis_data->y * GRAVITY_1G_VALUE / value_1g;
	data[2] = axis_data->z * GRAVITY_1G_VALUE / value_1g;

	REMAP_IF_MC3250_READ(data[0], data[1]);
	REMAP_IF_MC34XX_N(data[0], data[1]);
	REMAP_IF_MC35XX(data[0], data[1]);

	pCvt = &mc3xxx_cvt[position];
	axis_data->x = pCvt->sign[MC3XXX_AXIS_X] * data[pCvt->map[MC3XXX_AXIS_X]];
	axis_data->y = pCvt->sign[MC3XXX_AXIS_Y] * data[pCvt->map[MC3XXX_AXIS_Y]];
	axis_data->z = pCvt->sign[MC3XXX_AXIS_Z] * data[pCvt->map[MC3XXX_AXIS_Z]];

	return 0;
}
static int mc3xxx_device_init(struct i2c_client *client)
{
	int result;
	struct mc3xxx_data *pdata = i2c_get_clientdata(client);

	result = i2c_smbus_write_byte_data(client, 0x07, 0x43);
	if (result < 0)
		goto out;

	if (mc3xxx_is_high_end(pdata->product_code))
	{
		result = i2c_smbus_write_byte_data(client, 0x20, 0x2f);
		if (result < 0)
			goto out;

		pdata->value_1g = 1024;
	}
	else
	{
		result = i2c_smbus_write_byte_data(client, 0x20, 0x02);
		pdata->value_1g = 64;
	}

	pdata->active = MMA_STANDBY;
	msleep(MODE_CHANGE_DELAY_MS);
	return 0;
out:
	dev_err(&client->dev, "error when init mc3xxx:(%d)", result);
	return result;
}

static int mc3xxx_device_stop(struct i2c_client *client)
{
	u8 val;
	val = i2c_smbus_read_byte_data(client, 0x07);
	i2c_smbus_write_byte_data(client, 0x07,val | 0x03);
	return 0;
}

static int mc3xxx_read_data(struct i2c_client *client,struct mc3xxx_data_axis *data)
{
	struct mc3xxx_data *pdata = i2c_get_clientdata(client);
	u8 tmp_data[MC3XXX_BUF_SIZE];
	int ret;

	if (mc3xxx_is_high_end(pdata->product_code))
	{
		ret = i2c_smbus_read_i2c_block_data(client,
					    MC3XXX_XOUT_EX_L_REG, MC3XXX_BUF_SIZE, tmp_data);
		if (ret < MC3XXX_BUF_SIZE) {
			dev_err(&client->dev, "i2c block read failed 0\n");
			return -EIO;
		}

		data->x = ((tmp_data[1] << 8) & 0xff00) | tmp_data[0];
		data->y = ((tmp_data[3] << 8) & 0xff00) | tmp_data[2];
		data->z = ((tmp_data[5] << 8) & 0xff00) | tmp_data[4];
	}
	else
	{
		ret = i2c_smbus_read_i2c_block_data(client,
					    MC3XXX_XOUT_REG, 3, tmp_data);
		if (ret < 3) {
			dev_err(&client->dev, "i2c block read failed 1\n");
			return -EIO;
		}

		data->x = (s8)tmp_data[0];
		data->y = (s8)tmp_data[1];
		data->z = (s8)tmp_data[2];
	}
	return 0;
}

static void mc3xxx_report_data(struct mc3xxx_data* pdata)
{
	struct input_polled_dev * poll_dev = pdata->poll_dev;
	struct mc3xxx_data_axis data;
	mutex_lock(&pdata->data_lock);
	if(pdata->active == MMA_STANDBY){
		poll_dev->poll_interval = POLL_STOP_TIME;  // if standby ,set as 10s to slow the poll,
		goto out;
	}else{
		if(poll_dev->poll_interval == POLL_STOP_TIME)
			poll_dev->poll_interval = POLL_INTERVAL;
	}
	if (mc3xxx_read_data(pdata->client,&data) != 0)
		goto out;
	mc3xxx_data_convert(pdata,&data);
	input_report_abs(poll_dev->input, ABS_X, data.x);
	input_report_abs(poll_dev->input, ABS_Y, data.y);
	input_report_abs(poll_dev->input, ABS_Z, data.z);
	input_sync(poll_dev->input);
out:
	mutex_unlock(&pdata->data_lock);
}

static void mc3xxx_dev_poll(struct input_polled_dev *dev)
{
	struct mc3xxx_data* pdata = (struct mc3xxx_data*)dev->private;
	mc3xxx_report_data(pdata);
}

static ssize_t mc3xxx_enable_show(struct device *dev,
				   struct device_attribute *attr, char *buf)
{
	struct input_polled_dev *poll_dev = dev_get_drvdata(dev);
	struct mc3xxx_data *pdata = (struct mc3xxx_data *)(poll_dev->private);
	struct i2c_client *client = pdata->client;
	u8 val;
	int enable;

	mutex_lock(&pdata->data_lock);
	val = i2c_smbus_read_byte_data(client, 0x07);
	if((0x01 == (val & 0x03)) && pdata->active == MMA_ACTIVED)
		enable = 1;
	else
		enable = 0;
	mutex_unlock(&pdata->data_lock);
	return sprintf(buf, "%d\n", enable);
}

static ssize_t mc3xxx_enable_store(struct device *dev,
				    struct device_attribute *attr,
				    const char *buf, size_t count)
{
	struct input_polled_dev *poll_dev = dev_get_drvdata(dev);
	struct mc3xxx_data *pdata = (struct mc3xxx_data *)(poll_dev->private);
	struct i2c_client *client = pdata->client;
	int ret;
	unsigned long enable;
	u8 val = 0;
	enable = simple_strtoul(buf, NULL, 10);
	mutex_lock(&pdata->data_lock);
	enable = (enable > 0) ? 1 : 0;	if(enable && pdata->active == MMA_STANDBY)
	{
		val = i2c_smbus_read_byte_data(client,0x07);
		val &= 0xfc;
		ret = i2c_smbus_write_byte_data(client,0x07, val|0x01);
		if(!ret){
			pdata->active = MMA_ACTIVED;
			printk("mma enable setting active \n");
		}
	}
	else if(enable == 0  && pdata->active == MMA_ACTIVED)
	{
		val = i2c_smbus_read_byte_data(client,0x07);
		ret = i2c_smbus_write_byte_data(client, 0x07,val | 0x03);
		if(!ret){
			pdata->active= MMA_STANDBY;
			printk("mma enable setting inactive \n");
		}
	}
	mutex_unlock(&pdata->data_lock);
	return count;
}
static ssize_t mc3xxx_position_show(struct device *dev,
				   struct device_attribute *attr, char *buf)
{
	struct input_polled_dev *poll_dev = dev_get_drvdata(dev);
	struct mc3xxx_data *pdata = (struct mc3xxx_data *)(poll_dev->private);
	int position = 0;
	mutex_lock(&pdata->data_lock);
	position = pdata->position ;
	mutex_unlock(&pdata->data_lock);
	return sprintf(buf, "%d\n", position);
}

static ssize_t mc3xxx_position_store(struct device *dev,
				    struct device_attribute *attr,
				    const char *buf, size_t count)
{
	struct input_polled_dev *poll_dev = dev_get_drvdata(dev);
	struct mc3xxx_data *pdata = (struct mc3xxx_data *)(poll_dev->private);
	int  position;
	position = simple_strtoul(buf, NULL, 10);
	mutex_lock(&pdata->data_lock);
	pdata->position = position;
	mutex_unlock(&pdata->data_lock);
	return count;
}
static ssize_t mc3xxx_powerdown_store(struct device *dev,
                    struct device_attribute *attr,
                    const char *buf, size_t count)
{
    struct input_polled_dev *poll_dev = dev_get_drvdata(dev);
    struct mc3xxx_data *pdata = (struct mc3xxx_data *)(poll_dev->private);
    struct i2c_client *client = pdata->client;
    mutex_lock(&pdata->data_lock);
    mc3xxx_device_stop(client);
    printk("*****rda  the function of gsensor is stopped  \n");
    mutex_unlock(&pdata->data_lock);
    return count;
}

static DEVICE_ATTR(enable,S_IWUSR | S_IWGRP | S_IRUGO,
            mc3xxx_enable_show, mc3xxx_enable_store);
static DEVICE_ATTR(position,S_IWUSR | S_IWGRP | S_IRUGO,
            mc3xxx_position_show, mc3xxx_position_store);
static DEVICE_ATTR(powerdown,S_IWUSR | S_IWGRP | S_IRUGO,
            NULL, mc3xxx_powerdown_store);

static struct attribute *mc3xxx_attributes[] = {
            &dev_attr_enable.attr,
            &dev_attr_position.attr,
            &dev_attr_powerdown.attr,
            NULL
};

static const struct attribute_group mc3xxx_attr_group = {
	.attrs = mc3xxx_attributes,
};

static int mc3xxx_probe(struct i2c_client *client,
				   const struct i2c_device_id *id)
{
	int result = 0, chip_id;
	struct input_dev *idev;
	struct mc3xxx_data *pdata;
	struct i2c_adapter *adapter;
	struct input_polled_dev *poll_dev;

	adapter = to_i2c_adapter(client->dev.parent);
	result = i2c_check_functionality(adapter,
					 I2C_FUNC_SMBUS_BYTE |
					 I2C_FUNC_SMBUS_BYTE_DATA);
	if (!result)
		goto err_out;
	chip_id = i2c_smbus_read_byte_data(client, 0x3b);
	printk("*****rda mc3xxx_probe chipid=0x%x \n" ,chip_id);
	//if (chip_id != 0x88) {
	if (!mc3xxx_validate_pcode(chip_id))
	{
		dev_err(&client->dev,
		"mc3xxx_probe is not mCube mc3xxx!\n");
		result = -EINVAL;
		goto err_out;
	}
	pdata = kzalloc(sizeof(struct mc3xxx_data), GFP_KERNEL);
	if(!pdata){
		result = -ENOMEM;
		dev_err(&client->dev, "alloc data memory error!\n");
		goto err_out;
	}
	/* Initialize the MC3XXX chip */
	pdata->client = client;
	pdata->product_code = chip_id;
	pdata->mode = MODE_8G;
	pdata->position = mc3xxx_current_placement; //_TGT_AP_GSENSOR_POSITION;
	mutex_init(&pdata->data_lock);
	i2c_set_clientdata(client,pdata);
	mc3xxx_device_init(client);
	poll_dev = input_allocate_polled_device();
	if (!poll_dev) {
		result = -ENOMEM;
		dev_err(&client->dev, "alloc poll device failed!\n");
		goto err_alloc_poll_device;
	}
	poll_dev->poll = mc3xxx_dev_poll;
	poll_dev->poll_interval = POLL_STOP_TIME;
	poll_dev->poll_interval_min = POLL_INTERVAL_MIN;
	poll_dev->poll_interval_max = POLL_INTERVAL_MAX;
	poll_dev->private = pdata;
	idev = poll_dev->input;
	idev->name = RDA_GSENSOR_DRV_NAME;
	idev->uniq = "mc3xxx"; //mc3xxx_id2name(pdata->chip_id);
	idev->id.bustype = BUS_I2C;
	idev->evbit[0] = BIT_MASK(EV_ABS);
	input_set_abs_params(idev, ABS_X, -0x7fff, 0x7fff, 0, 0);
	input_set_abs_params(idev, ABS_Y, -0x7fff, 0x7fff, 0, 0);
	input_set_abs_params(idev, ABS_Z, -0x7fff, 0x7fff, 0, 0);
	pdata->poll_dev = poll_dev;
	result = input_register_polled_device(pdata->poll_dev);
	if (result) {
		dev_err(&client->dev, "register poll device failed!\n");
		goto err_register_polled_device;
	}
	result = sysfs_create_group(&idev->dev.kobj, &mc3xxx_attr_group);
	if (result) {
		dev_err(&client->dev, "create device file failed!\n");
		result = -EINVAL;
		goto err_create_sysfs;
	}
	kobject_uevent(&idev->dev.kobj, KOBJ_CHANGE);
	return 0;
err_create_sysfs:
	input_unregister_polled_device(pdata->poll_dev);
err_register_polled_device:
	input_free_polled_device(poll_dev);
err_alloc_poll_device:
	kfree(pdata);
err_out:
	return result;
}
static int mc3xxx_remove(struct i2c_client *client)
{
	struct mc3xxx_data *pdata = i2c_get_clientdata(client);
	struct input_polled_dev *poll_dev = pdata->poll_dev;
	mc3xxx_device_stop(client);
	if(pdata){
		input_unregister_polled_device(poll_dev);
		input_free_polled_device(poll_dev);
		kfree(pdata);
	}
	return 0;
}

#ifdef CONFIG_PM_SLEEP
static int mc3xxx_suspend(struct device *dev)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct mc3xxx_data *pdata = i2c_get_clientdata(client);
	if(pdata->active == MMA_ACTIVED)
		mc3xxx_device_stop(client);
	return 0;
}

static int mc3xxx_resume(struct device *dev)
{
	int val = 0;
	struct i2c_client *client = to_i2c_client(dev);
	struct mc3xxx_data *pdata = i2c_get_clientdata(client);
	if(pdata->active == MMA_ACTIVED){
		val = i2c_smbus_read_byte_data(client,0x07);
		val &= 0xfc;
		i2c_smbus_write_byte_data(client, 0x07, val|0x01);
	}

	return 0;
}
#endif

static const struct i2c_device_id mc3xxx_id[] = {
	{"mc3xxx", 0},
	{ }
};
MODULE_DEVICE_TABLE(i2c, mc3xxx_id);

static SIMPLE_DEV_PM_OPS(mc3xxx_pm_ops, mc3xxx_suspend, mc3xxx_resume);
static struct i2c_driver mc3xxx_driver = {
	.driver = {
		   .name = "mc3xxx",
		   .owner = THIS_MODULE,
		   .pm = &mc3xxx_pm_ops,
	},
	.probe = mc3xxx_probe,
	.remove = mc3xxx_remove,
	.id_table = mc3xxx_id,
};

static struct rda_gsensor_device_data rda_gsensor_data[] = {
	{
		.irqflags = IRQF_SHARED | IRQF_TRIGGER_RISING,
	},
};

static int __init mc3xxx_init(void)
{
	/* register driver */
	int res;

	static struct i2c_board_info i2c_dev_gsensor = {
	        I2C_BOARD_INFO("mc3xxx", 0),
	        .platform_data = rda_gsensor_data,
	};

	struct i2c_adapter *adapter;
	i2c_dev_gsensor.addr =  _DEF_I2C_ADDR_GSENSOR_MC3XXX;

	adapter = i2c_get_adapter(_TGT_AP_I2C_BUS_ID_GSENSOR);

	if (!adapter) {
		pr_err("%s, cannot get i2c adapter %d\n",
			__func__, _TGT_AP_I2C_BUS_ID_GSENSOR);
		return -ENODEV;
	}

	i2c_new_device(adapter, &i2c_dev_gsensor);

	res = i2c_add_driver(&mc3xxx_driver);
	if (res < 0) {
		printk(KERN_INFO "add mc3xxx i2c driver failed\n");
		return -ENODEV;
	}

	pr_info("rda_gs %s initialized, at i2c bus %d addr 0x%02x\n",
		"mc3xxx", _TGT_AP_I2C_BUS_ID_GSENSOR,
		i2c_dev_gsensor.addr);

	return res;
}


static void __exit mc3xxx_exit(void)
{
	i2c_del_driver(&mc3xxx_driver);
}

MODULE_AUTHOR("Freescale Semiconductor, Inc.");
MODULE_DESCRIPTION("MC3XXX 3-Axis Orientation/Motion Detection Sensor driver");
MODULE_LICENSE("GPL");

module_init(mc3xxx_init);
module_exit(mc3xxx_exit);
