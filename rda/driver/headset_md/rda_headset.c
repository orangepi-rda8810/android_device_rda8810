/*
 *  drivers/switch/rda_headset.c
 *
 * Copyright (C) 2008 Google, Inc.
 * Author: Mike Lockwood <lockwood@android.com>
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/switch.h>
#include <linux/input.h>
#include <linux/workqueue.h>
#include <linux/wakelock.h>
#include <linux/gpio.h>
#include <linux/irq.h>
#include <linux/spinlock.h>
#include <plat/md_sys.h>

#ifdef CONFIG_HAS_EARLYSUSPEND
// use when headset use early suspend, known issue : usb in, bl down, headset unable use, so we use kernel suspend&resume
//#define CONFIG_HEADSET_USE_EARLYSYSPEND 1
#endif

#ifdef CONFIG_HEADSET_USE_EARLYSYSPEND
#include <linux/earlysuspend.h>
#endif /* CONFIG_HEADSET_USE_EARLYSYSPEND */

#include "tgt_ap_gpio_setting.h"
#include "tgt_ap_headset_setting.h"

struct rda_headset_key_caps
{
	int key_code;
	int adc_min;
	int adc_max;
};

struct rda_headset_key_caps headset_key_caps[] = {
#ifdef HEADSET_KEY_CAPS
	HEADSET_KEY_CAPS
#else
	{KEY_MEDIA, 0, 2000}
#endif
};

enum
{
	UNPLUG = 0,
	PLUG   = 1,
};

// headset detect
struct rda_headset_detect_data {
	struct switch_dev sdev;
	unsigned gpio;
	const char *state_headset;
	const char *state_headphone;
	const char *state_out;
	int irq;
	struct delayed_work work;
	struct rda_headset_data* headset_data;
	int headset_type;
	int headset_plug_in;
	int first_notify_adc;
};

// headset keypad
struct rda_headset_keypad_data {
	struct rda_headset_data* headset_data;
	struct input_dev *idev;
	struct wake_lock headset_key_lock;
	struct work_struct work;
	int key_down;
	int key_code;
};

// headset data
struct rda_headset_data {
	struct msys_device *headset_msys;
	struct rda_headset_detect_data* headset_detect;
	struct rda_headset_keypad_data* headset_keypad;
	/** irq wake lock : to give enough time to let uplayers know headset state */
	struct wake_lock headset_irq_lock;
    atomic_t gpio_irq_work_pending;
#ifdef CONFIG_HEADSET_USE_EARLYSYSPEND
	struct early_suspend early_ops;
#endif /* CONFIG_HEADSET_USE_EARLYSYSPEND */

	int enabled;
};

static void tell_modem_headset_status(struct rda_headset_data* headset_data, int in, unsigned int gatevalue)
{
	u32 __dat[2];
	struct client_cmd headset_cmd;

	if(headset_data == NULL)
		return;

	__dat[0] = in;
	__dat[1] = gatevalue;

	memset(&headset_cmd, 0, sizeof(headset_cmd));
	headset_cmd.pmsys_dev = headset_data->headset_msys;
	headset_cmd.mod_id = SYS_PM_MOD;
	headset_cmd.mesg_id = SYS_PM_CMD_EP_STATUS;
	headset_cmd.pdata = (void *)&__dat;
	headset_cmd.data_size = sizeof(__dat);
	rda_msys_send_cmd(&headset_cmd);
}

static void keypad_msyscmd_work(struct work_struct *work)
{
	struct rda_headset_keypad_data *headset_keypad =
		container_of(work, struct rda_headset_keypad_data, work);
	struct rda_headset_detect_data *headset_detect = headset_keypad->headset_data->headset_detect;

	/** no headset now */
	if(headset_detect->headset_plug_in == 0)
		return;

	printk(KERN_INFO"rda headset : reset gpadc value work\n");

	tell_modem_headset_status(headset_keypad->headset_data, 1, 
			headset_key_caps[sizeof(headset_key_caps)/sizeof(struct rda_headset_key_caps)-1].adc_max);
	headset_detect->first_notify_adc = 0;
}

