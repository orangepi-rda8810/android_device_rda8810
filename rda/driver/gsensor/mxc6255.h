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
#ifndef __MXC6255_H__
#define __MXC6255_H__

#define MXC6255_ACC_DEV_NAME		   "mxc6255"
#define MXC6255_ACC_INPUT_NAME		   "accelerometer"
#define MXC6255_ACC_I2C_ADDR           0x15
#define MXC6255_ACC_I2C_NAME           MXC6255_ACC_DEV_NAME

/* MXC6255 register address */
#define MXC6255_REG_DATA		       0x00
#define MXC6255_REG_CTRL		       0x04
#define WHO_AM_I                    0x08        /*      WhoAmI register         */

#define MXC6255_CTRL_PWRON		       0x00	/* power on */
#define MXC6255_CTRL_PWRDN		       0x80	/* power donw */
#define G_MAX                   20000   /** Maximum polled-device-reported g value */
#define WHOAMI_MXC6255_ACC      0x05    /*      Expctd content for WAI  */

#endif /* __MXC6255_H__ */

