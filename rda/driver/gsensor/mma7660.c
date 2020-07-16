/*
 *  mma8451.c - Linux kernel modules for 3-Axis Orientation/Motion
 *  Detection Sensor
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
#ifdef CONFIG_HAS_EARLYSUSPEND
#include <linux/earlysuspend.h>
#endif /* CONFIG_HAS_EARLYSUSPEND */
#include <plat/devices.h>
#include "tgt_ap_board_config.h"

#define MMA7660_I2C_ADDR	0x4c

#define POLL_INTERVAL_MIN	200
#define POLL_INTERVAL_MAX	500
#define POLL_INTERVAL		200	/* msecs */
// if sensor is standby ,set POLL_STOP_TIME to slow down the poll
#define POLL_STOP_TIME		10000

#define INPUT_FUZZ		32
#define INPUT_FLAT		32
#define MODE_CHANGE_DELAY_MS	100

#define MMA7660_STATUS_ZYXDR	0x08
#define MMA7660_BUF_SIZE	3

/*MMA7660 register definition*/

#define MMA7660_XOUT			0x00
#define MMA7660_YOUT			0x01
#define MMA7660_ZOUT			0x02
#define MMA7660_TILT			0x03
#define MMA7660_SRST			0x04
#define MMA7660_SPCNT			0x05
#define MMA7660_INTSU			0x06
#define MMA7660_MODE			0x07
#define MMA7660_SR				0x08
#define MMA7660_PDET			0x09
#define MMA7660_PD				0x0A

enum {
	MMA_STANDBY = 0,
	MMA_ACTIVED,
};
struct mma7660_data_axis {
	short x;
	short y;
	short z;
};
struct mma7660_data {
	struct i2c_client *client;
	struct input_polled_dev *poll_dev;
	struct mutex data_lock;
	int active;
	int position;
	int suspend;
#ifdef CONFIG_HAS_EARLYSUSPEND
	struct early_suspend early_suspend;
#endif
};
static int mma7660_position_setting[8][3][3] = {
	{{-1, 0, 0}, {0, -1, 0}, {0, 0, 1}},
	{{0, 1, 0}, {-1, 0, 0}, {0, 0, 1}},
	{{1, 0, 0}, {0, 1, 0}, {0, 0, 1}},
	{{0, -1, 0}, {1, 0, 0}, {0, 0, 1}},

	{{-1, 0, 0}, {0, 1, 0}, {0, 0, -1}},
	{{0, 1, 0}, {1, 0, 0}, {0, 0, -1}},
	{{1, 0, 0}, {0, -1, 0}, {0, 0, -1}},
	{{0, -1, 0}, {-1, 0, 0}, {0, 0, -1}},

};

static int mma7660_read_data(struct i2c_client *client, struct mma7660_data_axis *data);

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
	ret = i2c_master_send(client, (char const *)tmp_buf, bytelen);
	if(ret < 0)
		dev_err(&client->dev, "rda_gsensor_i2c_write failed, ret = %d\n", ret);

	return ret;
}

static int rda_gsensor_i2c_read(struct i2c_client *client, u8 addr, u8 * pdata,
				unsigned int datalen)
{
	int ret = 0;

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

	ret = i2c_transfer(client->adapter, msgs, 2);
	if(ret < 0)
                dev_err(&client->dev, "rda_gsensor_i2c_read failed, ret = %d\n", ret);
	return ret;
}

static int mma7660_data_convert(struct mma7660_data *pdata,
				struct mma7660_data_axis *axis_data)
{
	short rawdata[3], data[3];
	int i, j;
	int position = pdata->position;
	if (position < 0 || position > 7)
		position = 0;
	rawdata[0] = axis_data->x;
	rawdata[1] = axis_data->y;
	rawdata[2] = axis_data->z;
	for (i = 0; i < 3; i++) {
		data[i] = 0;
		for (j = 0; j < 3; j++)
			data[i] +=
			    rawdata[j] *
			    mma7660_position_setting[position][i][j];
	}
	axis_data->x = data[0];
	axis_data->y = data[1];
	axis_data->z = data[2];
	return 0;
}

