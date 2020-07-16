/*
* Copyright (C) 2013 MEMSIC, Inc.
*
* Initial Code:
*	Robbie Cao
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation; either version 2 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
*
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
#include "mxc6255.h"


#define	FUZZ			    0
#define	FLAT			    0

//#define MXC6255_DEBUG

#define POLL_INTERVAL_MIN	200
#define POLL_INTERVAL_MAX	500

// if sensor is standby ,set POLL_STOP_TIME to slow down the poll
#define POLL_STOP_TIME		200

struct mxc6255_data {
	struct i2c_client *client;
	struct mutex lock;
	struct delayed_work input_work;
	struct input_polled_dev *poll_dev;
	int hw_initialized;
	int hw_working; // hw_working=-1 means not tested yet;
	atomic_t enabled;
	int active;
	int stop_by_user;
	int position;
#ifdef CONFIG_HAS_EARLYSUSPEND
	int early_suspended;
	struct early_suspend early_suspend;
#endif /* CONFIG_HAS_EARLYSUSPEND */
};

#ifdef CONFIG_HAS_EARLYSUSPEND
static void mxc6255_early_suspend(struct early_suspend *h);
static void mxc6255_late_resume(struct early_suspend *h);
#endif

/*
* Because misc devices can not carry a pointer from driver register to
* open, we keep this global.  This limits the driver to a single instance.
*/
static int mxc6255_acc_i2c_read(struct mxc6255_data *acc, u8 *buf, int len)
{
	int err   = -1;

	struct i2c_msg msgs[] = {
		{
			.addr  = acc->client->addr,
			.flags = acc->client->flags & I2C_M_TEN,
			.len   = 1,
			.buf   = buf,
		},
		{
			.addr  = acc->client->addr,
			.flags = (acc->client->flags & I2C_M_TEN) | I2C_M_RD,
			.len   = len,
			.buf   = buf,
		},
	};

	err = i2c_transfer(acc->client->adapter, msgs, 2);

	if (err != 2) {
		dev_err(&acc->client->dev, "read transfer error\n");
		err = -EIO;
	}
	else {
		err = 0;
	}

	return err;
}

static int mxc6255_acc_i2c_write(struct mxc6255_data *acc, u8 *buf, int len)
{
	int err   = -1;

	struct i2c_msg msgs[] = {
		{
			.addr  = acc->client->addr,
			.flags = acc->client->flags & I2C_M_TEN,
			.len   = len + 1,
			.buf   = buf,
		},
	};

	err = i2c_transfer(acc->client->adapter, msgs, 1);

	if (err != 1) {
		dev_err(&acc->client->dev, "write transfer error\n");
		err = -EIO;
	}
	else	{
		err = 0;
	}

	return err;
}

static int mxc6255_acc_hw_init(struct mxc6255_data *acc)
{
	int err   = -1;
	u8 buf[7] = {0};

	printk(KERN_INFO "%s: hw init start\n", MXC6255_ACC_DEV_NAME);

	buf[0] = WHO_AM_I;

	err    = mxc6255_acc_i2c_read(acc, buf, 1);
	if (err < 0){
		goto error_firstread;
	}
	else {
		acc->hw_working = 1;
	}

	if ((buf[0] & 0x3F) != WHOAMI_MXC6255_ACC){
		err = -1; /* choose the right coded error */
		goto error_unknown_device;
	}

	acc->hw_initialized = 1;
	printk(KERN_INFO "%s: hw init done\n", MXC6255_ACC_DEV_NAME);

	return 0;

error_firstread:
	acc->hw_working = 0;
	dev_warn(&acc->client->dev, "Error reading WHO_AM_I: is device "
			"available/working?\n");
	goto error1;

error_unknown_device:
	dev_err(&acc->client->dev,
			"device unknown. Expected: 0x%x,"
			" Replies: 0x%x\n", WHOAMI_MXC6255_ACC, buf[0]);

error1:
	acc->hw_initialized = 0;
	dev_err(&acc->client->dev, "hw init error 0x%x,0x%x: %d\n", buf[0],
			buf[1], err);

	return err;
}

static void mxc6255_acc_device_power_off(struct mxc6255_data *acc)
{
	int err;
	u8 buf[2] = {
		MXC6255_REG_CTRL,
		MXC6255_CTRL_PWRDN,
	};

	err = mxc6255_acc_i2c_write(acc, buf, 1);
	if (err < 0) {
		dev_err(&acc->client->dev, "soft power off failed: %d\n", err);
	}
}