static void reset_headset_gpadc_gate_value(struct rda_headset_keypad_data *headset_keypad)
{
	if (!work_pending(&headset_keypad->work))
		schedule_work(&headset_keypad->work);
}

static void gpio_switch_work(struct delayed_work *work)
{
	int state = 0;
	unsigned long out_debounce = jiffies;

	struct rda_headset_detect_data *headset_detect =
		container_of(work, struct rda_headset_detect_data, work);
	struct rda_headset_data *headset_data          = headset_detect->headset_data;
	struct rda_headset_keypad_data *headset_keypad = headset_data->headset_keypad;

	state = gpio_get_value(headset_detect->gpio);
	dev_info(headset_detect->sdev.dev, "%s, gpio state is [%d], HEADSET_OUT_GPIO_STATE %d\n",
			__func__, state, HEADSET_OUT_GPIO_STATE);

	if(state == HEADSET_OUT_GPIO_STATE) {
		if(headset_detect->headset_plug_in == 1) {
			// debounce second time
			out_debounce = jiffies + msecs_to_jiffies(GPIO_OUT_DETECT_DEBOUNCE_DELAY_MSECS);
			while(time_before(jiffies, out_debounce)) {
				schedule();
			}

			state = gpio_get_value(headset_detect->gpio);
			if(state == HEADSET_OUT_GPIO_STATE) {
				switch_set_state(&headset_detect->sdev, UNPLUG);
				headset_detect->headset_plug_in = 0;
				// tell modem close headset
				tell_modem_headset_status(headset_data, 0, 0x7fffffff);
				headset_detect->headset_type    = -1;
				headset_detect->first_notify_adc = 1;
				headset_keypad->key_down        = 0;
			}
		}
	}
	else {
		if(headset_detect->headset_plug_in != 1) {
			headset_detect->headset_plug_in = 1;
			headset_detect->first_notify_adc = 1;

            // BUG
            // do not do this, cause headset out will cause audio output path change : bad UE, we just wait for first adc value
			// we init headset_type 2 (headphone - no key) for quick headphone state report
			/** headset_detect->headset_type = 2; */
			/** switch_set_state(&headset_detect->sdev, headset_detect->headset_type); */

			// tell modem open headset , give modem a most adc gate value, so it will tell us first adc value
			tell_modem_headset_status(headset_data, 1, 0x7fffffff);
		}
	}

    atomic_set(&(headset_data->gpio_irq_work_pending), 0);
}

static irqreturn_t gpio_irq_handler(int irq, void *dev_id)
{
	int ret = 0, i = 0;
	struct rda_headset_detect_data *headset_detect = (struct rda_headset_detect_data *)dev_id;
	struct rda_headset_data *headset_data = headset_detect->headset_data;

    atomic_set(&(headset_data->gpio_irq_work_pending), 1);

	/** give them 5 seconds every time, enough? */
    printk(KERN_INFO"rda-headset : aquire wake lock for 5 secs for for headset irq.\n");
    wake_lock_timeout(&(headset_detect->headset_data->headset_irq_lock), 5*HZ);

    /** try 5 times */
    for(i = 0; i < 5; ++i) {
        ret = schedule_delayed_work(&headset_detect->work,
                msecs_to_jiffies(GPIO_IRQ_DETECT_DEBOUNCE_DELAY_MSECS));
        if(ret == 0)
            break;
    }
	if(ret > 0) {
		printk(KERN_INFO"rda-headset : gpio irq work enqueue fail, retry %d times, err %d\n", i, ret);
	}

	return IRQ_HANDLED;
}

