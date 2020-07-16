/*
 *  stk3x1x.c - Linux kernel modules for sensortek stk301x, stk321x and stk331x
 *  proximity/ambient light sensor
 *
 *  Copyright (C) 2012~2013 Lex Hsieh / sensortek <lex_hsieh@sensortek.com.tw>
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
#ifdef CONFIG_HAS_EARLYSUSPEND
//#include <linux/earlysuspend.h>
#endif
#include "stk3x1x.h"
#include "tgt_ap_board_config.h"

#define DRIVER_VERSION  "3.4.9"


/* Driver Settings */
#define CONFIG_STK_PS_ALS_USE_CHANGE_THRESHOLD
#ifdef CONFIG_STK_PS_ALS_USE_CHANGE_THRESHOLD
#define STK_ALS_CHANGE_THD	20	/* The threshold to trigger ALS interrupt, unit: lux */
#endif	/* #ifdef CONFIG_STK_PS_ALS_USE_CHANGE_THRESHOLD */
#define STK_INT_PS_MODE			1	/* 1, 2, or 3	*/
#define STK_POLL_PS
#define STK_POLL_ALS		/* ALS interrupt is valid only when STK_PS_INT_MODE = 1	or 4*/
//#define STK_TUNE0
//#define STK_TUNE1
//#define STK_DEBUG_PRINTF
//#define STK_ALS_FIR

/* Define Register Map */
#define STK_STATE_REG 			0x00
#define STK_PSCTRL_REG 			0x01
#define STK_ALSCTRL_REG 			0x02
#define STK_LEDCTRL_REG 			0x03
#define STK_INT_REG 				0x04
#define STK_WAIT_REG 			0x05
#define STK_THDH1_PS_REG 		0x06
#define STK_THDH2_PS_REG 		0x07
#define STK_THDL1_PS_REG 		0x08
#define STK_THDL2_PS_REG 		0x09
#define STK_THDH1_ALS_REG 		0x0A
#define STK_THDH2_ALS_REG 		0x0B
#define STK_THDL1_ALS_REG 		0x0C
#define STK_THDL2_ALS_REG 		0x0D
#define STK_FLAG_REG 			0x10
#define STK_DATA1_PS_REG	 	0x11
#define STK_DATA2_PS_REG 		0x12
#define STK_DATA1_ALS_REG 		0x13
#define STK_DATA2_ALS_REG 		0x14
#define STK_DATA1_OFFSET_REG 	0x15
#define STK_DATA2_OFFSET_REG 	0x16
#define STK_DATA1_IR_REG 		0x17
#define STK_DATA2_IR_REG 		0x18
#define STK_PDT_ID_REG 			0x3E
#define STK_RSRVD_REG 			0x3F
#define STK_SW_RESET_REG		0x80


/* Define state reg */
#define STK_STATE_EN_IRS_SHIFT	7
#define STK_STATE_EN_AK_SHIFT	6
#define STK_STATE_EN_ASO_SHIFT	5
#define STK_STATE_EN_IRO_SHIFT	4
#define STK_STATE_EN_WAIT_SHIFT	2
#define STK_STATE_EN_ALS_SHIFT	1
#define STK_STATE_EN_PS_SHIFT	0

#define STK_STATE_EN_IRS_MASK	0x80
#define STK_STATE_EN_AK_MASK	0x40
#define STK_STATE_EN_ASO_MASK	0x20
#define STK_STATE_EN_IRO_MASK	0x10
#define STK_STATE_EN_WAIT_MASK	0x04
#define STK_STATE_EN_ALS_MASK	0x02
#define STK_STATE_EN_PS_MASK	0x01

/* Define PS ctrl reg */
#define STK_PS_PRS_SHIFT		6
#define STK_PS_GAIN_SHIFT		4
#define STK_PS_IT_SHIFT		0

#define STK_PS_PRS_MASK			0xC0
#define STK_PS_GAIN_MASK			0x30
#define STK_PS_IT_MASK			0x0F

/* Define ALS ctrl reg */
#define STK_ALS_PRS_SHIFT		6
#define STK_ALS_GAIN_SHIFT	4
#define STK_ALS_IT_SHIFT			0

#define STK_ALS_PRS_MASK		0xC0
#define STK_ALS_GAIN_MASK		0x30
#define STK_ALS_IT_MASK			0x0F

/* Define LED ctrl reg */
#define STK_LED_IRDR_SHIFT	6
#define STK_LED_DT_SHIFT		0

#define STK_LED_IRDR_MASK		0xC0
#define STK_LED_DT_MASK			0x3F

/* Define interrupt reg */
#define STK_INT_CTRL_SHIFT	7
#define STK_INT_OUI_SHIFT	4
#define STK_INT_ALS_SHIFT	3
#define STK_INT_PS_SHIFT			0

#define STK_INT_CTRL_MASK		0x80
#define STK_INT_OUI_MASK			0x10
#define STK_INT_ALS_MASK			0x08
#define STK_INT_PS_MASK			0x07

#define STK_INT_ALS				0x08

/* Define flag reg */
#define STK_FLG_ALSDR_SHIFT	7
#define STK_FLG_PSDR_SHIFT	6
#define STK_FLG_ALSINT_SHIFT		5
#define STK_FLG_PSINT_SHIFT		4
#define STK_FLG_OUI_SHIFT		2
#define STK_FLG_IR_RDY_SHIFT		1
#define STK_FLG_NF_SHIFT		0

#define STK_FLG_ALSDR_MASK		0x80
#define STK_FLG_PSDR_MASK		0x40
#define STK_FLG_ALSINT_MASK		0x20
#define STK_FLG_PSINT_MASK		0x10
#define STK_FLG_OUI_MASK			0x04
#define STK_FLG_IR_RDY_MASK		0x02
#define STK_FLG_NF_MASK			0x01

/* misc define */
#define MIN_ALS_POLL_DELAY_NS	110000000

#define STK2213_PID			0x23
#define STK2213I_PID			0x22
#define STK3010_PID			0x33
#define STK3210_STK3310_PID	0x13
#define STK3211_STK3311_PID	0x12


#ifdef STK_TUNE0
	#define STK_MAX_MIN_DIFF	500
	#define STK_LT_N_CT	150
	#define STK_HT_N_CT	350

#elif defined(STK_TUNE1)
	#define K_TYPE_CT 	1
	#define K_TYPE_N		2
	#define K_TYPE_NF	3
	#define KTYPE_FAC  K_TYPE_CT

	//Common
	#define STK_CALI_VER0				0x48
	#define STK_CALI_VER1				(0x10 | KTYPE_FAC)
	#define STK_CALI_FILE 				"/data/misc/stk3x1xcali.conf"
	//#define STK_CALI_FILE 				"/system/etc/stk3x1xcali.conf"
	#define STK_CALI_FILE_SIZE 			10
	#define STK_CALI_SAMPLE_NO		5
	#define D_CTK_A	400

	#if (KTYPE_FAC == K_TYPE_CT)
		#define D_HT  500
		#define D_LT  300
		//#define DCTK	900
		#define STK_CALI_MIN				300
		#define STK_CALI_MAX				3000

	#elif (KTYPE_FAC == K_TYPE_NF)
		#define D_CT					250
		#define STK_CALI_HT_MIN		1000
		#define STK_CALI_HT_MAX		4000
		#define STK_CALI_LT_MIN		1000
		#define STK_CALI_LT_MAX		4000
		#define STK_CALI_HT_LT_MIN	50
		#define STK_CALI_LT_CT_MIN	50
	#else
		#pragma message("Please choose one KTYPE_FAC")

	#endif	/* #if (KTYPE_FAC == K_TYPE_CT) */
#endif	/* #ifdef STK_TUNE0 */

#define STK_IRC_MAX_ALS_CODE		20000
#define STK_IRC_MIN_ALS_CODE		25
#define STK_IRC_MIN_IR_CODE		50
#define STK_IRC_ALS_DENOMI		2
#define STK_IRC_ALS_NUMERA		5
#define STK_IRC_ALS_CORREC		748

#define ALS_NAME "lightsensor-level"
#define PS_NAME "proximity"

#ifdef STK_ALS_FIR
struct data_filter {
	s16 raw[8];
	int sum;
	int number;
	int idx;
};
#endif

struct stk3x1x_data {
	struct i2c_client *client;
#if (!defined(STK_POLL_PS) || !defined(STK_POLL_ALS))
	int32_t irq;
	struct work_struct stk_work;
	struct workqueue_struct *stk_wq;
#endif
	uint16_t ir_code;
	uint16_t als_correct_factor;
	uint8_t alsctrl_reg;
	uint8_t psctrl_reg;
	int		int_pin;
	uint8_t wait_reg;
#ifdef CONFIG_HAS_EARLYSUSPEND
	//struct early_suspend stk_early_suspend;
#endif
	uint16_t ps_thd_h;
	uint16_t ps_thd_l;
	struct mutex io_lock;
	struct input_dev *ps_input_dev;
	int32_t ps_distance_last;
	bool ps_enabled;
	struct wake_lock ps_wakelock;
#ifdef STK_POLL_PS
	struct hrtimer ps_timer;
	struct work_struct stk_ps_work;
	struct workqueue_struct *stk_ps_wq;
	struct wake_lock ps_nosuspend_wl;
#endif
	struct input_dev *als_input_dev;
	int32_t als_lux_last;
	uint32_t als_transmittance;
	bool als_enabled;
	ktime_t ps_poll_delay;
	ktime_t als_poll_delay;
//#ifdef STK_POLL_ALS
	struct work_struct stk_als_work;
	struct hrtimer als_timer;
	struct workqueue_struct *stk_als_wq;
//#endif
	bool first_boot;
#ifdef STK_TUNE0
	uint16_t psa;
	uint16_t psi;
	uint16_t psi_set;
	struct hrtimer ps_tune0_timer;
	struct workqueue_struct *stk_ps_tune0_wq;
	struct work_struct stk_ps_tune0_work;
	ktime_t ps_tune0_delay;
#elif defined(STK_TUNE1)
	uint16_t cta;
	uint16_t cti;
	uint16_t kdata;
	uint16_t kdata1;
	uint16_t aoffset;
	uint8_t state;
#endif
#ifdef STK_ALS_FIR
	struct data_filter fir;
#endif
};

#if( !defined(CONFIG_STK_PS_ALS_USE_CHANGE_THRESHOLD))
static uint32_t lux_threshold_table[] =
{
	3,
	10,
	40,
	65,
	145,
	300,
	550,
	930,
	1250,
	1700,
};

#define LUX_THD_TABLE_SIZE (sizeof(lux_threshold_table)/sizeof(uint32_t)+1)
static uint16_t code_threshold_table[LUX_THD_TABLE_SIZE+1];
#endif
static int32_t stk3x1x_enable_ps(struct stk3x1x_data *ps_data, uint8_t enable);
static int32_t stk3x1x_enable_als(struct stk3x1x_data *ps_data, uint8_t enable);
static int32_t stk3x1x_set_ps_thd_l(struct stk3x1x_data *ps_data, uint16_t thd_l);
static int32_t stk3x1x_set_ps_thd_h(struct stk3x1x_data *ps_data, uint16_t thd_h);
static int32_t stk3x1x_set_als_thd_l(struct stk3x1x_data *ps_data, uint16_t thd_l);
static int32_t stk3x1x_set_als_thd_h(struct stk3x1x_data *ps_data, uint16_t thd_h);
//static int32_t stk3x1x_set_ps_aoffset(struct stk3x1x_data *ps_data, uint16_t offset);
static int32_t stk3x1x_get_ir_reading(struct stk3x1x_data *ps_data);

#ifdef STK_TUNE0
static int stk_ps_tune_zero_func_fae(struct stk3x1x_data *ps_data);
#elif defined(STK_TUNE1)
static int32_t stk_get_ps_cali(struct stk3x1x_data *ps_data, uint16_t kd[]);
#endif

static int rda_lightsensor_i2c_write(struct i2c_client *client, u8 addr, u8 *pdata, int datalen)
{
	int ret = 0;
	u8 tmp_buf[128];
	unsigned int bytelen = 0;
	if (datalen > 125){
		return -1;
	}
	tmp_buf[0] = addr;
	bytelen++;
	if (datalen != 0 && pdata != NULL){
		memcpy(&tmp_buf[bytelen], pdata, datalen);
		bytelen += datalen;
	}
	ret = i2c_master_send(client, tmp_buf, bytelen);
	return ret;
}

int rda_lightsensor_i2c_read(struct i2c_client *client, u8 addr, u8 *pdata, unsigned int datalen)
{
	int ret = 0;

	if (datalen > 126){
		return -1;
	}

	ret = rda_lightsensor_i2c_write(client, addr, NULL, 0);

	if (ret < 0){
		return ret;
	}

	return i2c_master_recv(client, pdata, datalen);
}

inline uint32_t stk_alscode2lux(struct stk3x1x_data *ps_data, uint32_t alscode)
{
	alscode += ((alscode<<7)+(alscode<<3)+(alscode>>1));
	alscode<<=3;
	alscode/=ps_data->als_transmittance;
	return alscode;
}

inline uint32_t stk_lux2alscode(struct stk3x1x_data *ps_data, uint32_t lux)
{
	lux*=ps_data->als_transmittance;
	lux/=1100;
	if (unlikely(lux>=(1<<16)))
		lux = (1<<16) -1;
	return lux;
}

#ifndef CONFIG_STK_PS_ALS_USE_CHANGE_THRESHOLD
static void stk_init_code_threshold_table(struct stk3x1x_data *ps_data)
{
	uint32_t i,j;
	uint32_t alscode;

	code_threshold_table[0] = 0;
#ifdef STK_DEBUG_PRINTF
	printk(KERN_INFO "alscode[0]=%d\n",0);
#endif
	for (i=1,j=0;i<LUX_THD_TABLE_SIZE;i++,j++)
	{
		alscode = stk_lux2alscode(ps_data, lux_threshold_table[j]);
		code_threshold_table[i] = (uint16_t)(alscode);
	}
	code_threshold_table[i] = 0xffff;
}

static uint32_t stk_get_lux_interval_index(uint16_t alscode)
{
	uint32_t i;
	for (i=1;i<=LUX_THD_TABLE_SIZE;i++)
	{
		if ((alscode>=code_threshold_table[i-1])&&(alscode<code_threshold_table[i]))
		{
			return i;
		}
	}
	return LUX_THD_TABLE_SIZE;
}
#else
inline void stk_als_set_new_thd(struct stk3x1x_data *ps_data, uint16_t alscode)
{
	int32_t high_thd,low_thd;
	high_thd = alscode + stk_lux2alscode(ps_data, STK_ALS_CHANGE_THD);
	low_thd = alscode - stk_lux2alscode(ps_data, STK_ALS_CHANGE_THD);
	if (high_thd >= (1<<16))
		high_thd = (1<<16) -1;
	if (low_thd <0)
		low_thd = 0;
	stk3x1x_set_als_thd_h(ps_data, (uint16_t)high_thd);
	stk3x1x_set_als_thd_l(ps_data, (uint16_t)low_thd);
}
#endif // CONFIG_STK_PS_ALS_USE_CHANGE_THRESHOLD


