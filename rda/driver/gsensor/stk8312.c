/*
 *  stk8312.c - Linux kernel modules for 3-Axis Orientation/Motion

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

#define POLL_INTERVAL_MIN	200
#define POLL_INTERVAL_MAX	500
#define POLL_INTERVAL		200	/* msecs */
// if sensor is standby ,set POLL_STOP_TIME to slow down the poll
#define POLL_STOP_TIME		200

#define MODE_CHANGE_DELAY_MS	100

#define STK831X_INIT_ODR		1	//0:100Hz, 1:50Hz, 2:25Hz
#define STK831X_SAMPLE_TIME_BASE		2

#define	STK831X_XOUT	0x00	/* x-axis acceleration */
#define	STK831X_YOUT	0x01	/* y-axis acceleration */
#define	STK831X_ZOUT	0x02	/* z-axis acceleration */
#define	STK831X_TILT	0x03	/* Tilt Status */
#define	STK831X_SRST	0x04	/* Sampling Rate Status */
#define	STK831X_SPCNT	0x05	/* Sleep Count */
#define	STK831X_INTSU	0x06	/* Interrupt setup */
#define	STK831X_MODE	0x07
#define	STK831X_SR		0x08	/* Sample rate */
#define	STK831X_PDET	0x09	/* Tap Detection */
#define	STK831X_DEVID	0x0B	/* Device ID */
#define	STK831X_OFSX	0x0C	/* X-Axis offset */
#define	STK831X_OFSY	0x0D	/* Y-Axis offset */
#define	STK831X_OFSZ	0x0E	/* Z-Axis offset */
#define	STK831X_PLAT	0x0F	/* Tap Latency */
#define	STK831X_PWIN	0x10	/* Tap Window */
#define	STK831X_FTH	0x11	/* Free-Fall Threshold */
#define	STK831X_FTM	0x12	/* Free-Fall Time */
#define	STK831X_STH	0x13	/* Shake Threshold */
#define	STK831X_CTRL	0x14	/* Control Register */
#define	STK831X_RESET	0x20	/*software reset */

enum {
	RDA_GSENSOR_STANDBY = 0,
	RDA_GSENSOR_ACTIVED,
};
struct rda_gsensor_data_axis {
	short x;
	short y;
	short z;
};
struct rda_gsensor_data {
	struct i2c_client *client;
	struct input_polled_dev *poll_dev;
	struct mutex data_lock;
	int active;
	int position;
};
static int stk8312_read_data(struct i2c_client *client,
			     struct rda_gsensor_data_axis *data);
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
	if (ret < 0)
		dev_err(&client->dev, "rda_gsensor_i2c_read failed, ret = %d\n", ret);
	return ret;
}

static int stk8312_device_detect(struct i2c_client *client)
{
	u8 val = 0xaa, old_val;
	u8 ret;

	rda_gsensor_i2c_read(client, STK831X_MODE, &ret, 1);
	return 0;

	ret = ret & ~0x01;
	rda_gsensor_i2c_write(client, STK831X_MODE, &ret, 1);
	msleep(MODE_CHANGE_DELAY_MS);

	rda_gsensor_i2c_read(client, STK831X_SR, &old_val, 1);
	rda_gsensor_i2c_write(client, STK831X_SR, &val, 1);
	rda_gsensor_i2c_read(client, STK831X_SR, &ret, 1);
	if (ret == val) {
		rda_gsensor_i2c_write(client, STK831X_SR, &old_val, 1);
		return 0;
	}

	return -1;
}

