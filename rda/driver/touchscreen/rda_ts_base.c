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
#include <linux/timer.h>
#include "tgt_ap_board_config.h"

#ifdef CONFIG_HAS_EARLYSUSPEND
#include <linux/earlysuspend.h>
#endif /* CONFIG_HAS_EARLYSUSPEND */

#include <linux/platform_device.h>

#include <mach/hardware.h>
#include <plat/devices.h>
#include <mach/board.h>
#include <linux/gpio.h>
#include <plat/boot_mode.h>
#include "rda_ts.h"

#define PS_NAME "proximity"
static u8 proximity_need_open_after_resume = 0;

#ifdef GSL168X_PROC_DEBUG
#define TPD_PROC_DEBUG
#endif

#ifdef TPD_PROC_DEBUG
#include <asm/uaccess.h>
#include <linux/proc_fs.h>
static struct proc_dir_entry *gsl_config_proc = NULL;
#define GSL_CONFIG_PROC_FILE "gsl_config"
#define CONFIG_LEN 31
static char gsl_read[CONFIG_LEN];
static u8 gsl_data_proc[8] = {0};
static u8 gsl_proc_flag = 0;
#endif
//#define CONFIG_USE_TASKLET

#ifdef FT6X06_MODEL
extern int rda_ts_add_panel_ft6x06(void);
#endif

#ifdef MSG2133_MODEL
extern int rda_ts_add_panel_msg2133(void);
#endif

#ifdef GTP868_MODEL
extern int rda_ts_add_panel_gtp868(void);
#endif

#ifdef GTP960_MODEL
extern int rda_ts_add_panel_gtp960(void);
#endif

#ifdef ICN831X_MODEL
extern int rda_ts_add_panel_icn831x(void);
#endif

#ifdef IT7252_MODEL
extern int rda_ts_add_panel_it7252(void);
#endif

#ifdef GSL168X_MODEL
extern int rda_ts_add_panel_gsl168x(void);
#endif

#ifdef NT11004_MODEL
extern int rda_ts_add_panel_nt11004(void);
#endif

#ifdef EKT2527_MODEL
extern int rda_ts_add_panel_elan(void);
#endif

#define CONFIG_TOUCHSCREEN_RDA_VIRTUAL_KEY

static struct rda_ts_panel_info *ts_panel_info = NULL;
static struct rda_ts_panel_array	*ts_panel_array = NULL;
static struct workqueue_struct *rda_ts_wq;
static bool rda_ts_probe_done = false ;
static bool ts_i2c_work = true;


int rda_ts_register_driver(struct rda_ts_panel_info *info)
{
	static struct rda_ts_panel_array *ts;
	static struct rda_ts_panel_array *p;
	BUG_ON(!info);
	ts = kzalloc(sizeof(*ts), GFP_KERNEL);
	ts->panel_info = info;
	ts->next = NULL;
	if(ts_panel_array == NULL)
	{
		ts_panel_array = ts;
		return 0;
	}

	p = ts_panel_array;

	while(NULL != p->next)
		p = ts_panel_array->next;

	p->next = ts;

	return 0;
}
//EXPORT_SYMBOL(rda_ts_register_driver);

int rda_ts_i2c_write(struct i2c_client *client,
		u8 addr, u8 *pdata, int datalen)
{
	int ret = 0;
	u8 tmp_buf[128];
	unsigned int bytelen = 0;
	if (datalen > 125) {
		dev_err(&client->dev, "%s too big datalen = %d\n",
				__func__, datalen);
		return -1;
	}
	tmp_buf[0] = addr;
	bytelen++;
	if (datalen != 0 && pdata != NULL) {
		memcpy(&tmp_buf[bytelen], pdata, datalen);
		bytelen += datalen;
	}
	ret = i2c_master_send(client, (char const *)tmp_buf, bytelen);
	return ret;
}
//EXPORT_SYMBOL(rda_ts_i2c_write);

int rda_ts_i2c_read(struct i2c_client *client,
			u8 addr, u8 *pdata, unsigned int datalen)
{
	int ret = 0;

	if (datalen > 126) {
		dev_err(&client->dev, "%s too big datalen = %d\n",
				__func__, datalen);
		return -1;
	}

	ret = rda_ts_i2c_write(client, addr, NULL, 0);

	if (ret < 0) {
		dev_err(&client->dev, "%s set data address fail, ret = %d\n",
				__func__, ret);
		return ret;
	}

	return i2c_master_recv(client, (char *)pdata, datalen);
}
//EXPORT_SYMBOL(rda_ts_i2c_read);

