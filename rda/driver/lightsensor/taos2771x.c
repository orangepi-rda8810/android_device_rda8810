/********************************************************************************
*										*
*   File Name:    tmd2771.c							*
*   Description:   Linux device driver for Taos ambient light and		*
*   proximity sensors.								*
   Author:         John Koshi							*
*   History:	09/16/2009 - Initial creation					*
*				10/09/2009 - Triton version			*
*				12/21/2009 - Probe/remove mode			*
*				02/07/2010 - Add proximity			*
*										*
*******************************************************************************/
#include <linux/module.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/i2c.h>
#include <linux/mutex.h>
#include <linux/kdev_t.h>
#include <linux/fs.h>
#include <linux/input.h>
#include <linux/workqueue.h>
#include <linux/irq.h>
#include <linux/delay.h>
#include <linux/sched.h>
#include <linux/kthread.h>
#include <linux/errno.h>
#include <linux/wakelock.h>
#include <linux/interrupt.h>
#include <linux/gpio.h>
#include   <linux/fs.h>
#include  <asm/uaccess.h>
#include <plat/devices.h>
/* Driver Settings */
#define CONFIG_STK_PS_ALS_USE_CHANGE_THRESHOLD
#ifdef CONFIG_STK_PS_ALS_USE_CHANGE_THRESHOLD
#define TAOS_ALS_CHANGE_THD	20	/* The threshold to trigger ALS interrupt, unit: lux */
#endif	/* #ifdef CONFIG_STK_PS_ALS_USE_CHANGE_THRESHOLD */

#include "stk3x1x.h"
#include "tgt_ap_board_config.h"

#define DRIVER_VERSION  "3.4.9"

#define TAOS_SENSOR_TMD2771_NAME		"tmd2771x"
#define TAOS_DEVICE_NAME                TAOS_SENSOR_TMD2771_NAME
#define TAOS_DEVICE_ID                  TAOS_SENSOR_TMD2771_NAME
#define TAOS_INPUT_NAME   		TAOS_SENSOR_TMD2771_NAME
#define TAOS_SENSOR_INFO			"1.0"

#define TAOS_MAX_NUM_DEVICES            1
#define TAOS_MAX_DEVICE_REGS            32
#define I2C_MAX_ADAPTERS                9

#define STK_INT_PS_MODE			1	/* 1, 2, or 3	*/

// TRITON register offsets
#define TAOS_TRITON_CNTRL               0x00
#define TAOS_TRITON_ALS_TIME            0X01
#define TAOS_TRITON_PRX_TIME            0x02
#define TAOS_TRITON_WAIT_TIME           0x03
#define TAOS_TRITON_ALS_MINTHRESHLO     0X04
#define TAOS_TRITON_ALS_MINTHRESHHI     0X05
#define TAOS_TRITON_ALS_MAXTHRESHLO     0X06
#define TAOS_TRITON_ALS_MAXTHRESHHI     0X07
#define TAOS_TRITON_PRX_MINTHRESHLO     0X08
#define TAOS_TRITON_PRX_MINTHRESHHI     0X09
#define TAOS_TRITON_PRX_MAXTHRESHLO     0X0A
#define TAOS_TRITON_PRX_MAXTHRESHHI     0X0B
#define TAOS_TRITON_INTERRUPT           0x0C
#define TAOS_TRITON_PRX_CFG             0x0D
#define TAOS_TRITON_PRX_COUNT           0x0E
#define TAOS_TRITON_GAIN                0x0F
#define TAOS_TRITON_REVID               0x11
#define TAOS_TRITON_CHIPID              0x12
#define TAOS_TRITON_STATUS              0x13
#define TAOS_TRITON_ALS_CHAN0LO         0x14
#define TAOS_TRITON_ALS_CHAN0HI         0x15
#define TAOS_TRITON_ALS_CHAN1LO         0x16
#define TAOS_TRITON_ALS_CHAN1HI         0x17
#define TAOS_TRITON_PRX_LO              0x18
#define TAOS_TRITON_PRX_HI              0x19
#define TAOS_TRITON_TEST_STATUS         0x1F

// Triton cmd reg masks
#define TAOS_TRITON_CMD_REG             0X80
#define TAOS_TRITON_CMD_AUTO            0x10
#define TAOS_TRITON_CMD_BYTE_RW         0x00
#define TAOS_TRITON_CMD_SPL_FN          0x60
#define TAOS_TRITON_CMD_PROX_INTCLR     0X05
#define TAOS_TRITON_CMD_ALS_INTCLR      0X06
#define TAOS_TRITON_CMD_INTCLR		0X07

// Triton cntrl reg masks
#define TAOS_TRITON_CNTL_PROX_INT_ENBL  0X20
#define TAOS_TRITON_CNTL_ALS_INT_ENBL   0X10
#define TAOS_TRITON_CNTL_WAIT_TMR_ENBL  0X08
#define TAOS_TRITON_CNTL_PROX_DET_ENBL  0X04
#define TAOS_TRITON_CNTL_SENS_ENBL	0x0F
#define TAOS_TRITON_CNTL_ADC_ENBL       0x02
#define TAOS_TRITON_CNTL_PWRON          0x01

// Triton status reg masks
#define TAOS_TRITON_STATUS_ADCVALID     0x01

// lux constants
#define TAOS_MAX_LUX                    10000
#define TAOS_FILTER_DEPTH               3

#define ALS_NAME "lightsensor-level"
#define PS_NAME "proximity"
#define MIN_ALS_POLL_DELAY_NS	110000000