static int STK831x_ReadByteOTP(struct i2c_client *client, char rReg,
			       char *value)
{
	int redo = 0;
	int result;
	u8 val = 0;

	*value = 0;
	val = rReg;
	result = rda_gsensor_i2c_write(client, 0x3D, &val, 1);
	if (result < 0) {
		dev_err(&client->dev, "%s:failed\n", __func__);
		goto eng_i2c_r_err;
	}
	val = 0x02;
	result = rda_gsensor_i2c_write(client, 0x3F, &val, 1);
	if (result < 0) {
		dev_err(&client->dev, "%s:failed\n", __func__);
		goto eng_i2c_r_err;
	}
	msleep(1);
	do {
		result = rda_gsensor_i2c_read(client, 0x3F, &val, 1);
		if (result < 0) {
			dev_err(&client->dev, "%s:failed\n", __func__);
			goto eng_i2c_r_err;
		}
		if (val & 0x80) {
			break;
		}
		msleep(1);
		redo++;
	} while (redo < 5);

	if (redo == 5) {
		dev_err(&client->dev, 
			"%s:OTP read repeat read 5 times! Failed!\n",
			__func__);
		return -0xF1;
	}
	result = rda_gsensor_i2c_read(client, 0x3E, &val, 1);
	if (result < 0) {
		dev_err(&client->dev, "%s:failed\n", __func__);
		goto eng_i2c_r_err;
	}
	*value = val;

	// printk(KERN_INFO "%s: read 0x%x=0x%x\n", __func__, rReg, *value);

	return 0;

eng_i2c_r_err:
	return result;
}

static int STK831X_SetVD(struct i2c_client *client)
{
	int result;
	char val;
	char reg24;

	msleep(2);
	result = STK831x_ReadByteOTP(client, 0x70, &reg24);
	if (result < 0) {
		dev_err(&client->dev,
			"%s: read back error, result=%d\n", __func__,
			result);
		return result;
	}
	if (reg24 != 0) {
		rda_gsensor_i2c_write(client, 0x24, &reg24, 1);
		if (result < 0) {
			dev_err(&client->dev, "%s:failed\n", __func__);
			return result;
		}
	} else {
		return 0;
	}
	result = rda_gsensor_i2c_read(client, 0x24, &val, 1);
	if (result < 0) {
		dev_err(&client->dev, "%s:failed\n", __func__);
		return result;
	}
	if (val != reg24) {
		dev_err(&client->dev,
			"%s: error, reg24=0x%x, read=0x%x\n", __func__,
			reg24, val);
		return -1;
	}
	return 0;
}

static int stk8312_device_init(struct i2c_client *client)
{
	int result;
	u8 val = 0;
	struct rda_gsensor_data *pdata = i2c_get_clientdata(client);
	val = 0x00;
	result = rda_gsensor_i2c_write(client, STK831X_RESET, &val, 1);
	if (result < 0) {
		dev_err(&client->dev, "%s:failed\n", __func__);
		return result;
	}
	/* int pin is active high, psuh-pull */
	val = 0xC0;
	result = rda_gsensor_i2c_write(client, STK831X_MODE, &val, 1);
	if (result < 0) {
		dev_err(&client->dev, "%s:failed\n", __func__);
		return result;
	}
	val = STK831X_INIT_ODR + STK831X_SAMPLE_TIME_BASE;
	result = rda_gsensor_i2c_write(client, STK831X_SR, &val, 1);
	if (result < 0) {
		dev_err(&client->dev, "%s:failed\n", __func__);
		return result;
	}
	val = 0x42;
	result = rda_gsensor_i2c_write(client, STK831X_STH, &val, 1);
	if (result < 0) {
		dev_err(&client->dev, "%s:failed\n", __func__);
		return result;
	}
	pdata->active = RDA_GSENSOR_STANDBY;
	msleep(MODE_CHANGE_DELAY_MS);
	return 0;
}

static int stk8312_device_stop(struct i2c_client *client)
{
	u8 val;
	rda_gsensor_i2c_read(client, STK831X_MODE, &val, 1);
	val = val & ~0x01;
	rda_gsensor_i2c_write(client, STK831X_MODE, &val, 1);
	return 0;
}