static int32_t stk3x1x_init_all_reg(struct stk3x1x_data *ps_data, struct stk3x1x_platform_data *plat_data)
{
	int32_t ret;
	uint8_t w_reg;

	w_reg = plat_data->state_reg;
	ret = rda_lightsensor_i2c_write(ps_data->client, STK_STATE_REG, &w_reg, 1);
	if (ret < 0)
	{
		printk(KERN_ERR "%s: write i2c error\n", __func__);
		return ret;
	}

	ps_data->psctrl_reg = plat_data->psctrl_reg;
	ret = rda_lightsensor_i2c_write(ps_data->client, STK_PSCTRL_REG, &ps_data->psctrl_reg, 1);
	if (ret < 0)
	{
		printk(KERN_ERR "%s: write i2c error\n", __func__);
		return ret;
	}
	ps_data->alsctrl_reg = plat_data->alsctrl_reg;
	ret = rda_lightsensor_i2c_write(ps_data->client, STK_ALSCTRL_REG, &ps_data->alsctrl_reg, 1);
	if (ret < 0)
	{
		printk(KERN_ERR "%s: write i2c error\n", __func__);
		return ret;
	}
	w_reg = plat_data->ledctrl_reg;
	ret = rda_lightsensor_i2c_write(ps_data->client, STK_LEDCTRL_REG, &w_reg, 1);
	if (ret < 0)
	{
		printk(KERN_ERR "%s: write i2c error\n", __func__);
		return ret;
	}
	ps_data->wait_reg = plat_data->wait_reg;

	if(ps_data->wait_reg < 2)
	{
		printk(KERN_WARNING "%s: wait_reg should be larger than 2, force to write 2\n", __func__);
		ps_data->wait_reg = 2;
	}
	else if (ps_data->wait_reg > 0xFF)
	{
		printk(KERN_WARNING "%s: wait_reg should be less than 0xFF, force to write 0xFF\n", __func__);
		ps_data->wait_reg = 0xFF;
	}
	w_reg = plat_data->wait_reg;
	ret = rda_lightsensor_i2c_write(ps_data->client, STK_WAIT_REG, &w_reg, 1);
	if (ret < 0)
	{
		printk(KERN_ERR "%s: write i2c error\n", __func__);
		return ret;
	}
	ps_data->ps_thd_h = plat_data->ps_thd_h;
	ps_data->ps_thd_l = plat_data->ps_thd_l;
	//stk3x1x_set_ps_thd_h(ps_data, ps_data->ps_thd_h);
	//stk3x1x_set_ps_thd_l(ps_data, ps_data->ps_thd_l);

	w_reg = 0;
#ifndef STK_POLL_PS
	w_reg |= STK_INT_PS_MODE;
#else
	w_reg |= 0x01;
#endif

#ifdef STK_TUNE1
	w_reg |= STK_INT_OUI_MASK;
#endif

#if (!defined(STK_POLL_ALS) && (STK_INT_PS_MODE != 0x02) && (STK_INT_PS_MODE != 0x03))
	w_reg |= STK_INT_ALS;
#endif
	ret = rda_lightsensor_i2c_write(ps_data->client, STK_INT_REG, &w_reg, 1);
	if (ret < 0)
	{
		printk(KERN_ERR "%s: write i2c error\n", __func__);
		return ret;
	}
	return 0;
}

static int32_t stk3x1x_check_pid(struct stk3x1x_data *ps_data)
{
	uint8_t reg_val1, reg_val2;
	int32_t ret;

	ret = rda_lightsensor_i2c_read(ps_data->client,STK_PDT_ID_REG, &reg_val1, 1);
	if (ret < 0)
	{
		printk(KERN_ERR "%s: read i2c error, err=%d\n", __func__, ret);
		return ret;
	}

	ret = rda_lightsensor_i2c_read(ps_data->client,STK_RSRVD_REG, &reg_val2, 1);
	if (ret < 0)
	{
		printk(KERN_ERR "%s: read i2c error, err=%d\n", __func__, ret);
		return ret;
	}

	if(reg_val2 == 0xC0)
		printk(KERN_INFO "%s: RID=0xC0!!!!!!!!!!!!!\n", __func__);

	switch(reg_val1)
	{
		case STK2213_PID:
		case STK2213I_PID:
		case STK3010_PID:
		case STK3210_STK3310_PID:
		case STK3211_STK3311_PID:
			return 0;
		default:
			printk(KERN_ERR "%s: invalid PID(%#x)\n", __func__, reg_val1);
			return -1;
	}

	return 0;
}


static int32_t stk3x1x_software_reset(struct stk3x1x_data *ps_data)
{
	int32_t r;
	uint8_t w_reg, wreg_1;

	w_reg = 0x7F;
	r = rda_lightsensor_i2c_write(ps_data->client,STK_WAIT_REG, &w_reg, 1);
	if (r<0)
	{
		printk(KERN_ERR "%s: software reset: write i2c error, ret=%d\n", __func__, r);
		return r;
	}
	(void)rda_lightsensor_i2c_read(ps_data->client,STK_WAIT_REG, &wreg_1, 1);
	if (w_reg != wreg_1)
	{
		printk(KERN_ERR "%s: software reset: read-back value is not the same\n", __func__);
		return -1;
	}

	w_reg = 0;
	r = rda_lightsensor_i2c_write(ps_data->client,STK_SW_RESET_REG,&w_reg, 1);
	if (r<0)
	{
		printk(KERN_ERR "%s: software reset: read error after reset\n", __func__);
		return r;
	}
	msleep(1);
	return 0;
}


static int32_t stk3x1x_set_als_thd_l(struct stk3x1x_data *ps_data, uint16_t thd_l)
{
	uint8_t temp;
	uint8_t* pSrc = (uint8_t*)&thd_l;
	temp = *pSrc;
	*pSrc = *(pSrc+1);
	*(pSrc+1) = temp;
	return rda_lightsensor_i2c_write(ps_data->client,STK_THDL1_ALS_REG, (u8 *)&thd_l, 2);
}
static int32_t stk3x1x_set_als_thd_h(struct stk3x1x_data *ps_data, uint16_t thd_h)
{
	uint8_t temp;
	uint8_t* pSrc = (uint8_t*)&thd_h;
	temp = *pSrc;
	*pSrc = *(pSrc+1);
	*(pSrc+1) = temp;
	return rda_lightsensor_i2c_write(ps_data->client,STK_THDH1_ALS_REG,(u8 *)&thd_h, 2);
}

static int32_t stk3x1x_set_ps_thd_l(struct stk3x1x_data *ps_data, uint16_t thd_l)
{
	uint8_t temp;
	uint8_t* pSrc = (uint8_t*)&thd_l;

	temp = *pSrc;
	*pSrc = *(pSrc+1);
	*(pSrc+1) = temp;
	ps_data->ps_thd_l = thd_l;
	return rda_lightsensor_i2c_write(ps_data->client,STK_THDL1_PS_REG,(u8 *)&thd_l, 2);
}
static int32_t stk3x1x_set_ps_thd_h(struct stk3x1x_data *ps_data, uint16_t thd_h)
{
	uint8_t temp;
	uint8_t* pSrc = (uint8_t*)&thd_h;

	temp = *pSrc;
	*pSrc = *(pSrc+1);
	*(pSrc+1) = temp;
	ps_data->ps_thd_h = thd_h;
	return rda_lightsensor_i2c_write(ps_data->client,STK_THDH1_PS_REG,(u8 *)&thd_h, 2);
}
/*
static int32_t stk3x1x_set_ps_foffset(struct stk3x1x_data *ps_data, uint16_t offset)
{
	uint8_t temp;
	uint8_t* pSrc = (uint8_t*)&offset;
	temp = *pSrc;
	*pSrc = *(pSrc+1);
	*(pSrc+1) = temp;
	return i2c_smbus_write_word_data(ps_data->client,STK_DATA1_OFFSET_REG,offset);
}

static int32_t stk3x1x_set_ps_aoffset(struct stk3x1x_data *ps_data, uint16_t offset)
{
	uint8_t temp;
	uint8_t* pSrc = (uint8_t*)&offset;
	int ret;
	uint8_t w_state_reg;
	uint8_t re_en;

	ret = i2c_smbus_read_byte_data(ps_data->client, STK_STATE_REG);
	if (ret < 0)
	{
		printk(KERN_ERR "%s: write i2c error\n", __func__);
		return ret;
	}
	re_en = (ret & STK_STATE_EN_AK_MASK) ? 1: 0;
	if(re_en)
	{
		w_state_reg = (uint8_t)(ret & (~STK_STATE_EN_AK_MASK));
		ret = i2c_smbus_write_byte_data(ps_data->client, STK_STATE_REG, w_state_reg);
		if (ret < 0)
		{
			printk(KERN_ERR "%s: write i2c error\n", __func__);
			return ret;
		}
		msleep(1);
	}
	temp = *pSrc;
	*pSrc = *(pSrc+1);
	*(pSrc+1) = temp;
	ret = i2c_smbus_write_word_data(ps_data->client,0x0E,offset);
	if(!re_en)
		return ret;

	w_state_reg |= STK_STATE_EN_AK_MASK;
	ret = i2c_smbus_write_byte_data(ps_data->client, STK_STATE_REG, w_state_reg);
	if (ret < 0)
	{
		printk(KERN_ERR "%s: write i2c error\n", __func__);
		return ret;
	}

	return 0;
}
*/
static inline uint32_t stk3x1x_get_ps_reading(struct stk3x1x_data *ps_data)
{
	int32_t word_data, ret;
	uint8_t tmp_word_data[2];

	ret = rda_lightsensor_i2c_read(ps_data->client,STK_DATA1_PS_REG, tmp_word_data, 2);
	if(ret < 0)
	{
		printk(KERN_ERR "%s fail, err=0x%x:%x", __func__, tmp_word_data[0], tmp_word_data[1]);
		word_data = (tmp_word_data[1] | (tmp_word_data[0]) << 8);
		return word_data;
	}
	word_data = tmp_word_data[1] | tmp_word_data[0] << 8;
	return word_data;
}


static int32_t stk3x1x_set_flag(struct stk3x1x_data *ps_data, uint8_t org_flag_reg, uint8_t clr)
{
	uint8_t w_flag;
	w_flag = org_flag_reg | (STK_FLG_ALSINT_MASK | STK_FLG_PSINT_MASK | STK_FLG_OUI_MASK | STK_FLG_IR_RDY_MASK);
	w_flag &= (~clr);
	//printk(KERN_INFO "%s: org_flag_reg=0x%x, w_flag = 0x%x\n", __func__, org_flag_reg, w_flag);
	return rda_lightsensor_i2c_write(ps_data->client,STK_FLAG_REG, &w_flag, 1);
}

static int32_t stk3x1x_get_flag(struct stk3x1x_data *ps_data)
{
	uint8_t reg_val;
	int ret = 0;

	ret = rda_lightsensor_i2c_read(ps_data->client,STK_FLAG_REG, &reg_val, 1);
	if (ret < 0)
		return ret;
	
	return (int32_t)reg_val;
}

static int32_t stk3x1x_enable_ps(struct stk3x1x_data *ps_data, uint8_t enable)
{
	int32_t ret;
	uint8_t w_state_reg;
	uint8_t r_state_reg;
	uint8_t curr_ps_enable;
#ifdef STK_TUNE1
	uint16_t kda[3];
#endif
	uint32_t reading;
	int32_t near_far_state;

	curr_ps_enable = ps_data->ps_enabled?1:0;
	if(curr_ps_enable == enable)
		return 0;

#ifdef STK_TUNE0
	if (!(ps_data->psi_set) && !enable)
	{
		hrtimer_cancel(&ps_data->ps_tune0_timer);
		cancel_work_sync(&ps_data->stk_ps_tune0_work);
	}
#endif

	ret = rda_lightsensor_i2c_read(ps_data->client, STK_STATE_REG, &r_state_reg, 1);
	if (ret < 0)
	{
		printk(KERN_ERR "%s: write i2c error, ret=%d\n", __func__, ret);
		return ret;
	}
	w_state_reg = r_state_reg;
#ifdef STK_TUNE0
	if(ps_data->first_boot)
	{
		ps_data->first_boot = false;
		ps_data->ps_thd_h = 0xFFFF;
		ps_data->ps_thd_l = 0;
		ps_data->psa = 0x0;
		ps_data->psi = 0xFFFF;

		// ret = stk3x1x_set_ps_aoffset(ps_data, 0xFFFF);
		// if (ret < 0)
		// {
			// printk(KERN_ERR "%s: write i2c error, ret=%d\n", __func__, ret);
		//	return ret;
		// }
		stk3x1x_set_ps_thd_h(ps_data, ps_data->ps_thd_h);
		stk3x1x_set_ps_thd_l(ps_data, ps_data->ps_thd_l);
	}

#elif defined(STK_TUNE1)

	#if (KTYPE_FAC==K_TYPE_CT)
		if(ps_data->first_boot)
		{
			ps_data->first_boot = false;
			w_state_reg |= 0x08;
			ret = stk_get_ps_cali(ps_data, kda);
			if(ret == 0)
			{
				ps_data->kdata = kda[0];
				ps_data->ps_thd_h = kda[1];
				ps_data->ps_thd_l = kda[2];
				printk(KERN_INFO "%s: load kda[0]=%d,kda[1]=%d,kda[2]=%d\n", __func__, kda[0], kda[1], kda[2]);
				stk3x1x_set_ps_foffset(ps_data, ps_data->kdata);
				stk3x1x_set_ps_aoffset(ps_data, ps_data->kdata + D_CTK_A);
				ps_data->cta = ps_data->kdata + D_CTK_A;
				ps_data->cti = ps_data->kdata * 4 / 5;
				stk3x1x_set_ps_thd_h(ps_data, ps_data->ps_thd_h);
				stk3x1x_set_ps_thd_l(ps_data, ps_data->ps_thd_l);
			}
			else
			{
				printk(KERN_ERR "%s: no kdata\n", __func__);
				/*
					ps_data->kdata = DCTK;
					printk(KERN_INFO "%s: def kdata=%d\n", __func__, ps_data->kdata);
					stk3x1x_set_ps_foffset(ps_data, ps_data->kdata);
					stk3x1x_set_ps_aoffset(ps_data, ps_data->kdata + D_CTK_A * 3 / 2);
					ps_data->cta = ps_data->kdata + D_CTK_A * 3 / 2;
					ps_data->cti = ps_data->kdata * 4 / 5;
				 */
			}

		}
		else
		{
			if(ps_data->kdata != 0)
			{
				stk3x1x_set_ps_foffset(ps_data, ps_data->kdata);
				stk3x1x_set_ps_aoffset(ps_data, ps_data->kdata + D_CTK_A);
				ps_data->cta = ps_data->kdata + D_CTK_A;
				ps_data->cti = ps_data->kdata * 4 / 5;
			}
			else
				printk(KERN_ERR "%s: no kdata\n", __func__);
		}

	#elif (KTYPE_FAC==K_TYPE_NF)
		if(ps_data->first_boot)
		{
			ps_data->first_boot = false;
			w_state_reg |= 0x08;
			ret = stk_get_ps_cali(ps_data, kda);
			if(ret == 0)
			{
				ps_data->kdata = kda[0];
				ps_data->ps_thd_h = kda[1];
				ps_data->ps_thd_l = kda[2];
				printk(KERN_INFO "%s: load kda[0]=%d,kda[1]=%d,kda[2]=%d\n", __func__, kda[0], kda[1], kda[2]);
				stk3x1x_set_ps_foffset(ps_data, ps_data->kdata);
				stk3x1x_set_ps_aoffset(ps_data, ps_data->kdata + D_CTK_A);
				ps_data->cta = ps_data->kdata + D_CTK_A;
				ps_data->cti = ps_data->kdata * 4 / 5;
				stk3x1x_set_ps_thd_h(ps_data, ps_data->ps_thd_h);
				stk3x1x_set_ps_thd_l(ps_data, ps_data->ps_thd_l);
			}
			else
			{
				printk(KERN_ERR "%s: no kdata\n", __func__);
			}
		}
		else
		{
			if(ps_data->kdata != 0)
			{
				stk3x1x_set_ps_foffset(ps_data, ps_data->kdata);
				stk3x1x_set_ps_aoffset(ps_data, ps_data->kdata + D_CTK_A);
				ps_data->cta = ps_data->kdata + D_CTK_A;
				ps_data->cti = ps_data->kdata * 4 / 5;
			}
			else
				printk(KERN_ERR "%s: no kdata\n", __func__);
		}
	#endif
#else
	if(ps_data->first_boot)
	{
		ps_data->first_boot = false;
		stk3x1x_set_ps_thd_h(ps_data, ps_data->ps_thd_h);
		stk3x1x_set_ps_thd_l(ps_data, ps_data->ps_thd_l);
	}
#endif


	w_state_reg &= ~(STK_STATE_EN_PS_MASK | STK_STATE_EN_WAIT_MASK | STK_STATE_EN_AK_MASK);
	if(enable)
	{
		w_state_reg |= STK_STATE_EN_PS_MASK;
		if(!(ps_data->als_enabled))
			w_state_reg |= STK_STATE_EN_WAIT_MASK;
#ifdef STK_TUNE0
	//	w_state_reg |= 	STK_STATE_EN_AK_MASK;
#elif defined(STK_TUNE1)
		w_state_reg |= 	0x60;
#endif
	}
	ret = rda_lightsensor_i2c_write(ps_data->client, STK_STATE_REG, &w_state_reg, 1);
	if (ret < 0)
	{
		printk(KERN_ERR "%s: write i2c error, ret=%d\n", __func__, ret);
		return ret;
	}

	if(enable)
	{
#ifdef STK_TUNE0
		if (!(ps_data->psi_set))
			hrtimer_start(&ps_data->ps_tune0_timer, ps_data->ps_tune0_delay, HRTIMER_MODE_REL);
#endif

#ifdef STK_POLL_PS
		hrtimer_start(&ps_data->ps_timer, ps_data->ps_poll_delay, HRTIMER_MODE_REL);
		ps_data->ps_distance_last = -1;
#endif
#ifndef STK_POLL_PS
	#ifndef STK_POLL_ALS
		if(!(ps_data->als_enabled))
	#endif	/* #ifndef STK_POLL_ALS	*/
			enable_irq(ps_data->irq);
#endif	/* #ifndef STK_POLL_PS */
		ps_data->ps_enabled = true;
		msleep(2);
		ret = stk3x1x_get_flag(ps_data);
		if (ret < 0)
		{
			printk(KERN_ERR "%s: read i2c error, ret=%d\n", __func__, ret);
			return ret;
		}
		near_far_state = ret & STK_FLG_NF_MASK;
		ps_data->ps_distance_last = near_far_state;
		input_report_abs(ps_data->ps_input_dev, ABS_DISTANCE, near_far_state);
		input_sync(ps_data->ps_input_dev);
		wake_lock_timeout(&ps_data->ps_wakelock, 3*HZ);
		reading = stk3x1x_get_ps_reading(ps_data);
		//printk(KERN_INFO "%s: ps input event=%d, ps code = %d\n",__func__, near_far_state, reading);
	}
	else
	{
#ifdef STK_POLL_PS
		hrtimer_cancel(&ps_data->ps_timer);
#else
#ifndef STK_POLL_ALS
		if(!(ps_data->als_enabled))
#endif
			disable_irq(ps_data->irq);
#endif
		ps_data->ps_enabled = false;
	}
	return ret;
}

