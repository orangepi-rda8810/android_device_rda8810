#ifndef __LINUX_MSG2133_EX_FUN_H__
#define __LINUX_MSG2133_EX_FUN_H__
#include <linux/i2c.h>
int msg2133_create_sysfs(struct i2c_client *client);
void msg2133_release_sysfs(struct i2c_client *client);
#endif