static int stk8312_read_data(struct i2c_client *client,
			     struct rda_gsensor_data_axis *data)
{
	u8 tmp_data[3];
	int ret;
	int acc_xyz[3] = { 0 };
	ret = rda_gsensor_i2c_read(client, STK831X_XOUT, tmp_data, 3);
	if (ret < 0) {
		dev_err(&client->dev, "i2c block read failed\n");
		return ret;
	}
       //printk("stk_read_data  x=%d,y=%d,z=%d",tmp_data[0],tmp_data[1],tmp_data[2]);
	if (tmp_data[0] & 0x80)
		acc_xyz[0] = tmp_data[0] - 256;
	else
		acc_xyz[0] = tmp_data[0];
	if (tmp_data[1] & 0x80)
		acc_xyz[1] = tmp_data[1] - 256;
	else
		acc_xyz[1] = tmp_data[1];
	if (tmp_data[2] & 0x80)
		acc_xyz[2] = tmp_data[2] - 256;
	else
		acc_xyz[2] = tmp_data[2];

	data->x = acc_xyz[0];

	data->y = acc_xyz[1];

	data->z = acc_xyz[2];
	//printk("stk_read_data1  x=%d,y=%d,z=%d",data->x,data->y,data->z);
	return 0;
}

static void stk8312_report_data(struct rda_gsensor_data *pdata)
{
	struct input_polled_dev *poll_dev = pdata->poll_dev;
	struct rda_gsensor_data_axis data;
	mutex_lock(&pdata->data_lock);
	if (pdata->active == RDA_GSENSOR_STANDBY) {
		poll_dev->poll_interval = POLL_STOP_TIME;
		goto out;
	} else {
		if (poll_dev->poll_interval == POLL_STOP_TIME)
			poll_dev->poll_interval = POLL_INTERVAL;
	}

	if (stk8312_read_data(pdata->client, &data) != 0)
		goto out;
	input_report_abs(poll_dev->input, ABS_X, -data.y);
	input_report_abs(poll_dev->input, ABS_Y, -data.x);
	input_report_abs(poll_dev->input, ABS_Z, -data.z);
	input_sync(poll_dev->input);
out:
	mutex_unlock(&pdata->data_lock);
}

static void rda_gsensor_dev_poll(struct input_polled_dev *dev)
{
	struct rda_gsensor_data *pdata =
	    (struct rda_gsensor_data *)dev->private;
	stk8312_report_data(pdata);
}

static ssize_t rda_gsensor_enable_show(struct device *dev,
				       struct device_attribute *attr, char *buf)
{
	struct input_polled_dev *poll_dev = dev_get_drvdata(dev);
	struct rda_gsensor_data *pdata =
	    (struct rda_gsensor_data *)(poll_dev->private);
	struct i2c_client *client = pdata->client;
	u8 val;
	int enable;
	mutex_lock(&pdata->data_lock);
	rda_gsensor_i2c_read(client, STK831X_MODE, &val, 1);
	if ((val & 0x01) && pdata->active == RDA_GSENSOR_ACTIVED)
	{
		enable = 1;
	} else
		enable = 0;
	mutex_unlock(&pdata->data_lock);
	return sprintf(buf, "%d\n", enable);
}

static ssize_t rda_gsensor_enable_store(struct device *dev,
					struct device_attribute *attr,
					const char *buf, size_t count)
{
	struct input_polled_dev *poll_dev = dev_get_drvdata(dev);
	struct rda_gsensor_data *pdata =
	    (struct rda_gsensor_data *)(poll_dev->private);
	struct i2c_client *client = pdata->client;
	int ret;
	unsigned long enable;
	u8 val = 0;
	enable = simple_strtoul(buf, NULL, 10);

	mutex_lock(&pdata->data_lock);
	enable = (enable > 0) ? 1 : 0;
	if (enable && pdata->active == RDA_GSENSOR_STANDBY) {
		rda_gsensor_i2c_read(client, STK831X_MODE, &val, 1);
		val = val | 0x01;
		ret = rda_gsensor_i2c_write(client, STK831X_MODE, &val, 1);
		STK831X_SetVD(client);
		if (ret > 0) {
			pdata->active = RDA_GSENSOR_ACTIVED;
		}
	} else if (enable == 0 && pdata->active == RDA_GSENSOR_ACTIVED) {
		rda_gsensor_i2c_read(client, STK831X_MODE, &val, 1);
		val = val & 0xFE;
		ret = rda_gsensor_i2c_write(client, STK831X_MODE, &val, 1);
		if (ret > 0) {
			pdata->active = RDA_GSENSOR_STANDBY;
		}
	}

	mutex_unlock(&pdata->data_lock);
	return count;
}