static int32_t stk3x1x_enable_als(struct stk3x1x_data *ps_data, uint8_t enable)
{
	int32_t ret;
	uint8_t w_state_reg, r_state_reg;
	uint8_t curr_als_enable = (ps_data->als_enabled)?1:0;

	if(curr_als_enable == enable)
		return 0;

	if(enable)
	{
		ret = stk3x1x_get_ir_reading(ps_data);
		if(ret > 0)
			ps_data->ir_code = ret;
	}

#ifndef STK_POLL_ALS
	if (enable)
	{
		stk3x1x_set_als_thd_h(ps_data, 0x0000);
		stk3x1x_set_als_thd_l(ps_data, 0xFFFF);
	}
#endif
	ret = rda_lightsensor_i2c_read(ps_data->client, STK_STATE_REG, &r_state_reg, 1);
	if (ret < 0)
	{
		printk(KERN_ERR "%s: write i2c error\n", __func__);
		return ret;
	}
	w_state_reg = (uint8_t)(r_state_reg & (~(STK_STATE_EN_ALS_MASK | STK_STATE_EN_WAIT_MASK)));
	if(enable)
		w_state_reg |= STK_STATE_EN_ALS_MASK;
	else if (ps_data->ps_enabled)
		w_state_reg |= STK_STATE_EN_WAIT_MASK;


	ret = rda_lightsensor_i2c_write(ps_data->client, STK_STATE_REG, &w_state_reg, 1);
	if (ret < 0)
	{
		printk(KERN_ERR "%s: write i2c error\n", __func__);
		return ret;
	}

	if (enable)
	{
		ps_data->als_enabled = true;
#ifdef STK_POLL_ALS
		hrtimer_start(&ps_data->als_timer, ps_data->als_poll_delay, HRTIMER_MODE_REL);
#else
#ifndef STK_POLL_PS
		if(!(ps_data->ps_enabled))
#endif
			enable_irq(ps_data->irq);
#endif
	}
	else
	{
		ps_data->als_enabled = false;
#ifdef STK_POLL_ALS
		hrtimer_cancel(&ps_data->als_timer);
#else
#ifndef STK_POLL_PS
		if(!(ps_data->ps_enabled))
#endif
			disable_irq(ps_data->irq);
#endif
	}
	return ret;
}

static inline int32_t stk3x1x_get_als_reading(struct stk3x1x_data *ps_data)
{
	int32_t word_data,ret;
	uint8_t tmp_word_data[2];
#ifdef STK_ALS_FIR
	int index;
#endif

	ret = rda_lightsensor_i2c_read(ps_data->client, STK_DATA1_ALS_REG, tmp_word_data, 2);
	if(ret < 0)
	{
		printk(KERN_ERR "%s fail, err=0x%x:%x", __func__, tmp_word_data[0], tmp_word_data[1]);
		word_data = tmp_word_data[1] | tmp_word_data[0] << 8;
		return word_data;
	}
	word_data = tmp_word_data[1] | tmp_word_data[0] << 8;

#ifdef STK_ALS_FIR
	if(ps_data->fir.number < 8)
	{
		ps_data->fir.raw[ps_data->fir.number] = word_data;
		ps_data->fir.sum += word_data;
		ps_data->fir.number++;
		ps_data->fir.idx++;
	}
	else
	{
		index = ps_data->fir.idx % 8;
		ps_data->fir.sum -= ps_data->fir.raw[index];
		ps_data->fir.raw[index] = word_data;
		ps_data->fir.sum += word_data;
		ps_data->fir.idx++;
		word_data = ps_data->fir.sum/8;
	}
#endif

	return word_data;
}

static int32_t stk3x1x_set_irs_it_slp(struct stk3x1x_data *ps_data, uint16_t *slp_time)
{
	uint8_t irs_alsctrl;
	int32_t ret;

	irs_alsctrl = (ps_data->alsctrl_reg & 0x0F) - 2;
	switch(irs_alsctrl)
	{
		case 6:
			*slp_time = 12;
			break;
		case 7:
			*slp_time = 24;
			break;
		case 8:
			*slp_time = 48;
			break;
		case 9:
			*slp_time = 96;
			break;
		default:
			printk(KERN_ERR "%s: unknown ALS IT=0x%x\n", __func__, irs_alsctrl);
			ret = -EINVAL;
			return ret;
	}
	irs_alsctrl |= (ps_data->alsctrl_reg & 0xF0);
	ret = rda_lightsensor_i2c_write(ps_data->client, STK_ALSCTRL_REG, &irs_alsctrl, 1);
	if (ret < 0)
	{
		printk(KERN_ERR "%s: write i2c error\n", __func__);
		return ret;
	}
	return 0;
}

static int32_t stk3x1x_get_ir_reading(struct stk3x1x_data *ps_data)
{
	int32_t word_data, ret;
	uint8_t w_reg, retry = 0, tmp_word_data[2];
	uint8_t r_reg;
	uint16_t irs_slp_time = 100;
	bool re_enable_ps = false;

	if(ps_data->ps_enabled)
	{
#ifdef STK_TUNE0
		if (!(ps_data->psi_set))
		{
			hrtimer_cancel(&ps_data->ps_tune0_timer);
			cancel_work_sync(&ps_data->stk_ps_tune0_work);
		}
#endif
		stk3x1x_enable_ps(ps_data, 0);
		re_enable_ps = true;
	}

	ret = stk3x1x_set_irs_it_slp(ps_data, &irs_slp_time);
	if(ret < 0)
		goto irs_err_i2c_rw;

	ret = rda_lightsensor_i2c_read(ps_data->client, STK_STATE_REG, &r_reg, 1);
	if (ret < 0)
	{
		printk(KERN_ERR "%s: write i2c error\n", __func__);
		goto irs_err_i2c_rw;
	}

	w_reg = r_reg | STK_STATE_EN_IRS_MASK;
	ret = rda_lightsensor_i2c_write(ps_data->client, STK_STATE_REG, &w_reg, 1);
	if (ret < 0)
	{
		printk(KERN_ERR "%s: write i2c error\n", __func__);
		goto irs_err_i2c_rw;
	}
	msleep(irs_slp_time);

	do
	{
		msleep(3);
		ret = stk3x1x_get_flag(ps_data);
		if (ret < 0)
		{
			printk(KERN_ERR "%s: write i2c error\n", __func__);
			goto irs_err_i2c_rw;
		}
		retry++;
	}while(retry < 10 && ((ret&STK_FLG_IR_RDY_MASK) == 0));

	if(retry == 10)
	{
		printk(KERN_ERR "%s: ir data is not ready for 300ms\n", __func__);
		ret = -EINVAL;
		goto irs_err_i2c_rw;
	}
	/*
	ret = stk3x1x_get_flag(ps_data);
	if (ret < 0)
	{
		printk(KERN_ERR "%s: write i2c error\n", __func__);
		return ret;
	}
	*/
	ret = stk3x1x_set_flag(ps_data, ret, STK_FLG_IR_RDY_MASK);
	if (ret < 0)
	{
		printk(KERN_ERR "%s: write i2c error\n", __func__);
		goto irs_err_i2c_rw;
	}

	ret = rda_lightsensor_i2c_read(ps_data->client, STK_DATA1_IR_REG, tmp_word_data, 2);
	if(ret < 0)
	{
		printk(KERN_ERR "%s fail, err=0x%x:%x", __func__, tmp_word_data[0], tmp_word_data[1]);
		word_data = tmp_word_data[1] | tmp_word_data[0] << 8;
		ret = word_data;
		goto irs_err_i2c_rw;
	}
	word_data = tmp_word_data[1] | tmp_word_data[0] << 8;

	ret = rda_lightsensor_i2c_write(ps_data->client, STK_ALSCTRL_REG, &ps_data->alsctrl_reg, 1);
	if (ret < 0)
	{
		printk(KERN_ERR "%s: write i2c error\n", __func__);
		goto irs_err_i2c_rw;
	}
	if(re_enable_ps)
		stk3x1x_enable_ps(ps_data, 1);
	return word_data;

irs_err_i2c_rw:
	if(re_enable_ps)
		stk3x1x_enable_ps(ps_data, 1);
	return ret;
}

#ifdef STK_TUNE1
static int32_t stk3x1x_stk_get_ps_cali_file(char *r_buf, int8_t buf_size)
{
	struct file  *cali_file;
	mm_segment_t fs;
	ssize_t ret;

	cali_file = filp_open(STK_CALI_FILE, O_RDONLY,0);
	if(IS_ERR(cali_file))
	{
		printk(KERN_ERR "%s: filp_open error!\n", __func__);
		return -ENOENT;
	}
	else
	{
		fs = get_fs();
		set_fs(get_ds());
		ret = cali_file->f_op->read(cali_file,r_buf,buf_size,&cali_file->f_pos);
		if(ret < 0)
		{
			printk(KERN_ERR "%s: read error, ret=%d\n", __func__, ret);
			filp_close(cali_file,NULL);
			return -EIO;
		}
		set_fs(fs);
	}

	filp_close(cali_file,NULL);
	return 0;
}


static int32_t stk_stk_set_ps_cali_file(char *w_buf, int8_t w_buf_size)
{
	struct file  *cali_file;
	char r_buf[STK_CALI_FILE_SIZE] = {0};
	mm_segment_t fs;
	ssize_t ret;
	int8_t i;

	cali_file = filp_open(STK_CALI_FILE, O_CREAT | O_RDWR,0666);
	if(IS_ERR(cali_file))
	{
		printk(KERN_ERR "%s: filp_open for write error!\n", __func__);
		return -ENOENT;
	}
	else
	{
		fs = get_fs();
		set_fs(get_ds());

		ret = cali_file->f_op->write(cali_file,w_buf,w_buf_size,&cali_file->f_pos);
		if(ret != w_buf_size)
		{
			printk(KERN_ERR "%s: write error!\n", __func__);
			filp_close(cali_file,NULL);
			return -EIO;
		}
		cali_file->f_pos=0x00;
		ret = cali_file->f_op->read(cali_file ,r_buf ,w_buf_size ,&cali_file->f_pos);
		if(ret < 0)
		{
			printk(KERN_ERR "%s: read error!\n", __func__);
			filp_close(cali_file,NULL);
			return -EIO;
		}
		set_fs(fs);
		for(i=0;i<w_buf_size;i++)
		{
			if(r_buf[i] != w_buf[i])
			{
				printk(KERN_ERR "%s: read back error!, r_buf[%d](0x%x) != w_buf[%d](0x%x)\n", __func__, i, r_buf[i], i, w_buf[i]);
				filp_close(cali_file,NULL);
				return -EIO;
			}
		}

	}
	filp_close(cali_file,NULL);

	//printk(KERN_INFO "%s successfully\n", __func__);
	return 0;
}

static int32_t stk_get_ps_cali(struct stk3x1x_data *ps_data, uint16_t kd[])
{
	char r_buf[STK_CALI_FILE_SIZE] = {0};
	int32_t ret;

	if ((ret = stk3x1x_stk_get_ps_cali_file(r_buf, STK_CALI_FILE_SIZE)) < 0)
	{
		return ret;
	}
	else
	{
		if(r_buf[0] == STK_CALI_VER0 && r_buf[1] == STK_CALI_VER1)
		{
			kd[0] = ((uint16_t)r_buf[2] << 8) | r_buf[3];
			kd[1] = ((uint16_t)r_buf[4] << 8) | r_buf[5];
			kd[2] = ((uint16_t)r_buf[6] << 8) | r_buf[7];
		}
		else
		{
			printk(KERN_ERR "%s: cali version number error! r_buf=0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x\n", __func__, r_buf[0],
				r_buf[1], r_buf[2], r_buf[3], r_buf[4], r_buf[5], r_buf[6], r_buf[7]);
			return -EINVAL;
		}
	}
	return 0;
}


static int32_t stk_set_ps_to_file(uint16_t ps_ave[])
{
	char w_buf[STK_CALI_FILE_SIZE] = {0};

	w_buf[0] = STK_CALI_VER0;
	w_buf[1] = STK_CALI_VER1;
	w_buf[2] = ((ps_ave[0] & 0xFF00) >> 8);
	w_buf[3] = (ps_ave[0] & 0x00FF);
	w_buf[4] = ((ps_ave[1] & 0xFF00) >> 8);
	w_buf[5] = (ps_ave[1] & 0x00FF);
	w_buf[6] = ((ps_ave[2] & 0xFF00) >> 8);
	w_buf[7] = (ps_ave[2] & 0x00FF);
	return stk_stk_set_ps_cali_file(w_buf, sizeof(char)*STK_CALI_FILE_SIZE);
}


static int32_t stk_get_svrl_ps_data(struct stk3x1x_data *ps_data, uint16_t ps_stat_data[])
{
	uint32_t ps_adc;
	int8_t data_count = 0;
	uint32_t ave_ps_int32 = 0;
	int32_t ret;
	uint8_t w_state_reg, org_state_reg, org_int_reg,r_reg;
	uint8_t reg_temp;

	ps_stat_data[0] = 0;
	ps_stat_data[2] = 9999;
	ps_stat_data[1] = 0;

	ret = rda_lightsensor_i2c_read(ps_data->client, STK_STATE_REG, &r_reg, 1);
	if (ret < 0)
	{
		printk(KERN_ERR "%s: write i2c error\n", __func__);
		return ret;
	}
	org_state_reg = r_reg;

	w_state_reg = (uint8_t)(r_reg & (~(STK_STATE_EN_AK_MASK | STK_STATE_EN_ASO_MASK)));
	w_state_reg |= STK_STATE_EN_PS_MASK;
	ret = rda_lightsensor_i2c_write(ps_data->client, STK_STATE_REG, &w_state_reg, 1);
	if (ret < 0)
	{
		printk(KERN_ERR "%s: write i2c error\n", __func__);
		return ret;
	}

	ret = rda_lightsensor_i2c_read(ps_data->client, STK_INT_REG, &r_reg, 1);
	if (ret < 0)
	{
		printk(KERN_ERR "%s: write i2c error\n", __func__);
		return ret;
	}
	org_int_reg = r_reg;
	reg_temp = 0;
	ret = rda_lightsensor_i2c_write(ps_data->client, STK_INT_REG, &reg_temp, 1);
	if (ret < 0)
	{
		printk(KERN_ERR "%s: write i2c error\n", __func__);
		return ret;
	}


	while(data_count < STK_CALI_SAMPLE_NO)
	{
		msleep(110);
		ps_adc = stk3x1x_get_ps_reading(ps_data);
		printk(KERN_INFO "%s: ps_adc #%d=%d\n", __func__, data_count, ps_adc);
		if(ps_adc < 0)
			return ps_adc;

		ave_ps_int32 +=  ps_adc;
		if(ps_adc > ps_stat_data[0])
			ps_stat_data[0] = ps_adc;
		if(ps_adc < ps_stat_data[2])
			ps_stat_data[2] = ps_adc;
		data_count++;
	}
	ave_ps_int32 /= STK_CALI_SAMPLE_NO;
	ps_stat_data[1] = (int16_t)ave_ps_int32;

	ret = rda_lightsensor_i2c_write(ps_data->client, STK_INT_REG, &org_int_reg, 1);
	if (ret < 0)
	{
		printk(KERN_ERR "%s: write i2c error\n", __func__);
		return ret;
	}

	ret = rda_lightsensor_i2c_write(ps_data->client, STK_STATE_REG, &org_state_reg, 1);
	if (ret < 0)
	{
		printk(KERN_ERR "%s: write i2c error\n", __func__);
		return ret;
	}

	return 0;
}
#if (KTYPE_FAC==K_TYPE_CT)
static int32_t stk_judge_ct_range(struct stk3x1x_data *ps_data, uint16_t ps_stat)
{
	uint16_t saved_item[3];
	int32_t ret;
	if(ps_stat < STK_CALI_MAX && ps_stat > STK_CALI_MIN)
	{
		//printk(KERN_INFO "%s: ps_stat=%d\n", __func__, ps_stat);
		ps_data->kdata = ps_stat;
		saved_item[0] = ps_stat;
		saved_item[1] = D_HT;
		saved_item[2] = D_LT;
		ret = stk_set_ps_to_file(saved_item);
		return ret;
	}
	printk(KERN_ERR "%s: ps_stat(%d) out of range", __func__, ps_stat);
	return -1;
}
#elif (KTYPE_FAC==K_TYPE_NF)
static int32_t stk_judge_ht_range(struct stk3x1x_data *ps_data, uint16_t ps_stat)
{
	if(ps_stat < STK_CALI_HT_MAX && ps_stat > STK_CALI_HT_MIN)
	{
		//printk(KERN_INFO "%s: kdata=%d\n", __func__, ps_stat);
		ps_data->kdata = ps_stat;
		return 0;
	}
	printk(KERN_ERR "%s: kdata(%d) out of range", __func__, ps_stat);
	return -1;
}