#ifdef TPD_PROC_DEBUG
static int char_to_int(char ch)
{
	if(ch>='0' && ch<='9')
		return (ch-'0');
	else
		return (ch-'a'+10);
}
#define print_info(fmt, args...)   \
        do{                              \
                printk("[tp-gsl][%s]"fmt,__func__, ##args);     \
        }while(0)

static struct rda_ts_data *ddata = NULL;
static int gsl_config_read_proc(char *page, char **start, off_t off, int count, int *eof, void *data)
{
	char *ptr = page;
	char temp_data[5] = {0};
	unsigned int tmp=0;
	if('v'==gsl_read[0]&&'s'==gsl_read[1])
	{
#ifdef GSL_ALG_ID
		tmp=gsl_version_id();
#else
		tmp=0x20121215;
#endif
		ptr += sprintf(ptr,"version:%x\n",tmp);
	}
	else if('r'==gsl_read[0]&&'e'==gsl_read[1])
	{
		if('i'==gsl_read[3])
		{
#ifdef GSL_ALG_ID
			tmp=(gsl_data_proc[5]<<8) | gsl_data_proc[4];
			ptr +=sprintf(ptr,"gsl_config_data_id[%d] = ",tmp);
			if(tmp>=0&&tmp<ARRAY_SIZE(gsl_config_data_id))
				ptr +=sprintf(ptr,"%d\n",gsl_config_data_id[tmp]);
#endif
		}
		else
		{
			rda_ts_i2c_write(ddata->client,0xf0,&gsl_data_proc[4],4);
			rda_ts_i2c_read(ddata->client,gsl_data_proc[0],temp_data,4);
			ptr +=sprintf(ptr,"offset : {0x%02x,0x",gsl_data_proc[0]);
			ptr +=sprintf(ptr,"%02x",temp_data[3]);
			ptr +=sprintf(ptr,"%02x",temp_data[2]);
			ptr +=sprintf(ptr,"%02x",temp_data[1]);
			ptr +=sprintf(ptr,"%02x};\n",temp_data[0]);
		}
	}
	*eof = 1;
	return (ptr - page);
}
static int gsl_config_write_proc(struct file *file, const char *buffer, unsigned long count, void *data)
{
	u8 buf[8] = {0};
	char temp_buf[CONFIG_LEN];
	char *path_buf;
	//int tmp = 0;
	//int tmp1 = 0;
	if(count > 512)
	{
		print_info("size not match [%d:%ld]\n", CONFIG_LEN, count);
		return -EFAULT;
	}

	path_buf = memdup_user(buffer, count);
	if(IS_ERR(path_buf))
	{
		printk("alloc path_buf memory error \n");
		return PTE_ERR(path_buf);
	}
	memcpy(temp_buf,path_buf,(count<CONFIG_LEN?count:CONFIG_LEN));
	print_info("[tp-gsl][%s][%s]\n",__func__,temp_buf);

	buf[3]=char_to_int(temp_buf[14])<<4 | char_to_int(temp_buf[15]);
	buf[2]=char_to_int(temp_buf[16])<<4 | char_to_int(temp_buf[17]);
	buf[1]=char_to_int(temp_buf[18])<<4 | char_to_int(temp_buf[19]);
	buf[0]=char_to_int(temp_buf[20])<<4 | char_to_int(temp_buf[21]);

	buf[7]=char_to_int(temp_buf[5])<<4 | char_to_int(temp_buf[6]);
	buf[6]=char_to_int(temp_buf[7])<<4 | char_to_int(temp_buf[8]);
	buf[5]=char_to_int(temp_buf[9])<<4 | char_to_int(temp_buf[10]);
	buf[4]=char_to_int(temp_buf[11])<<4 | char_to_int(temp_buf[12]);
	if('v'==temp_buf[0]&& 's'==temp_buf[1])//version //vs
	{
		memcpy(gsl_read,temp_buf,4);
		printk("gsl version\n");
	}
	else if('s'==temp_buf[0]&& 't'==temp_buf[1])//start //st
	{
		gsl_proc_flag = 1;
		ddata->panel_info->ops.ts_reset_chip(ddata->client);
	}
	else if('e'==temp_buf[0]&&'n'==temp_buf[1])//end //en
	{
		msleep(20);
		ddata->panel_info->ops.ts_reset_chip(ddata->client);
		ddata->panel_info->ops.ts_reset_chip(ddata->client);
		gsl_proc_flag = 0;
	}
	else if('r'==temp_buf[0]&&'e'==temp_buf[1])//read buf //
	{
		memcpy(gsl_read,temp_buf,4);
		memcpy(gsl_data_proc,buf,8);
	}
	else if('w'==temp_buf[0]&&'r'==temp_buf[1])//write buf
	{
		rda_ts_i2c_write(ddata->client,buf[4],buf,4);
	}

#ifdef GSL_ALG_ID
	else if('i'==temp_buf[0]&&'d'==temp_buf[1])//write id config //
	{
		tmp1=(buf[7]<<24)|(buf[6]<<16)|(buf[5]<<8)|buf[4];
		tmp=(buf[3]<<24)|(buf[2]<<16)|(buf[1]<<8)|buf[0];
		if(tmp1>=0 && tmp1<ARRAY_SIZE(gsl_config_data_id))
		{
			gsl_config_data_id[tmp1] = tmp;
		}
	}
#endif
	kfree(path_buf);
	return count;
}
#endif

#if defined(CONFIG_TOUCHSCREEN_RDA_VIRTUAL_KEY)
static ssize_t virtual_keys_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
	struct rda_ts_vir_key * key = ts_panel_info->ts_para.vir_key;
	int i = 0;

	char *pBuf = buf;
	for(i = 0; i < ts_panel_info->ts_para.vir_key_num; i++){
		sprintf(pBuf, __stringify(EV_KEY) ":%d:%d:%d:%d:%d\n", key[i].key_value, key[i].key_x, key[i].key_y, key[i].key_x_width, key[i].key_y_width);
		pBuf = buf + strlen(buf);
	}
	return sprintf(buf, "%s", buf);
}


static struct kobj_attribute virtual_keys_attr = {
    .attr = {
        .name = "virtualkeys.rda-i2c-touchscreen",
        .mode = S_IRUGO,
    },
    .show = &virtual_keys_show,
};

static struct attribute *properties_attrs[] = {
    &virtual_keys_attr.attr,
    NULL
};

static struct attribute_group properties_attr_group = {
    .attrs = properties_attrs,
};

static ssize_t ts_name_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
	return sprintf(buf, "%s\n", ts_panel_info->ts_para.name);
}

static struct kobj_attribute ts_name_attr = {
    .attr = {
        .name = "touchscreen-name",
        .mode = S_IRUGO,
    },
    .show = &ts_name_show,
};

static struct attribute *ts_name_prop_attrs[] = {
    &ts_name_attr.attr,
    NULL
};

static struct attribute_group ts_name_prop_attr_group = {
    .attrs = ts_name_prop_attrs,
};

static ssize_t ts_i2c_work_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n", ts_i2c_work);
}

static struct kobj_attribute ts_i2c_work_attr = {
    .attr = {
        .name = "ts-i2c-work",
        .mode = S_IRUGO,
    },
    .show = &ts_i2c_work_show,
};

static struct attribute *ts_i2c_work_prop_attrs[] = {
    &ts_i2c_work_attr.attr,
    NULL
};

static struct attribute_group ts_i2c_work_prop_attr_group = {
    .attrs = ts_i2c_work_prop_attrs,
};

