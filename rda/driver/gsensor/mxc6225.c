/*
 *  mcx6225.c - Linux kernel modules for 3-Axis Orientation/Motion
 *  Detection Sensor MMA8652/MMA8653
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

#define MMA8652_ID		0x4A
#define MMA8653_ID		0x5A
#define MXC6225_ID		0x01

#define POLL_INTERVAL_MIN	200
#define POLL_INTERVAL_MAX	500
#define POLL_INTERVAL		200 /* msecs */
// if sensor is standby ,set POLL_STOP_TIME to slow down the poll
#define POLL_STOP_TIME		100  


#define INPUT_FUZZ		32
#define INPUT_FLAT		32
#define MODE_CHANGE_DELAY_MS	100

#define MMA865X_STATUS_ZYXDR	0x08
#define MXC6225_BUF_SIZE	3
#define CONFIG_SENSORS_MXC6225_POSITION 0x10

/* MXC622X register address */
#define MXC622X_REG_CTRL		0x04
#define MXC622X_REG_DATA		0x00
/* MXC622X control bit */
#define MXC622X_CTRL_PWRON		0x00	/* power on */
#define MXC622X_CTRL_PWRDN		0x80	/* power donw */

/* register enum for mcx6225 registers */
enum {
	MXC6225_STATUS = 0x00,
	MXC6225_OUT_X_MSB,
	MXC6225_OUT_X_LSB,
	MXC6225_OUT_Y_MSB,
	MXC6225_OUT_Y_LSB,
	MXC6225_OUT_Z_MSB,
	MXC6225_OUT_Z_LSB,

	MXC6225_F_SETUP = 0x09,
	MXC6225_TRIG_CFG,
	MXC6225_SYSMOD,
	MXC6225_INT_SOURCE,
	MXC6225_WHO_AM_I,
	MXC6225_XYZ_DATA_CFG,
	MXC6225_HP_FILTER_CUTOFF,

	MXC6225_PL_STATUS,
	MXC6225_PL_CFG,
	MXC6225_PL_COUNT,
	MXC6225_PL_BF_ZCOMP,
	MXC6225_P_L_THS_REG,

	MXC6225_FF_MT_CFG,
	MXC6225_FF_MT_SRC,
	MXC6225_FF_MT_THS,
	MXC6225_FF_MT_COUNT,

	MXC6225_TRANSIENT_CFG = 0x1D,
	MXC6225_TRANSIENT_SRC,
	MXC6225_TRANSIENT_THS,
	MXC6225_TRANSIENT_COUNT,

	MXC6225_PULSE_CFG,
	MXC6225_PULSE_SRC,
	MXC6225_PULSE_THSX,
	MXC6225_PULSE_THSY,
	MXC6225_PULSE_THSZ,
	MXC6225_PULSE_TMLT,
	MXC6225_PULSE_LTCY,
	MXC6225_PULSE_WIND,

	MXC6225_ASLP_COUNT,
	MXC6225_CTRL_REG1,
	MXC6225_CTRL_REG2,
	MXC6225_CTRL_REG3,
	MXC6225_CTRL_REG4,
	MXC6225_CTRL_REG5,

	MXC6225_OFF_X,
	MXC6225_OFF_Y,
	MXC6225_OFF_Z,