static int32_t stk_judge_lt_range(struct stk3x1x_data *ps_data, uint16_t ps_stat)
{
	if(ps_stat < STK_CALI_LT_MAX && ps_stat > STK_CALI_LT_MIN)
	{
		//printk(KERN_INFO "%s: kdata1=%d\n", __func__, ps_stat);
		ps_data->kdata1 = ps_stat;
		return 0;
	}
	printk(KERN_ERR "%s: kdata1(%d) out of range", __func__, ps_stat);
	return -1;
}
#endif
static int32_t stk_set_ps_cali(struct stk3x1x_data *ps_data, uint8_t ht)
{
	uint16_t ps_statistic_data[3] = {0};
	int32_t ret = 0;
	uint16_t saved_item[3];

	ret = stk_get_svrl_ps_data(ps_data, ps_statistic_data);
	if(ret < 0)
	{
		printk(KERN_ERR "%s: calibration fail\n", __func__);
		return ret;
	}

#if (KTYPE_FAC==K_TYPE_CT)
	ret = stk_judge_ct_range(ps_data, ps_statistic_data[1]);
	if(ret < 0)
		printk(KERN_ERR "%s: calibration failed\n", __func__);
#elif (KTYPE_FAC==K_TYPE_NF)
	if(ht == 1)
	{
		ret = stk_judge_ht_range(ps_data, ps_statistic_data[1]);
		if(ret < 0)
			printk(KERN_ERR "%s: calibration failed\n", __func__);
	}
	else
	{
		ret = stk_judge_lt_range(ps_data, ps_statistic_data[1]);
		if(ret < 0)
			printk(KERN_ERR "%s: calibration failed\n", __func__);
	}

	if(ps_data->kdata != 0 && ps_data->kdata1 != 0)
	{
		if((ps_data->kdata - ps_data->kdata1) >= STK_CALI_HT_LT_MIN &&
			(ps_data->kdata - ps_data->kdata1) < (D_CT - STK_CALI_LT_CT_MIN))
		{
			saved_item[0] = ps_data->kdata - D_CT;
			saved_item[1] = D_CT;
			saved_item[2] = ps_data->kdata1 - ps_data->kdata + D_CT;
			printk(KERN_INFO "%s: saved_item=%d, %d,%d", __func__, saved_item[0], saved_item[1], saved_item[2]);
			ret = stk_set_ps_to_file(saved_item);
		}
		else
			printk(KERN_ERR "%s: kdata(%d) and kdata1(%d) are too close\n", __func__, ps_data->kdata, ps_data->kdata1);
	}
#endif
	return ret;
}
#endif	/*#ifdef STK_TUNE1	*/

static ssize_t stk_als_code_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct stk3x1x_data *ps_data =  dev_get_drvdata(dev);
	int32_t reading;

	reading = stk3x1x_get_als_reading(ps_data);
	return scnprintf(buf, PAGE_SIZE, "%d\n", reading);
}


static ssize_t stk_als_enable_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct stk3x1x_data *ps_data =  dev_get_drvdata(dev);
	int32_t enable, ret;
	uint8_t r_reg;

	mutex_lock(&ps_data->io_lock);
	enable = (ps_data->als_enabled)?1:0;
	mutex_unlock(&ps_data->io_lock);
	(void)rda_lightsensor_i2c_read(ps_data->client,STK_STATE_REG, &r_reg, 1);
	ret= (r_reg & STK_STATE_EN_ALS_MASK)?1:0;

	if(enable != ret)
		printk(KERN_ERR "%s: driver and sensor mismatch! driver_enable=0x%x, sensor_enable=%x\n", __func__, enable, ret);

	return scnprintf(buf, PAGE_SIZE, "%d\n", ret);
}

static ssize_t stk_als_enable_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t size)
{
	struct stk3x1x_data *ps_data = dev_get_drvdata(dev);
	uint8_t en;
	if (sysfs_streq(buf, "1"))
		en = 1;
	else if (sysfs_streq(buf, "0"))
		en = 0;
	else
	{
		printk(KERN_ERR "%s, invalid value %d\n", __func__, *buf);
		return -EINVAL;
	}
	//printk(KERN_INFO "%s: Enable ALS : %d\n", __func__, en);
	mutex_lock(&ps_data->io_lock);
	stk3x1x_enable_als(ps_data, en);
	mutex_unlock(&ps_data->io_lock);
	return size;
}

static ssize_t stk_als_lux_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct stk3x1x_data *ps_data = dev_get_drvdata(dev);
	int32_t als_reading;
	uint32_t als_lux;
	als_reading = stk3x1x_get_als_reading(ps_data);
	als_lux = stk_alscode2lux(ps_data, als_reading);
	return scnprintf(buf, PAGE_SIZE, "%d lux\n", als_lux);
}

static ssize_t stk_als_lux_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t size)
{
	struct stk3x1x_data *ps_data =  dev_get_drvdata(dev);
	unsigned long value = 0;
	int ret;
	ret = strict_strtoul(buf, 16, &value);
	if(ret < 0)
	{
		printk(KERN_ERR "%s:strict_strtoul failed, ret=0x%x\n", __func__, ret);
		return ret;
	}
	ps_data->als_lux_last = value;
	input_report_abs(ps_data->als_input_dev, ABS_MISC, value);
	input_sync(ps_data->als_input_dev);
	//printk(KERN_INFO "%s: als input event %ld lux\n",__func__, value);

	return size;
}


static ssize_t stk_als_transmittance_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct stk3x1x_data *ps_data =  dev_get_drvdata(dev);
	int32_t transmittance;
	transmittance = ps_data->als_transmittance;
	return scnprintf(buf, PAGE_SIZE, "%d\n", transmittance);
}


static ssize_t stk_als_transmittance_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t size)
{
	struct stk3x1x_data *ps_data =  dev_get_drvdata(dev);
	unsigned long value = 0;
	int ret;
	ret = strict_strtoul(buf, 10, &value);
	if(ret < 0)
	{
		printk(KERN_ERR "%s:strict_strtoul failed, ret=0x%x\n", __func__, ret);
		return ret;
	}
	ps_data->als_transmittance = value;
	return size;
}

static ssize_t stk_als_delay_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct stk3x1x_data *ps_data =  dev_get_drvdata(dev);
	int64_t delay;
	mutex_lock(&ps_data->io_lock);
	delay = ktime_to_ns(ps_data->als_poll_delay);
	mutex_unlock(&ps_data->io_lock);
	return scnprintf(buf, PAGE_SIZE, "%lld\n", delay);
}


static ssize_t stk_als_delay_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t size)
{
	uint64_t value = 0;
	int ret;
	struct stk3x1x_data *ps_data =  dev_get_drvdata(dev);
	ret = strict_strtoull(buf, 10, &value);
	if(ret < 0)
	{
		printk(KERN_ERR "%s:strict_strtoull failed, ret=0x%x\n", __func__, ret);
		return ret;
	}
#ifdef STK_DEBUG_PRINTF
	printk(KERN_INFO "%s: set als poll delay=%lld\n", __func__, value);
#endif
	if(value < MIN_ALS_POLL_DELAY_NS)
	{
		printk(KERN_ERR "%s: delay is too small\n", __func__);
		value = MIN_ALS_POLL_DELAY_NS;
	}
	mutex_lock(&ps_data->io_lock);
	if(value != ktime_to_ns(ps_data->als_poll_delay))
		ps_data->als_poll_delay = ns_to_ktime(value);
	mutex_unlock(&ps_data->io_lock);
	return size;
}

static ssize_t stk_als_ir_code_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct stk3x1x_data *ps_data =  dev_get_drvdata(dev);
	int32_t reading;
	reading = stk3x1x_get_ir_reading(ps_data);
	return scnprintf(buf, PAGE_SIZE, "%d\n", reading);
}



static ssize_t stk_ps_code_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct stk3x1x_data *ps_data =  dev_get_drvdata(dev);
	uint32_t reading;
	reading = stk3x1x_get_ps_reading(ps_data);
	return scnprintf(buf, PAGE_SIZE, "%d\n", reading);
}

static ssize_t stk_ps_enable_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	int32_t enable, ret;
	uint8_t r_reg;
	struct stk3x1x_data *ps_data =  dev_get_drvdata(dev);

	mutex_lock(&ps_data->io_lock);
	enable = (ps_data->ps_enabled)?1:0;
	mutex_unlock(&ps_data->io_lock);
	(void)rda_lightsensor_i2c_read(ps_data->client,STK_STATE_REG, &r_reg, 1);
	ret = (r_reg & STK_STATE_EN_PS_MASK)?1:0;

	if(enable != ret)
		printk(KERN_ERR "%s: driver and sensor mismatch! driver_enable=0x%x, sensor_enable=%x\n", __func__, enable, ret);

	return scnprintf(buf, PAGE_SIZE, "%d\n", ret);
}

static ssize_t stk_ps_enable_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t size)
{
	struct stk3x1x_data *ps_data =  dev_get_drvdata(dev);
	uint8_t en;
	if (sysfs_streq(buf, "1"))
		en = 1;
	else if (sysfs_streq(buf, "0"))
		en = 0;
	else
	{
		printk(KERN_ERR "%s, invalid value %d\n", __func__, *buf);
		return -EINVAL;
	}
	//printk(KERN_INFO "%s: Enable PS : %d\n", __func__, en);
	mutex_lock(&ps_data->io_lock);
	stk3x1x_enable_ps(ps_data, en);
	mutex_unlock(&ps_data->io_lock);
	return size;
}

static ssize_t stk_ps_enable_aso_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	int32_t ret;
	uint8_t r_reg;
	struct stk3x1x_data *ps_data =  dev_get_drvdata(dev);

	(void)rda_lightsensor_i2c_read(ps_data->client,STK_STATE_REG, &r_reg, 1);
	ret = (r_reg & STK_STATE_EN_ASO_MASK)?1:0;

	return scnprintf(buf, PAGE_SIZE, "%d\n", ret);
}

static ssize_t stk_ps_enable_aso_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t size)
{
	struct stk3x1x_data *ps_data =  dev_get_drvdata(dev);
	uint8_t en;
	int32_t ret;
	uint8_t w_state_reg,r_reg;

	if (sysfs_streq(buf, "1"))
		en = 1;
	else if (sysfs_streq(buf, "0"))
		en = 0;
	else
	{
		printk(KERN_ERR "%s, invalid value %d\n", __func__, *buf);
		return -EINVAL;
	}
	//printk(KERN_INFO "%s: Enable PS ASO : %d\n", __func__, en);

	ret = rda_lightsensor_i2c_read(ps_data->client, STK_STATE_REG, &r_reg, 1);
	if (ret < 0)
	{
		printk(KERN_ERR "%s: write i2c error\n", __func__);
		return ret;
	}
	w_state_reg = (uint8_t)(r_reg & (~STK_STATE_EN_ASO_MASK));
	if(en)
		w_state_reg |= STK_STATE_EN_ASO_MASK;

	ret = rda_lightsensor_i2c_write(ps_data->client, STK_STATE_REG, &w_state_reg, 1);
	if (ret < 0)
	{
		printk(KERN_ERR "%s: write i2c error\n", __func__);
		return ret;
	}

	return size;
}


static ssize_t stk_ps_offset_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct stk3x1x_data *ps_data =  dev_get_drvdata(dev);
	int32_t word_data, ret;
	uint8_t tmp_word_data[2];

	ret = rda_lightsensor_i2c_read(ps_data->client, STK_DATA1_OFFSET_REG, tmp_word_data, 2);
	if(ret < 0)
	{
		printk(KERN_ERR "%s fail, err=0x%x:%x", __func__, tmp_word_data[0],tmp_word_data[1]);
		word_data = tmp_word_data[1] | tmp_word_data[0] << 8;
		return word_data;
	}

	word_data = tmp_word_data[1] | tmp_word_data[0] << 8;
	return scnprintf(buf, PAGE_SIZE, "%d\n", word_data);
}

static ssize_t stk_ps_offset_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t size)
{
	struct stk3x1x_data *ps_data =  dev_get_drvdata(dev);
	unsigned long value = 0;
	int ret;
	uint16_t offset;

	ret = strict_strtoul(buf, 10, &value);
	if(ret < 0)
	{
		printk(KERN_ERR "%s:strict_strtoul failed, ret=0x%x\n", __func__, ret);
		return ret;
	}
	if(value > 65535)
	{
		printk(KERN_ERR "%s: invalid value, offset=%ld\n", __func__, value);
		return -EINVAL;
	}

	offset = (uint16_t) ((value&0x00FF) << 8) | ((value&0xFF00) >>8);
	ret = rda_lightsensor_i2c_write(ps_data->client,STK_DATA1_OFFSET_REG,(u8 *)&offset, 2);
	if(ret < 0)
	{
		printk(KERN_ERR "%s: write i2c error\n", __func__);
		return ret;
	}
	return size;
}


static ssize_t stk_ps_distance_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct stk3x1x_data *ps_data =  dev_get_drvdata(dev);
	int32_t dist=1, ret;

	ret = stk3x1x_get_flag(ps_data);
	if(ret < 0)
	{
		printk(KERN_ERR "%s: stk3x1x_get_flag failed, ret=0x%x\n", __func__, ret);
		return ret;
	}
	dist = (ret & STK_FLG_NF_MASK)?1:0;

	ps_data->ps_distance_last = dist;
	input_report_abs(ps_data->ps_input_dev, ABS_DISTANCE, dist);
	input_sync(ps_data->ps_input_dev);
	wake_lock_timeout(&ps_data->ps_wakelock, 3*HZ);
	//printk(KERN_INFO "%s: ps input event %d cm\n",__func__, dist);
	return scnprintf(buf, PAGE_SIZE, "%d\n", dist);
}


static ssize_t stk_ps_distance_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t size)
{
	struct stk3x1x_data *ps_data =  dev_get_drvdata(dev);
	unsigned long value = 0;
	int ret;
	ret = strict_strtoul(buf, 10, &value);
	if(ret < 0)
	{
		printk(KERN_ERR "%s:strict_strtoul failed, ret=0x%x\n", __func__, ret);
		return ret;
	}
	ps_data->ps_distance_last = value;
	input_report_abs(ps_data->ps_input_dev, ABS_DISTANCE, value);
	input_sync(ps_data->ps_input_dev);
	wake_lock_timeout(&ps_data->ps_wakelock, 3*HZ);
	//printk(KERN_INFO "%s: ps input event %ld cm\n",__func__, value);
	return size;
}