static ssize_t switch_gpio_print_state(struct switch_dev *sdev, char *buf)
{
	struct rda_headset_detect_data *headset_detect =
		container_of(sdev, struct rda_headset_detect_data, sdev);
	const char *state;
	if (switch_get_state(sdev) == 1)
		state = headset_detect->state_headset;
	else if(switch_get_state(sdev) == 2)
		state = headset_detect->state_headphone;
	else
		state = headset_detect->state_out;

	if (state)
		return sprintf(buf, "%s\n", state);
	return -1;
}

static void headset_detect_and_report_key(struct rda_headset_data* headset_data, int adc_value)
{
	int i = 0, size = 0, key_code = 0;
	struct rda_headset_detect_data *headset_detect = NULL;
	struct rda_headset_keypad_data *headset_keypad = NULL;

	if(headset_data == NULL)
		return;

	headset_detect = headset_data->headset_detect;
	headset_keypad = headset_data->headset_keypad;

    if(!headset_detect->headset_plug_in)
        return;

	// we need to decide type first
	if(headset_detect->headset_type == -1) {
		if(adc_value < DETECT_HEADPHON_MAX_VALUE)
			headset_detect->headset_type = 2;
		else
			headset_detect->headset_type = 1;

		printk(KERN_INFO" rda headset : headset_type is [%d]\n", headset_detect->headset_type);

		switch_set_state(&headset_detect->sdev, headset_detect->headset_type);

		return;
	}

    /** last is not key down and irq is in progress, maybe this is a fake event, ignore this event */
    if(atomic_read(&(headset_data->gpio_irq_work_pending)) != 0 && !headset_keypad->key_down) {
		printk(KERN_INFO" rda headset :gpio_irq_work_pending  [%d], headset_keypad->key_down [%d]\n",
                atomic_read(&(headset_data->gpio_irq_work_pending)), headset_keypad->key_down);
        return;
    }

	// headphone no keys
	if(headset_detect->headset_type != 1) {
        // BUG
        // do not do this, cause headset out will cause audio output path change : bad UE, we just wait for first adc value
		// 1. wrong type we rec
		// 2. mic key down insert
		// will both rec headset to headphone, we need to rec again here
		/** if(adc_value >= DETECT_HEADPHON_MAX_VALUE) {
		 	headset_detect->headset_type = 1;
		 	printk(KERN_INFO" rda headset : re rec headset_type is [%d]\n", headset_detect->headset_type);
		 	// first out headphone
		 	switch_set_state(&headset_detect->sdev, UNPLUG);
		 	// then in headset
		 	switch_set_state(&headset_detect->sdev, headset_detect->headset_type);
		}
        */
		return;
	}

	size = sizeof(headset_key_caps)/sizeof(struct rda_headset_key_caps);

	for(i = 0; i < size; ++i) {
		printk(KERN_DEBUG"rda headset : index [%d], adc_min [0x%x], adc_max [0x%x], adc_value [0x%x] \n",
				i, headset_key_caps[i].adc_min, headset_key_caps[i].adc_max, adc_value);
		if(adc_value >= headset_key_caps[i].adc_min && adc_value <= headset_key_caps[i].adc_max) {
			key_code = headset_key_caps[i].key_code;

			printk(KERN_DEBUG"rda headset : find key is [%d] at index [%d]\n", key_code, i);
			break;
		}
	}

    // lock wake lock for key dispatch (most for suspending state)
    wake_lock_timeout(&(headset_keypad->headset_key_lock), 5*HZ);

	// already key down
	if(headset_keypad->key_down) {
		// up
		if(i == size) {
			printk(KERN_INFO"rda headset : key up [%d] \n", headset_keypad->key_code);
			input_event(headset_keypad->idev, EV_KEY, headset_keypad->key_code, 0);
			input_sync(headset_keypad->idev);
			headset_keypad->key_down = 0;
		}
	}
	// found one key down
	else if(i < size) {
		printk(KERN_INFO"rda headset : key down [%d] \n", key_code);
		input_event(headset_keypad->idev, EV_KEY, key_code, 1);
		input_sync(headset_keypad->idev);
		headset_keypad->key_down = 1;
		headset_keypad->key_code = key_code;
	}
}

