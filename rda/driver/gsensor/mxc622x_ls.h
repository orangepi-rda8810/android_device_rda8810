/*
 * Copyright (C) 2010 MEMSIC, Inc.
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

/*
 * Definitions for mxc622x accelorometer sensor chip.
 */
#ifndef __MXC622X_H__
#define __MXC622X_H__

#include <linux/ioctl.h>
#include <linux/input.h>

#ifndef DEBUG
    #define DEBUG
#endif

#define MXC622X_ACC_IOCTL_BASE          77
#define MXC622X_ACC_IOCTL_SET_DELAY     _IOW(MXC622X_ACC_IOCTL_BASE, 0,   int)
#define MXC622X_ACC_IOCTL_GET_DELAY     _IOR(MXC622X_ACC_IOCTL_BASE, 1,   int)
#define MXC622X_ACC_IOCTL_SET_ENABLE    _IOW(MXC622X_ACC_IOCTL_BASE, 2,   int)
#define MXC622X_ACC_IOCTL_GET_ENABLE    _IOR(MXC622X_ACC_IOCTL_BASE, 3,   int)
#define MXC622X_ACC_IOCTL_GET_COOR_XYZ  _IOW(MXC622X_ACC_IOCTL_BASE, 22,  int)
#define MXC622X_ACC_IOCTL_GET_CHIP_ID   _IOR(MXC622X_ACC_IOCTL_BASE, 255, char[32])
#define MXC622X_ACC_DEV_NAME            "mxc622x"
#define MXC622X_ACC_INPUT_NAME          "accelerometer"
#define MXC622X_ACC_I2C_ADDR            0x15
#define MXC622X_ACC_I2C_NAME            MXC622X_ACC_DEV_NAME

#define MXC622X_REG_CTRL                0x04
#define MXC622X_REG_DATA                0x00

#define MXC622X_CTRL_PWRON              0x00	/* power on */
#define MXC622X_CTRL_PWRDN              0x80	/* power donw */

#define I2C_BUS_NUM_STATIC_ALLOC
#define I2C_STATIC_BUS_NUM              (0)	// Need to be modified according to actual setting

#ifdef __KERNEL__
struct mxc622x_acc_platform_data {
	int poll_interval;
	int min_interval;
	int (*init)(void);
	void (*exit)(void);
	int (*power_on)(void);
	int (*power_off)(void);
};
#endif /* __KERNEL__ */

/*
 * This address comes must match the part# on your target.
 * Address to the sensor part# support as following list:
 *   MXC6220	- 0x10
 *   MXC6221	- 0x11
 *   MXC6222	- 0x12
 *   MXC6223	- 0x13
 *   MXC6224	- 0x14
 *   MXC6225	- 0x15
 *   MXC6226	- 0x16
 *   MXC6227	- 0x17
 * Please refer to sensor datasheet for detail.
 */

/* MXC622X register address */


/* MXC622X control bit */

/* Use 'm' as magic number */

/* IOCTLs for MXC622X device */
#endif /* __MXC622X_H__ */