static ssize_t stk_ps_code_thd_l_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	uint8_t ps_thd_l1_reg, ps_thd_l2_reg;
	int32_t ps_thd_data;
	struct stk3x1x_data *ps_data =  dev_get_drvdata(dev);

	(void)rda_lightsensor_i2c_read(ps_data->client,STK_THDL1_PS_REG, &ps_thd_l1_reg, 1);
	(void)rda_lightsensor_i2c_read(ps_data->client,STK_THDL2_PS_REG, &ps_thd_l2_reg, 1);

	ps_thd_data = ps_thd_l1_reg<<8 | ps_thd_l2_reg;
	return scnprintf(buf, PAGE_SIZE, "%d\n", ps_thd_data);
}


static ssize_t stk_ps_code_thd_l_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t size)
{
	struct stk3x1x_data *ps_data =  dev_get_drvdata(dev);
	unsigned long value = 0;
	int ret;
	ret = strict_strtoul(buf, 10, &value);
	if(ret < 0)
	{
		printk(KERN_ERR "%s:strict_strtoul failed, ret=0x%x\n", __func__, ret);
		return ret;
	}
	stk3x1x_set_ps_thd_l(ps_data, value);
	return size;
}

static ssize_t stk_ps_code_thd_h_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	uint8_t ps_thd_h1_reg, ps_thd_h2_reg;
	int32_t ps_thd_data;
	struct stk3x1x_data *ps_data =  dev_get_drvdata(dev);

	(void)rda_lightsensor_i2c_read(ps_data->client,STK_THDH1_PS_REG, &ps_thd_h1_reg, 1);
	(void)rda_lightsensor_i2c_read(ps_data->client,STK_THDH2_PS_REG, &ps_thd_h2_reg, 1);

	ps_thd_data = ps_thd_h1_reg<<8 | ps_thd_h2_reg;
	return scnprintf(buf, PAGE_SIZE, "%d\n", ps_thd_data);
}


static ssize_t stk_ps_code_thd_h_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t size)
{
	struct stk3x1x_data *ps_data =  dev_get_drvdata(dev);
	unsigned long value = 0;
	int ret;
	ret = strict_strtoul(buf, 10, &value);
	if(ret < 0)
	{
		printk(KERN_ERR "%s:strict_strtoul failed, ret=0x%x\n", __func__, ret);
		return ret;
	}
	stk3x1x_set_ps_thd_h(ps_data, value);
	return size;
}

#if 0
static ssize_t stk_als_lux_thd_l_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	int32_t als_thd_l0_reg,als_thd_l1_reg;
	struct stk3x1x_data *ps_data =  dev_get_drvdata(dev);
	uint32_t als_lux;

	als_thd_l0_reg = i2c_smbus_read_byte_data(ps_data->client,STK_THDL1_ALS_REG);
	als_thd_l1_reg = i2c_smbus_read_byte_data(ps_data->client,STK_THDL2_ALS_REG);
	if(als_thd_l0_reg < 0)
	{
		printk(KERN_ERR "%s fail, err=0x%x", __func__, als_thd_l0_reg);
		return -EINVAL;
	}
	if(als_thd_l1_reg < 0)
	{
		printk(KERN_ERR "%s fail, err=0x%x", __func__, als_thd_l1_reg);
		return -EINVAL;
	}
	als_thd_l0_reg|=(als_thd_l1_reg<<8);
	als_lux = stk_alscode2lux(ps_data, als_thd_l0_reg);
	return scnprintf(buf, PAGE_SIZE, "%d\n", als_lux);
}


static ssize_t stk_als_lux_thd_l_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t size)
{
	struct stk3x1x_data *ps_data =  dev_get_drvdata(dev);
	unsigned long value = 0;
	int ret;
	ret = strict_strtoul(buf, 10, &value);
	if(ret < 0)
	{
		printk(KERN_ERR "%s:strict_strtoul failed, ret=0x%x\n", __func__, ret);
		return ret;
	}
	value = stk_lux2alscode(ps_data, value);
	stk3x1x_set_als_thd_l(ps_data, value);
	return size;
}

static ssize_t stk_als_lux_thd_h_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	int32_t als_thd_h0_reg,als_thd_h1_reg;
	struct stk3x1x_data *ps_data =  dev_get_drvdata(dev);
	uint32_t als_lux;

	als_thd_h0_reg = i2c_smbus_read_byte_data(ps_data->client,STK_THDH1_ALS_REG);
	als_thd_h1_reg = i2c_smbus_read_byte_data(ps_data->client,STK_THDH2_ALS_REG);
	if(als_thd_h0_reg < 0)
	{
		printk(KERN_ERR "%s fail, err=0x%x", __func__, als_thd_h0_reg);
		return -EINVAL;
	}
	if(als_thd_h1_reg < 0)
	{
		printk(KERN_ERR "%s fail, err=0x%x", __func__, als_thd_h1_reg);
		return -EINVAL;
	}
	als_thd_h0_reg|=(als_thd_h1_reg<<8);
	als_lux = stk_alscode2lux(ps_data, als_thd_h0_reg);
	return scnprintf(buf, PAGE_SIZE, "%d\n", als_lux);
}


static ssize_t stk_als_lux_thd_h_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t size)
{
	struct stk3x1x_data *ps_data =  dev_get_drvdata(dev);
	unsigned long value = 0;
	int ret;
	ret = strict_strtoul(buf, 10, &value);
	if(ret < 0)
	{
		printk(KERN_ERR "%s:strict_strtoul failed, ret=0x%x\n", __func__, ret);
		return ret;
	}
	value = stk_lux2alscode(ps_data, value);
	stk3x1x_set_als_thd_h(ps_data, value);
	return size;
}
#endif


static ssize_t stk_all_reg_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	uint8_t ps_reg[27];
	int32_t ret;
	uint8_t cnt;
	struct stk3x1x_data *ps_data =  dev_get_drvdata(dev);
	for(cnt=0;cnt<25;cnt++)
	{
		ret = rda_lightsensor_i2c_read(ps_data->client, cnt, (ps_reg + cnt), 1);
		if(ret < 0)
		{
			printk(KERN_ERR "stk_all_reg_show:rda_lightsensor_i2c_read fail, ret=%d", ps_reg[cnt]);
			return -EINVAL;
		}
		else
		{
			printk(KERN_INFO "reg[0x%2X]=0x%2X\n", cnt, ps_reg[cnt]);
		}
	}
	ret = rda_lightsensor_i2c_read(ps_data->client, STK_PDT_ID_REG, (ps_reg + cnt), 1);
	if(ret < 0)
	{
		printk( KERN_ERR "all_reg_show:rda_lightsensor_i2c_read fail, ret=%d", ps_reg[cnt]);
		return -EINVAL;
	}
	printk( KERN_INFO "reg[0x%x]=0x%2X\n", STK_PDT_ID_REG, ps_reg[cnt]);
	cnt++;
	ret = rda_lightsensor_i2c_read(ps_data->client, STK_RSRVD_REG, (ps_reg + cnt), 1);
	if(ret < 0)
	{
		printk( KERN_ERR "all_reg_show:rda_lightsensor_i2c_read fail, ret=%d", ps_reg[cnt]);
		return -EINVAL;
	}
	printk( KERN_INFO "reg[0x%x]=0x%2X\n", STK_RSRVD_REG, ps_reg[cnt]);

	//return scnprintf(buf, PAGE_SIZE, "%2X %2X %2X %2X %2X,%2X %2X %2X %2X %2X,%2X %2X %2X %2X %2X,%2X %2X %2X %2X %2X,%2X %2X %2X %2X %2X,%2X %2X\n",
	return scnprintf(buf, PAGE_SIZE, "[0]%2X [1]%2X [2]%2X [3]%2X [4]%2X [5]%2X [6/7 HTHD]%2X,%2X [8/9 LTHD]%2X, %2X [A]%2X [B]%2X [C]%2X [D]%2X [E/F Aoff]%2X,%2X,[10]%2X [11/12 PS]%2X,%2X [13]%2X [14]%2X [15/16 Foff]%2X,%2X [17]%2X [18]%2X [3E]%2X [3F]%2X\n",
		ps_reg[0], ps_reg[1], ps_reg[2], ps_reg[3], ps_reg[4], ps_reg[5], ps_reg[6], ps_reg[7], ps_reg[8],
		ps_reg[9], ps_reg[10], ps_reg[11], ps_reg[12], ps_reg[13], ps_reg[14], ps_reg[15], ps_reg[16], ps_reg[17],
		ps_reg[18], ps_reg[19], ps_reg[20], ps_reg[21], ps_reg[22], ps_reg[23], ps_reg[24], ps_reg[25], ps_reg[26]);
}


static ssize_t stk_status_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	uint8_t ps_reg[27];
	int32_t ret;
	uint8_t cnt;
	struct stk3x1x_data *ps_data =  dev_get_drvdata(dev);
	for(cnt=0;cnt<25;cnt++)
	{
		ret = rda_lightsensor_i2c_read(ps_data->client, cnt, (ps_reg + cnt), 1);
		if(ret < 0)
		{
			printk(KERN_ERR "stk_all_reg_show:rda_lightsensor_i2c_read fail, ret=%d", ps_reg[cnt]);
			return -EINVAL;
		}
		else
		{
			//printk(KERN_INFO "reg[0x%2X]=0x%2X\n", cnt, ps_reg[cnt]);
		}
	}
	ret = rda_lightsensor_i2c_read(ps_data->client, STK_PDT_ID_REG, (ps_reg + cnt), 1);
	if(ret < 0)
	{
		printk( KERN_ERR "all_reg_show:rda_lightsensor_i2c_read fail, ret=%d", ps_reg[cnt]);
		return -EINVAL;
	}
	//printk( KERN_INFO "reg[0x%x]=0x%2X\n", STK_PDT_ID_REG, ps_reg[cnt]);
	cnt++;
	ret = rda_lightsensor_i2c_read(ps_data->client, STK_RSRVD_REG, (ps_reg + cnt), 1);
	if(ret < 0)
	{
		printk( KERN_ERR "all_reg_show:rda_lightsensor_i2c_read fail, ret=%d", ps_reg[cnt]);
		return -EINVAL;
	}
	//printk( KERN_INFO "reg[0x%x]=0x%2X\n", STK_RSRVD_REG, ps_reg[cnt]);

	return scnprintf(buf, PAGE_SIZE, "[PS=%2X] [ALS=%2X] [WAIT=%4Xms] [EN_ASO=%2X] [EN_AK=%2X] [NEAR/FAR=%2X] [FLAG_OUI=%2X] [FLAG_PSINT=%2X] [FLAG_ALSINT=%2X]\n",
		ps_reg[0]&0x01,(ps_reg[0]&0x02)>>1,((ps_reg[0]&0x04)>>2)*ps_reg[5]*6,(ps_reg[0]&0x20)>>5,
		(ps_reg[0]&0x40)>>6,ps_reg[16]&0x01,(ps_reg[16]&0x04)>>2,(ps_reg[16]&0x10)>>4,(ps_reg[16]&0x20)>>5);
}

static ssize_t stk_recv_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	return 0;
}


static ssize_t stk_recv_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t size)
{
	unsigned long value = 0;
	int ret;
	uint8_t recv_data;
	struct stk3x1x_data *ps_data =  dev_get_drvdata(dev);

	if((ret = strict_strtoul(buf, 16, &value)) < 0)
	{
		printk(KERN_ERR "%s:strict_strtoul failed, ret=0x%x\n", __func__, ret);
		return ret;
	}
	(void)rda_lightsensor_i2c_read(ps_data->client,value, &recv_data, 1);
	//printk("%s: reg 0x%x=0x%x\n", __func__, (int)value, recv_data);
	return size;
}


static ssize_t stk_send_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	return 0;
}


static ssize_t stk_send_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t size)
{
	int addr, cmd;
	u8 addr_u8, cmd_u8;
	int32_t ret, i;
	char *token[10];
	struct stk3x1x_data *ps_data =  dev_get_drvdata(dev);

	for (i = 0; i < 2; i++)
		token[i] = strsep((char **)&buf, " ");
	if((ret = strict_strtoul(token[0], 16, (unsigned long *)&(addr))) < 0)
	{
		printk(KERN_ERR "%s:strict_strtoul failed, ret=0x%x\n", __func__, ret);
		return ret;
	}
	if((ret = strict_strtoul(token[1], 16, (unsigned long *)&(cmd))) < 0)
	{
		printk(KERN_ERR "%s:strict_strtoul failed, ret=0x%x\n", __func__, ret);
		return ret;
	}
	//printk(KERN_INFO "%s: write reg 0x%x=0x%x\n", __func__, addr, cmd);

	addr_u8 = (u8) addr;
	cmd_u8 = (u8) cmd;
	ret = rda_lightsensor_i2c_write(ps_data->client,addr_u8, &cmd_u8, 1);
	if (0 != ret)
	{
		printk(KERN_ERR "%s: rda_lightsensor_i2c_write fail\n", __func__);
		return ret;
	}

	return size;
}

#ifdef STK_TUNE0

static ssize_t stk_ps_cali_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct stk3x1x_data *ps_data =  dev_get_drvdata(dev);
	int32_t word_data, ret;
	uint8_t tmp_word_data[2];

	ret = rda_lightsensor_i2c_read(ps_data->client, 0x20, tmp_word_data, 2);
	if(ret < 0)
	{
		printk(KERN_ERR "%s fail, err=0x%x:%x", __func__, tmp_word_data[0],tmp_word_data[1]);
		word_data = tmp_word_data[1] | tmp_word_data[0] << 8;
		return word_data;
	}
	word_data = tmp_word_data[1] | tmp_word_data[0] << 8;

	ret = rda_lightsensor_i2c_read(ps_data->client, 0x22, tmp_word_data, 2);
	if(ret < 0)
	{
		printk(KERN_ERR "%s fail, err=0x%x:%x", __func__, tmp_word_data[0],tmp_word_data[1]);
		return tmp_word_data[0];
	}
	word_data += tmp_word_data[1] | tmp_word_data[0] << 8;

	printk("%s: psi_set=%d, psa=%d,psi=%d, word_data=%d\n", __func__,
		ps_data->psi_set, ps_data->psa, ps_data->psi, word_data);

	return 0;
}

#elif defined(STK_TUNE1)

static ssize_t stk_ps_cali_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct stk3x1x_data *ps_data =  dev_get_drvdata(dev);
	int ret;
	uint16_t kd[3] = {0xFFFF, 0xFFFF, 0xFFFF};

	printk(KERN_INFO "%s: state=%d\n", __func__, ps_data->state);
	ret = stk_get_ps_cali(ps_data, kd);
	return scnprintf(buf, PAGE_SIZE, "%d %d %d\n", kd[0], kd[1], kd[2]);
}


static ssize_t stk_ps_cali_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t size)
{
	uint64_t value = 0;
	int ret;
	struct stk3x1x_data *ps_data =  dev_get_drvdata(dev);

	ret = strict_strtoull(buf, 10, &value);
	if(ret < 0)
	{
		printk(KERN_ERR "%s:strict_strtoull failed, ret=0x%x\n", __func__, ret);
		return ret;
	}

	if(value > 0)
	{
		/* 1 for HT and 2 for LH */
		ret = stk_set_ps_cali(ps_data, value);
		if(ret < 0)
			return ret;
	}
	else
		printk(KERN_ERR "%s: (%llu) not supported mode\n", __func__, value);

	return size;
}
#endif	/* #ifdef STK_TUNE0 */

static struct device_attribute als_enable_attribute = __ATTR(enable,0664,stk_als_enable_show,stk_als_enable_store);
static struct device_attribute als_lux_attribute = __ATTR(lux,0664,stk_als_lux_show,stk_als_lux_store);
static struct device_attribute als_code_attribute = __ATTR(code, 0444, stk_als_code_show, NULL);
static struct device_attribute als_transmittance_attribute = __ATTR(transmittance,0664,stk_als_transmittance_show,stk_als_transmittance_store);
#if 0
static struct device_attribute als_lux_thd_l_attribute = __ATTR(luxthdl,0664,stk_als_lux_thd_l_show,stk_als_lux_thd_l_store);
static struct device_attribute als_lux_thd_h_attribute = __ATTR(luxthdh,0664,stk_als_lux_thd_h_show,stk_als_lux_thd_h_store);
#endif
static struct device_attribute als_poll_delay_attribute = __ATTR(delay,0664,stk_als_delay_show,stk_als_delay_store);
static struct device_attribute als_ir_code_attribute = __ATTR(ircode,0444,stk_als_ir_code_show,NULL);