static int mxc6255_acc_device_power_on(struct mxc6255_data *acc)
{
	int err   = -1;
	u8 buf[2] = {
		MXC6255_REG_CTRL,
		MXC6255_CTRL_PWRON,
	};

	err = mxc6255_acc_i2c_write(acc, buf, 1);
	if (err < 0) {
		dev_err(&acc->client->dev, "soft power on failed: %d\n", err);
	}

	return 0;
}

struct mxc6255_data_axis{
	int x;
	int y;
	int z;
};


static int mxc6255_data_convert(struct mxc6255_data* pdata,struct mxc6255_data_axis *axis_data)
{
	int rawdata[3],data[3];
	int i,j;
	int position = pdata->position ;
	const int mxc6255_acc_setting[8][3][3] ={
		{{ 0, -1,  0}, { 1,  0,	0}, {0, 0, 1}},
		{{-1,  0,  0}, { 0, -1,	0}, {0, 0, 1}},
		{{ 0,  1,  0}, { 1,  0,	0}, {0, 0, 1}},
		{{ 1,  0,  0}, { 0,  1,	0}, {0, 0, 1}},

		{{ 0, -1,  0}, {-1,  0,	0}, {0, 0, -1}},
		{{-1,  0,  0}, { 0,  1,	0}, {0, 0, -1}},
		{{ 0,  1,  0}, { 1,  0,	0}, {0, 0, -1}},
		{{ 1,  0,  0}, { 0, -1,	0}, {0, 0, -1}},
	};

	if(position < 0 || position > 7 )
		position = 0;
	rawdata [0] = axis_data->x ;
	rawdata [1] = axis_data->y ;
	rawdata [2] = axis_data->z ;

	for(i = 0; i < 3 ; i++){
		data[i] = 0;
		for(j = 0; j < 3; j++)
			data[i] += rawdata[j] * mxc6255_acc_setting[position][i][j];
	}

	axis_data->x = data[0];
	axis_data->y = data[1];
	axis_data->z = data[2];

	return 0;
}

static int mxc6255_acc_get_acceleration_data(struct mxc6255_data *acc, int *xyz)
{
	int err = -1;
	/* Data bytes from hardware x, y */
	u8 acc_data[2];
	struct mxc6255_data_axis axis_data;
	acc_data[0] = MXC6255_REG_DATA;

	err = mxc6255_acc_i2c_read(acc, acc_data, 2);
	if (err < 0) {
		printk(KERN_INFO "%s I2C read error %d\n", MXC6255_ACC_I2C_NAME, err);
		return err;
	}

	xyz[0] = (signed char)acc_data[1];
	xyz[1] = (signed char)acc_data[0];
#if defined(MXC6255_Z_NEGATIVE)
	xyz[2] = -64;
#else
	xyz[2] = 64;
#endif
	axis_data.x = xyz[0];
	axis_data.y = xyz[1];
	axis_data.z = 64;
	mxc6255_data_convert(acc,&axis_data);

	xyz[0] = axis_data.x;
	xyz[1] = axis_data.y;
	xyz[2] = axis_data.z;

#ifdef MXC6255_DEBUG
	printk("x = %d, y = %d\n", xyz[0], xyz[1]);
#endif

#ifdef MXC6255_DEBUG
	printk(KERN_INFO "%s read x=%d, y=%d, z=%d\n", MXC6255_ACC_DEV_NAME, xyz[0], xyz[1], xyz[2]);
#endif

	return err;
}

static void mxc6255_acc_report_values(struct mxc6255_data *acc, int *xyz)
{
	struct input_polled_dev *poll_dev = acc->poll_dev;
	struct input_dev *idev = poll_dev->input ;
	input_report_abs(idev, ABS_X, xyz[0]);
	input_report_abs(idev, ABS_Y, xyz[1]);
	input_report_abs(idev, ABS_Z, xyz[2]);
	input_sync(idev);
}

static int mxc6255_acc_enable(struct mxc6255_data *acc)
{
	int err;

	if (!atomic_cmpxchg(&acc->enabled, 0, 1)) {
		err = mxc6255_acc_device_power_on(acc);
		if (err < 0) {
			atomic_set(&acc->enabled, 0);
			return err;
		}
		acc->active = 1;
	}

	return 0;
}

static int mxc6255_acc_disable(struct mxc6255_data *acc)
{
	if (atomic_cmpxchg(&acc->enabled, 1, 0)){
		mxc6255_acc_device_power_off(acc);
		acc->active = 0;
	}

	return 0;
}