static int rda_modem_headset_notify(struct notifier_block *nb,
		unsigned long mesg, void *data)
{
	int adc_value = 0;
	struct msys_device *headset_msys = container_of(nb, struct msys_device, notifier);
	struct rda_headset_keypad_data *headset_keypad = (struct rda_headset_keypad_data*)(headset_msys->private);
	struct rda_headset_detect_data *headset_detect = NULL;
	struct client_mesg *pmesg = (struct client_mesg *)data;

	if (pmesg->mod_id != SYS_PM_MOD 
			|| mesg != SYS_PM_MESG_EP_KEY_STATUS
			|| headset_keypad == NULL) {
		return NOTIFY_DONE;
	}

	headset_detect = headset_keypad->headset_data->headset_detect;

	if(headset_detect->first_notify_adc) {
		// tell modem headset adc gate value, this value is most key value
		reset_headset_gpadc_gate_value(headset_keypad);
	}

	adc_value = *((unsigned int *)&(pmesg->param));
	printk(KERN_INFO"%s: mesg %ld, adc_value %d\n", __func__, mesg, adc_value);
	headset_detect_and_report_key(headset_keypad->headset_data, adc_value);

	return NOTIFY_DONE;
}


#ifdef CONFIG_HEADSET_USE_EARLYSYSPEND
static void rda_headset_early_suspend(struct early_suspend *h)
{
	printk(KERN_INFO"rda headset (early): suspending... \n");
	printk(KERN_INFO"rda headset (early): suspending done \n");
}

static void rda_headset_late_resume(struct early_suspend *h)
{
	printk(KERN_INFO"rda headset (early): resuming... \n");
	printk(KERN_INFO"rda headset (early): resuming done \n");
}
#else /* CONFIG_HEADSET_USE_EARLYSYSPEND */
static int rda_headset_resume(struct device *dev)
{
	printk(KERN_INFO"rda headset : resuming... \n");
	printk(KERN_INFO"rda headset : resuming done \n");

	return 0;
}

static int rda_headset_suspend(struct device *dev)
{
	printk(KERN_INFO"rda headset : suspending... \n");
	printk(KERN_INFO"rda headset : suspending done \n");
	return 0;
}
#endif

static ssize_t rda_headset_enable_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct rda_headset_data *rda_hs = dev_get_drvdata(dev);

	return sprintf(buf, "%d\n", rda_hs->enabled);
}

static ssize_t rda_headset_enable_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	struct rda_headset_data *rda_hs = dev_get_drvdata(dev);
	int ret;
	int set;

	ret = kstrtoint(buf, 0, &set);
	if (ret < 0) {
		return ret;
	}

	set = !!set;

	if (rda_hs->enabled == set) {
		return count;
	}

	if (set) {
		ret = rda_headset_resume(dev);
	} else {
		ret = rda_headset_suspend(dev);
	}

	rda_hs->enabled = set;

	return count;
}

static DEVICE_ATTR(enabled, S_IWUSR | S_IWGRP | S_IRUGO,
	rda_headset_enable_show, rda_headset_enable_store);

static struct attribute *rda_headset_attrs[] = {
	&dev_attr_enabled.attr,
	NULL
};

static const struct attribute_group rda_headset_attr_group = {
	.attrs = rda_headset_attrs,
};