static int mma7660_device_detect(struct i2c_client *client)
{
	u8 val = 0xaa, old_val;
	u8 ret;

	rda_gsensor_i2c_read(client, MMA7660_MODE, &ret, 1);
	return 0;

	ret = ret & ~0x01;
	rda_gsensor_i2c_write(client, MMA7660_MODE, &ret, 1);
	msleep(MODE_CHANGE_DELAY_MS);

	rda_gsensor_i2c_read(client, MMA7660_SR, &old_val, 1);
	rda_gsensor_i2c_write(client, MMA7660_SR, &val, 1);
	rda_gsensor_i2c_read(client, MMA7660_SR, &ret, 1);
	if (ret == val) {
		rda_gsensor_i2c_write(client, MMA7660_SR, &old_val, 1);
		return 0;
	}

	return -1;
}

static int mma7660_device_init(struct i2c_client *client)
{
	int result;
	u8 val = 0;
	struct mma7660_data *pdata = i2c_get_clientdata(client);

	result = rda_gsensor_i2c_write(client, MMA7660_MODE, &val, 1);
	if (result < 0)
		goto out;

	pdata->active = MMA_STANDBY;
	msleep(MODE_CHANGE_DELAY_MS);
	return 0;
out:
	dev_err(&client->dev, "error when init mma7660:(%d)", result);
	return result;
}

static int mma7660_device_stop(struct i2c_client *client)
{
	u8 val;

	rda_gsensor_i2c_read(client, MMA7660_MODE, &val, 1);
	val = val & ~0x01;
	rda_gsensor_i2c_write(client, MMA7660_MODE, &val, 1);
	return 0;
}

static int mma7660_read_data(struct i2c_client *client,
			     struct mma7660_data_axis *data)
{
	u8 tmp_data[MMA7660_BUF_SIZE];
	int ret, read_num = 0;

	while (1) {
		ret = rda_gsensor_i2c_read(client, MMA7660_XOUT, tmp_data,
					 MMA7660_BUF_SIZE);
		if (ret < 0) {
			dev_err(&client->dev, "i2c block read failed\n");
			return ret;
		}
		if (((tmp_data[0] & 0x40) == 0x40)
		    || ((tmp_data[1] & 0x40) == 0x40)
		    || ((tmp_data[2] & 0x40) == 0x40)) {
			mdelay(1);
			read_num++;
			if (read_num > 5)
				break;
		} else
			break;
	}

	data->x = ((short)((tmp_data[0] << 10)) >> 2) * 3;
	data->y = ((short)((tmp_data[1] << 10)) >> 2) * 3;
	data->z = ((short)((tmp_data[2] << 10)) >> 2) * 3;
	return 0;
}

static void mma7660_report_data(struct mma7660_data *pdata)
{
	struct input_polled_dev *poll_dev = pdata->poll_dev;
	struct mma7660_data_axis data;
	mutex_lock(&pdata->data_lock);
	if (pdata->active == MMA_STANDBY) {
		poll_dev->poll_interval = POLL_STOP_TIME;
		goto out;
	} else {
		if (poll_dev->poll_interval == POLL_STOP_TIME)
			poll_dev->poll_interval = POLL_INTERVAL;
	}

	if (pdata->suspend == 1) {
		goto out;
	}

	if (mma7660_read_data(pdata->client, &data) != 0)
		goto out;

	mma7660_data_convert(pdata, &data);
	input_report_abs(poll_dev->input, ABS_X, data.x);
	input_report_abs(poll_dev->input, ABS_Y, data.y);
	input_report_abs(poll_dev->input, ABS_Z, data.z);
	input_sync(poll_dev->input);
out:
	mutex_unlock(&pdata->data_lock);
}

static void mma7660_dev_open(struct input_polled_dev *dev)
{
	struct mma7660_data* pdata = (struct mma7660_data*) dev->private;

	mma7660_device_init(pdata->client);
}

static void mma7660_dev_close(struct input_polled_dev *dev)
{
	struct mma7660_data* pdata = (struct mma7660_data*) dev->private;

	mma7660_device_stop(pdata->client);
}

static void mma7660_dev_poll(struct input_polled_dev *dev)
{
	struct mma7660_data *pdata = (struct mma7660_data *)dev->private;

	mma7660_report_data(pdata);
}