static int mxc6255_acc_resume(struct i2c_client *client)
{
	struct mxc6255_data *acc = i2c_get_clientdata(client);

#ifdef MXC6255_DEBUG
	printk(KERN_EMERG "%s.\n", __FUNCTION__);
#endif

	if (acc && acc->stop_by_user){
		acc->stop_by_user = 0;
		return mxc6255_acc_enable(acc);
	}

	return 0;
}

static int mxc6255_acc_suspend(struct i2c_client *client, pm_message_t mesg)
{
	struct mxc6255_data *acc = i2c_get_clientdata(client);

#ifdef MXC6255_DEBUG
	printk(KERN_EMERG "%s.\n", __FUNCTION__);
#endif

	if (acc){
		if (atomic_read(&acc->enabled)){
			acc->stop_by_user = 1;
			return mxc6255_acc_disable(acc);
		}
	}

	return 0;
}

static void mxc6255_acc_poll_dev(struct input_polled_dev *dev)
{

	struct mxc6255_data *acc = (struct mxc6255_data *)dev->private;
	int xyz[3]                   = {0};
	int err                      = -1;
	if(!acc)
		printk(KERN_EMERG"%s entry error no acc \n",__func__);
	if (acc){
		mutex_lock(&acc->lock);
		if(!acc->active){
			mutex_unlock(&acc->lock);
			return;
		}
		err = mxc6255_acc_get_acceleration_data(acc, xyz);
		if (err < 0){
			printk(KERN_EMERG"%s err=0x%x \n",__func__,err);
			dev_err(&acc->client->dev, "get_acceleration_data failed\n");
		}
		else{
			mxc6255_acc_report_values(acc, xyz);
		}

		mutex_unlock(&acc->lock);
	}
}

static int mxc6255_acc_input_init(struct mxc6255_data *acc)
{
	int err;

	struct input_polled_dev *poll_dev = acc->poll_dev;
	struct input_dev *idev = poll_dev->input ;

	if (!idev){
		err = -ENOMEM;
		dev_err(&acc->client->dev, "input device allocate failed\n");
		goto err0;
	}

	input_set_drvdata(idev, acc);

	set_bit(EV_ABS, idev->evbit);

	input_set_abs_params(idev, ABS_X, -G_MAX, G_MAX, FUZZ, FLAT);
	input_set_abs_params(idev, ABS_Y, -G_MAX, G_MAX, FUZZ, FLAT);
	input_set_abs_params(idev, ABS_Z, -G_MAX, G_MAX, FUZZ, FLAT);

	idev->name = MXC6255_ACC_INPUT_NAME;
	return 0;

err0:
	return err;
}

static void mxc6255_acc_input_cleanup(struct mxc6255_data *acc)
{
	struct input_polled_dev *poll_dev = acc->poll_dev;
	input_unregister_polled_device(poll_dev);
	input_free_polled_device(poll_dev);
}

static ssize_t mxc6255_enable_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct input_polled_dev *poll_dev = dev_get_drvdata(dev);
	struct mxc6255_data *pdata = (struct mxc6255_data *)(poll_dev->private);
	int enable;

	mutex_lock(&pdata->lock);

	if(pdata->active)
		enable = 1;
	else
		enable = 0;
	mutex_unlock(&pdata->lock);
	return sprintf(buf, "%d\n", enable);
}

static ssize_t mxc6255_enable_store(struct device *dev,
		struct device_attribute *attr,
		const char *buf, size_t count)
{
	struct input_polled_dev *poll_dev = dev_get_drvdata(dev);
	struct mxc6255_data *pdata = (struct mxc6255_data *)(poll_dev->private);
	struct i2c_client *client = pdata->client;
	unsigned long enable;
	int err = 0;

	enable = simple_strtoul(buf, NULL, 10);
	enable = (enable > 0) ? 1 : 0;
	mutex_lock(&pdata->lock);

	if (!pdata->hw_initialized){
		err = mxc6255_acc_hw_init(pdata);
		if ((pdata->hw_working == 1) && (err < 0))
			mxc6255_acc_device_power_off(pdata);
	}

#ifdef CONFIG_HAS_EARLYSUSPEND
	if(pdata->early_suspended){//early suspend
		mutex_unlock(&pdata->lock);
		return count;
	}
#endif
	if(!pdata->active&&enable)
		mxc6255_acc_resume(client);
	else if(pdata->active&&!enable)
		mxc6255_acc_suspend(client, (pm_message_t){.event = 0});
	mutex_unlock(&pdata->lock);

	return count;
}