static void virtual_keys_init(void)
{
    struct kobject *properties_kobj;
    int ret = 0;

    printk("%s\n",__func__);

    properties_kobj = kobject_create_and_add("board_properties", NULL);
    if (properties_kobj)
        ret = sysfs_create_group(properties_kobj,
                     &properties_attr_group);
    if (!properties_kobj || ret)
        pr_err("failed to create board_properties: virtual key\n");

    if (properties_kobj)
        ret = sysfs_create_group(properties_kobj,
                     &ts_name_prop_attr_group);

    if (!properties_kobj || ret)
        pr_err("failed to create ts_properties: ts name\n");

    if (properties_kobj)
        ret = sysfs_create_group(properties_kobj,
                     &ts_i2c_work_prop_attr_group);

    if (!properties_kobj || ret)
        pr_err("failed to create ts_properties: ts i2c work\n");

    kobject_uevent(properties_kobj, KOBJ_CHANGE);
}

#endif

static void rda_ts_up(struct rda_ts_data *ts)
{
#if defined(CONFIG_TOUCHSCREEN_RDA_VIRTUAL_KEY)
	int i = 0;

	if (!ts->bIsNormalMode) {
		for(i = 0; i < ts->panel_info->ts_para.vir_key_num; i++){
			if(ts->ts_key_down[i] && ts->ts_key_value[i]){
				input_event(ts->input_dev, EV_KEY,
						ts->ts_key_value[i], 0);
				ts->ts_key_down[i] = 0;
				ts->ts_key_value[i] = 0;
			}
		}
	}
#endif
	input_report_key(ts->input_dev, BTN_TOUCH, 0);
}

static void rda_ts_up_timer(unsigned int data)
{
	struct rda_ts_data *ts = (struct rda_ts_data *)data;

	rda_ts_up(ts);
	input_sync(ts->input_dev);
}

#if defined(CONFIG_TOUCHSCREEN_RDA_VIRTUAL_KEY)
void rda_ts_button_debug(struct rda_ts_data *ts,struct rda_ts_pos * rda_ts_data)
{
	struct rda_ts_vir_key * key = ts->panel_info->ts_para.vir_key;
	u16 num = ts->panel_info->ts_para.vir_key_num;
	int i = 0;

	for(i = 0; i < num; i++){
		if(rda_ts_data->x >= ((key + i)->key_x - (key + i)->key_x_width/2)
			&& rda_ts_data->x <= ((key + i)->key_x + (key + i)->key_x_width/2)
			&& rda_ts_data->y >= ((key + i)->key_y - (key + i)->key_y_width/2)
			&& rda_ts_data->y <= ((key + i)->key_y + (key + i)->key_y_width/2)){

			if ((key + i)->key_value == KEY_HOMEPAGE)
				pr_info("press HOME button\n");
			else if ((key + i)->key_value == KEY_BACK)
				pr_info("press BACK button\n");
			else if ((key + i)->key_value == KEY_MENU)
				pr_info("press MENU button\n");

		}
	}

	return ;
}

bool rda_ts_button(struct rda_ts_data *ts,struct rda_ts_pos * rda_ts_data)
{
	struct rda_ts_vir_key * key = ts->panel_info->ts_para.vir_key;
	u16 num = ts->panel_info->ts_para.vir_key_num;
	int i = 0;

	for(i = 0; i < num; i++){
		if(rda_ts_data->x >= ((key + i)->key_x - (key + i)->key_x_width/2)
			&& rda_ts_data->x <= ((key + i)->key_x + (key + i)->key_x_width/2)
			&& rda_ts_data->y >= ((key + i)->key_y - (key + i)->key_y_width/2)
			&& rda_ts_data->y <= ((key + i)->key_y + (key + i)->key_y_width/2)){
			if (0 == ts->ts_key_down[i]){

//				if ((key + i)->key_value == KEY_HOMEPAGE)
//					pr_info("press HOME button\n");
//				else if ((key + i)->key_value == KEY_BACK)
//					pr_info("press BACK button\n");
//				else if ((key + i)->key_value == KEY_MENU)
//					pr_info("press MENU button\n");
				input_event(ts->input_dev, EV_KEY,(key + i)->key_value, 1);
				input_report_key(ts->input_dev, BTN_TOUCH, 1);

				input_sync(ts->input_dev);
				ts->ts_key_down[i] = 1;
				ts->ts_key_value[i] = (key + i)->key_value;
				return true;
			}
		}
	}

	return false;
}
#endif

static void rda_report_distance(struct rda_ts_data *ts,
		int distance)
{
	ts->ps_distance = distance;
	input_report_abs(ts->ps_input_dev, ABS_DISTANCE, distance);
	input_sync(ts->ps_input_dev);
}

static void rda_report_event(struct rda_ts_data *ts,
			struct rda_ts_pos_data * ts_data)
{
	struct rda_ts_pos_data *ts_data_temp = ts_data;
	int i = 0;
	bool ret = false;
	int x, y;

	if (0 == ts_data_temp->point_num) {
		del_timer(&ts->rda_ts_timer);
		rda_ts_up(ts);
	} else {

		/* multi touch protocol */
		mod_timer(&ts->rda_ts_timer, jiffies + 1 * HZ);
		input_report_key(ts->input_dev, BTN_TOUCH, 1);
		for (i = 0; i < ts_data_temp->point_num; i++) {
			//we first scale the pos
			x = ts_data_temp->ts_position[i].x;
			y = ts_data_temp->ts_position[i].y;

			if (ts->bXneedScale) {
				x = (x * PANEL_XSIZE)/ts->panel_info->ts_para.x_max;
				ts_data_temp->ts_position[i].x = x;
			}

			if (ts->bYneedScale) {
				y = (y * PANEL_YSIZE)/ts->panel_info->ts_para.y_max;
				ts_data_temp->ts_position[i].y = y;
			}


#if defined(CONFIG_TOUCHSCREEN_RDA_VIRTUAL_KEY)

#if 0 /* for debug */
			if(ts->panel_info->ts_para.vir_key_num) {
				rda_ts_button_debug(ts,
					ts_data_temp->ts_position + i);
			}
#endif

			if(ts->panel_info->ts_para.vir_key_num && !ts->bIsNormalMode) {
				ret = rda_ts_button(ts,
					ts_data_temp->ts_position + i);
				if (ret)
					return;
			}
#endif
			//input_report_abs(input_dev, ABS_MT_TOUCH_MAJOR, 9);
			input_report_abs(ts->input_dev, ABS_MT_POSITION_X, x);
			input_report_abs(ts->input_dev, ABS_MT_POSITION_Y, y);
			input_report_abs(ts->input_dev, ABS_MT_PRESSURE, 128);
			input_mt_sync(ts->input_dev);
		}
	}