static int rda_headset_probe(struct platform_device *pdev)
{
	struct rda_headset_data *headset_data = NULL;
	struct rda_headset_detect_data *headset_detect = NULL;
	struct rda_headset_keypad_data *headset_keypad = NULL;
	int ret = 0, i = 0;

	headset_data = kzalloc(sizeof(struct rda_headset_data), GFP_KERNEL);
	if(!headset_data)
		return -ENOMEM;

	// 1. hp detect
	headset_detect = kzalloc(sizeof(struct rda_headset_detect_data), GFP_KERNEL);
	if (!headset_detect) {
		ret = -ENOMEM;
		goto err_alloc_detect;
	}
	headset_keypad = kzalloc(sizeof(struct rda_headset_keypad_data), GFP_KERNEL);
	if (!headset_keypad) {
		ret = -ENOMEM;
		goto err_alloc_keypad;
	}

	headset_data->headset_detect = headset_detect;
	headset_detect->gpio             = _TGT_AP_GPIO_HEADSET_DETECT;
	headset_detect->sdev.name        = RDA_HEADSET_DETECT_NAME;
	headset_detect->state_headset    = RDA_HEADSET_DETECT_STATE_HEADSET;
	headset_detect->state_headphone  = RDA_HEADSET_DETECT_STATE_HEADPHONE;
	headset_detect->state_out        = RDA_HEADSET_DETECT_STATE_OUT;
	headset_detect->sdev.print_state = switch_gpio_print_state;

	ret = switch_dev_register(&headset_detect->sdev);

	if (ret < 0) {
		printk(KERN_ERR"rda headset : switch_dev_register fail. ");
		goto err_switch_dev_register;
	}

	ret = gpio_request(headset_detect->gpio, pdev->name);
	if (ret < 0) {
		printk(KERN_ERR"rda headset : gpio_request fail. ");
		goto err_request_gpio;
	}

	ret = gpio_direction_input(headset_detect->gpio);
	if (ret < 0) {
		printk(KERN_ERR"rda headset : gpio_direction_input fail. ");
		goto err_set_gpio_input;
	}

	INIT_DELAYED_WORK(&headset_detect->work, (void *)gpio_switch_work);

	// -1 means do not know type yet
	headset_detect->headset_type    = -1;
	headset_detect->headset_plug_in = 0;
	headset_detect->headset_data    = headset_data;

	headset_detect->irq = gpio_to_irq(headset_detect->gpio);
	if (headset_detect->irq < 0) {
		ret = headset_detect->irq;
		printk(KERN_ERR"rda headset : gpio_to_irq fail. ");
		goto err_detect_irq_num_failed;
	}

	ret = request_irq(headset_detect->irq, gpio_irq_handler,
			IRQ_TYPE_EDGE_BOTH | IRQF_NO_SUSPEND, pdev->name, headset_detect);
	if (ret < 0) {
		printk(KERN_ERR"rda headset : request_irq fail. ");
		goto err_request_irq;
	}

	disable_irq(headset_detect->irq);

	// 2. hp keys
	headset_data->headset_keypad = headset_keypad;
	headset_keypad->idev = input_allocate_device();
	if(headset_keypad->idev == NULL) {
		printk(KERN_ERR"rda headset : input_allocate_device fail. ");
		goto err_input_alloc;
	}

	headset_keypad->key_down = 0;
	headset_keypad->idev->name = RDA_HEADSET_KEYPAD_NAME;
	headset_keypad->idev->id.bustype = BUS_HOST;
	headset_keypad->idev->id.vendor  = 0x0001;
	headset_keypad->idev->id.product = 0x0001;
	headset_keypad->idev->id.version = 0x0100;
	headset_keypad->headset_data     = headset_data;

	INIT_WORK(&headset_keypad->work, (void *)keypad_msyscmd_work);

	__set_bit(EV_KEY, headset_keypad->idev->evbit);
	__set_bit(EV_REP, headset_keypad->idev->evbit);
	for(i = 0; i < sizeof(headset_key_caps)/sizeof(struct rda_headset_key_caps); ++i) {
		input_set_capability(headset_keypad->idev, EV_KEY, headset_key_caps[i].key_code);
	}

	if(input_register_device(headset_keypad->idev)) {
		printk(KERN_ERR"rda headset : input_register_device fail. ");
		goto err_input_register;
	}

	// ap <---> modem pm
	headset_data->headset_msys = rda_msys_alloc_device();
	if (!headset_data->headset_msys) {
		printk(KERN_ERR"rda headset : rda_msys_alloc_device fail. ");
		goto err_msys;
	}

	headset_data->headset_msys->notifier.notifier_call =
		rda_modem_headset_notify;
	headset_data->headset_msys->private = (void *)(headset_keypad);

	rda_msys_register_device(headset_data->headset_msys);

	wake_lock_init(&(headset_data->headset_irq_lock), WAKE_LOCK_SUSPEND,
			"headset irq lock");

	wake_lock_init(&(headset_keypad->headset_key_lock), WAKE_LOCK_SUSPEND,
			"headset key lock");

#ifdef CONFIG_HEADSET_USE_EARLYSYSPEND
	headset_data->early_ops.level = EARLY_SUSPEND_LEVEL_BLANK_SCREEN + 1;
	headset_data->early_ops.suspend = rda_headset_early_suspend;
	headset_data->early_ops.resume = rda_headset_late_resume;

	register_early_suspend(&headset_data->early_ops);
#endif /* CONFIG_HEADSET_USE_EARLYSYSPEND */

	/* Perform initial detection */
    atomic_set(&(headset_data->gpio_irq_work_pending), 1);
	gpio_switch_work(&headset_detect->work);

	enable_irq(headset_detect->irq);

	platform_set_drvdata(pdev, headset_data);

	headset_data->enabled = 1;
	ret = sysfs_create_group(&pdev->dev.kobj, &rda_headset_attr_group);
	kobject_uevent(&pdev->dev.kobj, KOBJ_CHANGE);

	return 0;

err_msys:
err_input_register:
	input_free_device(headset_keypad->idev);
err_input_alloc:
err_request_irq:
err_detect_irq_num_failed:
err_set_gpio_input:
	gpio_free(headset_detect->gpio);
err_request_gpio:
	switch_dev_unregister(&headset_detect->sdev);
err_switch_dev_register:
	kfree(headset_keypad);
err_alloc_keypad:
	kfree(headset_detect);
err_alloc_detect:
	kfree(headset_data);

	return ret;
}

