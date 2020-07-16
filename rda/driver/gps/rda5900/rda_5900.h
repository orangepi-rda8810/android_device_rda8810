 /*
  * Copyright (C) 2013 RDA Microelectronics Inc.
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
  * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
  */

#ifndef _RDA_GPS_ANDROID_H_  
#define _RDA_GPS_ANDROID_H_  
  
#include <linux/cdev.h>  
#include <linux/semaphore.h>  
  
#define RDA_GPS_DEVICE_NODE_NAME  "rda5900"  
#define RDA_GPS_DEVICE_FILE_NAME  "gps"  
#define RDA_GPS_DEVICE_CLASS_NAME "gps"  
  
#define RDA_GPS_RF_DRINAME      RDA_GPS_RF_DEVNAME
#define RDA_GPS_DIG_DRINAME     RDA_GPS_DIG_DEVNAME

#define RDA_GPS_RF_DEVNAME      "rda_gps_rf"
#define RDA_GPS_RF_ADDR	        (0xae >> 1) // 0x57
#define RDA_GPS_RF_ID  	        0 
#define RDA_GPS_DIG_DEVNAME     "rda_gps_dig"
#define RDA_GPS_DIG_ADDR	    (0x3a >> 1) // 0x1d
#define RDA_GPS_DIG_ID  	    1

#define RDA_GPS_IOC_MAGIC       'w'
/* control message */
#define RDA_GPS_READ            _IOW(RDA_GPS_IOC_MAGIC, 0x01, unsigned int)
#define RDA_GPS_RF_WRITE        _IOW(RDA_GPS_IOC_MAGIC, 0x02, unsigned int)
#define RDA_GPS_DIG_WRITE       _IOW(RDA_GPS_IOC_MAGIC, 0x03, unsigned int)

struct gps_i2c_rf_data {
	unsigned char addr;
	unsigned short data;
};

struct gps_i2c_dig_data {
	unsigned int addr;
	unsigned int data;
};

#endif  