//static int wake_lock_hold = 0;
// per-device data
struct taos_data {
	struct i2c_client *client;
	//struct cdev cdev;
	struct mutex date_lock;
	struct semaphore update_lock;
	struct wake_lock taos_wake_lock;
	struct mutex io_lock;
	struct input_dev *ps_input_dev;
	struct wake_lock ps_wakelock;
	struct hrtimer ps_timer;
	struct work_struct taos_ps_work;
	struct workqueue_struct *taos_ps_wq;
	struct wake_lock ps_nosuspend_wl;
	struct input_dev *als_input_dev;
	ktime_t ps_poll_delay;
	ktime_t als_poll_delay;
	struct work_struct taos_als_work;
	struct hrtimer als_timer;
	struct workqueue_struct *taos_als_wq;
	int working;
	bool ps_enabled;
	bool als_enabled;
	u32   calibrate_target;
	u16   als_time;
	u16   scale_factor;
	u16   gain_trim;
	u8    filter_history;
	u8    filter_count;
	u8    gain;
	u16	 prox_threshold_hi;
	u16   prox_threshold_lo;
	u16	 als_threshold_hi;
	u16   als_threshold_lo;
	u8	prox_int_time;
	u8	prox_adc_time;
	u8	prox_wait_time;
	u8	prox_intr_filter;
	u8	prox_config;
	u8	prox_pulse_cnt;
	u8	prox_gain;
	u8   reg_store;
};


static int taos_probe(struct i2c_client *clientp,
		      const struct i2c_device_id *idp);
static int taos_remove(struct i2c_client *client);
static int taos_suspend(struct device *dev);
static int taos_resume(struct device *dev);
static int taos_get_lux(struct taos_data *ps_data);
static int sensor_on(struct taos_data *ps_data);
static int sensor_off(struct taos_data *ps_data);
static int taos_als_power_on(struct taos_data *ps_data);
static int taos_als_power_off(struct taos_data *ps_data);
static int taos_ps_power_on(struct taos_data *ps_data);
static int taos_ps_power_off(struct taos_data *ps_data);
static int32_t taos2771x_enable_ps(struct taos_data *ps_data, uint8_t enable);
static int32_t taos2771x_enable_als(struct taos_data *ps_data, uint8_t enable);
static uint16_t taos_prox_get_data(struct taos_data *ps_data);
static uint8_t taos_prox_get_flag(struct taos_data *ps_data);

DECLARE_WAIT_QUEUE_HEAD(waitqueue_read);
// device configuration
static u32 calibrate_target_param = 300000;
static u16 als_time_param = 200;
static u16 scale_factor_param = 1;
static u16 gain_trim_param = 512;
static u8 filter_history_param = 3;
static u8 gain_param = 2;


static u16 prox_threshold_hi_param = 920;
static u16 prox_threshold_lo_param = 700;

static u16 als_threshold_hi_param = 3000;
static u16 als_threshold_lo_param = 10;
static u8 prox_int_time_param = 0xEE;	//50ms
static u8 prox_adc_time_param = 0xFF;
static u8 prox_wait_time_param = 0xEE;
static u8 prox_intr_filter_param = 0x11;//0x13
static u8 prox_config_param = 0x00;
static u8 prox_pulse_cnt_param = 0x02;//0x08;
static u8 prox_gain_param = 0x62;//0x22;



// lux time scale
struct time_scale_factor {
	u16 numerator;
	u16 denominator;
	u16 saturation;
};
struct time_scale_factor TritonTime = { 1, 0, 0 };

struct time_scale_factor *lux_timep = &TritonTime;

// gain table
u8 taos_triton_gain_table[] = { 1, 8, 16, 120 };

// lux data
struct lux_data {
	u16 ratio;
	u16 clear;
	u16 ir;
};

struct lux_data TritonFN_lux_data[] = {
	{9830, 8320, 15360},
	{12452, 10554, 22797},
	{14746, 6234, 11430},
	{17695, 3968, 6400},
	{0, 0, 0}
};

struct lux_data *lux_tablep = TritonFN_lux_data;
static int lux_history[TAOS_FILTER_DEPTH] = { -ENODATA, -ENODATA, -ENODATA };

static int rda_lightsensor_i2c_write(struct i2c_client *client, u8 addr, u8 *pdata, int datalen)
{
	int ret = 0;
	u8 tmp_buf[128];
	unsigned int bytelen = 0;
	if (datalen > 125)
	{
		return -1;
	}
	tmp_buf[0] = addr;
	bytelen++;
	if (datalen != 0 && pdata != NULL)
	{
		memcpy(&tmp_buf[bytelen], pdata, datalen);
		bytelen += datalen;
	}
	ret = i2c_master_send(client, tmp_buf, bytelen);
	return ret;
}

int rda_lightsensor_i2c_read(struct i2c_client *client, u8 addr, u8 *pdata, unsigned int datalen)
{
	int ret = 0;

	if (datalen > 126)
	{
		return -1;
	}

	ret = rda_lightsensor_i2c_write(client, addr, NULL, 0);

	if (ret < 0)
	{
		return ret;
	}

	return i2c_master_recv(client, pdata, datalen);
}

static int taos_read_byte(struct i2c_client *client, u8 reg)
{
	u8 ret;

	reg &= ~TAOS_TRITON_CMD_SPL_FN;
	reg |= TAOS_TRITON_CMD_REG | TAOS_TRITON_CMD_BYTE_RW;

	rda_lightsensor_i2c_read(client, reg, &ret,1);

	return ret;
}