static ssize_t rda_gsensor_position_show(struct device *dev,
					 struct device_attribute *attr,
					 char *buf)
{
	struct input_polled_dev *poll_dev = dev_get_drvdata(dev);
	struct rda_gsensor_data *pdata =
	    (struct rda_gsensor_data *)(poll_dev->private);
	int position = 0;
	mutex_lock(&pdata->data_lock);
	position = pdata->position;
	mutex_unlock(&pdata->data_lock);
	return sprintf(buf, "%d\n", position);
}

static ssize_t rda_gsensor_position_store(struct device *dev,
					  struct device_attribute *attr,
					  const char *buf, size_t count)
{
	struct input_polled_dev *poll_dev = dev_get_drvdata(dev);
	struct rda_gsensor_data *pdata =
	    (struct rda_gsensor_data *)(poll_dev->private);
	int position;
	position = simple_strtoul(buf, NULL, 10);
	mutex_lock(&pdata->data_lock);
	pdata->position = position;
	mutex_unlock(&pdata->data_lock);
	return count;
}

static DEVICE_ATTR(enable, S_IWUSR | S_IRUGO,
		   rda_gsensor_enable_show, rda_gsensor_enable_store);
static DEVICE_ATTR(position, S_IWUSR | S_IRUGO,
		   rda_gsensor_position_show, rda_gsensor_position_store);

static struct attribute *stk8312_attributes[] = {
	&dev_attr_enable.attr,
	&dev_attr_position.attr,
	NULL
};

static const struct attribute_group stk8312_attr_group = {
	.attrs = stk8312_attributes,
};

static int __devinit stk8312_probe(struct i2c_client *client,
				   const struct i2c_device_id *id)
{
	int result;
	struct input_dev *idev;
	struct rda_gsensor_data *pdata;
	struct input_polled_dev *poll_dev;

	/*check connection */
	result = stk8312_device_detect(client);
	// printk("stk_device_detect result =%d\n",result);
	if (result) {
		result = -ENOMEM;
		dev_err(&client->dev, "detect the sensor connection error!\n");
		goto err_out;
	}

	pdata = kzalloc(sizeof(struct rda_gsensor_data), GFP_KERNEL);
	if (!pdata) {
		result = -ENOMEM;
		dev_err(&client->dev, "alloc data memory error!\n");
		goto err_out;
	}
	/* Initialize the STK chip */
	pdata->client = client;
	pdata->position = _TGT_AP_GSENSOR_POSITION;
	mutex_init(&pdata->data_lock);
	i2c_set_clientdata(client, pdata);
	stk8312_device_init(client);
	poll_dev = input_allocate_polled_device();
	if (!poll_dev) {
		result = -ENOMEM;
		dev_err(&client->dev, "alloc poll device failed!\n");
		goto err_alloc_poll_device;
	}
	poll_dev->poll = rda_gsensor_dev_poll;
	poll_dev->poll_interval = POLL_STOP_TIME;
	poll_dev->poll_interval_min = POLL_INTERVAL_MIN;
	poll_dev->poll_interval_max = POLL_INTERVAL_MAX;
	poll_dev->private = pdata;
	idev = poll_dev->input;
	idev->name = RDA_GSENSOR_DRV_NAME;
	idev->uniq = "stk8312";
	idev->id.bustype = BUS_I2C;
	idev->evbit[0] = BIT_MASK(EV_ABS);
	input_set_abs_params(idev, ABS_X, -128, 127, 0, 0);
	input_set_abs_params(idev, ABS_Y, -128, 127, 0, 0);
	input_set_abs_params(idev, ABS_Z, -128, 127, 0, 0);
	pdata->poll_dev = poll_dev;
	result = input_register_polled_device(pdata->poll_dev);
	if (result) {
		dev_err(&client->dev, "register poll device failed!\n");
		goto err_register_polled_device;
	}
	result = sysfs_create_group(&idev->dev.kobj, &stk8312_attr_group);
	if (result) {
		dev_err(&client->dev, "create device file failed!\n");
		result = -EINVAL;
		goto err_create_sysfs;
	}

	kobject_uevent(&idev->dev.kobj, KOBJ_CHANGE);
	dev_info(&client->dev, "STK device driver probe successfully\n");
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

static int __devexit stk8312_remove(struct i2c_client *client)
{
	struct rda_gsensor_data *pdata = i2c_get_clientdata(client);
	struct input_polled_dev *poll_dev = pdata->poll_dev;
	stk8312_device_stop(client);
	if (pdata) {
		input_unregister_polled_device(poll_dev);
		input_free_polled_device(poll_dev);
		kfree(pdata);
	}
	return 0;
}

#ifdef CONFIG_PM_SLEEP
static int stk8312_suspend(struct device *dev)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct rda_gsensor_data *pdata = i2c_get_clientdata(client);
	if (pdata->active == RDA_GSENSOR_ACTIVED)
		stk8312_device_stop(client);
	return 0;
}