	input_sync(ts->input_dev);
}

#ifdef CONFIG_USE_TASKLET
static void rda_ts_work_func_task(unsigned long ts_dev)
{
	int ret;
	struct rda_ts_pos_data ts_data;
	struct rda_ts_data *ts = (struct rda_ts_data *)ts_dev;

	disable_irq_nosync(ts->ts_irq);
	ts_data.data_type = TS_DATA;
	ret = ts->panel_info->ops.ts_get_parse_data(ts->client,&ts_data);
	if ((ret == TS_OK) || (ret == TS_NOTOUCH)) {
		if(ts_data.data_type == TS_DATA) {
			rda_report_event(ts, &ts_data);
		} else {
			rda_report_distance(ts, ts_data.distance);
		}
	} else if (ret == TS_I2C_ERROR) {
		dev_err(&ts->client->dev, "ts i2c error, reset ts\n");
		ts->tasklet_pending = false;
		/* start a qeuee to restart ic */
		queue_work(rda_ts_wq, &ts->work);
		return;
	} else if (ret == TS_I2C_RETRY) {
		ts->tasklet_pending = false;
		enable_irq(ts->ts_irq);
		rda_dbg_ts("ts i2c retry\n");
		tasklet_schedule(&ts->rda_ts_tasklet);
		return;
	} else if (ret == TS_PACKET_ERROR) {
		dev_err(&ts->client->dev, "ts packet error\n");
	} else {
		dev_err(&ts->client->dev, "ts unknown ret %d\n", ret);
	}

	ts->tasklet_pending = false;
	enable_irq(ts->ts_irq);
}

static void rda_ts_work_recovery(struct work_struct *work)
{
	struct rda_ts_data *ts;

	ts = container_of(work, struct rda_ts_data, work);
	ts->panel_info->ops.ts_reset_chip(ts->client);
	ts->panel_info->ops.ts_start_chip(ts->client);
	enable_irq(ts->ts_irq);
}
#else
static void rda_ts_work_queue(struct work_struct *work)
{
	int ret;
	struct rda_ts_pos_data ts_data;
	struct rda_ts_data *ts;
	ts = container_of(work, struct rda_ts_data, work);
#ifdef TPD_PROC_DEBUG
	if(gsl_proc_flag == 1){
		goto schedule;
	}
#endif

	ts_data.data_type = TS_DATA;
	ret = ts->panel_info->ops.ts_get_parse_data(ts->client,&ts_data);
	if ((ret == TS_OK) || (ret == TS_NOTOUCH)) {
		if(ts_data.data_type == TS_DATA) {
			rda_report_event(ts, &ts_data);
		} else {
			rda_report_distance(ts, ts_data.distance);
		}
	} else if (ret == TS_I2C_ERROR) {
		dev_err(&ts->client->dev, "ts i2c error, reset ts\n");
		ts->panel_info->ops.ts_reset_chip(ts->client);
		ts->panel_info->ops.ts_start_chip(ts->client);
	} else if (ret == TS_PACKET_ERROR) {
		dev_err(&ts->client->dev, "ts packet error\n");
	} else {
		dev_err(&ts->client->dev, "ts unknown ret %d\n", ret);
	}

#ifdef TPD_PROC_DEBUG
schedule:
#endif
		enable_irq(ts->ts_irq);
}
#endif

static irqreturn_t rda_ts_irq_handler(int irq, void *dev_id)
{
	struct rda_ts_data *ts = dev_id;

#ifdef CONFIG_USE_TASKLET
	if (!ts->tasklet_pending) {
		tasklet_schedule(&ts->rda_ts_tasklet);
		ts->tasklet_pending = true;
	}
#else
	disable_irq_nosync(ts->ts_irq);
	if (!work_pending(&ts->work))
	    queue_work(rda_ts_wq, &ts->work);
#endif

	return IRQ_HANDLED;
}

#ifdef CONFIG_HAS_EARLYSUSPEND
static void rda_ts_early_suspend(struct early_suspend *h);
static void rda_ts_late_resume(struct early_suspend *h);
#endif
static int rda_ts_suspend(struct i2c_client *client, pm_message_t mesg);
static int rda_ts_resume(struct i2c_client *client);

static ssize_t rda_ts_enable_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct rda_ts_data*ts = dev_get_drvdata(dev);

	return sprintf(buf, "%d\n", ts->enable);
}

static ssize_t rda_ts_enable_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	struct rda_ts_data*ts = dev_get_drvdata(dev);
	int rv;
	int set;

	rv = kstrtoint(buf, 0, &set);
	if (rv < 0) {
		return rv;
	}

	set = !!set;
	if (ts->enable == set) {
		return count;
	}

	if (set) {
		if (ts->panel_info->ops.ts_switch_ps_mode) {
			rda_dbg_ts( "%s enable?%x,suspend?0x%x\n",
				__func__ ,ts->ps_enable,ts->ps_enable_when_suspend);
			if(ts->ps_enable && ts->ps_enable_when_suspend) {
				rda_dbg_ts("proximity enable : exit rda_ts_enable_store\n");
				ts->ps_enable_when_suspend = 0;
				return count;
			}
		}
		rda_ts_resume(ts->client);
		enable_irq(ts->ts_irq);
		if (ts->panel_info->ops.ts_switch_ps_mode) {
			if(proximity_need_open_after_resume == 2) {
				mutex_lock(&ts->ps_data_lock);
				if(ts->panel_info->ops.ts_switch_ps_mode(ts->client, 1)) {
					mutex_unlock(&ts->ps_data_lock);
					return count;
				} else {
					ts->ps_enable = 1;
					ts->ps_distance = INT_MAX;
				}
				mutex_unlock(&ts->ps_data_lock);
			}
			proximity_need_open_after_resume = 0;
		}
	} else {
		if (ts->panel_info->ops.ts_switch_ps_mode) {
			rda_dbg_ts( "%s enable?%x\n",__func__ ,ts->ps_enable);
			if(ts->ps_enable) {
				rda_dbg_ts("proximity enable : exit rda_ts_enable_store\n");
				ts->ps_enable_when_suspend = 1;
				return count ;
			}
		}
		disable_irq_nosync(ts->ts_irq);
		rda_ts_suspend(ts->client, PMSG_SUSPEND);
		if (ts->panel_info->ops.ts_switch_ps_mode) {
			proximity_need_open_after_resume = 1;
		}
	}

	ts->enable = set;

	return count;
}