static int rda_headset_remove(struct platform_device *pdev)
{
	struct rda_headset_data *headset_data = platform_get_drvdata(pdev);

#ifdef CONFIG_HEADSET_USE_EARLYSYSPEND
	unregister_early_suspend(&headset_data->early_ops);
#endif /* CONFIG_HEADSET_USE_EARLYSYSPEND */

	sysfs_remove_group(&pdev->dev.kobj, &rda_headset_attr_group);

	cancel_delayed_work(&(headset_data->headset_detect->work));
	flush_delayed_work(&(headset_data->headset_detect->work));

	free_irq(headset_data->headset_detect->irq, NULL);
	gpio_free(headset_data->headset_detect->gpio);
	input_free_device(headset_data->headset_keypad->idev);
	switch_dev_unregister(&(headset_data->headset_detect->sdev));

	kfree(headset_data->headset_detect);
	kfree(headset_data->headset_keypad);
	kfree(headset_data);

	return 0;
}

static struct platform_driver rda_headset_driver = {
	.probe		= rda_headset_probe,
	.remove		= rda_headset_remove,
	.driver		= {
		.name	= "rda-headset",
		.owner	= THIS_MODULE,
	},
};

static int __init rda_headset_init(void)
{
	return platform_driver_register(&rda_headset_driver);
}

static void __exit rda_headset_exit(void)
{
	platform_driver_unregister(&rda_headset_driver);
}

module_init(rda_headset_init);
module_exit(rda_headset_exit);

MODULE_AUTHOR("Yulong Wang<yulongwang@rdamicro.com>");
MODULE_DESCRIPTION("RDA Headset driver");
MODULE_LICENSE("GPL");