	MXC6225_REG_END,
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
struct mxc6225_data_axis{
	int8_t x;
	int8_t y;
	int8_t z;
};
struct mxc6225_data{
	struct i2c_client * client;
	struct input_polled_dev *poll_dev;
	struct mutex data_lock;
	int active;
	int position;
	u8 chip_id;
	int mode;
};

static char * mxc6225_names[] ={
	"mma",
	"mma",
};

static int mxc6225_position_setting[8][3][3] =
{
	{{ 0, -1,  0}, { 1,  0,	0}, {0, 0,	1}},
	{{-1,  0,  0}, { 0, -1,	0}, {0, 0,	1}},
	{{ 0,  1,  0}, {-1,  0,	0}, {0, 0,	1}},
	{{ 1,  0,  0}, { 0,  1,	0}, {0, 0,	1}},

	{{ 0, -1,  0}, {-1,  0,	0}, {0, 0,  -1}},
	{{-1,  0,  0}, { 0,  1,	0}, {0, 0,  -1}},
	{{ 0,  1,  0}, { 1,  0,	0}, {0, 0,  -1}},
	{{ 1,  0,  0}, { 0, -1,	0}, {0, 0,  -1}},
};

static int rda_gsensor_i2c_write(struct i2c_client *client, u8 addr, u8 * pdata,
				 int datalen)
{
	int ret = 0;
	u8 tmp_buf[128];
	unsigned int bytelen = 0;
	if (datalen > 125) {
		return -1;
	}
	tmp_buf[0] = addr;
	bytelen++;
	if (datalen != 0 && pdata != NULL) {
		memcpy(&tmp_buf[bytelen], pdata, datalen);
		bytelen += datalen;
	}
	ret = i2c_master_send(client, tmp_buf, bytelen);
	return ret;
}

static int rda_gsensor_i2c_read(struct i2c_client *client, u8 addr, u8 * pdata,
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

static int mxc6225_data_convert(struct mxc6225_data* pdata, struct mxc6225_data_axis *axis_data)
{
	int8_t data[3];
	int i,j;
	int position = pdata->position ;
	int pos_set[3] = {0, 0, 0};

	if(position < 0 || position > 7 )
		position = 0;

	data [0] = axis_data->x;
	data [1] = axis_data->y;
	data [2] = axis_data->z & 0x3; /* only keep the orientation bits */

	for(i = 0; i < 3; i++) {
		for(j = 0; j < 3; j++) {
			pos_set[i] += mxc6225_position_setting[position][i][j];
		}
	}

	/* convert the orientation bits */
	if(pos_set[2] > 0) { /* top */
		if(pos_set[0] > 0 && pos_set[1] > 0) { /* 0 degree */
			/* do nothing */
		} else if(pos_set[0] < 0 && pos_set[1] > 0) { /* 240 degree */
			switch(data[2]){
			case 0x3:
				data[2] = 0x0;
				break;
			case 0x2:
				data[2] = 0x3;
				break;
			case 0x1:
				data[2] = 0x2;
				break;
			case 0x0:
			default:
				data[2] = 0x1;
				break;
			}
		} else if(pos_set[0] < 0 && pos_set[1] < 0) { /* 180 degree */
			switch(data[2]){
			case 0x3:
				data[2] = 0x1;
				break;
			case 0x2:
				data[2] = 0x0;
				break;
			case 0x1:
				data[2] = 0x3;
				break;
			case 0x0:
			default:
				data[2] = 0x2;
				break;
			}
		} else if(pos_set[0] > 0 && pos_set[1] < 0) { /* 90 degree */
			switch(data[2]){
			case 0x3:
				data[2] = 0x2;
				break;
			case 0x2:
				data[2] = 0x1;
				break;
			case 0x1:
				data[2] = 0x0;
				break;
			case 0x0:
			default:
				data[2] = 0x3;
				break;
			}
		}

	} else { /* bottom */

		if(pos_set[0] > 0 && pos_set[1] > 0) { /* 240 degree */
			switch(data[2]){
			case 0x3:
				data[2] = 0x0;
				break;
			case 0x2:
				data[2] = 0x1;
				break;
			case 0x1:
				data[2] = 0x2;
				break;
			case 0x0:
			default:
				data[2] = 0x3;
				break;
			}
		} else if(pos_set[0] < 0 && pos_set[1] > 0) { /* 0 degree */
			switch(data[2]){
			case 0x3:
				data[2] = 0x1;
				break;
			case 0x2:
				data[2] = 0x2;
				break;
			case 0x1:
				data[2] = 0x3;
				break;
			case 0x0:
			default:
				data[2] = 0x0;
				break;
			}
		} else if(pos_set[0] < 0 && pos_set[1] < 0) { /* 90 degree */
			switch(data[2]){
			case 0x3:
				data[2] = 0x2;
				break;
			case 0x2:
				data[2] = 0x3;
				break;
			case 0x1:
				data[2] = 0x0;
				break;
			case 0x0:
			default:
				data[2] = 0x1;
				break;
			}
		} else if(pos_set[0] > 0 && pos_set[1] < 0) { /* 180 degree */
			switch(data[2]){
			case 0x3:
				data[2] = 0x3;
				break;
			case 0x2:
				data[2] = 0x0;
				break;
			case 0x1:
				data[2] = 0x1;
				break;
			case 0x0:
			default:
				data[2] = 0x2;
				break;
			}
		}

		/* x */
		data[0] = -data[0];
	}

	axis_data->x = data[0];
	axis_data->y = data[1];
	axis_data->z = data[2] << 6;

	return 0;
}

static char * mxc6225_id2name(u8 id){
	int index = 0;
	if(id == MXC6225_ID)
		index = 0;
	else if(id == MMA8653_ID)
		index = 1;
	return mxc6225_names[index];
}

static int mxc6225_device_init(struct i2c_client *client)
{
	int result;
	u8 val = 0x00;
	struct mxc6225_data *pdata = i2c_get_clientdata(client);

	result = rda_gsensor_i2c_write(client, 0x04, &val,1);
	if (result < 0)
		goto out;
	#if 0
	val = (u8)pdata->mode;
	result = rda_gsensor_i2c_write(client, MXC6225_XYZ_DATA_CFG, &val, 1);
	if (result < 0)
		goto out;
	#endif
	pdata->active = MMA_STANDBY;
	msleep(MODE_CHANGE_DELAY_MS);
	return 0;

out:
	dev_err(&client->dev, "error when init mxc6225:(%d)", result);
	return result;
}
static int mxc6225_device_stop(struct i2c_client *client)
{
	u8 val;
	(void)rda_gsensor_i2c_read(client, MXC6225_CTRL_REG1, &val, 1);

	val = val & 0xfe;
	rda_gsensor_i2c_write(client, MXC6225_CTRL_REG1, &val, 1);
	return 0;
}

static int mxc6225_read_data(struct i2c_client *client,struct mxc6225_data_axis *data)
{
	int8_t tmp_data[MXC6225_BUF_SIZE];
	int ret;

	ret = rda_gsensor_i2c_read(client, MXC622X_REG_DATA, tmp_data, MXC6225_BUF_SIZE);
	if (ret < 0) {
		dev_err(&client->dev, "i2c block read failed\n");
		return -EIO;
	}
	data->x = tmp_data[0];
	data->y = tmp_data[1];
	data->z = tmp_data[2];
	return 0;
}

static void mxc6225_report_data(struct mxc6225_data* pdata)
{
	struct input_polled_dev * poll_dev = pdata->poll_dev;
	struct mxc6225_data_axis data;
	
	mutex_lock(&pdata->data_lock);
	if(pdata->active == MMA_STANDBY){
		/* if standby, set as 10s to slow down the poll */
		poll_dev->poll_interval = POLL_STOP_TIME;
		goto out;
	}else{
		if(poll_dev->poll_interval == POLL_STOP_TIME)
			poll_dev->poll_interval = POLL_INTERVAL;
	}

	if (mxc6225_read_data(pdata->client, &data) != 0)
		goto out;

	mxc6225_data_convert(pdata, &data);

	input_report_abs(poll_dev->input, ABS_X, data.x);
	input_report_abs(poll_dev->input, ABS_Y, data.y);
	input_report_abs(poll_dev->input, ABS_Z, data.z);
	
	input_sync(poll_dev->input);

#if 0
	printk("######################################################################\n");
	printk("%s: ABS_X = %d, ABS_Y = %d, ABS_Z = %d\n", __func__, data.x, data.y, data.z);
	printk("%s: poll_interval = %d\n", __func__, poll_dev->poll_interval);
	printk("######################################################################\n");
#endif

out:
	mutex_unlock(&pdata->data_lock);
}

static void mxc6225_dev_poll(struct input_polled_dev *dev)
{
	struct mxc6225_data* pdata = (struct mxc6225_data*)dev->private;

	mxc6225_report_data(pdata);
}

static ssize_t mxc6225_enable_show(struct device *dev,
				   		struct device_attribute *attr, char *buf)
{
	struct input_polled_dev *poll_dev = dev_get_drvdata(dev);
	struct mxc6225_data *pdata = (struct mxc6225_data *)(poll_dev->private);
	struct i2c_client *client = pdata->client;
	u8 val;
	int enable;

	mutex_lock(&pdata->data_lock);
	(void)rda_gsensor_i2c_read(client, MXC6225_CTRL_REG1, &val, 1);

	if((val & 0x01) && pdata->active == MMA_ACTIVED)
		enable = 1;
	else
		enable = 0;
	mutex_unlock(&pdata->data_lock);
	return sprintf(buf, "%d\n", enable);
}

static ssize_t mxc6225_enable_store(struct device *dev,
				    struct device_attribute *attr,
				    const char *buf, size_t count)
{
	struct input_polled_dev *poll_dev = dev_get_drvdata(dev);
	struct mxc6225_data *pdata = (struct mxc6225_data *)(poll_dev->private);
	struct i2c_client *client = pdata->client;
	int ret;
	unsigned long enable;
	u8 val = 0;

	enable = simple_strtoul(buf, NULL, 10);
	mutex_lock(&pdata->data_lock);
	enable = (enable > 0) ? 1 : 0;
	if(enable && pdata->active == MMA_STANDBY)
	{
		(void)rda_gsensor_i2c_read(client,MXC6225_CTRL_REG1, &val, 1);
		val = val | 0x01;
		ret = rda_gsensor_i2c_write(client, MXC6225_CTRL_REG1, &val, 1);  
		if(ret > 0){
			pdata->active = MMA_ACTIVED;
			printk("mma enable setting active \n");
		}
	}
	else if(enable == 0  && pdata->active == MMA_ACTIVED)
	{
		(void)rda_gsensor_i2c_read(client,MXC6225_CTRL_REG1, &val, 1);

		val = val & 0xFE;
		ret =  rda_gsensor_i2c_write(client, MXC6225_CTRL_REG1,&val, 1);
		if(ret > 0){
		 pdata->active= MMA_STANDBY;
		 printk("mma enable setting inactive \n");
		}
	}
	mutex_unlock(&pdata->data_lock);

	return count;
}
static ssize_t mxc6225_position_show(struct device *dev,
				   struct device_attribute *attr, char *buf)
{
	struct input_polled_dev *poll_dev = dev_get_drvdata(dev);
	struct mxc6225_data *pdata = (struct mxc6225_data *)(poll_dev->private);
	int position = 0;

	mutex_lock(&pdata->data_lock);
	position = pdata->position ;
	mutex_unlock(&pdata->data_lock);
	return sprintf(buf, "%d\n", position);
}

static ssize_t mxc6225_position_store(struct device *dev,
				    struct device_attribute *attr,
				    const char *buf, size_t count)
{
	struct input_polled_dev *poll_dev = dev_get_drvdata(dev);
	struct mxc6225_data *pdata = (struct mxc6225_data *)(poll_dev->private);
	int  position;
	position = simple_strtoul(buf, NULL, 10);
	mutex_lock(&pdata->data_lock);
	pdata->position = position;
	mutex_unlock(&pdata->data_lock);
	return count;
}

static ssize_t rda_gsensor_gsid_show(struct device *dev,
					struct device_attribute *attr,
					char *buf)
{
	struct input_polled_dev *poll_dev = dev_get_drvdata(dev);
	struct mxc6225_data *pdata = (struct mxc6225_data *)(poll_dev->private);
	char * gsidname = NULL;

	mutex_lock(&pdata->data_lock);
	gsidname = mxc6225_id2name(pdata->chip_id);
	mutex_unlock(&pdata->data_lock);

	return sprintf(buf, gsidname);
}

static DEVICE_ATTR(enable, S_IWUSR | S_IRUGO,
		   mxc6225_enable_show, mxc6225_enable_store);
static DEVICE_ATTR(position, S_IWUSR | S_IRUGO,
		   mxc6225_position_show, mxc6225_position_store);

static DEVICE_ATTR(gsid, S_IWUSR | S_IRUGO,
		   rda_gsensor_gsid_show, NULL);

static struct attribute *mxc6225_attributes[] = {
	&dev_attr_enable.attr,
	&dev_attr_position.attr,
	&dev_attr_gsid.attr,
	NULL
};

static const struct attribute_group mxc6225_attr_group = {
	.attrs = mxc6225_attributes,
};

static int mxc6225_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	int result;
	//u8 chip_id;
	struct input_dev *idev;
	struct mxc6225_data *pdata;
	struct i2c_adapter *adapter;
	struct input_polled_dev *poll_dev;
	u8 data[3];
	u8 ret;

	adapter = to_i2c_adapter(client->dev.parent);
	result = i2c_check_functionality(adapter,I2C_FUNC_I2C);
	if (!result)
		goto err_out;

	data[0] = MXC622X_REG_CTRL;
	data[1] = MXC622X_CTRL_PWRON;
	ret = rda_gsensor_i2c_write(client, data[0], &data[1], 1); 
	printk("%s,%d,ret=%d",__FUNCTION__,__LINE__,ret);

#if 0	
	while (1) {
	 	ret = rda_gsensor_i2c_read(client, MXC622X_REG_DATA, data, 3);
		for (i=0; i<3; i++) {
			printk("%s,%d,buf[%d]=,%d\n",__FUNCTION__,__LINE__,i,data[i]);
		}
		printk("/***********/\n");
		msleep(1000);
	}

	if (chip_id != MXC6225_ID && chip_id != MMA8653_ID  ) {
		dev_err(&client->dev,
			"read chip ID 0x%x is not equal to 0x%x or 0x%x!\n",
			result, MXC6225_ID, MMA8653_ID);
		result = -EINVAL;
		goto err_out;
	}
#endif

	pdata = kzalloc(sizeof(struct mxc6225_data), GFP_KERNEL);
	if(!pdata){
		result = -ENOMEM;
		dev_err(&client->dev, "alloc data memory error!\n");
		goto err_out;
	}
	/* Initialize the mxc6225 chip */
	pdata->client = client;
	//pdata->chip_id = chip_id;
	pdata->mode = MODE_2G;
	pdata->position = _TGT_AP_GSENSOR_POSITION;
	mutex_init(&pdata->data_lock);
	i2c_set_clientdata(client,pdata);
	mxc6225_device_init(client);
	poll_dev = input_allocate_polled_device();
	if (!poll_dev) {
		result = -ENOMEM;
		dev_err(&client->dev, "alloc poll device failed!\n");
		goto err_alloc_poll_device;
	}
	poll_dev->poll = mxc6225_dev_poll;
	poll_dev->poll_interval = POLL_STOP_TIME;
	poll_dev->poll_interval_min = POLL_INTERVAL_MIN;
	poll_dev->poll_interval_max = POLL_INTERVAL_MAX;
	poll_dev->private = pdata;
	idev = poll_dev->input;
	idev->name = RDA_GSENSOR_DRV_NAME;
	idev->uniq = "mxc6225";
	idev->id.bustype = BUS_I2C;
	idev->evbit[0] = BIT_MASK(EV_ABS);
	input_set_abs_params(idev, ABS_X, -0x7fff, 0x7fff, INPUT_FUZZ, INPUT_FLAT);
	input_set_abs_params(idev, ABS_Y, -0x7fff, 0x7fff, INPUT_FUZZ, INPUT_FLAT);
	input_set_abs_params(idev, ABS_Z, -0x7fff, 0x7fff, INPUT_FUZZ, INPUT_FLAT);
	pdata->poll_dev = poll_dev;

	result = input_register_polled_device(pdata->poll_dev);
	if (result) {
		dev_err(&client->dev, "register poll device failed!\n");
		goto err_register_polled_device;
	}

	result = sysfs_create_group(&idev->dev.kobj, &mxc6225_attr_group);
	if (result) {
		dev_err(&client->dev, "create device file failed!\n");
		result = -EINVAL;
		goto err_create_sysfs;
	}
	printk("mxc6225 device driver probe successfully\n");
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

static int mxc6225_remove(struct i2c_client *client)
{
	struct mxc6225_data *pdata = i2c_get_clientdata(client);
	struct input_polled_dev *poll_dev = pdata->poll_dev;
	mxc6225_device_stop(client);
	if(pdata){
		input_unregister_polled_device(poll_dev);
		input_free_polled_device(poll_dev);
		kfree(pdata);
	}
	return 0;
}

#ifdef CONFIG_PM_SLEEP
static int mxc6225_suspend(struct device *dev)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct mxc6225_data *pdata = i2c_get_clientdata(client);
	if(pdata->active == MMA_ACTIVED)
		mxc6225_device_stop(client);
	return 0;
}

static int mxc6225_resume(struct device *dev)
{
	u8 val = 0;
	struct i2c_client *client = to_i2c_client(dev);
	struct mxc6225_data *pdata = i2c_get_clientdata(client);

	if(pdata->active == MMA_ACTIVED){
		(void)rda_gsensor_i2c_read(client,MXC6225_CTRL_REG1, &val, 1);
		val = val | 0x01;
		rda_gsensor_i2c_write(client, MXC6225_CTRL_REG1, &val, 1);  
	}

	return 0;
}
#endif

static const struct i2c_device_id mxc6225_id[] = {
	{RDA_GSENSOR_DRV_NAME, 0},
	{ }
};
MODULE_DEVICE_TABLE(i2c, mxc6225_id);

static SIMPLE_DEV_PM_OPS(mxc6225_pm_ops, mxc6225_suspend, mxc6225_resume);
static struct i2c_driver mxc6225_driver = {
	.driver = {
		   .name = RDA_GSENSOR_DRV_NAME,
		   .owner = THIS_MODULE,
		   .pm = &mxc6225_pm_ops,
		   },
	.probe = mxc6225_probe,
	.remove = mxc6225_remove,
	.id_table = mxc6225_id,
};

static struct rda_gsensor_device_data rda_gsensor_data[] = {
	{
		.irqflags = IRQF_SHARED | IRQF_TRIGGER_RISING,
	},
};

static int __init mxc6225_init(void)
{
	/* register driver */
	int res;

	static struct i2c_board_info i2c_dev_gsensor = {
	        I2C_BOARD_INFO(RDA_GSENSOR_DRV_NAME, 0),
	        .platform_data = rda_gsensor_data,
	};

	struct i2c_adapter *adapter;
	i2c_dev_gsensor.addr =  _DEF_I2C_ADDR_GSENSOR_MXC6225;

	adapter = i2c_get_adapter(_TGT_AP_I2C_BUS_ID_GSENSOR);

	if (!adapter) {
		pr_err("%s, cannot get i2c adapter %d\n",
			__func__, _TGT_AP_I2C_BUS_ID_GSENSOR);
		return -ENODEV;
	}

	i2c_new_device(adapter, &i2c_dev_gsensor);

	res = i2c_add_driver(&mxc6225_driver);
	if (res < 0) {
		printk(KERN_INFO "add mxc6225 i2c driver failed\n");
		return -ENODEV;
	}

	pr_info("rda_gs %s initialized, at i2c bus %d addr 0x%02x\n",
		RDA_GSENSOR_DRV_NAME, _TGT_AP_I2C_BUS_ID_GSENSOR,
		i2c_dev_gsensor.addr);

	return res;
}

static void __exit mxc6225_exit(void)
{
	i2c_del_driver(&mxc6225_driver);
}

MODULE_AUTHOR("Freescale Semiconductor, Inc.");
MODULE_DESCRIPTION("mxc6225 3-Axis Orientation/Motion Detection Sensor driver");
MODULE_LICENSE("GPL");

module_init(mxc6225_init);
module_exit(mxc6225_exit);