static int taos_write_byte(struct i2c_client *client, u8 reg, u8 data)
{
	s32 ret;

	reg &= ~TAOS_TRITON_CMD_SPL_FN;
	reg |= TAOS_TRITON_CMD_REG | TAOS_TRITON_CMD_BYTE_RW;

	ret = rda_lightsensor_i2c_write(client, reg, &data,1);

	return (int)ret;
}
static int taos_resume(struct device *dev)
{
	struct taos_data *ps_data = dev_get_drvdata(dev);
	int ret = -1;
	int32_t near_far_state;
	if (ps_data->ps_enabled)
	{
		near_far_state =taos_prox_get_flag(ps_data);
		input_report_abs(ps_data->ps_input_dev, ABS_DISTANCE, near_far_state);
		input_sync(ps_data->ps_input_dev);
		wake_lock_timeout(&ps_data->ps_wakelock, 3*HZ);
	}
	else
	{
		if (ps_data->working == 1)
		{
			ps_data->working = 0;
			if ((ret = taos_write_byte(ps_data->client, TAOS_TRITON_CNTRL,ps_data->reg_store)) < 0)
			{
				printk(KERN_ERR"TAOS: write byte_data failed in ioctl als_off\n");
				return (ret);
			}
		}
	}
	return ret;
}

//suspend
static int taos_suspend(struct device *dev)
{
	struct taos_data *ps_data = dev_get_drvdata(dev);
	u8 reg_val = 0;
	int ret = -1;
	if (ps_data->ps_enabled)
	{
		return 0;
	}
	else
	{
		ps_data->working = 1;
		if ((ps_data->reg_store = taos_read_byte(ps_data->client, TAOS_TRITON_CNTRL)) < 0)
		{
			printk("TAOS: read byte is failed in suspend\n");
			return ret;
		}
		if ((ret =taos_write_byte(ps_data->client, TAOS_TRITON_CNTRL,reg_val)) < 0)
		{
			printk(KERN_ERR "TAOS: write byte failed in taos_suspend\n");
			return (ret);
		}
	}
	return ret;
}
static int sensor_on(struct taos_data *ps_data)
{
	int ret = 0;
	int i = 0;
	for (i = 0; i < TAOS_FILTER_DEPTH; i++) {
		lux_history[i] = -ENODATA;
	}
	/*ALS interrupt clear */
	if ((ret =
	     (i2c_smbus_write_byte
	      (ps_data->client,
	       (TAOS_TRITON_CMD_REG | TAOS_TRITON_CMD_SPL_FN |
		TAOS_TRITON_CMD_ALS_INTCLR)))) < 0) {
		printk(KERN_ERR
		       "TAOS: i2c_smbus_write_byte failed in ioctl als_on\n");
		return (ret);
	}
	if ((ret =
	     taos_write_byte(ps_data->client,
			     TAOS_TRITON_ALS_TIME,//0xee
			     ps_data->prox_int_time)) < 0) {
		printk(KERN_ERR
		       "TAOS: write als_time failed in ioctl prox_on\n");
		return (ret);
	}
	if ((ret =
	     taos_write_byte(ps_data->client,
			     TAOS_TRITON_PRX_TIME,//0xff
			     ps_data->prox_adc_time)) < 0) {
		printk(KERN_ERR
		       "TAOS: write prox_time failed in ioctl prox_on\n");
		return (ret);
	}
	if ((ret =
	     taos_write_byte(ps_data->client,
			     TAOS_TRITON_WAIT_TIME,//0xff
			     ps_data->prox_wait_time)) < 0) {
		printk(KERN_ERR
		       "TAOS: write wait_time failed in ioctl prox_on\n");
		return (ret);
	}
	if ((ret =
	     taos_write_byte(ps_data->client,
			     TAOS_TRITON_INTERRUPT,//0x11
			     ps_data->prox_intr_filter)) < 0) {
		printk(KERN_ERR
		       "TAOS: write interrupt failed in ioctl prox_on\n");
		return (ret);
	}
	if ((ret =
	     taos_write_byte(ps_data->client,
			     TAOS_TRITON_PRX_CFG,//0
			     ps_data->prox_config)) < 0) {
		printk(KERN_ERR
		       "TAOS: write prox_config failed in ioctl prox_on\n");
		return (ret);
	}
	if ((ret =
	     taos_write_byte(ps_data->client,
			     TAOS_TRITON_PRX_CFG,//
			     ps_data->prox_config)) < 0) {
		printk(KERN_ERR
		       "TAOS: write prox_config failed in ioctl prox_on\n");
		return (ret);
	}
	if ((ret =
	     taos_write_byte(ps_data->client,
			     TAOS_TRITON_PRX_COUNT,//0x08
			     ps_data->prox_pulse_cnt)) < 0) {
		printk(KERN_ERR
		       "TAOS: write prox_config failed in ioctl prox_on\n");
		return (ret);
	}
	if ((ret =
	     taos_write_byte(ps_data->client,//0x01
			     TAOS_TRITON_GAIN, ps_data->prox_gain)) < 0) {
		printk(KERN_ERR
		       "TAOS: write prox_config failed in ioctl prox_on\n");
		return (ret);
	}
	if ((ret =
	     taos_write_byte(ps_data->client,
			     TAOS_TRITON_CNTRL,
			     TAOS_TRITON_CNTL_SENS_ENBL)) < 0) {
		printk(KERN_ERR
		       "TAOS: write prox_config failed in ioctl prox_on\n");
		return (ret);
	}
	return 0;
}
static int sensor_off(struct taos_data *ps_data)
{
	int ret = 0;
	if ((ret =
	     taos_write_byte(ps_data->client,
			     TAOS_TRITON_CNTRL, 0x00)) < 0) {
		printk(KERN_ERR
		       "TAOS: write sensor_off failed in ioctl prox_on\n");
		return (ret);
	}
	return ret;
}