static ssize_t mxc6255_position_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct input_polled_dev *poll_dev = dev_get_drvdata(dev);
	struct mxc6255_data *pdata = (struct mxc6255_data *)(poll_dev->private);
	int position = 0;

	mutex_lock(&pdata->lock);
	position = pdata->position ;
	mutex_unlock(&pdata->lock);
	return sprintf(buf, "%d\n", position);
}

static ssize_t mxc6255_position_store(struct device *dev,
		struct device_attribute *attr,
		const char *buf, size_t count)
{
	struct input_polled_dev *poll_dev = dev_get_drvdata(dev);
	struct mxc6255_data *pdata = (struct mxc6255_data *)(poll_dev->private);
	int  position;
	position = simple_strtoul(buf, NULL, 10);
	mutex_lock(&pdata->lock);
	pdata->position = position;
	mutex_unlock(&pdata->lock);
	return count;
}

#define CALI_DATA_MAX_SIZE	16
static char cali_data[CALI_DATA_MAX_SIZE] = {0};
static int cali_data_size = 0;

static int mxc6255_do_calibrate(void)
{
	/* only for test */
	cali_data[0] = 0x11;
	cali_data[1] = 0x22;
	cali_data[2] = 0x33;
	cali_data[3] = 0x44;
	cali_data[4] = 0x55;
	cali_data[5] = 0x66;
	cali_data[6] = 0x77;
	cali_data[7] = 0x88;
	cali_data_size = 8;

	return 0;
}

static ssize_t mxc6255_calibrate_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	memcpy(buf, cali_data, cali_data_size);
	return cali_data_size;
}

static ssize_t mxc6255_calibrate_store(struct device *dev,
		struct device_attribute *attr,
		const char *buf, size_t count)
{
	int ret = 0;
	if(count < 4) { /* start calibrate */
		ret = mxc6255_do_calibrate();

	} else { /* store calibration data */
		memcpy(cali_data, buf, count);
		cali_data_size = count;

		/* Enable cali data in here */
		/* TODO */
	}
	return ret ? 0 : count;
}

static DEVICE_ATTR(position, S_IWUSR | S_IWGRP | S_IRUGO,
		mxc6255_position_show, mxc6255_position_store);

static DEVICE_ATTR(enable, S_IWUSR | S_IWGRP | S_IRUGO,
		mxc6255_enable_show, mxc6255_enable_store);

static DEVICE_ATTR(calibrate, S_IWUSR | S_IWGRP | S_IRUGO,
		mxc6255_calibrate_show, mxc6255_calibrate_store);

static struct attribute *mxc6255_attributes[] = {
	&dev_attr_enable.attr,
	&dev_attr_position.attr,
	&dev_attr_calibrate.attr,
	NULL
};

static const struct attribute_group mxc6255_attr_group = {
	.attrs = mxc6255_attributes,
};