static ssize_t mma7660_enable_show(struct device *dev,
				   struct device_attribute *attr, char *buf)
{
	struct input_polled_dev *poll_dev = dev_get_drvdata(dev);
	struct mma7660_data *pdata = (struct mma7660_data *)(poll_dev->private);
	struct i2c_client *client = pdata->client;
	u8 val;
	int enable;

	mutex_lock(&pdata->data_lock);

	rda_gsensor_i2c_read(client, MMA7660_MODE, &val, 1);
	if ((val & 0x01) && pdata->active == MMA_ACTIVED)
		enable = 1;
	else
		enable = 0;
	mutex_unlock(&pdata->data_lock);
	return sprintf(buf, "%d\n", enable);
}

static ssize_t mma7660_enable_store(struct device *dev,
				    struct device_attribute *attr,
				    const char *buf, size_t count)
{
	struct input_polled_dev *poll_dev = dev_get_drvdata(dev);
	struct mma7660_data *pdata = (struct mma7660_data *)(poll_dev->private);
	struct i2c_client *client = pdata->client;
	int ret;
	unsigned long enable;
	u8 val = 0;
	enable = simple_strtoul(buf, NULL, 10);

	mutex_lock(&pdata->data_lock);
	enable = (enable > 0) ? 1 : 0;
	if (enable && pdata->active == MMA_STANDBY) {
		ret = rda_gsensor_i2c_read(client, MMA7660_MODE, &val, 1);
		if (ret < 0) {
			mutex_unlock(&pdata->data_lock);
			return ret;
		}
		val = val | 0x01;

		ret = rda_gsensor_i2c_write(client, MMA7660_MODE, &val, 1);
		if (ret > 0) {
			pdata->active = MMA_ACTIVED;
		}
	} else if (enable == 0 && pdata->active == MMA_ACTIVED) {
		ret = rda_gsensor_i2c_read(client, MMA7660_MODE, &val, 1);
		if (ret < 0) {
			mutex_unlock(&pdata->data_lock);
			return ret;
		}
		val = val & 0xFE;
		ret = rda_gsensor_i2c_write(client, MMA7660_MODE, &val, 1);
		if (ret > 0) {
			pdata->active = MMA_STANDBY;
		}
	}

	mutex_unlock(&pdata->data_lock);
	return count;
}

static ssize_t mma7660_position_show(struct device *dev,
				     struct device_attribute *attr, char *buf)
{
	struct input_polled_dev *poll_dev = dev_get_drvdata(dev);
	struct mma7660_data *pdata = (struct mma7660_data *)(poll_dev->private);
	int position = 0;
	mutex_lock(&pdata->data_lock);
	position = pdata->position;
	mutex_unlock(&pdata->data_lock);
	return sprintf(buf, "%d\n", position);
}

static ssize_t mma7660_position_store(struct device *dev,
				      struct device_attribute *attr,
				      const char *buf, size_t count)
{
	struct input_polled_dev *poll_dev = dev_get_drvdata(dev);
	struct mma7660_data *pdata = (struct mma7660_data *)(poll_dev->private);
	int position;
	position = simple_strtoul(buf, NULL, 10);
	mutex_lock(&pdata->data_lock);
	pdata->position = position;
	mutex_unlock(&pdata->data_lock);
	return count;
}

static DEVICE_ATTR(enable, S_IWUSR | S_IWGRP | S_IRUGO,
		   mma7660_enable_show, mma7660_enable_store);
static DEVICE_ATTR(position, S_IWUSR | S_IWGRP | S_IRUGO,
		   mma7660_position_show, mma7660_position_store);

static struct attribute *mma7660_attributes[] = {
	&dev_attr_enable.attr,
	&dev_attr_position.attr,
	NULL
};

static const struct attribute_group mma7660_attr_group = {
	.attrs = mma7660_attributes,
};