//static int taos_sensors_als_on(void)
static int taos_als_power_on(struct taos_data *ps_data)
{
	int ret = 0, i = 0;
	u8 itime = 0, reg_val = 0, reg_cntrl = 0;
	for (i = 0; i < TAOS_FILTER_DEPTH; i++)
		lux_history[i] = -ENODATA;
	/*ALS_INTERUPTER IS CLEAR */
	if ((ret =
	     (i2c_smbus_write_byte
	      (ps_data->client,
	       (TAOS_TRITON_CMD_REG | TAOS_TRITON_CMD_SPL_FN |
		TAOS_TRITON_CMD_ALS_INTCLR)))) < 0) {
		printk(KERN_ERR
		       "TAOS: i2c_smbus_write_byte failed in ioctl als_on\n");
		return (ret);
	}
	itime = (((ps_data->als_time / 50) * 18) - 1);
	itime = (~itime);
	if ((ret =
	     taos_write_byte(ps_data->client, TAOS_TRITON_ALS_TIME,
			     itime)) < -1) {
		printk(KERN_ERR "%s: write the als time is failed\n", __func__);
		return ret;
	}
	if ((ret =
	     taos_write_byte(ps_data->client, TAOS_TRITON_INTERRUPT,
			     ps_data->prox_intr_filter)) < 0) {
		printk(KERN_ERR "%s: write the als time is failed\n", __func__);
		return ret;
	}
	if ((reg_val =
	     taos_read_byte(ps_data->client, TAOS_TRITON_GAIN)) < 0) {
		printk(KERN_ERR "%s: read the als gain is failed\n", __func__);
		return ret;
	}
	reg_val = reg_val & 0xFC;
	reg_val = reg_val | (ps_data->gain & 0x03);
	if ((ret =
	     taos_write_byte(ps_data->client, TAOS_TRITON_GAIN,
			     reg_val)) < 0) {
		printk(KERN_ERR "%s: write the als gain is failed\n", __func__);
		return ret;
	}
	/*if ((reg_cntrl =
	     taos_read_byte(taos_datap->client, TAOS_TRITON_CNTRL)) < 0) {
		printk(KERN_ERR
		       "%s:  taos_read_byte TAOS_TRITON_CNTRL is failed\n",
		       __func__);
		return ret;
	}*/    //TAOS
	reg_cntrl |=
	    (TAOS_TRITON_CNTL_ADC_ENBL | TAOS_TRITON_CNTL_PWRON |
	     TAOS_TRITON_CNTL_ALS_INT_ENBL);
	if ((ret =
	     taos_write_byte(ps_data->client, TAOS_TRITON_CNTRL,
			     reg_cntrl)) < 0) {
		printk(KERN_ERR "%s: write the als data is failed\n", __func__);
		return ret;
	}
	return ret;
}

static int taos_als_power_off(struct taos_data *ps_data)
{
	int ret = 0;
	u8 reg_val = 0;
	if ((reg_val =
	     taos_read_byte(ps_data->client, TAOS_TRITON_CNTRL)) < 0) {
		printk(KERN_ERR "TAOS: read CNTRL failed in ioctl prox_on\n");
		return ret;
	}
	reg_val = reg_val & ~(1 << 1);
	reg_val = reg_val & ~(1 << 4);
	if ((ret =
	     taos_write_byte(ps_data->client,
			     TAOS_TRITON_CNTRL, reg_val)) < 0) {
		printk(KERN_ERR "TAOS: write CNTRL failed in %s\n", __func__);
		return (ret);
	}
	return (ret);
}

static int taos_ps_power_on(struct taos_data *ps_data)
{
	int ret = 0;
	u8 reg_cntrl = 0;
	if ((ret =
	     taos_write_byte(ps_data->client,
			     TAOS_TRITON_PRX_TIME,//
			     ps_data->prox_adc_time)) < -1) {
		printk(KERN_ERR
		       "%s: write the prox time is failed\n", __func__);
		return ret;
	}
	if ((ret =
	     taos_write_byte(ps_data->client,
			     TAOS_TRITON_WAIT_TIME,
			     ps_data->prox_wait_time)) < -1) {
		printk(KERN_ERR
		       "%s: write the wait time is failed\n", __func__);
		return ret;
	}
	if ((ret =
	     taos_write_byte(ps_data->client,
			     TAOS_TRITON_INTERRUPT,//0x11
			     ps_data->prox_intr_filter)) < -1) {
		printk(KERN_ERR
		       "%s: write the interrupt time is failed\n", __func__);
		return ret;
	}

	if ((ret =
	     taos_write_byte(ps_data->client,
			     TAOS_TRITON_PRX_CFG,//0
			     ps_data->prox_config)) < -1) {
		printk(KERN_ERR
		       "%s: write the prox_config is failed\n", __func__);
		return ret;
	}
	if ((ret =
	     taos_write_byte(ps_data->client,
			     TAOS_TRITON_PRX_COUNT,
			     ps_data->prox_pulse_cnt)) < -1) {
		printk(KERN_ERR
		       "%s: write the pulse count time is failed\n", __func__);
		return ret;
	}
	if ((ret =
	     taos_write_byte(ps_data->client,
			     TAOS_TRITON_GAIN, ps_data->prox_gain)) < -1) {
		printk(KERN_ERR
		       "%s: write the gain time is failed\n", __func__);
		return ret;
	}
	/*if ((reg_cntrl =
	     taos_read_byte(taos_datap->client, TAOS_TRITON_CNTRL)) < 0) {
		printk(KERN_ERR "TAOS: read CNTRL failed in ioctl prox_on\n");
		return ret;
	}*/
	reg_cntrl |=
	    TAOS_TRITON_CNTL_PROX_DET_ENBL |
	    TAOS_TRITON_CNTL_PWRON |
	    TAOS_TRITON_CNTL_PROX_INT_ENBL | TAOS_TRITON_CNTL_WAIT_TMR_ENBL| TAOS_TRITON_CNTL_ALS_INT_ENBL ; //TAOS
	if ((ret =
	     (i2c_smbus_write_byte_data
	      (ps_data->client,
	       (TAOS_TRITON_CMD_REG | TAOS_TRITON_CNTRL), reg_cntrl))) < 0) {
		printk(KERN_ERR
		       "TAOS: write reg_cntrl failed in ioctl prox_on\n");
		return (ret);
	}
	return ret;
}