static ssize_t rda_ts_show_vkey_num(struct device *dev,
			struct device_attribute *attr, char *buf)
{
	struct rda_ts_data *ts = dev_get_drvdata(dev);

	return sprintf(buf, "%d\n", ts->panel_info->ts_para.vir_key_num);
}

static DEVICE_ATTR(vkey, S_IWUSR | S_IWGRP | S_IRUGO, rda_ts_show_vkey_num, NULL);
static DEVICE_ATTR(enabled, S_IWUSR | S_IWGRP | S_IRUGO,
	rda_ts_enable_show, rda_ts_enable_store);

static struct attribute *rda_ts_attributes[] = {
	&dev_attr_vkey.attr,
	&dev_attr_enabled.attr,
	NULL
};
static const struct attribute_group rda_ts_attribute_group = {
	.name = "driver",
	.attrs = rda_ts_attributes,
};

static ssize_t psensor_enable_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	int en = 0;
	struct rda_ts_data *pdata = dev_get_drvdata(dev);

	if (pdata->panel_info->ops.ts_switch_ps_mode) {
		mutex_lock(&pdata->ps_data_lock);
		en = pdata->ps_enable;
		mutex_unlock(&pdata->ps_data_lock);
	}
	return sprintf(buf, "%d\n", en);
}

static ssize_t psensor_enable_store(struct device *dev,
		struct device_attribute *attr,
		const char *buf, size_t count)
{
	int en = 0;
	struct rda_ts_data *pdata = dev_get_drvdata(dev);

	if (sysfs_streq(buf, "1") && pdata->ps_enable == 0) {
		en = 1;
	}
	else if (sysfs_streq(buf, "0") && pdata->ps_enable == 1) {
		en = 0;
	} else {
		pr_err("%s : invalid value, pdata->ps_enable %d\n", __func__, pdata->ps_enable);
		return count;
	}

	if (pdata->panel_info->ops.ts_switch_ps_mode) {
		rda_dbg_ts("%s : en %d\n", __func__, en);

		mutex_lock(&pdata->ps_data_lock);
		if (pdata->panel_info->ops.ts_switch_ps_mode(pdata->client, en) < 0) {
			if (en && (proximity_need_open_after_resume == 1))
				proximity_need_open_after_resume = 2;
			pr_err("%s : switch ps mode %d fail, switch_ps_mode function 0x%p\n",
					__func__, en, pdata->panel_info->ops.ts_switch_ps_mode);
			mutex_unlock(&pdata->ps_data_lock);
			return count;
		} else {
			pdata->ps_enable = en;
			pdata->ps_distance = INT_MAX;
		}
		mutex_unlock(&pdata->ps_data_lock);
	} else {
		pr_err("%s : switch ps mode %d fail, switch_ps_mode function is NULL!\n",
			__func__, en);
	}
	return count;
}

static ssize_t psensor_distance_show(struct device *dev,
		struct device_attribute *attr,
		char *buf)
{
	int distance = 0;
	struct rda_ts_data *pdata = dev_get_drvdata(dev);

	if (pdata->panel_info->ops.ts_switch_ps_mode) {
		mutex_lock(&pdata->ps_data_lock);
		distance = pdata->ps_distance;
		mutex_unlock(&pdata->ps_data_lock);
	}
	return sprintf(buf, "%d\n", distance);
}

static ssize_t psensor_has_ps_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	int en = 0;
	struct rda_ts_data *pdata = dev_get_drvdata(dev);

	if (pdata->panel_info->ops.ts_switch_ps_mode) {
		en = 1;
	} else {
		en = 0;
	}

	return sprintf(buf, "%d\n", en);
}

static DEVICE_ATTR(enable, S_IWUSR | S_IWGRP | S_IRUGO,
		psensor_enable_show, psensor_enable_store);
static DEVICE_ATTR(distance, S_IWUSR | S_IWGRP | S_IRUGO,
		psensor_distance_show, NULL);
static DEVICE_ATTR(has_ps, S_IWUSR | S_IWGRP | S_IRUGO,
		psensor_has_ps_show, NULL);


static struct attribute *ps_attributes[] = {
	&dev_attr_enable.attr,
	&dev_attr_distance.attr,
	&dev_attr_has_ps.attr,
	NULL
};

static const struct attribute_group ps_attr_group = {
	.name = "driver",
	.attrs = ps_attributes,
};

