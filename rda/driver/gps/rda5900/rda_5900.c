 /*
  * rda5900.c - RDA5900 Device Driver.
  *
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
#include <linux/init.h>
#include <linux/module.h>
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/sched.h>
#include <linux/device.h>
#include <linux/slab.h>
#include <linux/i2c.h>
//#include <linux/sched.h>
#include <asm/uaccess.h>
#include <plat/rda_debug.h> // kernel/arch/arm/plat-rda/include/plat/rda_debug.h

#include "rda_5900.h"

#define I2C_REPETION_TIMES          4

#define GPS_LOG_DBG                 3
#define GPS_LOG_INFO                2
#define GPS_LOG_WARN                1
#define GPS_LOG_ERR                 0

// the switch is in kernel/arch/arm/mach-rda8810/board-rda8810.c
unsigned int gDbgLevel = GPS_LOG_DBG;

#define DBG_PRINT(fmt, arg...)    if(gDbgLevel >= GPS_LOG_DBG) {rda_dbg_gps("%s: "  fmt, __FUNCTION__ ,##arg);}
#define INFO_PRINT(fmt, arg...)   if(gDbgLevel >= GPS_LOG_INFO) {rda_dbg_gps("%s: "  fmt, __FUNCTION__ ,##arg);}
#define WARN_PRINT(fmt, arg...)   if(gDbgLevel >= GPS_LOG_WARN) {rda_dbg_gps("%s: "  fmt, __FUNCTION__ ,##arg);}
#define ERR_PRINT(fmt, arg...)    if(gDbgLevel >= GPS_LOG_ERR) {rda_dbg_gps("%s: "  fmt, __FUNCTION__ ,##arg);}
#define TRC_PRINT(f)              if(gDbgLevel >= GPS_LOG_DBG) {rda_dbg_gps("<%s> <%d>\n", __FUNCTION__, __LINE__);}


struct rda5900_android_dev {
    struct semaphore sem;
    struct cdev dev;
    dev_t devno;
    struct class* rda5900_class;
    struct i2c_client *dig_client;
    struct i2c_client *rf_client;
};

struct rda5900_android_dev *p5900_dev = NULL;

static int rda5900_write_reg(unsigned int addr, unsigned int data, unsigned long opt)
{
    unsigned char send_buf[8];
    long ret = 0;
    unsigned char send_len;

    if (opt == RDA_GPS_RF_ID)
    {
        send_buf[0] = addr & 0xff;
        send_buf[1] = (data >> 8) & 0xff;
        send_buf[2] = data & 0xff;

        send_len = 3;
    }
    else if (opt == RDA_GPS_DIG_ID)
    {
        send_buf[0] = (addr >> 24) & 0xff;
        send_buf[1] = (addr >> 16) & 0xff;
        send_buf[2] = (addr >> 8) & 0xff;
        send_buf[3] = addr & 0xff;
        send_buf[4] = (data >> 24) & 0xff;
        send_buf[5] = (data >> 16) & 0xff;
        send_buf[6] = (data >> 8) & 0xff;
        send_buf[7] = data & 0xff;

        send_len = 8;
    }
    else
    {
        ERR_PRINT("unknow client.\n");
        return -ENXIO;
    }

    if (opt == RDA_GPS_RF_ID)
        ret = i2c_master_send(p5900_dev->rf_client, send_buf, send_len);
    else if (opt == RDA_GPS_DIG_ID)
        ret = i2c_master_send(p5900_dev->dig_client, send_buf, send_len);

    if (ret < 0) {
        ERR_PRINT("i2c send error.\n");
    }

    return ret;
}

static long rda5900_ioctl_write(unsigned long arg, unsigned long opt)
{
    void *user_addr;
    struct gps_i2c_rf_data rf_data = {0,0};
    struct gps_i2c_dig_data dig_data = {0,0};
    int ret, i;

    user_addr = (void __user*)arg;
    if (user_addr == NULL) {
        return -EINVAL;
    }

    if (opt == RDA_GPS_RF_ID)
    {
        if (copy_from_user(&rf_data, user_addr, sizeof(struct gps_i2c_rf_data))) {
            return -EFAULT;
        }

        DBG_PRINT("RF WRITE 0x%x <= 0x%x\n", rf_data.addr, rf_data.data);

        for (i = 0; i < I2C_REPETION_TIMES; i++)
        {
            ret = rda5900_write_reg(rf_data.addr, rf_data.data, opt);
            if (ret >= 0)
                break;

            schedule_timeout(80);
            DBG_PRINT("RF i2c send repeatedly\n");
        }
        if (i >= I2C_REPETION_TIMES)
            return -1;
    }
    else if (opt == RDA_GPS_DIG_ID)
    {
        if (copy_from_user(&dig_data, user_addr, sizeof(struct gps_i2c_dig_data))) {
            return -EFAULT;
        }

        DBG_PRINT("DIG WRITE 0x%x <= 0x%x\n", dig_data.addr, dig_data.data);

        for (i = 0; i < I2C_REPETION_TIMES; i++)
        {
            ret = rda5900_write_reg(dig_data.addr, dig_data.data, opt);
            if (ret >= 0)
                break;

            schedule_timeout(80);
            DBG_PRINT("DIG i2c send repeatedly\n");
        }
        if (i >= I2C_REPETION_TIMES)
            return -1;
    }

    return 0;
}

static int rda5900_open(struct inode* inode, struct file* filp)
{
    struct rda5900_android_dev* dev;

    dev = container_of(inode->i_cdev, struct rda5900_android_dev, dev);
    filp->private_data = dev;
    DBG_PRINT("open rda5900.\n");

    return 0;
}

static int rda5900_release(struct inode* inode, struct file* filp)
{
    DBG_PRINT("release rda5900.\n");
    return 0;
}

static long rda5900_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
    long ret = 0;

    switch (cmd)
    {
    case RDA_GPS_RF_WRITE:
        DBG_PRINT("ioctl cmd: RDA_GPS_RF_WRITE.\n");
        ret = rda5900_ioctl_write(arg, RDA_GPS_RF_ID);
        break;
    case RDA_GPS_DIG_WRITE:
        DBG_PRINT("ioctl cmd: RDA_GPS_DIG_WRITE.\n");
        ret = rda5900_ioctl_write(arg, RDA_GPS_DIG_ID);
        break;
    }

    return ret;
}

static struct file_operations rda5900_fops = {
        .owner = THIS_MODULE,
        .unlocked_ioctl = rda5900_ioctl,
        .open = rda5900_open,
        .release = rda5900_release,
};

static int  __rda5900_setup_dev(struct rda5900_android_dev* dev)
{
    int err;

    cdev_init(&(dev->dev), &rda5900_fops);
    dev->dev.owner = THIS_MODULE;
    dev->dev.ops = &rda5900_fops;

    err = cdev_add(&(dev->dev), dev->devno, 1);
    if(err) {
        return err;
    }

    sema_init(&(dev->sem), 1);

    return 0;
}

static int rda_gps_rf_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
    if (p5900_dev)
        p5900_dev->rf_client = client;
    else
        ERR_PRINT("5900 device haven't initialised.\n");

    if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
        ERR_PRINT("client is not i2c capable.\n");
        return -EPERM;
    }

    return 0;
}

static int rda_gps_rf_remove(struct i2c_client *client)
{
    return 0;
}

static const struct i2c_device_id rda_gps_rf_id[] =
{
    {RDA_GPS_RF_DEVNAME, RDA_GPS_RF_ID},
    {}
};
MODULE_DEVICE_TABLE(i2c, rda_gps_rf_id);

struct i2c_driver rda_gps_rf_driver = {
    .probe = rda_gps_rf_probe,
    .remove = rda_gps_rf_remove,
    .driver = {
        .name = RDA_GPS_RF_DRINAME,
    },
    .id_table = rda_gps_rf_id,
};

static int rda_gps_dig_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
    if (p5900_dev)
        p5900_dev->dig_client = client;
    else
        ERR_PRINT("5900 device haven't initialised.\n");

    if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
        ERR_PRINT("client is not i2c capable.\n");
        return -EPERM;
    }

    return 0;
}

static int rda_gps_dig_remove(struct i2c_client *client)
{
    return 0;
}

static const struct i2c_device_id rda_gps_dig_id[] =
{
    {RDA_GPS_DIG_DEVNAME, RDA_GPS_DIG_ID},
    {}
};
MODULE_DEVICE_TABLE(i2c, rda_gps_dig_id);

struct i2c_driver rda_gps_dig_driver = {
    .probe = rda_gps_dig_probe,
    .remove = rda_gps_dig_remove,
    .driver = {
        .name = RDA_GPS_DIG_DRINAME,
    },
    .id_table = rda_gps_dig_id,
};

// moved to kernel/arch/arm/mach-rda8810/devices.c
/* static struct i2c_board_info __initdata i2c_rda5900_info = {
    I2C_BOARD_INFO(RDA_GPS_RF_DEVNAME, RDA_GPS_RF_ADDR)
    I2C_BOARD_INFO(RDA_GPS_DIG_DEVNAME, RDA_GPS_DIG_ADDR)
};*/