#ifdef CONFIG_HAS_EARLYSUSPEND
static void mma7660_early_suspend(struct early_suspend *h);
static void mma7660_late_resume(struct early_suspend *h);
#endif
static int mma7660_probe(struct i2c_client *client,
				   const struct i2c_device_id *id)
{
	int result;
	struct input_dev *idev;
	struct mma7660_data *pdata;
	struct input_polled_dev *poll_dev;

	/*check connection */

	result = mma7660_device_detect(client);
	if (result) {
		result = -ENOMEM;
		dev_err(&client->dev, "detect the sensor connection error!\n");
		goto err_out;
	}
	pdata = kzalloc(sizeof(struct mma7660_data), GFP_KERNEL);
	if (!pdata) {
		result = -ENOMEM;
		dev_err(&client->dev, "alloc data memory error!\n");
		goto err_out;
	}
	/* Initialize the MMA7660 chip */
	pdata->client = client;
	pdata->position = _TGT_AP_GSENSOR_POSITION;
	mutex_init(&pdata->data_lock);
	i2c_set_clientdata(client, pdata);
	mma7660_device_init(client);
	poll_dev = input_allocate_polled_device();
	if (!poll_dev) {
		result = -ENOMEM;
		dev_err(&client->dev, "alloc poll device failed!\n");
		goto err_alloc_poll_device;
	}
	poll_dev->poll = mma7660_dev_poll;
	poll_dev->poll_interval = POLL_INTERVAL;
	poll_dev->poll_interval_min = POLL_INTERVAL_MIN;
	poll_dev->poll_interval_max = POLL_INTERVAL_MAX;
	poll_dev->private = pdata;
	poll_dev->open = mma7660_dev_open;
	poll_dev->close = mma7660_dev_close;

	idev = poll_dev->input;
	idev->name = RDA_GSENSOR_DRV_NAME;
	idev->uniq = "mma7660";
	idev->id.bustype = BUS_I2C;
	set_bit(EV_ABS, idev->evbit);
	input_set_abs_params(idev, ABS_X, -0x7fff, 0x7fff, 0, 0);
	input_set_abs_params(idev, ABS_Y, -0x7fff, 0x7fff, 0, 0);
	input_set_abs_params(idev, ABS_Z, -0x7fff, 0x7fff, 0, 0);
	pdata->poll_dev = poll_dev;

#ifdef CONFIG_HAS_EARLYSUSPEND
	pdata->early_suspend.level = EARLY_SUSPEND_LEVEL_BLANK_SCREEN + 2;
	pdata->early_suspend.suspend = mma7660_early_suspend;
	pdata->early_suspend.resume = mma7660_late_resume;
	register_early_suspend(&pdata->early_suspend);
#endif /* CONFIG_HAS_EARLYSUSPEND */

	result = input_register_polled_device(pdata->poll_dev);
	if (result) {
		dev_err(&client->dev, "register poll device failed!\n");
		goto err_register_polled_device;
	}
	result = sysfs_create_group(&idev->dev.kobj, &mma7660_attr_group);
	if (result) {
		dev_err(&client->dev, "create device file failed!\n");
		result = -EINVAL;
		goto err_create_sysfs;
	}
	kobject_uevent(&idev->dev.kobj, KOBJ_CHANGE);
	printk("mma7660 device driver probe successfully\n");
	return 0;
err_create_sysfs:
	input_unregister_polled_device(pdata->poll_dev);
err_register_polled_device:
#ifdef CONFIG_HAS_EARLYSUSPEND
	unregister_early_suspend(&pdata->early_suspend);
#endif /* CONFIG_HAS_EARLYSUSPEND */
	input_free_polled_device(poll_dev);
err_alloc_poll_device:
	kfree(pdata);
err_out:
	return result;
}

static int mma7660_remove(struct i2c_client *client)
{
	struct mma7660_data *pdata = i2c_get_clientdata(client);
	struct input_polled_dev *poll_dev = pdata->poll_dev;

#ifdef CONFIG_HAS_EARLYSUSPEND
	unregister_early_suspend(&pdata->early_suspend);
#endif /* CONFIG_HAS_EARLYSUSPEND */
	if (pdata) {
		input_unregister_polled_device(poll_dev);
		input_free_polled_device(poll_dev);
		kfree(pdata);
	}
	return 0;
}