static int rda_ts_probe(struct i2c_client *client,
			const struct i2c_device_id *id)
{
	struct rda_ts_data *ts;
	int ret = 0;
	unsigned long irqflags;
	struct rda_ts_device_data *pdata;
	static u8 i2c_tmp_data[3] = {0};
	int i = 0;
#ifdef _TGT_AP_GPIO_TP_INTPIN_PULLCTRL
	gpio_request(_TGT_AP_GPIO_TP_INTPIN_PULLCTRL, "touch screen int pin pullresistor ctrl");
	gpio_direction_output(_TGT_AP_GPIO_TP_INTPIN_PULLCTRL, 1);
	gpio_set_value(_TGT_AP_GPIO_TP_INTPIN_PULLCTRL, 1);
#endif
	rda_dbg_ts("rda_ts_probe, client %s on adapter %s %d\n",
		   client->name, client->adapter->name, client->adapter->nr);

	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
		dev_err(&client->dev, "rda_ts_probe: need I2C_FUNC_I2C\n");
		ret = -ENODEV;
		goto err_check_functionality_failed;
	}

	ts = kzalloc(sizeof(*ts), GFP_KERNEL);
	if (ts == NULL) {
		ret = -ENOMEM;
		goto err_alloc_data_failed;
	}
	if (!ts_panel_array){
		dev_err(&client->dev, "need ts panel array\n");
		goto err_add_rda_ts_panel;
	}

	if(ts_panel_array->next&&!rda_ts_probe_done){
		if(ts_panel_info->ops.ts_init_gpio)
			ts_panel_info->ops.ts_init_gpio();
		//check if i2c dev exist

		ret = rda_ts_i2c_read(client,0,i2c_tmp_data,1);
		if(ret > 0){
			if(ts_panel_info->ops.ts_get_chip_id)
				ret = ts_panel_info->ops.ts_get_chip_id(client);
		}
	}

	if(ret < 0){
		dev_err(&client->dev, "rda_ts_probe: No DEV\n");
		ret = -ENODEV;
		goto err_check_functionality_failed;
	}

	ts->panel_info = ts_panel_info;

	ts->bXneedScale = (PANEL_XSIZE != ts->panel_info->ts_para.x_max)
			? true : false;
	ts->bYneedScale = (PANEL_YSIZE != ts->panel_info->ts_para.y_max)
			? true : false;


	ts->bIsNormalMode = (rda_get_boot_mode() == BM_NORMAL) ? true : false;

	printk("rda ts res %dx%d lcd res %dx%d\n",
			ts->panel_info->ts_para.x_max,
			ts->panel_info->ts_para.y_max,
			PANEL_XSIZE, PANEL_YSIZE);

#ifdef TPD_PROC_DEBUG
	ddata = ts;
	gsl_config_proc = create_proc_entry(GSL_CONFIG_PROC_FILE, 0666, NULL);
	if (gsl_config_proc == NULL)
	{
		print_info("create_proc_entry %s failed\n", GSL_CONFIG_PROC_FILE);
	}
	else
	{
		gsl_config_proc->read_proc = gsl_config_read_proc;
		gsl_config_proc->write_proc = gsl_config_write_proc;
	}
	gsl_proc_flag = 0;
#endif

#ifdef CONFIG_USE_TASKLET
	tasklet_init(&ts->rda_ts_tasklet, rda_ts_work_func_task,
			(unsigned long)ts);
	INIT_WORK(&ts->work, rda_ts_work_recovery);
	ts->tasklet_pending = false;
#else
	INIT_WORK(&ts->work, rda_ts_work_queue);
#endif

	ts->client = client;
	i2c_set_clientdata(client, ts);
	pdata = client->dev.platform_data;

	ts->input_dev = input_allocate_device();
	if (ts->input_dev == NULL) {
		ret = -ENOMEM;
		dev_err(&client->dev, "failed to allocate input device\n");
		goto err_input_dev_alloc_failed;
	}
	ts->input_dev->name = "rda-i2c-touchscreen";
	set_bit(EV_SYN, ts->input_dev->evbit);
	set_bit(EV_KEY, ts->input_dev->evbit);
	set_bit(EV_REP, ts->input_dev->evbit);

	set_bit(BTN_TOUCH, ts->input_dev->keybit);
	set_bit(EV_ABS, ts->input_dev->evbit);
	set_bit(INPUT_PROP_DIRECT, ts->input_dev->propbit);

	input_set_abs_params(ts->input_dev, ABS_MT_POSITION_X, 0,
			PANEL_XSIZE, 0, 0);
	input_set_abs_params(ts->input_dev, ABS_MT_POSITION_Y, 0,
			PANEL_YSIZE, 0, 0);
	input_set_abs_params(ts->input_dev, ABS_MT_TOUCH_MAJOR, 0, 0xFF, 0, 0);
	input_set_abs_params(ts->input_dev, ABS_MT_PRESSURE,
		     0, 0xFF, 0, 0);

	for(i = 0; i < ts->panel_info->ts_para.vir_key_num; i++)
		set_bit(ts->panel_info->ts_para.vir_key[i].key_value,
			ts->input_dev->keybit);

#if defined(CONFIG_TOUCHSCREEN_RDA_VIRTUAL_KEY)
	if (ts->panel_info->ts_para.vir_key_num)
		virtual_keys_init();
#endif
	ret = input_register_device(ts->input_dev);
	if (ret) {
		dev_err(&client->dev, "fail to register %s input device\n",
			   ts->input_dev->name);
		goto err_input_register_device_failed;
	}

	input_set_drvdata(ts->input_dev, ts);

	ret = sysfs_create_group(&ts->input_dev->dev.kobj,
			&rda_ts_attribute_group);
	if (ret < 0)
	{
		dev_err(&client->dev,
			"%s:could not create sysfs group for rda ts\n",
			__func__);
		goto err_input_register_device_failed;
	}
	kobject_uevent(&ts->input_dev->dev.kobj, KOBJ_CHANGE);

	if(ts->panel_info->ops.ts_init_gpio)
		ts->panel_info->ops.ts_init_gpio();

	//check if ts i2c work
	ret = rda_ts_i2c_read(client,0,i2c_tmp_data,1);
	if(ret > 0){
		if(ts->panel_info->ops.ts_get_chip_id)
			ret = ts->panel_info->ops.ts_get_chip_id(client);
	}

	if(ret < 0){
		dev_err(&client->dev, "rda_ts_probe: ts i2c not work\n");
		ts_i2c_work = false;
	}

	ts->panel_info->ops.ts_init_chip(client);

	mutex_init(&ts->ps_data_lock);
	ts->ps_input_dev = input_allocate_device();
	if (!ts->ps_input_dev) {
		ret = -ENOMEM;
		pr_err("%s : failed to allocate ps input device\n", __func__);
		goto err_ps_input_allocate_device;
	}

	ts->ps_input_dev->name = PS_NAME;
	set_bit(EV_ABS, ts->ps_input_dev->evbit);
	input_set_abs_params(ts->ps_input_dev, ABS_DISTANCE, 0, 1, 0, 0);
	ret = input_register_device(ts->ps_input_dev);
	if (ret) {
		pr_err("%s : fail to register ps input device\n", __func__);
		goto err_ps_input_register_device;
	}

	ret = sysfs_create_group(&(ts->ps_input_dev->dev.kobj), &ps_attr_group);
	if (ret) {
		dev_err(&ts->ps_input_dev->dev, "create ps device file failed!\n");
		ret = -EINVAL;
		goto err_create_sysfs;
	}
	input_set_drvdata(ts->ps_input_dev, ts);
	ts->ps_enable = 0;
	ts->ps_enable_when_suspend = 0;
	ts->ps_distance = INT_MAX;
	kobject_uevent(&(ts->ps_input_dev->dev.kobj), KOBJ_CHANGE);

	ts->enable = 1;