static struct attribute *stk_als_attrs [] =
{
	&als_enable_attribute.attr,
	&als_lux_attribute.attr,
	&als_code_attribute.attr,
	&als_transmittance_attribute.attr,
#if 0
	&als_lux_thd_l_attribute.attr,
	&als_lux_thd_h_attribute.attr,
#endif
	&als_poll_delay_attribute.attr,
	&als_ir_code_attribute.attr,
	NULL
};

static struct attribute_group stk_als_attribute_group = {
	.name = "driver",
	.attrs = stk_als_attrs,
};


static struct device_attribute ps_enable_attribute = __ATTR(enable,0664,stk_ps_enable_show,stk_ps_enable_store);
static struct device_attribute ps_enable_aso_attribute = __ATTR(enableaso,0664,stk_ps_enable_aso_show,stk_ps_enable_aso_store);
static struct device_attribute ps_distance_attribute = __ATTR(distance,0664,stk_ps_distance_show, stk_ps_distance_store);
static struct device_attribute ps_offset_attribute = __ATTR(offset,0664,stk_ps_offset_show, stk_ps_offset_store);
static struct device_attribute ps_code_attribute = __ATTR(code, 0444, stk_ps_code_show, NULL);
static struct device_attribute ps_code_thd_l_attribute = __ATTR(codethdl,0664,stk_ps_code_thd_l_show,stk_ps_code_thd_l_store);
static struct device_attribute ps_code_thd_h_attribute = __ATTR(codethdh,0664,stk_ps_code_thd_h_show,stk_ps_code_thd_h_store);
static struct device_attribute recv_attribute = __ATTR(recv,0664,stk_recv_show,stk_recv_store);
static struct device_attribute send_attribute = __ATTR(send,0664,stk_send_show, stk_send_store);
static struct device_attribute all_reg_attribute = __ATTR(allreg, 0444, stk_all_reg_show, NULL);
static struct device_attribute status_attribute = __ATTR(status, 0444, stk_status_show, NULL);
#ifdef STK_TUNE0
static struct device_attribute ps_cali_attribute = __ATTR(cali,0444,stk_ps_cali_show, NULL);
#elif defined(STK_TUNE1)
static struct device_attribute ps_cali_attribute = __ATTR(cali,0664,stk_ps_cali_show, stk_ps_cali_store);
#endif

static struct attribute *stk_ps_attrs [] =
{
	&ps_enable_attribute.attr,
	&ps_enable_aso_attribute.attr,
	&ps_distance_attribute.attr,
	&ps_offset_attribute.attr,
	&ps_code_attribute.attr,
	&ps_code_thd_l_attribute.attr,
	&ps_code_thd_h_attribute.attr,
	&recv_attribute.attr,
	&send_attribute.attr,
	&all_reg_attribute.attr,
	&status_attribute.attr,
#if (defined(STK_TUNE1) || defined(STK_TUNE0))
	&ps_cali_attribute.attr,
#endif
	NULL
};

static struct attribute_group stk_ps_attribute_group = {
	.name = "driver",
	.attrs = stk_ps_attrs,
};

#ifdef STK_TUNE0
static int stk_ps_tune_zero_val(struct stk3x1x_data *ps_data)
{
	int mode;
	int32_t word_data, ret, lii;
	uint8_t tmp_word_data[2];

	ret = rda_lightsensor_i2c_read(ps_data->client, 0x20, tmp_word_data, 2);
	if(ret < 0)
	{
		printk(KERN_ERR "%s fail, err=0x%x:%x", __func__, tmp_word_data[0], tmp_word_data[1]);
		word_data = tmp_word_data[1] | tmp_word_data[0] << 8;
		return word_data;
	}
	word_data = tmp_word_data[1] | tmp_word_data[0] << 8;

	ret = rda_lightsensor_i2c_read(ps_data->client, 0x22, tmp_word_data, 2);
	if(ret < 0)
	{
		printk(KERN_ERR "%s fail, err=0x%x:%x", __func__, tmp_word_data[0], tmp_word_data[1]);
		return tmp_word_data[0];
	}
	word_data += tmp_word_data[1] | tmp_word_data[0] << 8 ;

	mode = (ps_data->psctrl_reg) & 0x3F;
	if(mode == 0x30)
		lii = 100;
	else if (mode == 0x31)
		lii = 200;
	else if (mode == 0x32)
		lii = 400;
	else if (mode == 0x33)
		lii = 800;
	else
	{
		printk(KERN_ERR "%s: unsupported PS_IT(0x%x)\n", __func__, mode);
		return -1;
	}

	if(word_data > lii)
	{
		printk(KERN_INFO "%s: word_data=%d, lii=%d\n", __func__, word_data, lii);
		return 0xFFFF;
	}
	return 0;
}

static int stk_ps_tune_zero_func_fae(struct stk3x1x_data *ps_data)
{
	int32_t word_data;
	int ret;
	uint8_t tmp_word_data[2];

	if(ps_data->psi_set || !(ps_data->ps_enabled))
	{
		printk(KERN_INFO "%s: FAE tune0 close\n", __func__);
		return 0;
	}
	printk(KERN_INFO "%s: FAE tune0 entry\n", __func__);

	ret = stk3x1x_get_flag(ps_data);
	if(ret < 0)
	{
		printk(KERN_ERR "%s: get flag failed, err=0x%x\n", __func__, ret);
		return ret;
	}
	if(!(ret&STK_FLG_PSDR_MASK))
	{
		//printk(KERN_INFO "%s: ps data is not ready yet\n", __func__);
		return 0;
	}

	ret = stk_ps_tune_zero_val(ps_data);
	if(ret == 0)
	{
		ret = rda_lightsensor_i2c_read(ps_data->client, 0x11, tmp_word_data, 2);
		if(ret < 0)
		{
			printk(KERN_ERR "%s fail, err=0x%x:%x", __func__, tmp_word_data[0], tmp_word_data[1]);
			word_data = tmp_word_data[1] | tmp_word_data[0] << 8;
			return word_data;
		}
		word_data = tmp_word_data[1] | tmp_word_data[0] << 8;
		printk(KERN_INFO "%s: word_data=%d\n", __func__, word_data);

		if(word_data == 0)
		{
			printk(KERN_ERR "%s: incorrect word data (0)\n", __func__);
			return 0xFFFF;
		}

		if(word_data > ps_data->psa)
		{
			ps_data->psa = word_data;
			printk(KERN_INFO "%s: update psa: psa=%d,psi=%d\n", __func__, ps_data->psa, ps_data->psi);
		}
		if(word_data < ps_data->psi)
		{
			ps_data->psi = word_data;
			printk(KERN_INFO "%s: update psi: psa=%d,psi=%d\n", __func__, ps_data->psa, ps_data->psi);
		}
	}

	if((ps_data->psa - ps_data->psi) > STK_MAX_MIN_DIFF)
	{
		ps_data->psi_set = ps_data->psi;
		ps_data->ps_thd_h = ps_data->psi + STK_HT_N_CT;
		ps_data->ps_thd_l = ps_data->psi + STK_LT_N_CT;
		stk3x1x_set_ps_thd_h(ps_data, ps_data->ps_thd_h);
		stk3x1x_set_ps_thd_l(ps_data, ps_data->ps_thd_l);
#ifdef STK_DEBUG_PRINTF
		printk(KERN_INFO "%s: FAE tune0 psa-psi > DIFF found\n", __func__);
#endif
		hrtimer_cancel(&ps_data->ps_tune0_timer);
	}

	//printk(KERN_INFO "%s: FAE tune0 exit\n", __func__);
	return 0;
}

static void stk_ps_tune0_work_func(struct work_struct *work)
{
	struct stk3x1x_data *ps_data = container_of(work, struct stk3x1x_data, stk_ps_tune0_work);
	stk_ps_tune_zero_func_fae(ps_data);
	return;
}


static enum hrtimer_restart stk_ps_tune0_timer_func(struct hrtimer *timer)
{
	struct stk3x1x_data *ps_data = container_of(timer, struct stk3x1x_data, ps_tune0_timer);
	queue_work(ps_data->stk_ps_tune0_wq, &ps_data->stk_ps_tune0_work);
	hrtimer_forward_now(&ps_data->ps_tune0_timer, ps_data->ps_tune0_delay);
	return HRTIMER_RESTART;
}

/*
static int stk_ps_tune_zero_pre(struct stk3x1x_data *ps_data, uint8_t flag_reg, uint8_t *dis_flag)
{
	int32_t ret;
	int32_t word_data, tmp_word_data;

	if(flag_reg & STK_FLG_OUI_MASK)
	{
		*dis_flag |= STK_FLG_OUI_MASK;
		ret = stk_ps_tune_zero_val(ps_data);

		if(ret == 0)
		{
			tmp_word_data = i2c_smbus_read_word_data(ps_data->client, 0x0E);
			if(tmp_word_data < 0)
			{
				printk(KERN_ERR "%s fail, err=0x%x", __func__, tmp_word_data);
				return tmp_word_data;
			}
			word_data = ((tmp_word_data & 0xFF00) >> 8) | ((tmp_word_data & 0x00FF) << 8) ;
			printk(KERN_INFO "%s: word_data=%d\n", __func__, word_data);
			if(word_data > ps_data->psa)
			{
				ps_data->psa = word_data;
				printk("%s: psa=%d,psi=%d\n", __func__, ps_data->psa, ps_data->psi);
			}
			if(word_data < ps_data->psi)
			{
				ps_data->psi = word_data;
				printk("%s: psa=%d,psi=%d\n", __func__, ps_data->psa, ps_data->psi);
			}
			return 1;
		}
		return 2;
	}
	return 0;
}

static int stk_ps_tune_zero_nex(struct stk3x1x_data *ps_data, uint8_t proceed)
{
	int32_t ret;
	uint32_t reading;
	uint8_t w_state_reg;
	int32_t near_far_state;

	ret = stk3x1x_set_ps_aoffset(ps_data, 0xFFFF);
	if (ret < 0)
	{
		printk(KERN_ERR "%s: write i2c error\n", __func__);
		return ret;
	}

	if(proceed == 1)
	{
		if((ps_data->psa - ps_data->psi) >= STK_MAX_MIN_DIFF)
		{
			ps_data->psi_set = ps_data->psi;
#ifdef STK_DEBUG_PRINTF
			printk(KERN_INFO "%s: psi_set=%d, psa=%d\n", __func__, ps_data->psi_set, ps_data->psa);
#endif
			ret = stk3x1x_set_ps_foffset(ps_data, ps_data->psi_set);
			if (ret < 0)
			{
				printk(KERN_ERR "%s: write i2c error\n", __func__);
				return ret;
			}
			stk3x1x_set_ps_thd_h(ps_data, STK_HT_N_CT);
			stk3x1x_set_ps_thd_l(ps_data, STK_LT_N_CT);

			ret = i2c_smbus_read_byte_data(ps_data->client, STK_STATE_REG);
			if (ret < 0)
			{
				printk(KERN_ERR "%s: write i2c error\n", __func__);
				return ret;
			}
			w_state_reg = (uint8_t)(ret | 0x28);
			w_state_reg &= (~STK_STATE_EN_AK_MASK);
			ret = i2c_smbus_write_byte_data(ps_data->client, STK_STATE_REG, w_state_reg);
			if (ret < 0)
			{
				printk(KERN_ERR "%s: write i2c error\n", __func__);
				return ret;
			}
			printk(KERN_INFO "%s: ps_tune_done\n", __func__);

			msleep(1);
			ret = stk3x1x_get_flag(ps_data);
			if (ret < 0)
			{
				printk(KERN_ERR "%s: read i2c error, ret=%d\n", __func__, ret);
				return ret;
			}
			near_far_state = ret & STK_FLG_NF_MASK;
			ps_data->ps_distance_last = near_far_state;
			input_report_abs(ps_data->ps_input_dev, ABS_DISTANCE, near_far_state);
			input_sync(ps_data->ps_input_dev);
			wake_lock_timeout(&ps_data->ps_wakelock, 3*HZ);
			reading = stk3x1x_get_ps_reading(ps_data);
			printk(KERN_INFO "%s: ps input event=%d, ps code = %d\n",__func__, near_far_state, reading);
		}
	}
	return 0;
}
*/
#elif defined(STK_TUNE1)
static int stk_ps_tune_one_pre(struct stk3x1x_data *ps_data, uint8_t o_flag_reg, uint8_t *dis_flag)
{
	int32_t ret;
	uint8_t tmp_word_data[2];

	if (o_flag_reg & STK_FLG_OUI_MASK)
	{
		*dis_flag |= STK_FLG_OUI_MASK;

		ret = rda_lightsensor_i2c_read(ps_data->client, 0x0E, tmp_word_data, 2);
		if(ret < 0)
		{
			printk(KERN_ERR "%s fail, err=0x%x:%x", __func__, tmp_word_data[0], tmp_word_data[1]);
			return tmp_word_data[0];
		}
		ps_data->aoffset  = tmp_word_data[1] | tmp_word_data[0] << 8;
		return 1;
	}
	return 0;
}

static int stk_ps_tune_one_chkstat(struct stk3x1x_data *ps_data, uint8_t w_state_reg, uint8_t target08)
{
	int32_t ret;

	if((w_state_reg & 0x8) ^ target08)
	{
		if(target08)
			w_state_reg |= 0x8;
		else
			w_state_reg &= (~0x8);
		//printk(KERN_INFO "%s: w_state_reg=0x%x\n", __func__, w_state_reg);
		ret = rda_lightsensor_i2c_write(ps_data->client, STK_STATE_REG, &w_state_reg, 1);
		if (ret < 0)
		{
			printk(KERN_ERR "%s: write i2c error\n", __func__);
			return ret;
		}
	}

	return 0;
}

static int stk_ps_tune_one_nex(struct stk3x1x_data *ps_data)
{
	int32_t ret;
	int32_t word_data2;
	uint8_t w_state_reg, r_reg, tmp_word_data[2];

	ret = rda_lightsensor_i2c_read(ps_data->client, STK_STATE_REG, &r_reg, 1);
	if (ret < 0)
	{
		printk(KERN_ERR "%s: write i2c error\n", __func__);
		return ret;
	}
	w_state_reg = r_reg;
	if(ps_data->aoffset <= ps_data->cta)
	{
		if(ps_data->aoffset < ps_data->cti)
		{
			stk3x1x_set_ps_aoffset(ps_data, ps_data->kdata *2);
			stk_ps_tune_one_chkstat(ps_data, w_state_reg, 8);
			ps_data->state = 1;
			printk(KERN_INFO "%s: state 1, aoffset=%d, sfoffset=1 \n", __func__, ps_data->aoffset);
		}
		else
		{
			ret = rda_lightsensor_i2c_read(ps_data->client, STK_DATA1_OFFSET_REG, tmp_word_data, 2);
			if(ret < 0)
			{
				printk(KERN_ERR "%s fail, err=0x%x:%x", __func__, tmp_word_data[0], tmp_word_data[1]);
				return tmp_word_data[0];
			}
			word_data2 = tmp_word_data[1] | tmp_word_data[0] << 8;
			//printk(KERN_INFO "%s: foffset=%d\n", __func__, word_data2);
			if(ps_data->aoffset > word_data2)
			{
				stk_ps_tune_one_chkstat(ps_data, w_state_reg, 0);
				ps_data->state = 2;
				printk(KERN_INFO "%s: state 2, aoffset=%d, sfoffset=0 \n", __func__, ps_data->aoffset);
			}
			else
			{
				stk_ps_tune_one_chkstat(ps_data, w_state_reg, 8);
				ps_data->state = 3;
				printk(KERN_INFO "%s: state 3, aoffset=%d, sfoffset=1 \n", __func__, ps_data->aoffset);
			}
		}
	}
	else
	{
		stk_ps_tune_one_chkstat(ps_data, w_state_reg, 8);
		ps_data->state = 4;
		printk(KERN_INFO "%s: state 4, aoffset=%d, sfoffset=1 \n", __func__, ps_data->aoffset);
	}
	return 0;
}
#endif	/*#ifdef STK_TUNE0	*/


static enum hrtimer_restart stk_als_timer_func(struct hrtimer *timer)
{
	struct stk3x1x_data *ps_data = container_of(timer, struct stk3x1x_data, als_timer);
	queue_work(ps_data->stk_als_wq, &ps_data->stk_als_work);
#ifdef STK_POLL_ALS
	hrtimer_forward_now(&ps_data->als_timer, ps_data->als_poll_delay);
	return HRTIMER_RESTART;
#else
	hrtimer_cancel(&ps_data->als_timer);
	return HRTIMER_NORESTART;
#endif
}