static int mxc6255_acc_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	struct mxc6255_data *acc = NULL;
	struct input_polled_dev *poll_dev;
	struct input_dev *idev;

	int err                      = -1;
	int tempvalue                = 0;

	pr_info("%s: probe start.\n", MXC6255_ACC_DEV_NAME);

	if (!client->dev.platform_data){
		dev_err(&client->dev, "platform data is NULL. exiting.\n");
		err = -ENODEV;
		goto exit_check_functionality_failed;
	}

	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)){
		dev_err(&client->dev, "client not i2c capable\n");
		err = -ENODEV;
		goto exit_check_functionality_failed;
	}

	if (!i2c_check_functionality(client->adapter,
				I2C_FUNC_SMBUS_BYTE |
				I2C_FUNC_SMBUS_BYTE_DATA |
				I2C_FUNC_SMBUS_WORD_DATA)){
		dev_err(&client->dev, "client not smb-i2c capable:2\n");
		err = -EIO;
		goto exit_check_functionality_failed;
	}


	if (!i2c_check_functionality(client->adapter,
				I2C_FUNC_SMBUS_I2C_BLOCK)){
		dev_err(&client->dev, "client not smb-i2c capable:3\n");
		err = -EIO;
		goto exit_check_functionality_failed;
	}

	/*
	 * OK. From now, we presume we have a valid client. We now create the
	 * client structure, even though we cannot fill it completely yet.
	 */
	acc = kzalloc(sizeof(*acc), GFP_KERNEL);
	if (!acc){
		err = -ENOMEM;
		dev_err(&client->dev, "failed to allocate memory for module data: %d\n", err);
		goto exit_alloc_data_failed;
	}

	mutex_init(&acc->lock);

	acc->client        = client;
	i2c_set_clientdata(client, acc);

	/* read chip id */
	tempvalue = i2c_smbus_read_word_data(client, WHO_AM_I);

	if ((tempvalue & 0x003F) == WHOAMI_MXC6255_ACC)
		printk(KERN_INFO "%s I2C driver registered!\n", MXC6255_ACC_DEV_NAME);
	else{
		acc->client = NULL;
		printk(KERN_INFO "I2C driver not registered! Device unknown 0x%x\n", tempvalue);
		goto err2;
	}


	i2c_set_clientdata(client, acc);

	err = mxc6255_acc_device_power_on(acc);
	if (err < 0){
		dev_err(&client->dev, "power on failed: %d\n", err);
		goto err2;
	}

	atomic_set(&acc->enabled, 0);

	poll_dev = input_allocate_polled_device();
	if (!poll_dev) {
		err = -ENOMEM;
		dev_err(&client->dev, "alloc poll device failed!\n");
		goto err_power_off;
	}

	acc->poll_dev = poll_dev;
	idev = poll_dev->input;
	err = mxc6255_acc_input_init(acc);
	if (err < 0){
		dev_err(&client->dev, "input init failed\n");
		goto err_power_off;
	}

	poll_dev->poll = mxc6255_acc_poll_dev;
	poll_dev->poll_interval = POLL_STOP_TIME;
	poll_dev->poll_interval_min = POLL_INTERVAL_MIN;
	poll_dev->poll_interval_max = POLL_INTERVAL_MAX;
	poll_dev->private = acc;
	idev->name = RDA_GSENSOR_DRV_NAME;
	idev->uniq = "mxc6255";
	idev->id.bustype = BUS_I2C;
	idev->evbit[0] = BIT_MASK(EV_ABS);
#ifdef CONFIG_HAS_EARLYSUSPEND
	acc->early_suspended = 0;
	acc->early_suspend.level = EARLY_SUSPEND_LEVEL_BLANK_SCREEN + 2;
	acc->early_suspend.suspend = mxc6255_early_suspend;
	acc->early_suspend.resume = mxc6255_late_resume;
	register_early_suspend(&acc->early_suspend);
#endif /* CONFIG_HAS_EARLYSUSPEND */
	err = input_register_polled_device(acc->poll_dev);
	if(err){
		dev_err(&client->dev, "register poll device failed!\n");
		err = -EINVAL;
		goto err_power_off;
	}
	err = sysfs_create_group(&idev->dev.kobj, &mxc6255_attr_group);
	if (err) {
		dev_err(&client->dev, "create device file failed!\n");
		err = -EINVAL;
		goto err_power_off;
	}

	kobject_uevent(&idev->dev.kobj, KOBJ_CHANGE);
	mxc6255_acc_device_power_off(acc);

	/* As default, do not report information */
	atomic_set(&acc->enabled, 0);

	acc->stop_by_user = 1;
	acc->position = _TGT_AP_GSENSOR_POSITION;



	dev_info(&client->dev, "%s: probed\n", MXC6255_ACC_DEV_NAME);

	return 0;

err_power_off:
#ifdef CONFIG_HAS_EARLYSUSPEND
	unregister_early_suspend(&acc->early_suspend);
#endif /* CONFIG_HAS_EARLYSUSPEND */
	mxc6255_acc_input_cleanup(acc);
	mxc6255_acc_device_power_off(acc);

err2:
	kfree(acc);
	i2c_set_clientdata(client, NULL);

exit_alloc_data_failed:
exit_check_functionality_failed:
	printk(KERN_ERR "%s: Driver Init failed\n", MXC6255_ACC_DEV_NAME);

	return err;
}

static int mxc6255_acc_remove(struct i2c_client *client)
{
	/* TODO: revisit ordering here once _probe order is finalized */
	struct mxc6255_data *acc = i2c_get_clientdata(client);
#ifdef CONFIG_HAS_EARLYSUSPEND
	unregister_early_suspend(&acc->early_suspend);
#endif /* CONFIG_HAS_EARLYSUSPEND */

	mxc6255_acc_input_cleanup(acc);
	mxc6255_acc_device_power_off(acc);
	kfree(acc);

	return 0;
}


static const struct i2c_device_id mxc6255_acc_id[] = {
	{MXC6255_ACC_DEV_NAME, 0},
	{},
};
MODULE_DEVICE_TABLE(i2c, mxc6255_acc_id);