#ifdef CONFIG_HAS_EARLYSUSPEND
	ts->early_suspend.level = EARLY_SUSPEND_LEVEL_BLANK_SCREEN + 1;
	ts->early_suspend.suspend = rda_ts_early_suspend;
	ts->early_suspend.resume = rda_ts_late_resume;
	register_early_suspend(&ts->early_suspend);
#endif
#ifdef TS_FIRMWARE_UPDATE
	if(ts_panel_info->ops.ts_register_utils_funcs) {
		printk("%s : ts_register_utils_funcs\n", __func__);
		ts_panel_info->ops.ts_register_utils_funcs(client);
	}
#endif
	rda_ts_probe_done = true;

	if (pdata->use_irq) {
		irqflags = pdata->irqflags |
			ts->panel_info->ts_para.irqTriggerMode;
		ts->ts_irq = gpio_to_irq(ts->panel_info->ts_para.gpio_irq);
		ret = request_irq(ts->ts_irq, rda_ts_irq_handler,
				irqflags, client->name, ts);
		printk("rda_ts_probe, irq gpio is %d, irq flags %x\n",
				ts->panel_info->ts_para.gpio_irq,
				(int)irqflags);
		if (ret == 0) {
			ts->use_irq = 1;
		} else {
			printk("rda ts request_irq failed\n");
		}
	}
	printk("start %s in %s mode\n",
		 ts->input_dev->name, ts->use_irq ? "interrupt" : "polling");

	init_timer(&ts->rda_ts_timer);
	ts->rda_ts_timer.expires = jiffies + 1 * HZ;
	ts->rda_ts_timer.data = (unsigned long)ts;
	ts->rda_ts_timer.function = (void (*)(unsigned long))(rda_ts_up_timer);

	return 0;

	sysfs_remove_group(&ts->ps_input_dev->dev.kobj, &ps_attr_group);
err_create_sysfs:
	input_unregister_device(ts->ps_input_dev);
err_ps_input_register_device:
	input_free_device(ts->ps_input_dev);
err_ps_input_allocate_device:

err_input_register_device_failed:
	input_free_device(ts->input_dev);
err_input_dev_alloc_failed:
err_add_rda_ts_panel:
	kfree(ts);
err_alloc_data_failed:
err_check_functionality_failed:
	return ret;
}

static int rda_ts_remove(struct i2c_client *client)
{
	struct rda_ts_data *ts = i2c_get_clientdata(client);

	if (ts->panel_info->ops.ts_switch_ps_mode) {
		sysfs_remove_group(&ts->ps_input_dev->dev.kobj, &ps_attr_group);
		input_unregister_device(ts->ps_input_dev);
		input_free_device(ts->ps_input_dev);
	}

#ifdef TPD_PROC_DEBUG
	if(gsl_config_proc!=NULL)
		remove_proc_entry(GSL_CONFIG_PROC_FILE, NULL);
#endif

#ifdef CONFIG_HAS_EARLYSUSPEND
	unregister_early_suspend(&ts->early_suspend);
#endif

#ifdef CONFIG_USE_TASKLET
	tasklet_kill(&ts->rda_ts_tasklet);
#endif
#ifdef TS_FIRMWARE_UPDATE
	if(ts_panel_info->ops.ts_unregister_utils_funcs) {
		printk("%s : ts_unregister_utils_funcs\n", __func__);
		ts_panel_info->ops.ts_unregister_utils_funcs(client);
	}
#endif
	input_unregister_device(ts->input_dev);
	kfree(ts);
	return 0;
}

static int rda_ts_suspend(struct i2c_client *client, pm_message_t mesg)
{
	struct rda_ts_data *ts = i2c_get_clientdata(client);

	flush_workqueue(rda_ts_wq);

#ifdef TPD_PROC_DEBUG
	if(gsl_proc_flag == 1){
		return 0;
	}
#endif

	if (ts->panel_info->ops.ts_suspend) {
		ts->panel_info->ops.ts_suspend(client, mesg);
	}

	return 0;
}

static int rda_ts_resume(struct i2c_client *client)
{
	struct rda_ts_data *ts = i2c_get_clientdata(client);

#ifdef TPD_PROC_DEBUG
	if(gsl_proc_flag == 1){
		return 0;
	}
#endif

	if (ts->panel_info->ops.ts_resume) {
		ts->panel_info->ops.ts_resume(client);
	}

	return 0;
}

#ifdef CONFIG_HAS_EARLYSUSPEND
static void rda_ts_early_suspend(struct early_suspend *h)
{
	struct rda_ts_data *ts;
	ts = container_of(h, struct rda_ts_data, early_suspend);

	if (ts->panel_info->ops.ts_switch_ps_mode) {
		rda_dbg_ts( "%s enable?%x\n",__func__ ,ts->ps_enable );
		if(ts->ps_enable) {
			rda_dbg_ts("proximity enable : exit rda_ts_early_suspend");
			ts->ps_enable_when_suspend = 1;
			return;
		}
	}

	disable_irq_nosync(ts->ts_irq);
	rda_ts_suspend(ts->client, PMSG_SUSPEND);

	if (ts->panel_info->ops.ts_switch_ps_mode) {
		proximity_need_open_after_resume = 1;
	}
}

