#ifndef __RDA_TS_H
#define __RDA_TS_H

#include <plat/rda_debug.h>
#include "tgt_ap_board_config.h"
#include "tgt_ap_ts_setting.h"
#include "tgt_ap_panel_setting.h"

#define MAX_FINGERS	  5
#define MAX_TOUCH_POINTS 10
#define MAX_VIRTUAL_KEY_NUM 4

enum {
	TS_PACKET_ERROR = -2,
	TS_ERROR = -1,
	TS_I2C_ERROR = TS_ERROR,
	TS_OK,
	TS_NOTOUCH,
	TS_I2C_RETRY
};
struct rda_ts_pos{
	int x;
	int y;
	int id;
};

enum rda_ts_datatype{
	TS_DATA = 0,
	PS_DATA,
};

/* When touch_point_num is zero, it means you have left the touch screen */
struct rda_ts_pos_data{
	struct rda_ts_pos ts_position[MAX_TOUCH_POINTS];
	int point_num;
	int distance;
	enum rda_ts_datatype data_type;
};

struct rda_ts_ops {
	void (*ts_init_gpio) (void);
	void (*ts_reset_gpio) (void);
	void (*ts_reset_chip) (struct i2c_client *client);
	void (*ts_init_chip) (struct i2c_client *client);
	void (*ts_start_chip) (struct i2c_client *client);
	int (*ts_get_chip_id) (struct i2c_client *client);
	int (*ts_suspend)(struct i2c_client *client, pm_message_t mesg);
	int (*ts_resume)(struct i2c_client *client);
	int (*ts_get_parse_data)(struct i2c_client *client, struct rda_ts_pos_data * pos_data);
	/*
	 * Note:
	 *  if the tp don't have distance function,the ts_switch_ps_mode pointer must be NULL.
	 *  but if the tp have, the pointer isn't NULL.
	 */
	int (*ts_switch_ps_mode)(struct i2c_client* client, int enable);

#ifdef TS_FIRMWARE_UPDATE
	int (*ts_register_utils_funcs)(struct i2c_client *client);
	int (*ts_unregister_utils_funcs)(struct i2c_client *client);
#endif
};

struct rda_ts_vir_key{
	u16 key_value;
	u16 key_x;
	u16 key_y;
	u16 key_x_width;
	u16 key_y_width;
};

struct rda_ts_para{
	char name[16];
	u16 i2c_addr;
	u16 x_max;
	u16 y_max;
	u16 gpio_irq;
	u16 vir_key_num;
	int irqTriggerMode;
	struct rda_ts_vir_key vir_key[MAX_VIRTUAL_KEY_NUM];
};

struct rda_ts_panel_info{
	struct rda_ts_ops ops;
	struct rda_ts_para ts_para;
};
struct rda_ts_panel_array{
	struct rda_ts_panel_info	*panel_info;
	struct rda_ts_panel_array    *next;
};

struct rda_ts_data {
	struct i2c_client *client;
	struct input_dev *input_dev;
#ifdef	CONFIG_HAS_EARLYSUSPEND
	struct early_suspend early_suspend;
#endif
	int use_irq;
	int ts_irq;
	bool bIsNormalMode;
	bool bXneedScale;
	bool bYneedScale;
	u8  ts_key_down[MAX_VIRTUAL_KEY_NUM];
	u16 ts_key_value[MAX_VIRTUAL_KEY_NUM];
	struct work_struct work;
	struct rda_ts_panel_info *panel_info;
#ifdef CONFIG_USE_TASKLET
	struct tasklet_struct rda_ts_tasklet;
	bool tasklet_pending;
#endif

	int enable;
	struct timer_list rda_ts_timer;

	struct input_dev *ps_input_dev;
	int ps_enable;
	int ps_enable_when_suspend;
	int ps_distance;
	struct mutex ps_data_lock;
};


extern int rda_ts_register_driver(struct rda_ts_panel_info *info);
extern int rda_ts_i2c_write(struct i2c_client *client, u8 addr, u8 *pdata, int datalen);
extern int rda_ts_i2c_read(struct i2c_client *client, u8 addr, u8 *pdata, unsigned int datalen);

#endif /* __RDA_TS_H */