static int stk8312_resume(struct device *dev)
{
	u8 val = 0;
	struct i2c_client *client = to_i2c_client(dev);
	struct rda_gsensor_data *pdata = i2c_get_clientdata(client);

	if (pdata->active == RDA_GSENSOR_ACTIVED) {
		rda_gsensor_i2c_read(client, STK831X_MODE, &val, 1);
		val = val | (u8) 0x01;
		rda_gsensor_i2c_write(client, STK831X_MODE, &val, 1);
	}
	return 0;

}
#endif

static const struct i2c_device_id rda_gsensor_id[] = {
	{"stk8312", 0},
	{}
};

MODULE_DEVICE_TABLE(i2c, rda_gsensor_id);

static SIMPLE_DEV_PM_OPS(stk8312_pm_ops, stk8312_suspend, stk8312_resume);
static struct i2c_driver stk8312_driver = {
	.driver = {
		   .name = "stk8312",
		   .owner = THIS_MODULE,
		   .pm = &stk8312_pm_ops,
		   },
	.probe = stk8312_probe,
	.remove = __devexit_p(stk8312_remove),
	.id_table = rda_gsensor_id,
};

static struct rda_gsensor_device_data rda_gsensor_data[] = {
        {
         .irqflags = IRQF_SHARED | IRQF_TRIGGER_RISING,
         },
};

static int __init stk8312_init(void)
{
	/* register driver */
	int res;
	static struct i2c_board_info i2c_dev_gsensor = {
	        I2C_BOARD_INFO("stk8312", 0),
	        .platform_data = rda_gsensor_data,
	};

	struct i2c_adapter *adapter;
	i2c_dev_gsensor.addr =  _DEF_I2C_ADDR_GSENSOR_STK8312;

	adapter = i2c_get_adapter(_TGT_AP_I2C_BUS_ID_GSENSOR);

	if (!adapter) {
		pr_err("%s, cannot get i2c adapter %d\n",
			__func__, _TGT_AP_I2C_BUS_ID_GSENSOR);
		return -ENODEV;
	}

	i2c_new_device(adapter, &i2c_dev_gsensor);

	res = i2c_add_driver(&stk8312_driver);
	if (res < 0) {
		printk(KERN_ERR "%s:failed\n", __func__);
		return -ENODEV;
	}

	pr_info("rda_gs %s initialized, at i2c bus %d addr 0x%02x\n",
		"stk8312", _TGT_AP_I2C_BUS_ID_GSENSOR, 
		i2c_dev_gsensor.addr);

	return res;
}

static void __exit stk8312_exit(void)
{
	i2c_del_driver(&stk8312_driver);
}

MODULE_AUTHOR("RDA Inc.");
MODULE_DESCRIPTION("STK Motion Detection Sensor driver");
MODULE_LICENSE("GPL");

module_init(stk8312_init);
module_exit(stk8312_exit);