static int __init rda5900_init(void)
{
    int err = -1;
    dev_t devno = 0;
    struct device* temp = NULL;
    struct rda5900_android_dev *gps_dev;

    DBG_PRINT("Initializing rda5900 device.\n");

    err = alloc_chrdev_region(&devno, 0, 1, RDA_GPS_DEVICE_NODE_NAME);
        if(err < 0) {
        ERR_PRINT("Failed to alloc char dev region.\n");
        goto fail;
    }

    gps_dev = kzalloc(sizeof(struct rda5900_android_dev), GFP_KERNEL);
    if(!gps_dev) {
        err = -ENOMEM;
        ERR_PRINT("Failed to alloc gps_dev.\n");
        goto unregister;
    }
    gps_dev->devno = devno;
    p5900_dev = gps_dev;

    //i2c_register_board_info(RDA_GPS_I2C_BUS_NUM, &i2c_rda5900_info, 2);
    if (i2c_add_driver(&rda_gps_dig_driver)) {
        ERR_PRINT("fail to add device into i2c\n");
        err = -ENODEV;
        return err;
    }

    if (i2c_add_driver(&rda_gps_rf_driver)) {
        ERR_PRINT("fail to add device into i2c\n");
        err = -ENODEV;
        return err;
    }

    err = __rda5900_setup_dev(gps_dev);
    if(err) {
        ERR_PRINT("Failed to setup dev: %d.\n", err);
        goto cleanup;
    }

    gps_dev->rda5900_class = class_create(THIS_MODULE, RDA_GPS_DEVICE_CLASS_NAME);
    if(IS_ERR(gps_dev->rda5900_class)) {
        err = PTR_ERR(gps_dev->rda5900_class);
        ERR_PRINT("Failed to create rda5900 class.\n");
        goto destroy_cdev;
    }

    temp = device_create(gps_dev->rda5900_class, NULL, devno, "%s", RDA_GPS_DEVICE_FILE_NAME);
        if(IS_ERR(temp)) {
        err = PTR_ERR(temp);
        ERR_PRINT("Failed to create rda5900 device.");
        goto destroy_class;
    }

    dev_set_drvdata(temp, gps_dev);

    DBG_PRINT("Succedded to initialize rda5900 device.\n");
    return 0;

destroy_class:
    class_destroy(gps_dev->rda5900_class);

destroy_cdev:
    cdev_del(&(gps_dev->dev));

cleanup:
    kfree(gps_dev);

unregister:
    unregister_chrdev_region(devno, 1);

fail:
    return err;
}

static void __exit rda5900_exit(void)
{
    DBG_PRINT("Destroy rda5900 device.\n");

    i2c_del_driver(&rda_gps_dig_driver);
    i2c_del_driver(&rda_gps_rf_driver);

    if(p5900_dev->rda5900_class) {
        device_destroy(p5900_dev->rda5900_class, p5900_dev->devno);
        class_destroy(p5900_dev->rda5900_class);
    }

    if(p5900_dev) {
        cdev_del(&(p5900_dev->dev));
    }

    unregister_chrdev_region(p5900_dev->devno, 1);
    p5900_dev = NULL;
    kfree(p5900_dev);
}

module_init(rda5900_init);
module_exit(rda5900_exit);

MODULE_AUTHOR("Linzhao Wu <linzhaowu@rdamicro.com>");
MODULE_DESCRIPTION("The RDA5900 driver for Linux");
MODULE_LICENSE("GPL");