static void rda_ts_late_resume(struct early_suspend *h)
{
	struct rda_ts_data *ts;
	ts = container_of(h, struct rda_ts_data, early_suspend);

	if (ts->panel_info->ops.ts_switch_ps_mode) {
		rda_dbg_ts( "%s enable?%x,suspend?0x%x\n",
			__func__ ,ts->ps_enable,ts->ps_enable_when_suspend);
		if(ts->ps_enable && ts->ps_enable_when_suspend) {
			rda_dbg_ts("proximity enable : exit rda_ts_late_resume");
			ts->ps_enable_when_suspend = 0;
			return;
		}
	}

	rda_ts_resume(ts->client);
	enable_irq(ts->ts_irq);

	if (ts->panel_info->ops.ts_switch_ps_mode) {
		if(proximity_need_open_after_resume == 2) {
			mutex_lock(&ts->ps_data_lock);
			if(ts->panel_info->ops.ts_switch_ps_mode(ts->client, 1)) {
				mutex_unlock(&ts->ps_data_lock);
				return;
			} else {
				ts->ps_enable = 1;
				ts->ps_distance = INT_MAX;
			}
			mutex_unlock(&ts->ps_data_lock);
		}
		proximity_need_open_after_resume = 0;
	}
}
#endif

static struct rda_ts_device_data rda_ts_data[] = {
	{
	 .use_irq  = 1,
	 .irqflags = IRQF_SHARED,
	 },
};

static const struct i2c_device_id rda_ts_id[] = {
	{RDA_TS_DRV_NAME, 0},
	{}
};

static struct i2c_driver rda_ts_driver = {
	.probe = rda_ts_probe,
	.remove = rda_ts_remove,
	.id_table = rda_ts_id,
	.driver = {
		.name = RDA_TS_DRV_NAME,
	},
};


static int __init rda_ts_init(void)
{
	int ret = 0;
	struct i2c_board_info i2c_dev_ts = {
		I2C_BOARD_INFO(RDA_TS_DRV_NAME, 0),
		.platform_data = rda_ts_data,
	};
	struct i2c_adapter *adapter;
	struct rda_ts_panel_array *ts_array;

#ifdef FT6X06_MODEL
	if (rda_ts_add_panel_ft6x06()){
		pr_err("%s, rda_ts_add_panel failed\n", __func__);
		return -ENODEV;
	}
#endif
#ifdef MSG2133_MODEL
	if (rda_ts_add_panel_msg2133()){
		pr_err("%s, rda_ts_add_panel failed\n", __func__);
		return -ENODEV;
	}
#endif
#ifdef GTP868_MODEL
	if (rda_ts_add_panel_gtp868()){
		pr_err("%s, rda_ts_add_panel failed\n", __func__);
		return -ENODEV;
	}
#endif
#ifdef GTP960_MODEL
	if (rda_ts_add_panel_gtp960()){
		pr_err("%s, rda_ts_add_panel failed\n", __func__);
		return -ENODEV;
	}
#endif
#ifdef ICN831X_MODEL
	if (rda_ts_add_panel_icn831x()){
		pr_err("%s, rda_ts_add_panel failed\n", __func__);
		return -ENODEV;
	}
#endif
#ifdef IT7252_MODEL
	if (rda_ts_add_panel_it7252()){
		pr_err("%s, rda_ts_add_panel failed\n", __func__);
		return -ENODEV;
	}
#endif
#ifdef GSL168X_MODEL
	if (rda_ts_add_panel_gsl168x()){
		pr_err("%s, rda_ts_add_panel failed\n", __func__);
		return -ENODEV;
	}
#endif
#ifdef NT11004_MODEL
	if (rda_ts_add_panel_nt11004()){
		pr_err("%s, rda_ts_add_panel failed\n", __func__);
		return -ENODEV;
	}
#endif
#ifdef EKT2527_MODEL
	if (rda_ts_add_panel_elan()){
		pr_err("%s, rda_ts_add_panel failed\n", __func__);
		return -ENODEV;
	}
#endif

	if(!ts_panel_array)
	{
		pr_err("%s, no ts register!\n", __func__);
		return -ENODEV;
	}

	ts_panel_info = ts_panel_array->panel_info;

	if (!ts_panel_info){
		pr_err("%s, need rda_ts_data\n", __func__);
		return -ENODEV;
	}

	rda_ts_wq = create_singlethread_workqueue("rda_ts_wq");
	if (!rda_ts_wq)
		return -ENOMEM;

	ts_array = ts_panel_array;
	adapter = i2c_get_adapter(_TGT_AP_I2C_BUS_ID_TS);
	if (!adapter) {
		pr_err("%s, cannot get i2c adapter %d\n",
			__func__, _TGT_AP_I2C_BUS_ID_TS);
		return -ENODEV;
	}
	ret = i2c_add_driver(&rda_ts_driver);
	if(ret != 0)
		pr_err("%s, i2c_add_driver failed\n", __func__);

	i2c_dev_ts.addr = ts_array->panel_info->ts_para.i2c_addr;
	i2c_new_device(adapter, &i2c_dev_ts);
	pr_info("%s, i2c_add_driver done?%d\n", __func__,rda_ts_probe_done);
	while(!rda_ts_probe_done)
	{
		ts_array = ts_array->next ;
		if(!ts_array->next)
			rda_ts_probe_done = true;
		ts_panel_info = ts_array->panel_info;
		//	i2c_del_adapter(adapter);
		//pr_err("%s, del adapter \n", __func__);
		//	adapter = i2c_get_adapter(_TGT_AP_I2C_BUS_ID_TS);
		i2c_dev_ts.addr = ts_array->panel_info->ts_para.i2c_addr;
		i2c_new_device(adapter, &i2c_dev_ts);
	}


	pr_info("rda_ts %s initialized, at i2c bus %d addr 0x%02x\n",
		ts_panel_info->ts_para.name,
		_TGT_AP_I2C_BUS_ID_TS,
		ts_panel_info->ts_para.i2c_addr);

	return ret;
}

static void __exit rda_ts_exit(void)
{
	i2c_del_driver(&rda_ts_driver);
	if (rda_ts_wq)
		destroy_workqueue(rda_ts_wq);
}

module_init(rda_ts_init);
module_exit(rda_ts_exit);

MODULE_DESCRIPTION("RDA touchscreen driver");
MODULE_LICENSE("GPL");