static int taos_ps_power_off(struct taos_data *ps_data)
{
	int ret = 0;
	u8 reg_val = 0;
	if ((reg_val =
	     taos_read_byte(ps_data->client, TAOS_TRITON_CNTRL)) < 0) {
		return ret;
	}
	reg_val = reg_val & ~(1 << 2);
	reg_val = reg_val & ~(1 << 5);
	if ((ret =
	     taos_write_byte(ps_data->client,
			     TAOS_TRITON_CNTRL, reg_val)) < 0) {
		printk(KERN_ERR "TAOS: write CNTRL failed in %s\n", __func__);
		return (ret);
	}
#if 0
	if(ps_data->als_enabled) {
		taos_als_power_off(ps_data);
		taos_als_power_on(ps_data);
	}
#endif
	return ret;
}
// ioctls
// read/calculate lux value
static int taos_get_lux(struct taos_data *ps_data)
{
	u16 raw_clear = 0, raw_ir = 0, raw_lux = 0;
	u32 lux = 0;
	u32 ratio = 0;
	u8 dev_gain = 0;
	u16 Tint = 0;
	struct lux_data *p;
	int ret = 0;
	u8 chdata[4];
	int tmp = 0, i = 0;
	for (i = 0; i < 4; i++) {

		if ((chdata[i] =
		     (taos_read_byte
		      (ps_data->client, TAOS_TRITON_ALS_CHAN0LO + i))) < 0) {
			printk(KERN_ERR
			       "TAOS: read chan0lo/li failed in taos_get_lux()\n");
			return (ret);
		}
	}

	//if atime =100  tmp = (atime+25)/50=2.5   tine = 2.7*(256-atime)=  412.5
	tmp = (ps_data->als_time + 25) / 50;
	TritonTime.numerator = 1;
	TritonTime.denominator = tmp;

	//tmp = 300*atime  400
	tmp = 300 * ps_data->als_time;
	if (tmp > 65535)
		tmp = 65535;
	TritonTime.saturation = tmp;
	raw_clear = chdata[1];
	raw_clear <<= 8;
	raw_clear |= chdata[0];
	raw_ir = chdata[3];
	raw_ir <<= 8;
	raw_ir |= chdata[2];

	raw_clear *= (ps_data->scale_factor);
	raw_ir *= (ps_data->scale_factor);

	if (raw_ir > raw_clear) {
		raw_lux = raw_ir;
		raw_ir = raw_clear;
		raw_clear = raw_lux;
	}
	dev_gain = taos_triton_gain_table[ps_data->gain & 0x3];
	if (raw_clear >= lux_timep->saturation)
		return (TAOS_MAX_LUX);
	if (raw_ir >= lux_timep->saturation)
		return (TAOS_MAX_LUX);
	if (raw_clear == 0)
		return (0);
	if (dev_gain == 0 || dev_gain > 127) {
		printk(KERN_ERR
		       "TAOS: dev_gain = 0 or > 127 in taos_get_lux()\n");
		return -1;
	}
	if (lux_timep->denominator == 0) {
		printk(KERN_ERR
		       "TAOS: lux_timep->denominator = 0 in taos_get_lux()\n");
		return -1;
	}
	ratio = (raw_ir << 15) / raw_clear;
	for (p = lux_tablep; p->ratio && p->ratio < ratio; p++) ;
	if (!p->ratio) {
		if (lux_history[0] < 0)
			return 0;
		else
			return lux_history[0];
	}
	Tint = ps_data->als_time;
	raw_clear =
	    ((raw_clear * 400 + (dev_gain >> 1)) / dev_gain +
	     (Tint >> 1)) / Tint;
	raw_ir =
	    ((raw_ir * 400 + (dev_gain >> 1)) / dev_gain + (Tint >> 1)) / Tint;
	lux = ((raw_clear * (p->clear)) - (raw_ir * (p->ir)));
	lux = 4 * (lux + 32000) / 64000;
	if (lux > TAOS_MAX_LUX) {
		lux = TAOS_MAX_LUX;
	}
	return (lux);
}
// proximity poll
static int32_t taos2771x_enable_ps(struct taos_data *ps_data, uint8_t enable)
{
	uint8_t curr_ps_enable;
	int32_t near_far_state;
	curr_ps_enable = ps_data->ps_enabled?1:0;
	if(curr_ps_enable == enable)
		return 0;

	if(enable)
	{
		hrtimer_start(&ps_data->ps_timer, ps_data->ps_poll_delay, HRTIMER_MODE_REL);
		ps_data->ps_enabled = true;
		msleep(2);

		near_far_state =taos_prox_get_flag(ps_data);
		input_report_abs(ps_data->ps_input_dev, ABS_DISTANCE, near_far_state);
		input_sync(ps_data->ps_input_dev);
		wake_lock_timeout(&ps_data->ps_wakelock, 3*HZ);
	}
	else
	{
		hrtimer_cancel(&ps_data->ps_timer);
		ps_data->ps_enabled = false;
	}
	return 0;
}
static int32_t taos2771x_enable_als(struct taos_data *ps_data, uint8_t enable)
{
	uint8_t curr_als_enable = (ps_data->als_enabled)?1:0;
	if(curr_als_enable == enable)
		return 0;
	if (enable)
	{
		ps_data->als_enabled = true;
		hrtimer_start(&ps_data->als_timer, ps_data->als_poll_delay, HRTIMER_MODE_REL);
	}
	else
	{
		ps_data->als_enabled = false;
		hrtimer_cancel(&ps_data->als_timer);
	}
	return 0;

}
static ssize_t taos_als_enable_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct taos_data *ps_data =  dev_get_drvdata(dev);
	return sprintf(buf, "%d\n", ! !(ps_data->als_enabled));

}