#ifdef CONFIG_HAS_EARLYSUSPEND
static void mma7660_early_suspend(struct early_suspend *h)
{
	struct mma7660_data *pdata = container_of(h, struct mma7660_data, early_suspend);

	mma7660_device_stop(pdata->client);
	pdata->suspend = 1;
}

static void mma7660_late_resume(struct early_suspend *h)
{
	struct mma7660_data *pdata = container_of(h, struct mma7660_data, early_suspend);
	int val = 0;

	val = i2c_smbus_read_byte_data(pdata->client, MMA7660_MODE);
	i2c_smbus_write_byte_data(pdata->client, MMA7660_MODE, val |0x01);
	pdata->suspend = 0;
}
#else
#ifdef CONFIG_PM_SLEEP
static int mma7660_suspend(struct device *dev)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct mma7660_data *pdata = i2c_get_clientdata(client);
	if (pdata->active == MMA_ACTIVED)
		mma7660_device_stop(client);
	return 0;
}

static int mma7660_resume(struct device *dev)
{
	u8 val = 0;
	struct i2c_client *client = to_i2c_client(dev);
	struct mma7660_data *pdata = i2c_get_clientdata(client);

	if (pdata->active == MMA_ACTIVED) {
		rda_gsensor_i2c_read(client, MMA7660_MODE, &val, 1);
		val = val | (u8) 0x01;
		rda_gsensor_i2c_write(client, MMA7660_MODE, &val, 1);
	}
	return 0;

}

static SIMPLE_DEV_PM_OPS(mma7660_pm_ops, mma7660_suspend, mma7660_resume);
#endif /* CONFIG_PM_SLEEP */
#endif  /* CONFIG_HAS_EARLYSUSPEND */

static const struct i2c_device_id mma7660_id[] = {
	{"mma7660", 0},
	{}
};

MODULE_DEVICE_TABLE(i2c, mma7660_id);

static struct i2c_driver mma7660_driver = {
	.driver = {
		.name = "mma7660",
		.owner = THIS_MODULE,
#ifndef CONFIG_HAS_EARLYSUSPEND
# ifdef CONFIG_PM_SLEEP
		.pm = &mma7660_pm_ops,
# endif /* CONFIG_PM_SLEEP */
#endif /* CONFIG_HAS_EARLYSUSPEND */
		   },
	.probe = mma7660_probe,
	.remove = mma7660_remove,
	.id_table = mma7660_id,
};

static struct rda_gsensor_device_data rda_gsensor_data[] = {
        {
         .irqflags = IRQF_SHARED | IRQF_TRIGGER_RISING,
         },
};

static int __init mma7660_init(void)
{
	/* register driver */
	int res;

	static struct i2c_board_info i2c_dev_gsensor = {
	        I2C_BOARD_INFO("mma7660", 0),
	        .platform_data = rda_gsensor_data,
	};

	struct i2c_adapter *adapter;
	i2c_dev_gsensor.addr =  _DEF_I2C_ADDR_GSENSOR_MMA7760;

	adapter = i2c_get_adapter(_TGT_AP_I2C_BUS_ID_GSENSOR);

	if (!adapter) {
		pr_err("%s, cannot get i2c adapter %d\n",
			__func__, _TGT_AP_I2C_BUS_ID_GSENSOR);
		return -ENODEV;
	}

	i2c_new_device(adapter, &i2c_dev_gsensor);

	res = i2c_add_driver(&mma7660_driver);
	if (res < 0) {
		printk(KERN_INFO "add mma7660 i2c driver failed\n");
		return -ENODEV;
	}

	pr_info("rda_gs %s initialized, at i2c bus %d addr 0x%02x\n",
		"mma7660", _TGT_AP_I2C_BUS_ID_GSENSOR, 
		i2c_dev_gsensor.addr);

	return res;
}

static void __exit mma7660_exit(void)
{
	i2c_del_driver(&mma7660_driver);
}

module_init(mma7660_init);
module_exit(mma7660_exit);

MODULE_AUTHOR("Freescale Semiconductor, Inc.");
MODULE_DESCRIPTION("MMA7660 3-Axis Orientation/Motion Detection Sensor driver");
MODULE_LICENSE("GPL");