#ifdef CONFIG_HAS_EARLYSUSPEND
static void mxc6255_early_suspend(struct early_suspend *h)
{
	struct mxc6255_data *pdata = container_of(h, struct mxc6255_data, early_suspend);

	mutex_lock(&pdata->lock);
	if(pdata->stop_by_user) {//already suspend
		pdata->early_suspended = 0;
		mutex_unlock(&pdata->lock);
		return ;
	}
	mxc6255_acc_suspend(pdata->client, (pm_message_t){.event = 0});
	pdata->early_suspended = 1;
	mutex_unlock(&pdata->lock);

	return ;
}

static void mxc6255_late_resume(struct early_suspend *h)
{
	struct mxc6255_data *pdata = container_of(h, struct mxc6255_data, early_suspend);

	if(!pdata->early_suspended)
		return ;
	mutex_lock(&pdata->lock);
	pdata->early_suspended = 0;
	mxc6255_acc_resume(pdata->client);
	mutex_unlock(&pdata->lock);
	return ;
}

#else
#ifdef CONFIG_PM_SLEEP
static int mxc6255_pm_suspend(struct device *dev)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct mxc6255_data *pdata = i2c_get_clientdata(client);
	mutex_lock(&pdata->lock);
	mxc6255_acc_suspend(client, (pm_message_t){.event = 0});
	mutex_unlock(&pdata->lock);

	return 0;
}

static int mxc6255_pm_resume(struct device *dev)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct mxc6255_data *pdata = i2c_get_clientdata(client);
	mutex_lock(&pdata->lock);
	mxc6255_acc_resume(client);
	mutex_unlock(&pdata->lock);
	return 0;
}

static SIMPLE_DEV_PM_OPS(mxc6255_pm_ops,mxc6255_pm_suspend,mxc6255_pm_resume);

#endif /* CONFIG_PM_SLEEP */

#endif /* CONFIG_HAS_EARLYSUSPEND */

static struct i2c_driver mxc6255_acc_driver = {
	.probe     = mxc6255_acc_probe,
	.remove    = mxc6255_acc_remove,
	.id_table  = mxc6255_acc_id,
	.driver = {
		.owner = THIS_MODULE,
		.name  = MXC6255_ACC_I2C_NAME,
#ifndef CONFIG_HAS_EARLYSUSPEND
# ifdef CONFIG_PM_SLEEP
		.pm = &mxc6255_pm_ops,
# endif /* CONFIG_PM_SLEEP */
#endif /* CONFIG_HAS_EARLYSUSPEND */
	},
};

static struct rda_gsensor_device_data rda_gsensor_data[] = {
	{
		.irqflags = IRQF_SHARED | IRQF_TRIGGER_RISING,
	},
};

static int __init mxc6255_acc_init(void)
{
	/* register driver */
	int res;

	static struct i2c_board_info i2c_dev_gsensor = {
		I2C_BOARD_INFO("mxc6255", 0),
		.platform_data = rda_gsensor_data,
	};

	struct i2c_adapter *adapter;
	i2c_dev_gsensor.addr =  _DEF_I2C_ADDR_GSENSOR_MXC6255;

	adapter = i2c_get_adapter(_TGT_AP_I2C_BUS_ID_GSENSOR);

	if (!adapter) {
		pr_err("%s, cannot get i2c adapter %d\n",
				__func__, _TGT_AP_I2C_BUS_ID_GSENSOR);
		return -ENODEV;
	}

	i2c_new_device(adapter, &i2c_dev_gsensor);

	res = i2c_add_driver(&mxc6255_acc_driver);
	if (res < 0) {
		printk(KERN_INFO "add mma7660 i2c driver failed\n");
		return -ENODEV;
	}

	pr_info("rda_gs %s initialized, at i2c bus %d addr 0x%02x\n",
			"mxc6255", _TGT_AP_I2C_BUS_ID_GSENSOR,
			i2c_dev_gsensor.addr);

	return res;
}

static void __exit mxc6255_acc_exit(void)
{
	printk(KERN_INFO "%s accelerometer driver exit\n", MXC6255_ACC_DEV_NAME);

	i2c_del_driver(&mxc6255_acc_driver);

	return;
}

// module_init(mxc6255_acc_init);
late_initcall(mxc6255_acc_init);
module_exit(mxc6255_acc_exit);

MODULE_DESCRIPTION("mxc6255 accelerometer misc driver");
MODULE_AUTHOR("Memsic");
MODULE_LICENSE("GPL");