static ssize_t taos_als_enable_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t size)
{
	struct taos_data *ps_data =  dev_get_drvdata(dev);
	uint8_t en;
	if (sysfs_streq(buf, "1"))
	{
		en = 1;
		sensor_on(ps_data);
		//taos_ps_calibrate();
		taos_als_power_on(ps_data);
	}
	else if (sysfs_streq(buf, "0"))
	{
		en = 0;
		taos_als_power_off(ps_data);
		sensor_off(ps_data);
	}
	else
	{
		printk(KERN_ERR "%s, invalid value %d\n", __func__, *buf);
		return -EINVAL;
	}

        mutex_lock(&ps_data->io_lock);
        taos2771x_enable_als(ps_data,en);
        mutex_unlock(&ps_data->io_lock);
        return size;
}


static uint16_t taos_prox_get_data(struct taos_data *ps_data)
{
	int i = 0;
	u8 chdata[6];
	u16 proxdata = 0;
	for (i = 0; i < 6; i++) {
		chdata[i] =
		    taos_read_byte(ps_data->client,
				   TAOS_TRITON_ALS_CHAN0LO + i);
	}
	proxdata = chdata[4] | (chdata[5] << 8);
	return proxdata;
}
static uint8_t taos_prox_get_flag(struct taos_data *ps_data)
{
	uint8_t ret=0;
	if(taos_prox_get_data(ps_data)<ps_data->prox_threshold_lo)
	{
		ret=1;
	}
	else if(taos_prox_get_data(ps_data)>ps_data->prox_threshold_hi)
	{
		ret=0;
	}
        else
        {
		ret=1;
        }
	return ret;
}
static ssize_t taos_ps_enable_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct taos_data *ps_data =  dev_get_drvdata(dev);
	return sprintf(buf, "%d\n", ! !(ps_data->ps_enabled));
}

static ssize_t taos_ps_enable_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t size)
{
	struct taos_data *ps_data =  dev_get_drvdata(dev);
	uint8_t en;
	if (sysfs_streq(buf, "1"))
	{
		en = 1;
		sensor_on(ps_data);
		//taos_ps_calibrate();
		taos_ps_power_on(ps_data);
	}
	else if (sysfs_streq(buf, "0"))
	{
		en = 0;
		taos_ps_power_off(ps_data);
		sensor_off(ps_data);
	}
	else
	{
		printk(KERN_ERR "%s, invalid value %d\n", __func__, *buf);
		return -EINVAL;
	}

        mutex_lock(&ps_data->io_lock);
        taos2771x_enable_ps(ps_data,en);
        mutex_unlock(&ps_data->io_lock);
        return size;
}
static struct device_attribute ps_enable_attribute = __ATTR(enable,0664,taos_ps_enable_show,taos_ps_enable_store);
static struct device_attribute als_enable_attribute = __ATTR(enable,0777,taos_als_enable_show,taos_als_enable_store);
static struct attribute *taos_als_attrs [] =
{
	&als_enable_attribute.attr,
       NULL
};
static struct attribute *taos_ps_attrs [] =
{
	&ps_enable_attribute.attr,
	NULL
};

static struct attribute_group taos_als_attribute_group = {
	.name = "driver",
	.attrs = taos_als_attrs,
};

static struct attribute_group taos_ps_attribute_group = {
	.name = "driver",
	.attrs = taos_ps_attrs,
};

static const struct dev_pm_ops taos2771x_pm_ops = {
	SET_SYSTEM_SLEEP_PM_OPS(taos_suspend, taos_resume)
};



static enum hrtimer_restart taos_als_timer_func(struct hrtimer *timer)
{
	struct taos_data *ps_data = container_of(timer, struct taos_data, als_timer);
	queue_work(ps_data->taos_als_wq, &ps_data->taos_als_work);
	hrtimer_forward_now(&ps_data->als_timer, ps_data->als_poll_delay);
	return HRTIMER_RESTART;
}

static void taos_als_poll_work_func(struct work_struct *work)
{
	struct taos_data *ps_data = container_of(work, struct taos_data, taos_als_work);
	int32_t reading_lux;
	reading_lux =  taos_get_lux(ps_data);
	input_report_abs(ps_data->als_input_dev, ABS_MISC, reading_lux);
	input_sync(ps_data->als_input_dev);
	return;
}