static void stk_als_poll_work_func(struct work_struct *work)
{
	struct stk3x1x_data *ps_data = container_of(work, struct stk3x1x_data, stk_als_work);
	int32_t reading, reading_lux, als_comperator, flag_reg;

	flag_reg = stk3x1x_get_flag(ps_data);
	if(flag_reg < 0)
	{
		printk(KERN_ERR "%s: stk3x1x_get_flag fail, ret=%d", __func__, flag_reg);
		return;
	}

	if(!(flag_reg&STK_FLG_ALSDR_MASK))
	{
		return;
	}

	reading = stk3x1x_get_als_reading(ps_data);
	if(reading < 0)
	{
		return;
	}

	if(ps_data->ir_code)
	{
		if(reading < STK_IRC_MAX_ALS_CODE && reading > STK_IRC_MIN_ALS_CODE &&
			ps_data->ir_code > STK_IRC_MIN_IR_CODE)
		{
			als_comperator = reading * STK_IRC_ALS_NUMERA / STK_IRC_ALS_DENOMI;
			if(ps_data->ir_code > als_comperator)
				ps_data->als_correct_factor = STK_IRC_ALS_CORREC;
			else
				ps_data->als_correct_factor = 1000;
		}
		//printk(KERN_INFO "%s: als=%d, ir=%d, als_correct_factor=%d", __func__, reading, ps_data->ir_code, ps_data->als_correct_factor);
		ps_data->ir_code = 0;
	}
	reading = reading * ps_data->als_correct_factor / 1000;

	reading_lux = stk_alscode2lux(ps_data, reading);
	if(abs(ps_data->als_lux_last - reading_lux) >= STK_ALS_CHANGE_THD)
	{
		ps_data->als_lux_last = reading_lux;
		input_report_abs(ps_data->als_input_dev, ABS_MISC, reading_lux);
		input_sync(ps_data->als_input_dev);
		//printk(KERN_INFO "%s: als input event %d lux\n",__func__, reading_lux);
	}
	return;
}



#ifdef STK_POLL_PS
static enum hrtimer_restart stk_ps_timer_func(struct hrtimer *timer)
{
	struct stk3x1x_data *ps_data = container_of(timer, struct stk3x1x_data, ps_timer);
	queue_work(ps_data->stk_ps_wq, &ps_data->stk_ps_work);
	hrtimer_forward_now(&ps_data->ps_timer, ps_data->ps_poll_delay);
	return HRTIMER_RESTART;
}

static void stk_ps_poll_work_func(struct work_struct *work)
{
	struct stk3x1x_data *ps_data = container_of(work, struct stk3x1x_data, stk_ps_work);
	uint32_t reading;
	int32_t near_far_state;
	uint8_t org_flag_reg;
	int32_t ret;
	uint8_t disable_flag = 0;
#ifdef STK_TUNE0
	//uint8_t proceed_tune0 = 0;
#elif defined STK_TUNE1
	uint8_t proceed_tune1 = 0;
#endif

	org_flag_reg = stk3x1x_get_flag(ps_data);
	if(org_flag_reg < 0)
	{
		printk(KERN_ERR "%s: stk3x1x_get_flag fail, ret=%d", __func__, org_flag_reg);
		goto err_i2c_rw;
	}

	if(!(org_flag_reg&STK_FLG_PSDR_MASK))
	{
		return;
	}

#ifdef STK_TUNE0
/*
	ret = stk_ps_tune_zero_pre(ps_data, org_flag_reg, &disable_flag);
	if(ret < 0)
	{
		printk(KERN_ERR "%s: stk_ps_tune_zero_pre fail, ret=%d", __func__, ret);
		goto err_i2c_rw;
	}
	else if (ret > 0)
	{
		proceed_tune0 = ret;
		printk(KERN_INFO "%s: proceed_tune0=%d", __func__, proceed_tune0);
	}
*/
#elif defined(STK_TUNE1)
	ret = stk_ps_tune_one_pre(ps_data, org_flag_reg, &disable_flag);
	if(ret < 0)
	{
		printk(KERN_ERR "%s: stk_ps_tune_one_pre fail, ret=%d", __func__, ret);
		goto err_i2c_rw;
	}
	else if(ret == 1)
	{
		proceed_tune1 = 1;
		printk(KERN_INFO "%s: proceed_tune1=1\n", __func__);
	}
#endif
	near_far_state = (org_flag_reg & STK_FLG_NF_MASK)?1:0;
	reading = stk3x1x_get_ps_reading(ps_data);
	if(ps_data->ps_distance_last != near_far_state)
	{
		ps_data->ps_distance_last = near_far_state;
		input_report_abs(ps_data->ps_input_dev, ABS_DISTANCE, near_far_state);
		input_sync(ps_data->ps_input_dev);
		wake_lock_timeout(&ps_data->ps_wakelock, 3*HZ);
#ifdef STK_DEBUG_PRINTF
		printk(KERN_INFO "%s: ps input event %d cm, ps code = %d\n",__func__, near_far_state, reading);
#endif
	}
	ret = stk3x1x_set_flag(ps_data, org_flag_reg, disable_flag);
	if(ret < 0)
	{
		printk(KERN_ERR "%s:stk3x1x_set_flag fail, ret=%d\n", __func__, ret);
		goto err_i2c_rw;
	}
	//msleep(1);
#ifdef STK_TUNE0
/*
	if(proceed_tune0)
	{
		ret = stk_ps_tune_zero_nex(ps_data, proceed_tune0);
		if(ret < 0)
		{
			printk(KERN_ERR "%s:stk_ps_tune_zero_nex fail, ret=%d\n", __func__, ret);
			goto err_i2c_rw;
		}
	}
*/
#elif defined STK_TUNE1
	if(proceed_tune1)
	{
		ret = stk_ps_tune_one_nex(ps_data);
		if(ret < 0)
		{
			printk(KERN_ERR "%s:stk_ps_tune_one_nex fail, ret=%d\n", __func__, ret);
			goto err_i2c_rw;
		}
	}
#endif
	return;

err_i2c_rw:
	msleep(30);
	return;
}
#endif

#if (!defined(STK_POLL_PS) || !defined(STK_POLL_ALS))
static void stk_work_func(struct work_struct *work)
{
	uint32_t reading;
#if ((STK_INT_PS_MODE != 0x03) && (STK_INT_PS_MODE != 0x02))
	int32_t ret;
	uint8_t disable_flag = 0;
	uint8_t org_flag_reg;
#endif	/* #if ((STK_INT_PS_MODE != 0x03) && (STK_INT_PS_MODE != 0x02)) */

#ifdef STK_TUNE0
	//uint8_t proceed_tune0 = 0;
#elif defined STK_TUNE1
	uint8_t proceed_tune1 = 0;
#endif
#ifndef CONFIG_STK_PS_ALS_USE_CHANGE_THRESHOLD
	uint32_t nLuxIndex;
#endif
	struct stk3x1x_data *ps_data = container_of(work, struct stk3x1x_data, stk_work);
	int32_t near_far_state;
	int32_t als_comperator;

#if (STK_INT_PS_MODE	== 0x03)
	near_far_state = gpio_get_value(ps_data->int_pin);
#elif	(STK_INT_PS_MODE	== 0x02)
	near_far_state = !(gpio_get_value(ps_data->int_pin));
#endif

#if ((STK_INT_PS_MODE == 0x03) || (STK_INT_PS_MODE	== 0x02))
	ps_data->ps_distance_last = near_far_state;
	input_report_abs(ps_data->ps_input_dev, ABS_DISTANCE, near_far_state);
	input_sync(ps_data->ps_input_dev);
	wake_lock_timeout(&ps_data->ps_wakelock, 3*HZ);
	reading = stk3x1x_get_ps_reading(ps_data);
#ifdef STK_DEBUG_PRINTF
	printk(KERN_INFO "%s: ps input event %d cm, ps code = %d\n",__func__, near_far_state, reading);
#endif
#else
	/* mode 0x01 or 0x04 */
	org_flag_reg = stk3x1x_get_flag(ps_data);
	if(org_flag_reg < 0)
	{
		printk(KERN_ERR "%s: stk3x1x_get_flag fail, org_flag_reg=%d", __func__, org_flag_reg);
		goto err_i2c_rw;
	}

#ifdef STK_TUNE0
/*
	ret = stk_ps_tune_zero_pre(ps_data, org_flag_reg, &disable_flag);
	if(ret < 0)
	{
		printk(KERN_ERR "%s: stk_ps_tune_zero_pre fail, ret=%d", __func__, ret);
		goto err_i2c_rw;
	}
	else if (ret > 0)
		proceed_tune0 = ret;
*/
#elif defined(STK_TUNE1)
	ret = stk_ps_tune_one_pre(ps_data, org_flag_reg, &disable_flag);
	if(ret < 0)
		goto err_i2c_rw;
	else if(ret == 1)
	{
		proceed_tune1 = 1;
	}
#endif	/* #ifdef STK_TUNE0 */

	if (org_flag_reg & STK_FLG_ALSINT_MASK)
	{
		disable_flag |= STK_FLG_ALSINT_MASK;
		reading = stk3x1x_get_als_reading(ps_data);
		if(reading < 0)
		{
			printk(KERN_ERR "%s: stk3x1x_get_als_reading fail, ret=%d", __func__, reading);
			goto err_i2c_rw;
		}
#ifndef CONFIG_STK_PS_ALS_USE_CHANGE_THRESHOLD
		nLuxIndex = stk_get_lux_interval_index(reading);
		stk3x1x_set_als_thd_h(ps_data, code_threshold_table[nLuxIndex]);
		stk3x1x_set_als_thd_l(ps_data, code_threshold_table[nLuxIndex-1]);
#else
		stk_als_set_new_thd(ps_data, reading);
#endif //CONFIG_STK_PS_ALS_USE_CHANGE_THRESHOLD

		if(ps_data->ir_code)
		{
			if(reading < STK_IRC_MAX_ALS_CODE && reading > STK_IRC_MIN_ALS_CODE &&
			ps_data->ir_code > STK_IRC_MIN_IR_CODE)
			{
				als_comperator = reading * STK_IRC_ALS_NUMERA / STK_IRC_ALS_DENOMI;
				if(ps_data->ir_code > als_comperator)
					ps_data->als_correct_factor = STK_IRC_ALS_CORREC;
				else
					ps_data->als_correct_factor = 1000;
			}
			//printk(KERN_INFO "%s: als=%d, ir=%d, als_correct_factor=%d", __func__, reading, ps_data->ir_code, ps_data->als_correct_factor);
			ps_data->ir_code = 0;
		}

		reading = reading * ps_data->als_correct_factor / 1000;

		ps_data->als_lux_last = stk_alscode2lux(ps_data, reading);
		input_report_abs(ps_data->als_input_dev, ABS_MISC, ps_data->als_lux_last);
		input_sync(ps_data->als_input_dev);
#ifdef STK_DEBUG_PRINTF
		printk(KERN_INFO "%s: als input event %d lux\n",__func__, ps_data->als_lux_last);
#endif
	}
	if (org_flag_reg & STK_FLG_PSINT_MASK)
	{
		disable_flag |= STK_FLG_PSINT_MASK;
		near_far_state = (org_flag_reg & STK_FLG_NF_MASK)?1:0;

		ps_data->ps_distance_last = near_far_state;
		input_report_abs(ps_data->ps_input_dev, ABS_DISTANCE, near_far_state);
		input_sync(ps_data->ps_input_dev);
		wake_lock_timeout(&ps_data->ps_wakelock, 3*HZ);
		reading = stk3x1x_get_ps_reading(ps_data);
#ifdef STK_DEBUG_PRINTF
		printk(KERN_INFO "%s: ps input event=%d, ps code = %d\n",__func__, near_far_state, reading);
#endif
	}

	ret = stk3x1x_set_flag(ps_data, org_flag_reg, disable_flag);
	if(ret < 0)
	{
		printk(KERN_ERR "%s:reset_int_flag fail, ret=%d\n", __func__, ret);
		goto err_i2c_rw;
	}
#endif

#ifdef STK_TUNE0
/*
	if(proceed_tune0)
	{
		ret = stk_ps_tune_zero_nex(ps_data, proceed_tune0);
		if(ret < 0)
		{
			printk(KERN_ERR "%s:stk_ps_tune_zero_nex fail, ret=%d\n", __func__, ret);
			goto err_i2c_rw;
		}
	}
*/
#elif defined STK_TUNE1
	if(proceed_tune1)
	{
		stk_ps_tune_one_nex(ps_data);
		if(ret < 0)
			goto err_i2c_rw;
	}
#endif

	msleep(1);
	enable_irq(ps_data->irq);
	return;

err_i2c_rw:
	msleep(30);
	enable_irq(ps_data->irq);
	return;
}

static irqreturn_t stk_oss_irq_handler(int irq, void *data)
{
	struct stk3x1x_data *pData = data;
	disable_irq_nosync(irq);
	queue_work(pData->stk_wq,&pData->stk_work);
	return IRQ_HANDLED;
}
#endif	/*	#if (!defined(STK_POLL_PS) || !defined(STK_POLL_ALS))	*/
static int32_t stk3x1x_init_all_setting(struct i2c_client *client, struct stk3x1x_platform_data *plat_data)
{
	int32_t ret;
	struct stk3x1x_data *ps_data = i2c_get_clientdata(client);

	ps_data->als_enabled = false;
	ps_data->ps_enabled = false;

	ret = stk3x1x_software_reset(ps_data);
	if(ret < 0)
		return ret;

	ret = stk3x1x_check_pid(ps_data);
	if(ret < 0)
		return ret;

	ret = stk3x1x_init_all_reg(ps_data, plat_data);
	if(ret < 0)
		return ret;
	ps_data->ir_code = 0;
	ps_data->als_correct_factor = 1000;
#ifndef CONFIG_STK_PS_ALS_USE_CHANGE_THRESHOLD
	stk_init_code_threshold_table(ps_data);
#endif
	ps_data->first_boot = true;
#ifdef STK_TUNE0
	ps_data->psi_set = 0;

#elif defined STK_TUNE1
	ps_data->state = 0;
	ps_data->kdata	= 0;
	ps_data->kdata1	= 0;
#endif
#ifdef STK_ALS_FIR
	memset(&ps_data->fir, 0x00, sizeof(ps_data->fir));
#endif
	return 0;
}

#if (!defined(STK_POLL_PS) || !defined(STK_POLL_ALS))
static int stk3x1x_setup_irq(struct i2c_client *client)
{
	int irq, err = -EIO;
	struct stk3x1x_data *ps_data = i2c_get_clientdata(client);

	err = gpio_request(ps_data->int_pin,"stk-int");
	if(err < 0)
	{
		printk(KERN_ERR "%s: gpio_request, err=%d", __func__, err);
		return err;
	}
	err = gpio_direction_input(ps_data->int_pin);
	if(err < 0)
	{
		printk(KERN_ERR "%s: gpio_direction_input, err=%d", __func__, err);
		goto err_request_any_context_irq;
	}

	irq = gpio_to_irq(ps_data->int_pin);
#ifdef STK_DEBUG_PRINTF
	printk(KERN_INFO "%s: int pin #=%d, irq=%d\n",__func__, ps_data->int_pin, irq);
#endif
	if (irq < 0)
	{
		printk(KERN_ERR "irq number is not specified, irq # = %d, int pin=%d\n",irq, ps_data->int_pin);
		err = irq;
		goto err_request_any_context_irq;
	}
	ps_data->irq = irq;
	
#if ((STK_INT_PS_MODE == 0x03) || (STK_INT_PS_MODE	== 0x02))
	err = request_any_context_irq(irq, stk_oss_irq_handler, IRQF_TRIGGER_FALLING|IRQF_TRIGGER_RISING, RDA_LIGHTSENSOR_DRV_NAME, ps_data);
#else
	err = request_any_context_irq(irq, stk_oss_irq_handler, IRQF_TRIGGER_LOW, RDA_LIGHTSENSOR_DRV_NAME, ps_data);
#endif
	if (err < 0)
	{
		printk(KERN_WARNING "%s: request_any_context_irq(%d) failed for (%d)\n", __func__, irq, err);
		goto err_request_any_context_irq;
	}
	disable_irq(irq);

	return 0;
	
err_request_any_context_irq:
	gpio_free(ps_data->int_pin);
	
	return err;
}
#endif

#ifdef CONFIG_PM
static int stk3x1x_suspend(struct device *dev)
{
	struct stk3x1x_data *ps_data = dev_get_drvdata(dev);
#ifndef STK_POLL_PS
	int err;
#endif

	mutex_lock(&ps_data->io_lock);
	if(ps_data->als_enabled)
	{
		stk3x1x_enable_als(ps_data, 0);
		ps_data->als_enabled = true;
	}

	if(ps_data->ps_enabled)
	{
#ifdef STK_POLL_PS
		wake_lock(&ps_data->ps_nosuspend_wl);
#else
		if(device_may_wakeup(&client->dev))
		{
			err = enable_irq_wake(ps_data->irq);
			if (err)
				printk(KERN_WARNING "%s: set_irq_wake(%d) failed, err=(%d)\n", __func__, ps_data->irq, err);
		}
		else
		{
			printk(KERN_ERR "%s: not support wakeup source", __func__);
		}
#endif
	}
	
	mutex_unlock(&ps_data->io_lock);

	return 0;
}

static int stk3x1x_resume(struct device *dev)
{
	struct stk3x1x_data *ps_data = dev_get_drvdata(dev);
#ifndef STK_POLL_PS
	int err;
#endif

	mutex_lock(&ps_data->io_lock);
	if(ps_data->als_enabled)
		stk3x1x_enable_als(ps_data, 1);

	if(ps_data->ps_enabled)
	{
#ifdef STK_POLL_PS
		wake_unlock(&ps_data->ps_nosuspend_wl);
#else
		if(device_may_wakeup(&client->dev))
		{
			err = disable_irq_wake(ps_data->irq);
			if (err)
				printk(KERN_WARNING "%s: disable_irq_wake(%d) failed, err=(%d)\n", __func__, ps_data->irq, err);
		}
#endif
	}
	
	mutex_unlock(&ps_data->io_lock);

	return 0;
}
#endif /*  CONFIG_PM */

static const struct dev_pm_ops stk3x1x_pm_ops = {
	SET_SYSTEM_SLEEP_PM_OPS(stk3x1x_suspend, stk3x1x_resume)
};

//#ifdef CONFIG_HAS_EARLYSUSPEND
#if 0
static void stk3x1x_early_suspend(struct early_suspend *h)
{
	struct stk3x1x_data *ps_data = container_of(h, struct stk3x1x_data, stk_early_suspend);
#ifndef STK_POLL_PS
	int err;
#endif

	printk(KERN_INFO "%s", __func__);

	mutex_lock(&ps_data->io_lock);
	if(ps_data->als_enabled)
	{
		stk3x1x_enable_als(ps_data, 0);
		ps_data->als_enabled = true;
	}

	if(ps_data->ps_enabled)
	{
#ifdef STK_POLL_PS
		wake_lock(&ps_data->ps_nosuspend_wl);
#else
		err = enable_irq_wake(ps_data->irq);
		if (err)
			printk(KERN_WARNING "%s: set_irq_wake(%d) failed, err=(%d)\n", __func__, ps_data->irq, err);
#endif
	}

	mutex_unlock(&ps_data->io_lock);

	return;
}

static void stk3x1x_late_resume(struct early_suspend *h)
{
	struct stk3x1x_data *ps_data = container_of(h, struct stk3x1x_data, stk_early_suspend);
#ifndef STK_POLL_PS
	int err;
#endif

	printk(KERN_INFO "%s", __func__);

	mutex_lock(&ps_data->io_lock);
	if(ps_data->als_enabled)
		stk3x1x_enable_als(ps_data, 1);

	if(ps_data->ps_enabled)
	{
#ifdef STK_POLL_PS
		wake_unlock(&ps_data->ps_nosuspend_wl);
#else
		err = disable_irq_wake(ps_data->irq);
		if (err)
			printk(KERN_WARNING "%s: disable_irq_wake(%d) failed, err=(%d)\n", __func__, ps_data->irq, err);
#endif
	}
	mutex_unlock(&ps_data->io_lock);

	return;
}
#endif	//#ifdef CONFIG_HAS_EARLYSUSPEND


static int stk3x1x_probe(struct i2c_client *client,
						const struct i2c_device_id *id)
{
	int err = -ENODEV;
	struct stk3x1x_data *ps_data;
	struct stk3x1x_platform_data *plat_data;
	//printk(KERN_INFO "%s: driver version = %s\n", __func__, DRIVER_VERSION);

	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
		printk("stk3x1x_probe: need I2C_FUNC_I2C\n");
		return -ENODEV;
	}

	ps_data = kzalloc(sizeof(struct stk3x1x_data),GFP_KERNEL);
	if(!ps_data)
	{
		printk(KERN_ERR "%s: failed to allocate stk3x1x_data\n", __func__);
		return -ENOMEM;
	}
	ps_data->client = client;
	i2c_set_clientdata(client,ps_data);
	mutex_init(&ps_data->io_lock);
	wake_lock_init(&ps_data->ps_wakelock,WAKE_LOCK_SUSPEND, "stk_input_wakelock");

#ifdef STK_POLL_PS
	wake_lock_init(&ps_data->ps_nosuspend_wl,WAKE_LOCK_SUSPEND, "stk_nosuspend_wakelock");
#endif

	if(client->dev.platform_data != NULL)
	{
		plat_data = (struct stk3x1x_platform_data *) (client->dev.platform_data);
		ps_data->als_transmittance = plat_data->transmittance;
		ps_data->int_pin = plat_data->int_pin;
		if(ps_data->als_transmittance == 0)
		{
			printk(KERN_ERR "%s: Please set als_transmittance in platform data\n", __func__);
			goto err_als_input_allocate;
		}
	}
	else
	{
		printk(KERN_ERR "%s: no stk3x1x platform data!\n", __func__);
		goto err_als_input_allocate;
	}

	ps_data->als_input_dev = input_allocate_device();
	if (ps_data->als_input_dev==NULL)
	{
		printk(KERN_ERR "%s: could not allocate als device\n", __func__);
		err = -ENOMEM;
		goto err_als_input_allocate;
	}
	ps_data->ps_input_dev = input_allocate_device();
	if (ps_data->ps_input_dev==NULL)
	{
		printk(KERN_ERR "%s: could not allocate ps device\n", __func__);
		err = -ENOMEM;
		goto err_ps_input_allocate;
	}
	ps_data->als_input_dev->name = ALS_NAME;
	ps_data->ps_input_dev->name = PS_NAME;
	set_bit(EV_ABS, ps_data->als_input_dev->evbit);
	set_bit(EV_ABS, ps_data->ps_input_dev->evbit);
	input_set_abs_params(ps_data->als_input_dev, ABS_MISC, 0, stk_alscode2lux(ps_data, (1<<16)-1), 0, 0);
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
	
	input_set_drvdata(ps_data->als_input_dev, ps_data);
	input_set_drvdata(ps_data->ps_input_dev, ps_data);

//#ifdef STK_POLL_ALS
	ps_data->stk_als_wq = create_singlethread_workqueue("stk_als_wq");
	INIT_WORK(&ps_data->stk_als_work, stk_als_poll_work_func);
	hrtimer_init(&ps_data->als_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	ps_data->als_poll_delay = ns_to_ktime(110 * NSEC_PER_MSEC);
	ps_data->als_timer.function = stk_als_timer_func;
//#endif

#ifdef STK_POLL_PS
	ps_data->stk_ps_wq = create_singlethread_workqueue("stk_ps_wq");
	INIT_WORK(&ps_data->stk_ps_work, stk_ps_poll_work_func);
	hrtimer_init(&ps_data->ps_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	ps_data->ps_poll_delay = ns_to_ktime(60 * NSEC_PER_MSEC);
	ps_data->ps_timer.function = stk_ps_timer_func;
#endif

#ifdef STK_TUNE0
	ps_data->stk_ps_tune0_wq = create_singlethread_workqueue("stk_ps_tune0_wq");
	INIT_WORK(&ps_data->stk_ps_tune0_work, stk_ps_tune0_work_func);
	hrtimer_init(&ps_data->ps_tune0_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	ps_data->ps_tune0_delay = ns_to_ktime(60 * NSEC_PER_MSEC);
	ps_data->ps_tune0_timer.function = stk_ps_tune0_timer_func;
#endif

#if (!defined(STK_POLL_ALS) || !defined(STK_POLL_PS))
	ps_data->stk_wq = create_singlethread_workqueue("stk_wq");
	INIT_WORK(&ps_data->stk_work, stk_work_func);

	err = stk3x1x_setup_irq(client);
	if(err < 0)
		goto err_stk3x1x_setup_irq;
#endif
	device_init_wakeup(&client->dev, true);
	err = stk3x1x_init_all_setting(client, plat_data);
	if(err < 0)
		goto err_als_sysfs_create_group;

	err = sysfs_create_group(&ps_data->als_input_dev->dev.kobj, &stk_als_attribute_group);
	if (err < 0)
	{
		printk(KERN_ERR "%s:could not create sysfs group for als\n", __func__);
		goto err_als_sysfs_create_group;
	}
	err = sysfs_create_group(&ps_data->ps_input_dev->dev.kobj, &stk_ps_attribute_group);
	if (err < 0)
	{
		printk(KERN_ERR "%s:could not create sysfs group for ps\n", __func__);
		goto err_ps_sysfs_create_group;
	}
	kobject_uevent(&ps_data->ps_input_dev->dev.kobj, KOBJ_CHANGE);
//#ifdef CONFIG_HAS_EARLYSUSPEND
#if 0
	ps_data->stk_early_suspend.level = EARLY_SUSPEND_LEVEL_BLANK_SCREEN + 1;
	ps_data->stk_early_suspend.suspend = stk3x1x_early_suspend;
	ps_data->stk_early_suspend.resume = stk3x1x_late_resume;
	register_early_suspend(&ps_data->stk_early_suspend);
#endif
	printk(KERN_INFO "%s: probe successfully", __func__);
	return 0;

err_ps_sysfs_create_group:
	sysfs_remove_group(&ps_data->als_input_dev->dev.kobj, &stk_als_attribute_group);
err_als_sysfs_create_group:
	device_init_wakeup(&client->dev, false);
#if (!defined(STK_POLL_ALS) || !defined(STK_POLL_PS))
	free_irq(ps_data->irq, ps_data);

	gpio_free(ps_data->int_pin);

#endif	/* #ifndef STK_POLL_PS	*/
#if (!defined(STK_POLL_ALS) || !defined(STK_POLL_PS))
err_stk3x1x_setup_irq:
#endif
//#ifdef STK_POLL_ALS
	hrtimer_try_to_cancel(&ps_data->als_timer);
	destroy_workqueue(ps_data->stk_als_wq);
//#endif
#ifdef STK_TUNE0
	destroy_workqueue(ps_data->stk_ps_tune0_wq);
#endif
#ifdef STK_POLL_PS
	hrtimer_try_to_cancel(&ps_data->ps_timer);
	destroy_workqueue(ps_data->stk_ps_wq);
#endif
#if (!defined(STK_POLL_ALS) || !defined(STK_POLL_PS))
	destroy_workqueue(ps_data->stk_wq);
#endif
	input_unregister_device(ps_data->ps_input_dev);
err_ps_input_register:
	input_unregister_device(ps_data->als_input_dev);
err_als_input_register:
	input_free_device(ps_data->ps_input_dev);
err_ps_input_allocate:
	input_free_device(ps_data->als_input_dev);
err_als_input_allocate:
#ifdef STK_POLL_PS
	wake_lock_destroy(&ps_data->ps_nosuspend_wl);
#endif
	wake_lock_destroy(&ps_data->ps_wakelock);
	mutex_destroy(&ps_data->io_lock);
	kfree(ps_data);
	return err;
}


static int stk3x1x_remove(struct i2c_client *client)
{
	struct stk3x1x_data *ps_data = i2c_get_clientdata(client);
	device_init_wakeup(&client->dev, false);
#ifndef STK_POLL_PS
	free_irq(ps_data->irq, ps_data);
	gpio_free(ps_data->int_pin);
#endif	/* #ifndef STK_POLL_PS */
#ifdef STK_POLL_ALS
	hrtimer_try_to_cancel(&ps_data->als_timer);
	destroy_workqueue(ps_data->stk_als_wq);
#endif
#ifdef STK_TUNE0
	destroy_workqueue(ps_data->stk_ps_tune0_wq);
#endif
#ifdef STK_POLL_PS
	hrtimer_try_to_cancel(&ps_data->ps_timer);
	destroy_workqueue(ps_data->stk_ps_wq);
#endif
#if (!defined(STK_POLL_ALS) || !defined(STK_POLL_PS))
	destroy_workqueue(ps_data->stk_wq);
#endif
	sysfs_remove_group(&ps_data->ps_input_dev->dev.kobj, &stk_ps_attribute_group);
	sysfs_remove_group(&ps_data->als_input_dev->dev.kobj, &stk_als_attribute_group);
	input_unregister_device(ps_data->ps_input_dev);
	input_unregister_device(ps_data->als_input_dev);
	input_free_device(ps_data->ps_input_dev);
	input_free_device(ps_data->als_input_dev);
#ifdef STK_POLL_PS
	wake_lock_destroy(&ps_data->ps_nosuspend_wl);
#endif
	wake_lock_destroy(&ps_data->ps_wakelock);
	mutex_destroy(&ps_data->io_lock);
	kfree(ps_data);

	return 0;
}

static const struct i2c_device_id stk_ps_id[] =
{
	{RDA_LIGHTSENSOR_DRV_NAME, 0},
	{}
};
MODULE_DEVICE_TABLE(i2c, stk_ps_id);

static struct i2c_driver stk_ps_driver =
{
	.driver = {
		.name = RDA_LIGHTSENSOR_DRV_NAME,
		.owner = THIS_MODULE,
		.pm = &stk3x1x_pm_ops,
	},
	.probe = stk3x1x_probe,
	.remove = stk3x1x_remove,
	.id_table = stk_ps_id,
};

static struct i2c_client * ls_i2c_client = NULL;

static struct stk3x1x_platform_data rda_lightsensor_data[] = {
		{
			.state_reg = 0x0,	/* disable all */
			.psctrl_reg = 0x71,	/* ps_persistance=4, ps_gain=64X, PS_IT=0.391ms */
			.alsctrl_reg = 0x38,
			.ledctrl_reg = 0xBF,	/* 50mA IRDR, 64/64 LED duty */
			.wait_reg = 0x08,	/* 100 ms */
			.ps_thd_h =1700,
			.ps_thd_l = 1500,
			.int_pin = 0,
			.transmittance = 500,
		},
};

static int __init stk3x1x_init(void)
{
	int ret;
	struct i2c_client * i2c_client_temp = NULL;

	static struct i2c_board_info i2c_dev_lightsensor = {
			I2C_BOARD_INFO(RDA_LIGHTSENSOR_DRV_NAME, 0),
			.platform_data = rda_lightsensor_data,
	};

	struct i2c_adapter *adapter;
	i2c_dev_lightsensor.addr =  _DEF_I2C_ADDR_LSENSOR_STK3x1x;

	adapter = i2c_get_adapter(_TGT_AP_I2C_BUS_ID_LSENSOR);

	if (!adapter) {
		pr_err("%s, cannot get i2c adapter %d\n",
			__func__, _TGT_AP_I2C_BUS_ID_LSENSOR);
		return -ENODEV;
	}

	i2c_client_temp = i2c_new_device(adapter, &i2c_dev_lightsensor);
	if (NULL == i2c_client_temp){
		pr_err("%s, cannot creat new device with adapter %d\n",
			__func__, _TGT_AP_I2C_BUS_ID_LSENSOR);
		return -ENODEV;
	}

	ret = i2c_add_driver(&stk_ps_driver);

	if (ret) {
		i2c_unregister_device(i2c_client_temp);
		return ret;
	}

	pr_info("rda_gs %s initialized, at i2c bus %d addr 0x%02x\n",
		RDA_LIGHTSENSOR_DRV_NAME, _TGT_AP_I2C_BUS_ID_LSENSOR, 
		i2c_dev_lightsensor.addr);

	ls_i2c_client = i2c_client_temp;
	return 0;
}

static void __exit stk3x1x_exit(void)
{
	i2c_del_driver(&stk_ps_driver);

	if (NULL != ls_i2c_client)
		i2c_unregister_device(ls_i2c_client);
	ls_i2c_client = NULL;
}

module_init(stk3x1x_init);
module_exit(stk3x1x_exit);
MODULE_AUTHOR("Lex Hsieh <lex_hsieh@sensortek.com.tw>");
MODULE_DESCRIPTION("Sensortek stk3x1x Proximity Sensor driver");
MODULE_LICENSE("GPL");
MODULE_VERSION(DRIVER_VERSION);