static enum hrtimer_restart taos_ps_timer_func(struct hrtimer *timer)
{
	struct taos_data *ps_data = container_of(timer, struct taos_data, ps_timer);
	queue_work(ps_data->taos_ps_wq, &ps_data->taos_ps_work);
	hrtimer_forward_now(&ps_data->ps_timer, ps_data->ps_poll_delay);
	return HRTIMER_RESTART;
}

static void taos_ps_poll_work_func(struct work_struct *work)
{
	struct taos_data *ps_data = container_of(work, struct taos_data, taos_ps_work);
	int32_t near_far_state=0;
	msleep(2);
	near_far_state =taos_prox_get_flag(ps_data);
	input_report_abs(ps_data->ps_input_dev, ABS_DISTANCE, near_far_state);
	input_sync(ps_data->ps_input_dev);
	wake_lock_timeout(&ps_data->ps_wakelock, 3*HZ);
	return;
}

// client probe
static int taos_probe(struct i2c_client *client,
                        const struct i2c_device_id *id)
{
    int err = -ENODEV;
    uint8_t chip_id=0;
    struct taos_data *ps_data;
	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
		printk("taos_probe: need I2C_FUNC_I2C\n");
		return -ENODEV;
	}

	ps_data = kzalloc(sizeof(struct taos_data),GFP_KERNEL);
	if(!ps_data)
	{
		printk(KERN_ERR "%s: failed to allocate taos_data\n", __func__);
		return -ENOMEM;
	}
	ps_data->client = client;
	i2c_set_clientdata(client,ps_data);
	mutex_init(&ps_data->io_lock);
	wake_lock_init(&ps_data->ps_wakelock,WAKE_LOCK_SUSPEND, "taos_input_wakelock");
	wake_lock_init(&ps_data->ps_nosuspend_wl,WAKE_LOCK_SUSPEND, "taos_nosuspend_wakelock");
	ps_data->als_input_dev = input_allocate_device();
	if (ps_data->als_input_dev==NULL)
	{
		printk("could not allocate als device\n");
		err = -ENOMEM;
		goto err_als_input_allocate;
	}
	ps_data->ps_input_dev = input_allocate_device();
	if (ps_data->ps_input_dev==NULL)
	{
		printk("could not allocate ps device\n");
		err = -ENOMEM;
		goto err_ps_input_allocate;
	}
	ps_data->als_input_dev->name = ALS_NAME;
	ps_data->ps_input_dev->name = PS_NAME;
	set_bit(EV_ABS, ps_data->als_input_dev->evbit);
	set_bit(EV_ABS, ps_data->ps_input_dev->evbit);
	input_set_abs_params(ps_data->als_input_dev, ABS_MISC, 0,500, 0, 0);
	input_set_abs_params(ps_data->ps_input_dev, ABS_DISTANCE, 0,1, 0, 0);
	err = input_register_device(ps_data->als_input_dev);
	if (err<0)
	{
		printk(KERN_ERR "%s: can not register als input device\n", __func__);
		goto err_als_input_register;
	}
	err = input_register_device(ps_data->ps_input_dev);
	if (err<0)
	{
		printk(KERN_ERR "%s: can not register ps input device\n", __func__);
		goto err_ps_input_register;
	}

	err = sysfs_create_group(&ps_data->als_input_dev->dev.kobj, &taos_als_attribute_group);
	if (err < 0)
	{
		printk(KERN_ERR "%s:could not create sysfs group for als\n", __func__);
		goto err_als_sysfs_create_group;
	}
	err = sysfs_create_group(&ps_data->ps_input_dev->dev.kobj, &taos_ps_attribute_group);
	if (err < 0)
	{
		printk(KERN_ERR "%s:could not create sysfs group for ps\n", __func__);
		goto err_ps_sysfs_create_group;
	}
	input_set_drvdata(ps_data->als_input_dev, ps_data);
	input_set_drvdata(ps_data->ps_input_dev, ps_data);
	ps_data->taos_als_wq = create_singlethread_workqueue("taos_als_wq");
	INIT_WORK(&ps_data->taos_als_work, taos_als_poll_work_func);
	hrtimer_init(&ps_data->als_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	ps_data->als_poll_delay = ns_to_ktime(110 * NSEC_PER_MSEC);
	ps_data->als_timer.function = taos_als_timer_func;
	ps_data->taos_ps_wq = create_singlethread_workqueue("taos_ps_wq");
	INIT_WORK(&ps_data->taos_ps_work, taos_ps_poll_work_func);
	hrtimer_init(&ps_data->ps_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	ps_data->ps_poll_delay = ns_to_ktime(60 * NSEC_PER_MSEC);
	ps_data->ps_timer.function = taos_ps_timer_func;
	ps_data->calibrate_target = calibrate_target_param;
	ps_data->als_time = als_time_param;
	ps_data->scale_factor = scale_factor_param;
	ps_data->gain_trim = gain_trim_param;
	ps_data->filter_history = filter_history_param;
	ps_data->gain = gain_param;
	ps_data->als_threshold_hi = als_threshold_hi_param;
	ps_data->als_threshold_lo = als_threshold_lo_param;
	ps_data->prox_threshold_hi = prox_threshold_hi_param;
	ps_data->prox_threshold_lo = prox_threshold_lo_param;
	ps_data->prox_int_time = prox_int_time_param;
	ps_data->prox_adc_time = prox_adc_time_param;
	ps_data->prox_wait_time = prox_wait_time_param;
	ps_data->prox_intr_filter = prox_intr_filter_param;
	ps_data->prox_config = prox_config_param;
	ps_data->prox_pulse_cnt = prox_pulse_cnt_param;
	ps_data->prox_gain = prox_gain_param;

	ps_data->als_enabled = false;
	ps_data->ps_enabled = false;
	ps_data->reg_store = 0;
	device_init_wakeup(&client->dev, true);
	if ((err =
	     taos_write_byte(ps_data->client, TAOS_TRITON_CNTRL,
			     0x00)) < 0) {
		printk(KERN_ERR "%s: write the chip power down is failed\n",
		       __func__);
		goto err_init_all_setting;
	}
	chip_id =taos_read_byte(ps_data->client, TAOS_TRITON_CHIPID);
	printk("*****rda taos 2771x chip_id=0x%x",chip_id);

	return 0;

err_init_all_setting:
	device_init_wakeup(&client->dev, false);
	hrtimer_try_to_cancel(&ps_data->als_timer);
	destroy_workqueue(ps_data->taos_als_wq);
	hrtimer_try_to_cancel(&ps_data->ps_timer);
	destroy_workqueue(ps_data->taos_ps_wq);
	sysfs_remove_group(&ps_data->ps_input_dev->dev.kobj, &taos_ps_attribute_group);
err_ps_sysfs_create_group:
printk("*****rda err_ps_sysfs_create_group\n");
	sysfs_remove_group(&ps_data->als_input_dev->dev.kobj, &taos_als_attribute_group);
err_als_sysfs_create_group:
printk("*****rda err_als_sysfs_create_group\n");
	input_unregister_device(ps_data->ps_input_dev);
err_ps_input_register:
printk("*****rda err_ps_input_register\n");
	input_unregister_device(ps_data->als_input_dev);
err_als_input_register:
printk("*****rda err_als_input_register\n");
	input_free_device(ps_data->ps_input_dev);
err_ps_input_allocate:
printk("*****rda err_ps_input_allocate\n");
	input_free_device(ps_data->als_input_dev);
err_als_input_allocate:
printk("*****rda err_als_input_allocate\n");
    wake_lock_destroy(&ps_data->ps_nosuspend_wl);
    wake_lock_destroy(&ps_data->ps_wakelock);
    mutex_destroy(&ps_data->io_lock);
	kfree(ps_data);
    return err;
}

// client remove
static int taos_remove(struct i2c_client *client)
{
	struct taos_data *ps_data = i2c_get_clientdata(client);
	device_init_wakeup(&client->dev, false);
	hrtimer_try_to_cancel(&ps_data->als_timer);
	destroy_workqueue(ps_data->taos_als_wq);
	hrtimer_try_to_cancel(&ps_data->ps_timer);
	destroy_workqueue(ps_data->taos_ps_wq);
	sysfs_remove_group(&ps_data->ps_input_dev->dev.kobj, &taos_ps_attribute_group);
	sysfs_remove_group(&ps_data->als_input_dev->dev.kobj, &taos_als_attribute_group);
	input_unregister_device(ps_data->ps_input_dev);
	input_unregister_device(ps_data->als_input_dev);
	input_free_device(ps_data->ps_input_dev);
	input_free_device(ps_data->als_input_dev);
	wake_lock_destroy(&ps_data->ps_nosuspend_wl);
	wake_lock_destroy(&ps_data->ps_wakelock);
	mutex_destroy(&ps_data->io_lock);
	kfree(ps_data);
    return 0;
}

static const struct i2c_device_id taos_ps_id[] =
{
    {RDA_LIGHTSENSOR_DRV_NAME, 0},
    {}
};
MODULE_DEVICE_TABLE(i2c, taos_ps_id);
static struct i2c_driver taos_ps_driver =
{
    .driver = {
        .name = RDA_LIGHTSENSOR_DRV_NAME,
		.owner = THIS_MODULE,
		.pm = &taos2771x_pm_ops,
    },
    .probe = taos_probe,
    .remove = taos_remove,
    .id_table = taos_ps_id,
};
static struct rda_lightsensor_device_data rda_lightsensor_data[] = {
        {
         .irqflags = IRQF_SHARED | IRQF_TRIGGER_RISING,
         },
};
/* ATTR end */
// driver init
static int __init taos_init(void)
{
	int ret;

	static struct i2c_board_info i2c_dev_lightsensor = {
	        I2C_BOARD_INFO(RDA_LIGHTSENSOR_DRV_NAME, 0),
	        .platform_data = rda_lightsensor_data,
	};

	struct i2c_adapter *adapter;
	i2c_dev_lightsensor.addr =  _DEF_I2C_ADDR_LSENSOR_TAOS2771x;

	adapter = i2c_get_adapter(_TGT_AP_I2C_BUS_ID_LSENSOR);

	if (!adapter) {
		pr_err("%s, cannot get i2c adapter %d\n",
			__func__, _TGT_AP_I2C_BUS_ID_LSENSOR);
		return -ENODEV;
	}

	i2c_new_device(adapter, &i2c_dev_lightsensor);

	ret = i2c_add_driver(&taos_ps_driver);

	if (ret) {
		i2c_del_driver(&taos_ps_driver);
		return ret;
	}

	pr_info("rda_gs %s initialized, at i2c bus %d addr 0x%02x\n",
		RDA_LIGHTSENSOR_DRV_NAME, _TGT_AP_I2C_BUS_ID_LSENSOR,
		i2c_dev_lightsensor.addr);

	 return 0;
}

// driver exit
static void __exit taos_exit(void)
{
    i2c_del_driver(&taos_ps_driver);
}

MODULE_AUTHOR("John Koshi - Surya Software");
MODULE_DESCRIPTION("TAOS ambient light and proximity sensor driver");
MODULE_LICENSE("GPL");

module_init(taos_init);
module_exit(taos_exit);

