#include <linux/gpio.h>
#include <linux/i2c.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/gfp.h>
#include <linux/export.h>
#include <linux/err.h>
#include <linux/slab.h>
#include <linux/kthread.h>
#include <mach/board.h>

#include "rda_ts.h"
#include "msg2133_auto_update_firmware.h"

/* Use For Get CTP Data By I2C */
static struct i2c_client *i2c_client;
static u8 curr_ic_type;
#define CTP_ID_MSG21XX 1
#define CTP_ID_MSG21XXA 2
static unsigned short curr_ic_major;
static unsigned short curr_ic_minor;
static unsigned short update_bin_major;
static unsigned short update_bin_minor;

/* 0777为打开apk升级功能，0664为关闭apk升级功能 */
#define CTP_AUTHORITY 0664

#if 0
#define TP_DEBUG(format, ...) printk(KERN_INFO "MSG2133_MSG21XXA_updating ***" format "\n", ## __VA_ARGS__)
#else
#define TP_DEBUG(format, ...)
#endif
#if 0
#define TP_DEBUG_ERR(format, ...) printk(KERN_ERR "MSG2133_MSG21XXA_updating_err ***" format "\n", ## __VA_ARGS__)
#else
#define TP_DEBUG_ERR(format, ...)
#endif
static unsigned char fw_version[20];
static u8 handle_crc[94][1024];
static u8 g_dwiic_info_data[1024];   /* Buffer for info data */
static u32 crc_tab[256];
static int rda_ts_irq;

static int FwDataCnt;
static struct class  *firmware_class;
static struct device *firmware_cmd_dev;

#define N_BYTE_PER_TIME (8)   /* 1024的约数,根据平台修改 */
#define UPDATE_TIMES (1024/N_BYTE_PER_TIME)

#if 0   /* 根据平台不同选择不同位的i2c地址 */
#define FW_ADDR_MSG21XX (0xC4)
#define FW_ADDR_MSG21XX_TP (0x4C)
#define FW_UPDATE_ADDR_MSG21XX (0x92)
#else
#define FW_ADDR_MSG21XX (0xC4 >> 1)
#define FW_ADDR_MSG21XX_TP (0x4C >> 1)
#define FW_UPDATE_ADDR_MSG21XX (0x92 >> 1)
#endif

static void msg2133_reset(void)
{
	gpio_request(GPIO_TOUCH_RESET, "touch screen reset");
	gpio_set_value(GPIO_TOUCH_RESET, 1);
	msleep(10);
	gpio_set_value(GPIO_TOUCH_RESET, 0);
	msleep(150);
	gpio_set_value(GPIO_TOUCH_RESET, 1);
	msleep(200);
}

static int  get_ts_irq(struct i2c_client *client)
{
	struct rda_ts_data *ts = i2c_get_clientdata(client);
	ts->ts_irq = gpio_to_irq(ts->panel_info->ts_para.gpio_irq);
	return ts->ts_irq;
}

/* 以下5个以Hal开头的函数需要根据平台修改*/
/* disable irq
   static void disable_irq(void)
   {
   mt_eint_mask(CUST_EINT_TOUCH_PANEL_NUM);
   mt_eint_registration(CUST_EINT_TOUCH_PANEL_NUM, EINTF_TRIGGER_FALLING, NULL, 1);
   }
   enable irq
   static void enable_irq(void)
   {
   mt_eint_registration(CUST_EINT_TOUCH_PANEL_NUM, EINTF_TRIGGER_FALLING, tpd_eint_interrupt_handler, 1);
   mt_eint_unmask(CUST_EINT_TOUCH_PANEL_NUM);
   }
   reset the chip
*/

static void _HalTscrHWReset(void)
{
	msg2133_reset();
}

static int HalTscrCReadI2CSeq(u8 addr, u8* read_data, u16 size)
{
	int ret;
	i2c_client->addr = addr;
	ret = i2c_master_recv(i2c_client, read_data, size);
	if (addr != FW_ADDR_MSG21XX_TP)
		msleep(10);
	i2c_client->addr = FW_ADDR_MSG21XX_TP;
	if (ret <= 0)
		TP_DEBUG_ERR("HalTscrCReadI2CSeq error %d,addr = %x\n", ret,addr);

	return ret;
}

static int HalTscrCDevWriteI2CSeq(u16 addr, u8* data, u16 size)
{
	int ret;
	i2c_client->addr = addr;
	ret = i2c_master_send(i2c_client, data, size);
	if (addr != FW_ADDR_MSG21XX_TP)
		msleep(10);
	i2c_client->addr = FW_ADDR_MSG21XX_TP;
	if (ret <= 0)
		TP_DEBUG_ERR("HalTscrCDevWriteI2CSeq error %d,addr = %x\n", ret,addr);

	return ret;
}

static void dbbusDWIICEnterSerialDebugMode(void)
{
	u8 data[5];

	/* Enter the Serial Debug Mode */
	data[0] = 0x53;
	data[1] = 0x45;
	data[2] = 0x52;
	data[3] = 0x44;
	data[4] = 0x42;
	TP_DEBUG("*******%s*******\n",__func__);
	HalTscrCDevWriteI2CSeq(FW_ADDR_MSG21XX, data, 5);
	TP_DEBUG("======= %s ======= \n", __func__);
	msleep(10);
}

static void dbbusDWIICStopMCU(void)
{
	u8 data[1];

	/* Stop the MCU */
	data[0] = 0x37;
	HalTscrCDevWriteI2CSeq(FW_ADDR_MSG21XX, data, 1);
	msleep(10);
}

static void dbbusDWIICIICUseBus(void)
{
	u8 data[1];

	/* IIC Use Bus */
	data[0] = 0x35;
	HalTscrCDevWriteI2CSeq(FW_ADDR_MSG21XX, data, 1);
}

static void dbbusDWIICIICReshape(void)
{
	u8 data[1];

	/* IIC Re-shape */
	data[0] = 0x71;
	HalTscrCDevWriteI2CSeq(FW_ADDR_MSG21XX, data, 1);
}

static void dbbusDWIICIICNotUseBus(void)
{
	u8 data[1];
	TP_DEBUG(" *******= %s************* \n", __func__);
	/* IIC Not Use Bus */
	data[0] = 0x34;
	HalTscrCDevWriteI2CSeq(FW_ADDR_MSG21XX, data, 1);
	msleep(10);
}

static void dbbusDWIICNotStopMCU(void)
{
	u8 data[1];
	TP_DEBUG(" *******= %s************* \n", __func__);
	/* Not Stop the MCU */
	data[0] = 0x36;
	HalTscrCDevWriteI2CSeq(FW_ADDR_MSG21XX, data, 1);
	msleep(10);
}

static void dbbusDWIICExitSerialDebugMode(void)
{
	u8 data[1];
	TP_DEBUG(" *******= %s************* \n", __func__);
	/* Exit the Serial Debug Mode */
	data[0] = 0x45;

	if (HalTscrCDevWriteI2CSeq(FW_ADDR_MSG21XX, data, 1) < 0) {
		TP_DEBUG("retry after 10ms.");
		msleep(10);
		HalTscrCDevWriteI2CSeq(FW_ADDR_MSG21XX, data, 1);
	}

	/* Delay some interval to guard the next transaction */
	udelay(150);
}

static void drvISP_EntryIspMode(void)
{
	u8 bWriteData[5] = {0x4D, 0x53, 0x54, 0x41, 0x52};
	TP_DEBUG("\n******%s come in*******\n",__FUNCTION__);
	HalTscrCDevWriteI2CSeq(FW_UPDATE_ADDR_MSG21XX, bWriteData, 5);
	udelay(150);
}

/* First it needs send 0x11 to notify we want to get flash data back. */
static u8 drvISP_Read(u8 n, u8* pDataToRead)
{
	u8 Read_cmd = 0x11;
	unsigned char dbbus_rx_data[2] = {0};
	HalTscrCDevWriteI2CSeq(FW_UPDATE_ADDR_MSG21XX, &Read_cmd, 1);
	udelay(800);
	if (n == 1) {
		HalTscrCReadI2CSeq(FW_UPDATE_ADDR_MSG21XX, &dbbus_rx_data[0], 2);
		*pDataToRead = dbbus_rx_data[0];
		TP_DEBUG("dbbus=%d,%d===drvISP_Read=====\n",dbbus_rx_data[0],dbbus_rx_data[1]);
	} else {
		HalTscrCReadI2CSeq(FW_UPDATE_ADDR_MSG21XX, pDataToRead, n);
	}

	return 0;
}

static void drvISP_WriteEnable(void)
{
	u8 bWriteData[2] = {0x10, 0x06};
	u8 bWriteData1 = 0x12;
	HalTscrCDevWriteI2CSeq(FW_UPDATE_ADDR_MSG21XX, bWriteData, 2);
	udelay(150);
	HalTscrCDevWriteI2CSeq(FW_UPDATE_ADDR_MSG21XX, &bWriteData1, 1);
}

static void drvISP_ExitIspMode(void)
{
	u8 bWriteData = 0x24;
	HalTscrCDevWriteI2CSeq(FW_UPDATE_ADDR_MSG21XX, &bWriteData, 1);
	udelay(150);
}

static u8 drvISP_ReadStatus(void)
{
	u8 bReadData = 0;
	u8 bWriteData[2] = {0x10, 0x05};
	u8 bWriteData1 = 0x12;

	HalTscrCDevWriteI2CSeq(FW_UPDATE_ADDR_MSG21XX, bWriteData, 2);
	udelay(150);
	drvISP_Read(1, &bReadData);
	HalTscrCDevWriteI2CSeq(FW_UPDATE_ADDR_MSG21XX, &bWriteData1, 1);
	return bReadData;
}

static void drvISP_Program(u16 k, u8* pDataToWrite)
{
	u16 i = 0;
	u16 j = 0;
	u8 TX_data[133];
	u8 bWriteData1 = 0x12;
	u32 addr = k * 1024;
	u32 timeOutCount=0;

	/* 128*8 cycle */
	for (j = 0; j < 8; j++) {
		TX_data[0] = 0x10;
		TX_data[1] = 0x02; /* Page Program CMD */
		TX_data[2] = (addr + 128 * j) >> 16;
		TX_data[3] = (addr + 128 * j) >> 8;
		TX_data[4] = (addr + 128 * j);
		for (i = 0; i < 128; i++) {
			TX_data[5 + i] = pDataToWrite[j * 128 + i];
		}
		udelay(150);
		timeOutCount=0;
		while ((drvISP_ReadStatus() & 0x01) == 0x01) {
			timeOutCount++;
			if (timeOutCount >= 100000)
				break; /* around 1 sec timeout */
		}

		drvISP_WriteEnable();
		HalTscrCDevWriteI2CSeq(FW_UPDATE_ADDR_MSG21XX, TX_data, 133); /* write 133 byte per cycle */
		HalTscrCDevWriteI2CSeq(FW_UPDATE_ADDR_MSG21XX, &bWriteData1, 1);
	}
}

static void drvISP_Verify(u16 k, u8* pDataToVerify)
{
	u16 i = 0, j = 0;
	u8 bWriteData[5] = {0x10, 0x03, 0, 0, 0};
	u8 RX_data[256];
	u8 bWriteData1 = 0x12;
	u32 addr = k * 1024;
	u8 index = 0;
	u32 timeOutCount;
	/* 128*8 cycle */
	for (j = 0; j < 8; j++) {
		bWriteData[2] = (u8) ((addr + j * 128) >> 16);
		bWriteData[3] = (u8) ((addr + j * 128) >> 8);
		bWriteData[4] = (u8) (addr + j * 128);
		udelay(100);

		timeOutCount = 0;
		while ((drvISP_ReadStatus() & 0x01) == 0x01) {
			timeOutCount++;
			if (timeOutCount >= 100000)
				break; /* around 1 sec timeout */
		}

		HalTscrCDevWriteI2CSeq(FW_UPDATE_ADDR_MSG21XX, bWriteData, 5); /* write read flash addr */
		udelay(100);
		drvISP_Read (128, RX_data);
		HalTscrCDevWriteI2CSeq(FW_UPDATE_ADDR_MSG21XX, &bWriteData1, 1); /* cmd end */
		for (i = 0; i < 128; i++) { /* log out if verify error */
			if ((RX_data[i] != 0) && index < 10)
				index++;

			if (RX_data[i] != pDataToVerify[128 * j + i])
				TP_DEBUG("k=%d,j=%d,i=%d=====Update Firmware Error=====", k, j, i);
		}
	}
}

static void drvISP_ChipErase(void)
{
	u8 bWriteData[5] = { 0x00, 0x00, 0x00, 0x00, 0x00 };
	u8 bWriteData1 = 0x12;
	u32 timeOutCount = 0;
	drvISP_WriteEnable();

	/* Enable write status register */
	bWriteData[0] = 0x10;
	bWriteData[1] = 0x50;
	HalTscrCDevWriteI2CSeq(FW_UPDATE_ADDR_MSG21XX, bWriteData, 2);
	HalTscrCDevWriteI2CSeq(FW_UPDATE_ADDR_MSG21XX, &bWriteData1, 1);

	/* Write Status */
	bWriteData[0] = 0x10;
	bWriteData[1] = 0x01;
	bWriteData[2] = 0x00;
	HalTscrCDevWriteI2CSeq(FW_UPDATE_ADDR_MSG21XX, bWriteData, 3);
	HalTscrCDevWriteI2CSeq(FW_UPDATE_ADDR_MSG21XX, &bWriteData1, 1);

	/* Write disable */
	bWriteData[0] = 0x10;
	bWriteData[1] = 0x04;
	HalTscrCDevWriteI2CSeq(FW_UPDATE_ADDR_MSG21XX, bWriteData, 2);
	HalTscrCDevWriteI2CSeq(FW_UPDATE_ADDR_MSG21XX, &bWriteData1, 1);
	udelay(100);
	timeOutCount = 0;
	while ((drvISP_ReadStatus() & 0x01) == 0x01) {
		timeOutCount++;
		if (timeOutCount >= 100000)
			break; /* around 1 sec timeout */
	}
	drvISP_WriteEnable();

	bWriteData[0] = 0x10;
	bWriteData[1] = 0xC7;

	HalTscrCDevWriteI2CSeq(FW_UPDATE_ADDR_MSG21XX, bWriteData, 2);
	HalTscrCDevWriteI2CSeq(FW_UPDATE_ADDR_MSG21XX, &bWriteData1, 1);
	udelay(100);
	timeOutCount = 0;
	while ((drvISP_ReadStatus() & 0x01) == 0x01) {
		timeOutCount++;
		if (timeOutCount >= 500000)
			break; /* around 5 sec timeout */
	}
}

/*
 * update the firmware part, used by apk
 * show the fw version
 */
static ssize_t firmware_update_c2(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
	u8 i;
	u8 dbbus_tx_data[4];
	unsigned char dbbus_rx_data[2] = {0};
	/* set FRO to 50M */
	dbbus_tx_data[0] = 0x10;
	dbbus_tx_data[1] = 0x11;
	dbbus_tx_data[2] = 0xE2;
	HalTscrCDevWriteI2CSeq(FW_ADDR_MSG21XX, dbbus_tx_data, 3);
	dbbus_rx_data[0] = 0;
	dbbus_rx_data[1] = 0;
	HalTscrCReadI2CSeq(FW_ADDR_MSG21XX, &dbbus_rx_data[0], 2);
	TP_DEBUG("dbbus_rx_data[0]=0x%x", dbbus_rx_data[0]);
	dbbus_tx_data[3] = dbbus_rx_data[0] & 0xF7; /* Clear Bit 3 */
	HalTscrCDevWriteI2CSeq(FW_ADDR_MSG21XX, dbbus_tx_data, 4);

	/* set MCU clock,SPI clock =FRO */
	dbbus_tx_data[0] = 0x10;
	dbbus_tx_data[1] = 0x1E;
	dbbus_tx_data[2] = 0x22;
	dbbus_tx_data[3] = 0x00;
	HalTscrCDevWriteI2CSeq(FW_ADDR_MSG21XX, dbbus_tx_data, 4);

	dbbus_tx_data[0] = 0x10;
	dbbus_tx_data[1] = 0x1E;
	dbbus_tx_data[2] = 0x23;
	dbbus_tx_data[3] = 0x00;
	HalTscrCDevWriteI2CSeq(FW_ADDR_MSG21XX, dbbus_tx_data, 4);

	/* Enable slave's ISP ECO mode */
	dbbus_tx_data[0] = 0x10;
	dbbus_tx_data[1] = 0x08;
	dbbus_tx_data[2] = 0x0c;
	dbbus_tx_data[3] = 0x08;
	HalTscrCDevWriteI2CSeq(FW_ADDR_MSG21XX, dbbus_tx_data, 4);

	/* Enable SPI Pad */
	dbbus_tx_data[0] = 0x10;
	dbbus_tx_data[1] = 0x1E;
	dbbus_tx_data[2] = 0x02;
	HalTscrCDevWriteI2CSeq(FW_ADDR_MSG21XX, dbbus_tx_data, 3);
	HalTscrCReadI2CSeq(FW_ADDR_MSG21XX, &dbbus_rx_data[0], 2);
	TP_DEBUG("dbbus_rx_data[0]=0x%x", dbbus_rx_data[0]);
	dbbus_tx_data[3] = (dbbus_rx_data[0] | 0x20); /* Set Bit 5 */
	HalTscrCDevWriteI2CSeq(FW_ADDR_MSG21XX, dbbus_tx_data, 4);

	/* WP overwrite */
	dbbus_tx_data[0] = 0x10;
	dbbus_tx_data[1] = 0x1E;
	dbbus_tx_data[2] = 0x0E;
	dbbus_tx_data[3] = 0x02;
	HalTscrCDevWriteI2CSeq(FW_ADDR_MSG21XX, dbbus_tx_data, 4);

	/* set pin high */
	dbbus_tx_data[0] = 0x10;
	dbbus_tx_data[1] = 0x1E;
	dbbus_tx_data[2] = 0x10;
	dbbus_tx_data[3] = 0x08;
	HalTscrCDevWriteI2CSeq(FW_ADDR_MSG21XX, dbbus_tx_data, 4);

	dbbusDWIICIICNotUseBus();
	dbbusDWIICNotStopMCU();
	dbbusDWIICExitSerialDebugMode();

	drvISP_EntryIspMode();
	drvISP_ChipErase();
	_HalTscrHWReset();
	msleep(300);

	/* Program and Verify */
	dbbusDWIICEnterSerialDebugMode();
	dbbusDWIICStopMCU();
	dbbusDWIICIICUseBus();
	dbbusDWIICIICReshape();

	/* Disable the Watchdog */
	dbbus_tx_data[0] = 0x10;
	dbbus_tx_data[1] = 0x3C;
	dbbus_tx_data[2] = 0x60;
	dbbus_tx_data[3] = 0x55;
	HalTscrCDevWriteI2CSeq(FW_ADDR_MSG21XX, dbbus_tx_data, 4);
	dbbus_tx_data[0] = 0x10;
	dbbus_tx_data[1] = 0x3C;
	dbbus_tx_data[2] = 0x61;
	dbbus_tx_data[3] = 0xAA;
	HalTscrCDevWriteI2CSeq(FW_ADDR_MSG21XX, dbbus_tx_data, 4);

	/* Stop MCU */
	dbbus_tx_data[0] = 0x10;
	dbbus_tx_data[1] = 0x0F;
	dbbus_tx_data[2] = 0xE6;
	dbbus_tx_data[3] = 0x01;
	HalTscrCDevWriteI2CSeq(FW_ADDR_MSG21XX, dbbus_tx_data, 4);

	/* set FRO to 50M */
	dbbus_tx_data[0] = 0x10;
	dbbus_tx_data[1] = 0x11;
	dbbus_tx_data[2] = 0xE2;
	HalTscrCDevWriteI2CSeq(FW_ADDR_MSG21XX, dbbus_tx_data, 3);
	dbbus_rx_data[0] = 0;
	dbbus_rx_data[1] = 0;
	HalTscrCReadI2CSeq(FW_ADDR_MSG21XX, &dbbus_rx_data[0], 2);
	TP_DEBUG("dbbus_rx_data[0]=0x%x", dbbus_rx_data[0]);
	dbbus_tx_data[3] = dbbus_rx_data[0] & 0xF7; /* Clear Bit 3 */
	HalTscrCDevWriteI2CSeq(FW_ADDR_MSG21XX, dbbus_tx_data, 4);

	/* set MCU clock,SPI clock =FRO */
	dbbus_tx_data[0] = 0x10;
	dbbus_tx_data[1] = 0x1E;
	dbbus_tx_data[2] = 0x22;
	dbbus_tx_data[3] = 0x00;
	HalTscrCDevWriteI2CSeq(FW_ADDR_MSG21XX, dbbus_tx_data, 4);

	dbbus_tx_data[0] = 0x10;
	dbbus_tx_data[1] = 0x1E;
	dbbus_tx_data[2] = 0x23;
	dbbus_tx_data[3] = 0x00;
	HalTscrCDevWriteI2CSeq(FW_ADDR_MSG21XX, dbbus_tx_data, 4);

	/* Enable slave's ISP ECO mode */
	dbbus_tx_data[0] = 0x10;
	dbbus_tx_data[1] = 0x08;
	dbbus_tx_data[2] = 0x0c;
	dbbus_tx_data[3] = 0x08;
	HalTscrCDevWriteI2CSeq(FW_ADDR_MSG21XX, dbbus_tx_data, 4);

	/* Enable SPI Pad */
	dbbus_tx_data[0] = 0x10;
	dbbus_tx_data[1] = 0x1E;
	dbbus_tx_data[2] = 0x02;
	HalTscrCDevWriteI2CSeq(FW_ADDR_MSG21XX, dbbus_tx_data, 3);
	HalTscrCReadI2CSeq(FW_ADDR_MSG21XX, &dbbus_rx_data[0], 2);
	TP_DEBUG("dbbus_rx_data[0]=0x%x", dbbus_rx_data[0]);
	dbbus_tx_data[3] = (dbbus_rx_data[0] | 0x20); /* Set Bit 5 */
	HalTscrCDevWriteI2CSeq(FW_ADDR_MSG21XX, dbbus_tx_data, 4);

	/* WP overwrite */
	dbbus_tx_data[0] = 0x10;
	dbbus_tx_data[1] = 0x1E;
	dbbus_tx_data[2] = 0x0E;
	dbbus_tx_data[3] = 0x02;
	HalTscrCDevWriteI2CSeq(FW_ADDR_MSG21XX, dbbus_tx_data, 4);

	/* set pin high */
	dbbus_tx_data[0] = 0x10;
	dbbus_tx_data[1] = 0x1E;
	dbbus_tx_data[2] = 0x10;
	dbbus_tx_data[3] = 0x08;
	HalTscrCDevWriteI2CSeq(FW_ADDR_MSG21XX, dbbus_tx_data, 4);

	dbbusDWIICIICNotUseBus();
	dbbusDWIICNotStopMCU();
	dbbusDWIICExitSerialDebugMode();

	/* Start to load firmware */
	drvISP_EntryIspMode();

	for (i = 0; i < 94; i++) { /* total  94 KB : 1 byte per R/W */
		drvISP_Program(i, handle_crc[i]); /* program to slave's flash */
		drvISP_Verify(i, handle_crc[i]); /* verify data */
	}
	TP_DEBUG("update_C2 OK\n");
	drvISP_ExitIspMode();
	_HalTscrHWReset();
	FwDataCnt = 0;
	enable_irq(rda_ts_irq);
	return size;
}

static u32 Reflect(u32 ref, char ch)
{
	u32 value = 0;
	u32 i = 0;

	for (i = 1; i < (ch + 1); i++) {
		if (ref & 1)
			value |= 1 << (ch - i);

		ref >>= 1;
	}
	return value;
}

u32 Get_CRC(u32 text, u32 prevCRC, u32 *crc32_table)
{
	u32  ulCRC = prevCRC;
	ulCRC = (ulCRC >> 8) ^ crc32_table[(ulCRC & 0xFF) ^ text];
	return ulCRC ;
}

static void Init_CRC32_Table(u32 *crc32_table)
{
	u32 magicnumber = 0x04c11db7;
	u32 i = 0, j;

	for (i = 0; i <= 0xFF; i++) {
		crc32_table[i] = Reflect(i, 8) << 24;
		for (j = 0; j < 8; j++) {
			crc32_table[i] = (crc32_table[i] << 1) ^ (crc32_table[i] & (0x80000000L) ? magicnumber : 0);
		}
		crc32_table[i] = Reflect(crc32_table[i], 32);
	}
}

typedef enum
{
	EMEM_ALL = 0,
	EMEM_MAIN,
	EMEM_INFO,
} EMEM_TYPE_t;

static void drvDB_WriteReg8Bit(u8 bank, u8 addr, u8 data)
{
	u8 tx_data[4] = {0x10, bank, addr, data};
	HalTscrCDevWriteI2CSeq(FW_ADDR_MSG21XX, tx_data, 4);
}

static void drvDB_WriteReg(u8 bank, u8 addr, u16 data)
{
	u8 tx_data[5] = {0x10, bank, addr, data & 0xFF, data >> 8};
	HalTscrCDevWriteI2CSeq(FW_ADDR_MSG21XX, tx_data, 5);
}

static unsigned short drvDB_ReadReg(u8 bank, u8 addr)
{
	u8 tx_data[3] = {0x10, bank, addr};
	u8 rx_data[2] = {0};

	HalTscrCDevWriteI2CSeq(FW_ADDR_MSG21XX, tx_data, 3);
	HalTscrCReadI2CSeq(FW_ADDR_MSG21XX, &rx_data[0], 2);
	return (rx_data[1] << 8 | rx_data[0]);
}

static int drvTP_erase_emem_c32 (void)
{
	/* enter gpio mode */
	drvDB_WriteReg(0x16, 0x1E, 0xBEAF);

	/* before gpio mode, set the control pin as the orginal status */
	drvDB_WriteReg(0x16, 0x08, 0x0000);
	drvDB_WriteReg8Bit(0x16, 0x0E, 0x10);
	msleep(10);

	/* ptrim = 1, h'04[2] */
	drvDB_WriteReg8Bit(0x16, 0x08, 0x04);
	drvDB_WriteReg8Bit(0x16, 0x0E, 0x10);
	msleep(10);

	/* ptm = 6, h'04[12:14] = b'110 */
	drvDB_WriteReg8Bit(0x16, 0x09, 0x60);
	drvDB_WriteReg8Bit(0x16, 0x0E, 0x10);

	/* pmasi = 1, h'04[6] */
	drvDB_WriteReg8Bit(0x16, 0x08, 0x44);
	/* pce = 1, h'04[11] */
	drvDB_WriteReg8Bit(0x16, 0x09, 0x68);
	/* perase = 1, h'04[7] */
	drvDB_WriteReg8Bit(0x16, 0x08, 0xC4);
	/* pnvstr = 1, h'04[5] */
	drvDB_WriteReg8Bit(0x16, 0x08, 0xE4);
	/* pwe = 1, h'04[9] */
	drvDB_WriteReg8Bit(0x16, 0x09, 0x6A);
	/* trigger gpio load */
	drvDB_WriteReg8Bit(0x16, 0x0E, 0x10);

	return (1);
}

static ssize_t firmware_update_c32(struct device *dev, struct device_attribute *attr,
		const char *buf, size_t size,  EMEM_TYPE_t emem_type)
{
	/* Buffer for slave's firmware */
	u32 i, j;
	u32 crc_main, crc_main_tp;
	u32 crc_info, crc_info_tp;
	u16 reg_data = 0;
	int retry = 5;
	bool read_success = false;

	crc_main = 0xffffffff;
	crc_info = 0xffffffff;
	printk("   %s    ",__func__);

	drvTP_erase_emem_c32();
	msleep(1000);

	_HalTscrHWReset();
	dbbusDWIICEnterSerialDebugMode();
	dbbusDWIICStopMCU();
	dbbusDWIICIICUseBus();
	dbbusDWIICIICReshape();
	msleep(300);

	/* Reset Watchdog */
	drvDB_WriteReg8Bit(0x3C, 0x60, 0x55);
	drvDB_WriteReg8Bit(0x3C, 0x61, 0xAA);

	/* polling 0x3CE4 is 0x1C70 */
	do {
		msleep(100);
		reg_data = drvDB_ReadReg(0x3C, 0xE4);
		if (reg_data == 0x1C70) {
			read_success = true;
			break;
		}
	} while ((retry-- > 0) && (reg_data != 0x1C70));

	if (!read_success) {
		printk(KERN_ERR "%s: r[0x3CE4] == 0x1C70 failed!\n", __func__);
		return -ENODEV;
	}

	drvDB_WriteReg(0x3C, 0xE4, 0xE38F); /* for all-blocks */

	/* polling 0x3CE4 is 0x2F43 */
	do {
		msleep(100);
		reg_data = drvDB_ReadReg(0x3C, 0xE4);
		if (reg_data == 0x2F43) {
			read_success = true;
			break;
		}
	} while ((retry-- > 0) && (reg_data != 0x2F43));

	if (!read_success) {
		printk(KERN_ERR "%s: r[0x3CE4] == 0x2F43 failed!\n", __func__);
		return -ENODEV;
	}

	/* calculate CRC 32 */
	Init_CRC32_Table(&crc_tab[0]);

	for (i = 0; i < 33; i++) { /* total  33 KB : 2 byte per R/W */
		if (i < 32) { /* emem_main */
			if (i == 31) {
				handle_crc[i][1014] = 0x5A;
				handle_crc[i][1015] = 0xA5;

				for (j = 0; j < 1016; j++) {
					crc_main = Get_CRC(handle_crc[i][j], crc_main, &crc_tab[0]);
				}
			} else {
				for (j = 0; j < 1024; j++) {
					crc_main = Get_CRC(handle_crc[i][j], crc_main, &crc_tab[0]);
				}
			}
		} else { /* emem_info */
			for (j = 0; j < 1024; j++) {
				crc_info = Get_CRC(handle_crc[i][j], crc_info, &crc_tab[0]);
			}
		}

		HalTscrCDevWriteI2CSeq(FW_ADDR_MSG21XX_TP, handle_crc[i], 1024);

		/* polling 0x3CE4 is 0xD0BC */
		do {
			msleep(100);
			reg_data = drvDB_ReadReg(0x3C, 0xE4);
			if (reg_data == 0xD0BC) {
				read_success = true;
				break;
			}
		} while ((retry-- > 0) && (reg_data != 0xD0BC));

		if (!read_success) {
			printk(KERN_ERR "%s: r[0x3CE4] == 0xD0BC failed!\n", __func__);
			return -ENODEV;
		}

		drvDB_WriteReg(0x3C, 0xE4, 0x2F43);
	}

	/* write file done */
	drvDB_WriteReg(0x3C, 0xE4, 0x1380);

	msleep(10);
	/* polling 0x3CE4 is 0x9432 */
	do {
		msleep(100);
		reg_data = drvDB_ReadReg(0x3C, 0xE4);
		if (reg_data == 0x9432) {
			read_success = true;
			break;
		}
	} while ((retry-- > 0) && (reg_data != 0x9432));

	if (!read_success) {
		printk(KERN_ERR "%s: r[0x3CE4] == 0x9432 failed!\n", __func__);
		return -ENODEV;
	}

	crc_main = crc_main ^ 0xffffffff;
	crc_info = crc_info ^ 0xffffffff;

	/* CRC Main from TP */
	crc_main_tp = drvDB_ReadReg(0x3C, 0x80);
	crc_main_tp = (crc_main_tp << 16) | drvDB_ReadReg(0x3C, 0x82);

	/* CRC Info from TP */
	crc_info_tp = drvDB_ReadReg(0x3C, 0xA0);
	crc_info_tp = (crc_info_tp << 16) | drvDB_ReadReg(0x3C, 0xA2);

	TP_DEBUG("crc_main=0x%x, crc_info=0x%x, crc_main_tp=0x%x, crc_info_tp=0x%x\n",
		crc_main, crc_info, crc_main_tp, crc_info_tp);

	if ((crc_main_tp != crc_main) || (crc_info_tp != crc_info)) {
		TP_DEBUG_ERR("update_C32 FAILED\n");
		_HalTscrHWReset();
		FwDataCnt = 0;
		enable_irq(rda_ts_irq);
		return (0);
	}

	TP_DEBUG_ERR("update_C32 OK\n");
	_HalTscrHWReset();
	FwDataCnt = 0;
	enable_irq(rda_ts_irq);

	return size;
}

static int drvTP_erase_emem_c33(EMEM_TYPE_t emem_type)
{
	/* stop mcu */
	drvDB_WriteReg(0x0F, 0xE6, 0x0001);

	/* disable watch dog */
	drvDB_WriteReg8Bit(0x3C, 0x60, 0x55);
	drvDB_WriteReg8Bit(0x3C, 0x61, 0xAA);

	/* set PROGRAM password */
	drvDB_WriteReg8Bit(0x16, 0x1A, 0xBA);
	drvDB_WriteReg8Bit(0x16, 0x1B, 0xAB);

	drvDB_WriteReg8Bit(0x16, 0x18, 0x80);

	if (emem_type == EMEM_ALL)
		drvDB_WriteReg8Bit(0x16, 0x08, 0x10);

	drvDB_WriteReg8Bit(0x16, 0x18, 0x40);
	msleep(10);

	drvDB_WriteReg8Bit(0x16, 0x18, 0x80);

	/* erase trigger */
	if (emem_type == EMEM_MAIN)
		drvDB_WriteReg8Bit(0x16, 0x0E, 0x04); /* erase main */
	else
		drvDB_WriteReg8Bit(0x16, 0x0E, 0x08); /* erase all block */

	return (1);
}

static int drvTP_read_info_dwiic_c33(void)
{
	u8  dwiic_tx_data[5];
	u16 reg_data=0;
	int retry = 5;
	bool read_success = false;

	msleep(300);

	/* Stop Watchdog */
	drvDB_WriteReg8Bit(0x3C, 0x60, 0x55);
	drvDB_WriteReg8Bit(0x3C, 0x61, 0xAA);
	drvDB_WriteReg(0x3C, 0xE4, 0xA4AB);
	drvDB_WriteReg(0x1E, 0x04, 0x7d60);

	/* TP SW reset */
	drvDB_WriteReg(0x1E, 0x04, 0x829F);
	msleep(1);
	dwiic_tx_data[0] = 0x10;
	dwiic_tx_data[1] = 0x0F;
	dwiic_tx_data[2] = 0xE6;
	dwiic_tx_data[3] = 0x00;
	HalTscrCDevWriteI2CSeq(FW_ADDR_MSG21XX, dwiic_tx_data, 4);
	msleep(100);

	/* polling 0x3CE4 is 0x5B58 */
	do {
		msleep(100);
		reg_data = drvDB_ReadReg(0x3C, 0xE4);
		if (reg_data == 0x5B58) {
			read_success = true;
			break;
		}
	} while ((retry-- > 0) && (reg_data != 0x5B58));

	if (!read_success) {
		printk(KERN_ERR "%s: r[0x3CE4] == 0x5B58 failed!\n", __func__);
		return -ENODEV;
	}

	TP_DEBUG_ERR("33333333333333");
	dwiic_tx_data[0] = 0x72;
	dwiic_tx_data[1] = 0x80;
	dwiic_tx_data[2] = 0x00;
	dwiic_tx_data[3] = 0x04;
	dwiic_tx_data[4] = 0x00;
	HalTscrCDevWriteI2CSeq(FW_ADDR_MSG21XX_TP , dwiic_tx_data, 5);
	TP_DEBUG_ERR("4444444444444");
	msleep(50);

	/* recive info data */
	HalTscrCReadI2CSeq(FW_ADDR_MSG21XX_TP, &g_dwiic_info_data[0], 8);
	TP_DEBUG_ERR("55555555555555");
	return (1);
}

static ssize_t firmware_update_c33(struct device *dev, struct device_attribute *attr,
		const char *buf, size_t size, EMEM_TYPE_t emem_type)
{
	u32 i, j;
	u32 crc_main, crc_main_tp;
	u32 crc_info, crc_info_tp;
	int update_pass = 1;
	u16 reg_data = 0;
	int retry = 5;
	bool read_success = false;

	crc_main = 0xffffffff;
	crc_info = 0xffffffff;

	TP_DEBUG("111111111111");
	drvTP_read_info_dwiic_c33();

	/* erase main */
	TP_DEBUG("aaaaaaaaaaa");
	drvTP_erase_emem_c33(EMEM_MAIN);
	msleep(1000);

	_HalTscrHWReset();

	dbbusDWIICEnterSerialDebugMode();
	dbbusDWIICStopMCU();
	dbbusDWIICIICUseBus();
	dbbusDWIICIICReshape();
	msleep(300);

	if ((emem_type == EMEM_ALL) || (emem_type == EMEM_MAIN)) {
		/* polling 0x3CE4 is 0x1C70 */
		do {
			msleep(100);
			reg_data = drvDB_ReadReg(0x3C, 0xE4);
			if (reg_data == 0x1C70) {
				read_success = true;
				break;
			}
		} while ((retry-- > 0) && (reg_data != 0x1C70));

		if (!read_success) {
			printk(KERN_ERR "%s: r[0x3CE4] == 0x1C70 failed!\n", __func__);
			return -ENODEV;
		}
	}

	switch (emem_type) {
	case EMEM_ALL:
		drvDB_WriteReg(0x3C, 0xE4, 0xE38F); /* for all-blocks */
		break;
	case EMEM_MAIN:
		drvDB_WriteReg(0x3C, 0xE4, 0x7731); /* for main block */
		break;
	case EMEM_INFO:
		drvDB_WriteReg(0x3C, 0xE4, 0x7731); /* for info block */
		drvDB_WriteReg8Bit(0x0F, 0xE6, 0x01);
		drvDB_WriteReg8Bit(0x3C, 0xE4, 0xC5);
		drvDB_WriteReg8Bit(0x3C, 0xE5, 0x78);
		drvDB_WriteReg8Bit(0x1E, 0x04, 0x9F);
		drvDB_WriteReg8Bit(0x1E, 0x05, 0x82);
		drvDB_WriteReg8Bit(0x0F, 0xE6, 0x00);
		msleep(100);
		break;
	}
	TP_DEBUG("bbbbbbbbbbbbbb");

	/* polling 0x3CE4 is 0x2F43 */
	do {
		msleep(100);
		reg_data = drvDB_ReadReg(0x3C, 0xE4);
		if (reg_data == 0x2F43) {
			read_success = true;
			break;
		}
	} while ((retry-- > 0) && (reg_data != 0x2F43));

	if (!read_success) {
		printk(KERN_ERR "%s: r[0x3CE4] == 0x2F43 failed!\n", __func__);
		return -ENODEV;
	}

	TP_DEBUG("ccccccccccccc");
	/* calculate CRC 32 */
	Init_CRC32_Table(&crc_tab[0]);

	for (i = 0; i < 33; i++) { /* total  33 KB : 2 byte per R/W */
		if (emem_type == EMEM_INFO)
			i = 32;

		if (i < 32) { /* emem_main */
			if (i == 31) {
				handle_crc[i][1014] = 0x5A;
				handle_crc[i][1015] = 0xA5;

				for (j = 0; j < 1016; j++) {
					crc_main = Get_CRC(handle_crc[i][j], crc_main, &crc_tab[0]);
				}
			} else {
				for (j = 0; j < 1024; j++) {
					crc_main = Get_CRC(handle_crc[i][j], crc_main, &crc_tab[0]);
				}
			}
		} else { /* emem_info */
			for (j = 0; j < 1024; j++) {
				crc_info = Get_CRC(g_dwiic_info_data[j], crc_info, &crc_tab[0]);
			}
			if (emem_type == EMEM_MAIN)
				break;
		}
		TP_DEBUG("dddddddddddddd");
#if 1
		{
			u32 n = 0;
			for (n=0;n < UPDATE_TIMES;n++) {
				HalTscrCDevWriteI2CSeq(FW_ADDR_MSG21XX_TP, handle_crc[i] + n * N_BYTE_PER_TIME,
					N_BYTE_PER_TIME);
			}
		}
#else
		HalTscrCDevWriteI2CSeq(FW_ADDR_MSG21XX_TP, handle_crc[i], 1024);
#endif
		TP_DEBUG("eeeeeeeeeeee");

		/* polling 0x3CE4 is 0xD0BC */
		do {
			msleep(100);
			reg_data = drvDB_ReadReg(0x3C, 0xE4);
			if (reg_data == 0xD0BC) {
				read_success = true;
				break;
			}
		} while ((retry-- > 0) && (reg_data != 0xD0BC));

		if (!read_success) {
			printk(KERN_ERR "%s: r[0x3CE4] == 0xD0BC failed!\n", __func__);
			return -ENODEV;
		}

		TP_DEBUG("ffffffffffffff");
		drvDB_WriteReg(0x3C, 0xE4, 0x2F43);
	}
	TP_DEBUG("ggggggggg");
	if ((emem_type == EMEM_ALL) || (emem_type == EMEM_MAIN)) {
		/* write file done and check crc */
		drvDB_WriteReg(0x3C, 0xE4, 0x1380);
		TP_DEBUG("hhhhhhhhhhhhhh");
	}
	msleep(10);

	if ((emem_type == EMEM_ALL) || (emem_type == EMEM_MAIN)) {
		TP_DEBUG("iiiiiiiiii");
		/* polling 0x3CE4 is 0x9432 */
		do {
			msleep(100);
			reg_data = drvDB_ReadReg(0x3C, 0xE4);
			if (reg_data == 0x9432) {
				read_success = true;
				break;
			}
		} while ((retry-- > 0) && (reg_data != 0x9432));

		if (!read_success) {
			printk(KERN_ERR "%s: r[0x3CE4] == 0x9432 failed!\n", __func__);
			return -ENODEV;
		}

		TP_DEBUG("jjjjjjjjjjjjj");
	}

	crc_main = crc_main ^ 0xffffffff;
	crc_info = crc_info ^ 0xffffffff;

	if ((emem_type == EMEM_ALL) || (emem_type == EMEM_MAIN)) {
		TP_DEBUG("kkkkkkkkkkk");
		/* CRC Main from TP */
		crc_main_tp = drvDB_ReadReg(0x3C, 0x80);
		crc_main_tp = (crc_main_tp << 16) | drvDB_ReadReg(0x3C, 0x82);

		/* CRC Info from TP */
		crc_info_tp = drvDB_ReadReg(0x3C, 0xA0);
		crc_info_tp = (crc_info_tp << 16) | drvDB_ReadReg(0x3C, 0xA2);
	}
	TP_DEBUG("crc_main=0x%x, crc_info=0x%x, crc_main_tp=0x%x, crc_info_tp=0x%x\n",
		crc_main, crc_info, crc_main_tp, crc_info_tp);

	TP_DEBUG("lllllllllllll");
	update_pass = 1;
	if ((emem_type == EMEM_ALL) || (emem_type == EMEM_MAIN)) {
		if (crc_main_tp != crc_main)
			update_pass = 0;

		if (crc_info_tp != crc_info)
			update_pass = 0;
	}

	if (!update_pass) {
		printk("[TP] update_C33 update_pass ok!\n");
		_HalTscrHWReset();
		FwDataCnt = 0;
		enable_irq(rda_ts_irq);
		return size;
	}

	printk("[TP] update_C33 OK!\n");
	_HalTscrHWReset();
	FwDataCnt = 0;
	enable_irq(rda_ts_irq);
	return size;
}

static ssize_t firmware_update_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%s\n", fw_version);
}

static ssize_t firmware_update_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
	u8 i;
	u8 dbbus_tx_data[4];
	unsigned char dbbus_rx_data[2] = {0};
	if (curr_ic_type == CTP_ID_MSG21XXA) {
		disable_irq(rda_ts_irq);
		_HalTscrHWReset();
		msleep(100);
		dbbusDWIICEnterSerialDebugMode();
		dbbusDWIICStopMCU();
		dbbusDWIICIICUseBus();
		dbbusDWIICIICReshape();
		msleep(300);
		/* Disable the Watchdog */
		dbbus_tx_data[0] = 0x10;
		dbbus_tx_data[1] = 0x3C;
		dbbus_tx_data[2] = 0x60;
		dbbus_tx_data[3] = 0x55;
		HalTscrCDevWriteI2CSeq(FW_ADDR_MSG21XX, dbbus_tx_data, 4);
		dbbus_tx_data[0] = 0x10;
		dbbus_tx_data[1] = 0x3C;
		dbbus_tx_data[2] = 0x61;
		dbbus_tx_data[3] = 0xAA;
		HalTscrCDevWriteI2CSeq(FW_ADDR_MSG21XX, dbbus_tx_data, 4);
		/* Stop MCU */
		dbbus_tx_data[0] = 0x10;
		dbbus_tx_data[1] = 0x0F;
		dbbus_tx_data[2] = 0xE6;
		dbbus_tx_data[3] = 0x01;
		HalTscrCDevWriteI2CSeq(FW_ADDR_MSG21XX, dbbus_tx_data, 4);

		/*
		 * Difference between C2 and C3
		 * c2:2133 c32:2133a(2) c33:2138
		 * check id
		 */
		dbbus_tx_data[0] = 0x10;
		dbbus_tx_data[1] = 0x1E;
		dbbus_tx_data[2] = 0xCC;
		HalTscrCDevWriteI2CSeq(FW_ADDR_MSG21XX, dbbus_tx_data, 3);
		HalTscrCReadI2CSeq(FW_ADDR_MSG21XX, &dbbus_rx_data[0], 2);
		TP_DEBUG_ERR("111dbbus_rx version[0]=0x%x", dbbus_rx_data[0]);
		if (dbbus_rx_data[0] == 2) {
			/* check version */
			dbbus_tx_data[0] = 0x10;
			dbbus_tx_data[1] = 0x3C;
			dbbus_tx_data[2] = 0xEA;
			HalTscrCDevWriteI2CSeq(FW_ADDR_MSG21XX, dbbus_tx_data, 3);
			HalTscrCReadI2CSeq(FW_ADDR_MSG21XX, &dbbus_rx_data[0], 2);
			TP_DEBUG_ERR("dbbus_rx version[0]=0x%x", dbbus_rx_data[0]);

			if (dbbus_rx_data[0] == 3)
				return firmware_update_c33(dev, attr, buf, size, EMEM_MAIN);
			else
				return firmware_update_c32(dev, attr, buf, size, EMEM_ALL);
		} else {
			return firmware_update_c2(dev, attr, buf, size);
		}
	} else if(curr_ic_type == CTP_ID_MSG21XX) {
		disable_irq(rda_ts_irq);
		_HalTscrHWReset();
		msleep(100);
		dbbusDWIICEnterSerialDebugMode();
		dbbusDWIICStopMCU();
		dbbusDWIICIICUseBus();
		dbbusDWIICIICReshape();
		msleep(300);

		/* Disable the Watchdog */
		dbbus_tx_data[0] = 0x10;
		dbbus_tx_data[1] = 0x3C;
		dbbus_tx_data[2] = 0x60;
		dbbus_tx_data[3] = 0x55;
		HalTscrCDevWriteI2CSeq(FW_ADDR_MSG21XX, dbbus_tx_data, 4);
		dbbus_tx_data[0] = 0x10;
		dbbus_tx_data[1] = 0x3C;
		dbbus_tx_data[2] = 0x61;
		dbbus_tx_data[3] = 0xAA;
		HalTscrCDevWriteI2CSeq(FW_ADDR_MSG21XX, dbbus_tx_data, 4);

		/* Stop MCU */
		dbbus_tx_data[0] = 0x10;
		dbbus_tx_data[1] = 0x0F;
		dbbus_tx_data[2] = 0xE6;
		dbbus_tx_data[3] = 0x01;
		HalTscrCDevWriteI2CSeq(FW_ADDR_MSG21XX, dbbus_tx_data, 4);

		/* set FRO to 50M */
		dbbus_tx_data[0] = 0x10;
		dbbus_tx_data[1] = 0x11;
		dbbus_tx_data[2] = 0xE2;
		HalTscrCDevWriteI2CSeq(FW_ADDR_MSG21XX, dbbus_tx_data, 3);
		dbbus_rx_data[0] = 0;
		dbbus_rx_data[1] = 0;
		HalTscrCReadI2CSeq(FW_ADDR_MSG21XX, &dbbus_rx_data[0], 2);
		TP_DEBUG("dbbus_rx_data[0]=0x%x", dbbus_rx_data[0]);
		dbbus_tx_data[3] = dbbus_rx_data[0] & 0xF7; /* Clear Bit 3 */
		HalTscrCDevWriteI2CSeq(FW_ADDR_MSG21XX, dbbus_tx_data, 4);

		/* set MCU clock,SPI clock =FRO */
		dbbus_tx_data[0] = 0x10;
		dbbus_tx_data[1] = 0x1E;
		dbbus_tx_data[2] = 0x22;
		dbbus_tx_data[3] = 0x00;
		HalTscrCDevWriteI2CSeq(FW_ADDR_MSG21XX, dbbus_tx_data, 4);

		dbbus_tx_data[0] = 0x10;
		dbbus_tx_data[1] = 0x1E;
		dbbus_tx_data[2] = 0x23;
		dbbus_tx_data[3] = 0x00;
		HalTscrCDevWriteI2CSeq(FW_ADDR_MSG21XX, dbbus_tx_data, 4);

		/* Enable slave's ISP ECO mode */
		dbbus_tx_data[0] = 0x10;
		dbbus_tx_data[1] = 0x08;
		dbbus_tx_data[2] = 0x0c;
		dbbus_tx_data[3] = 0x08;
		HalTscrCDevWriteI2CSeq(FW_ADDR_MSG21XX, dbbus_tx_data, 4);

		/* Enable SPI Pad */
		dbbus_tx_data[0] = 0x10;
		dbbus_tx_data[1] = 0x1E;
		dbbus_tx_data[2] = 0x02;
		HalTscrCDevWriteI2CSeq(FW_ADDR_MSG21XX, dbbus_tx_data, 3);
		HalTscrCReadI2CSeq(FW_ADDR_MSG21XX, &dbbus_rx_data[0], 2);
		TP_DEBUG("dbbus_rx_data[0]=0x%x", dbbus_rx_data[0]);
		dbbus_tx_data[3] = (dbbus_rx_data[0] | 0x20); /* Set Bit 5 */
		HalTscrCDevWriteI2CSeq(FW_ADDR_MSG21XX, dbbus_tx_data, 4);

		/* WP overwrite */
		dbbus_tx_data[0] = 0x10;
		dbbus_tx_data[1] = 0x1E;
		dbbus_tx_data[2] = 0x0E;
		dbbus_tx_data[3] = 0x02;
		HalTscrCDevWriteI2CSeq(FW_ADDR_MSG21XX, dbbus_tx_data, 4);

		/* set pin high */
		dbbus_tx_data[0] = 0x10;
		dbbus_tx_data[1] = 0x1E;
		dbbus_tx_data[2] = 0x10;
		dbbus_tx_data[3] = 0x08;
		HalTscrCDevWriteI2CSeq(FW_ADDR_MSG21XX, dbbus_tx_data, 4);

		dbbusDWIICIICNotUseBus();
		dbbusDWIICNotStopMCU();
		dbbusDWIICExitSerialDebugMode();

		drvISP_EntryIspMode();
		drvISP_ChipErase();
		_HalTscrHWReset();
		msleep(300);

		/* Program and Verify */
		dbbusDWIICEnterSerialDebugMode();
		dbbusDWIICStopMCU();
		dbbusDWIICIICUseBus();
		dbbusDWIICIICReshape();

		/* Disable the Watchdog */
		dbbus_tx_data[0] = 0x10;
		dbbus_tx_data[1] = 0x3C;
		dbbus_tx_data[2] = 0x60;
		dbbus_tx_data[3] = 0x55;
		HalTscrCDevWriteI2CSeq(FW_ADDR_MSG21XX, dbbus_tx_data, 4);
		dbbus_tx_data[0] = 0x10;
		dbbus_tx_data[1] = 0x3C;
		dbbus_tx_data[2] = 0x61;
		dbbus_tx_data[3] = 0xAA;
		HalTscrCDevWriteI2CSeq(FW_ADDR_MSG21XX, dbbus_tx_data, 4);

		/* Stop MCU */
		dbbus_tx_data[0] = 0x10;
		dbbus_tx_data[1] = 0x0F;
		dbbus_tx_data[2] = 0xE6;
		dbbus_tx_data[3] = 0x01;
		HalTscrCDevWriteI2CSeq(FW_ADDR_MSG21XX, dbbus_tx_data, 4);

		/* set FRO to 50M */
		dbbus_tx_data[0] = 0x10;
		dbbus_tx_data[1] = 0x11;
		dbbus_tx_data[2] = 0xE2;
		HalTscrCDevWriteI2CSeq(FW_ADDR_MSG21XX, dbbus_tx_data, 3);
		dbbus_rx_data[0] = 0;
		dbbus_rx_data[1] = 0;
		HalTscrCReadI2CSeq(FW_ADDR_MSG21XX, &dbbus_rx_data[0], 2);
		TP_DEBUG("dbbus_rx_data[0]=0x%x", dbbus_rx_data[0]);
		dbbus_tx_data[3] = dbbus_rx_data[0] & 0xF7; /* Clear Bit 3 */
		HalTscrCDevWriteI2CSeq(FW_ADDR_MSG21XX, dbbus_tx_data, 4);

		/* set MCU clock,SPI clock =FRO */
		dbbus_tx_data[0] = 0x10;
		dbbus_tx_data[1] = 0x1E;
		dbbus_tx_data[2] = 0x22;
		dbbus_tx_data[3] = 0x00;
		HalTscrCDevWriteI2CSeq(FW_ADDR_MSG21XX, dbbus_tx_data, 4);

		dbbus_tx_data[0] = 0x10;
		dbbus_tx_data[1] = 0x1E;
		dbbus_tx_data[2] = 0x23;
		dbbus_tx_data[3] = 0x00;
		HalTscrCDevWriteI2CSeq(FW_ADDR_MSG21XX, dbbus_tx_data, 4);

		/* Enable slave's ISP ECO mode */
		dbbus_tx_data[0] = 0x10;
		dbbus_tx_data[1] = 0x08;
		dbbus_tx_data[2] = 0x0c;
		dbbus_tx_data[3] = 0x08;
		HalTscrCDevWriteI2CSeq(FW_ADDR_MSG21XX, dbbus_tx_data, 4);

		/* Enable SPI Pad */
		dbbus_tx_data[0] = 0x10;
		dbbus_tx_data[1] = 0x1E;
		dbbus_tx_data[2] = 0x02;
		HalTscrCDevWriteI2CSeq(FW_ADDR_MSG21XX, dbbus_tx_data, 3);
		HalTscrCReadI2CSeq(FW_ADDR_MSG21XX, &dbbus_rx_data[0], 2);
		TP_DEBUG("dbbus_rx_data[0]=0x%x", dbbus_rx_data[0]);
		dbbus_tx_data[3] = (dbbus_rx_data[0] | 0x20); /* Set Bit 5 */
		HalTscrCDevWriteI2CSeq(FW_ADDR_MSG21XX, dbbus_tx_data, 4);

		/* WP overwrite */
		dbbus_tx_data[0] = 0x10;
		dbbus_tx_data[1] = 0x1E;
		dbbus_tx_data[2] = 0x0E;
		dbbus_tx_data[3] = 0x02;
		HalTscrCDevWriteI2CSeq(FW_ADDR_MSG21XX, dbbus_tx_data, 4);

		/* set pin high */
		dbbus_tx_data[0] = 0x10;
		dbbus_tx_data[1] = 0x1E;
		dbbus_tx_data[2] = 0x10;
		dbbus_tx_data[3] = 0x08;
		HalTscrCDevWriteI2CSeq(FW_ADDR_MSG21XX, dbbus_tx_data, 4);

		dbbusDWIICIICNotUseBus();
		dbbusDWIICNotStopMCU();
		dbbusDWIICExitSerialDebugMode();
		drvISP_EntryIspMode(); /* Start to load firmware */

		for (i = 0; i < 94; i++) { /* total  94 KB : 1 byte per R/W */
			drvISP_Program(i, handle_crc[i]); /* program to slave's flash */
			drvISP_Verify(i, handle_crc[i]); /* verify data */
		}
		TP_DEBUG("update OK\n");
		drvISP_ExitIspMode();
		_HalTscrHWReset();
		FwDataCnt = 0;
		enable_irq(rda_ts_irq);
		return size;
	}
	return 0;
}

static DEVICE_ATTR(update, CTP_AUTHORITY, firmware_update_show, firmware_update_store);

static const unsigned char MSG21XX_update_bin[] =
{
#include "MSG_2133A_ZLWY_V2.01_35004_S303_20140704.i"
};

static int fwAutoUpdate(void *unused)
{
	firmware_update_store(NULL, NULL, NULL, 0);
	return 0;
};

static u8 getchipType(void)
{
	u8 curr_ic_type = 0;
	u8 dbbus_tx_data[4];
	unsigned char dbbus_rx_data[4] = {0};
	disable_irq(rda_ts_irq);
	TP_DEBUG(" *******= %s************* \n", __func__);
	msleep(100);

	dbbusDWIICEnterSerialDebugMode();
	dbbusDWIICStopMCU();
	dbbusDWIICIICUseBus();
	dbbusDWIICIICReshape();
	msleep(100);
	/* Disable the Watchdog */
	dbbus_tx_data[0] = 0x10;
	dbbus_tx_data[1] = 0x3C;
	dbbus_tx_data[2] = 0x60;
	dbbus_tx_data[3] = 0x55;
	HalTscrCDevWriteI2CSeq(FW_ADDR_MSG21XX, dbbus_tx_data, 4);
	dbbus_tx_data[0] = 0x10;
	dbbus_tx_data[1] = 0x3C;
	dbbus_tx_data[2] = 0x61;
	dbbus_tx_data[3] = 0xAA;
	HalTscrCDevWriteI2CSeq(FW_ADDR_MSG21XX, dbbus_tx_data, 4);
	/* Stop MCU */
	dbbus_tx_data[0] = 0x10;
	dbbus_tx_data[1] = 0x0F;
	dbbus_tx_data[2] = 0xE6;
	dbbus_tx_data[3] = 0x01;
	HalTscrCDevWriteI2CSeq(FW_ADDR_MSG21XX, dbbus_tx_data, 4);

	/*
	 * Difference between C2 and C3
	 * c2:2133 c32:2133a(2) c33:2138
	 * check id
	 */
	dbbus_tx_data[0] = 0x10;
	dbbus_tx_data[1] = 0x1E;
	dbbus_tx_data[2] = 0xCC;

	HalTscrCDevWriteI2CSeq(FW_ADDR_MSG21XX, dbbus_tx_data, 3);
	HalTscrCReadI2CSeq(FW_ADDR_MSG21XX, &dbbus_rx_data[0], 2);
	TP_DEBUG(" dbbus_rx_data[0]  = %d \n", dbbus_rx_data[0]);
	TP_DEBUG(" dbbus_rx_data[1]  = %d \n", dbbus_rx_data[1]);

	if (dbbus_rx_data[0] == 2)
		curr_ic_type = CTP_ID_MSG21XXA;
	else
		curr_ic_type = CTP_ID_MSG21XX;

	TP_DEBUG("CURR_IC_TYPE = %d \n",curr_ic_type);
	dbbusDWIICIICNotUseBus();
	dbbusDWIICNotStopMCU();
	dbbusDWIICExitSerialDebugMode();
	TP_DEBUG(" Exit serialDebugMode");
	enable_irq(rda_ts_irq);

	return curr_ic_type;
}

static int getMSG21XXFWVersion(u8 curr_ic_type)
{
	unsigned char dbbus_tx_data[3];
	unsigned char dbbus_rx_data[4] ;
	int ret_w = 0,ret_f = 0;

	_HalTscrHWReset();
	TP_DEBUG(" %x   %s \n",curr_ic_type,__func__);
	disable_irq(rda_ts_irq);
	msleep(100);

	dbbus_tx_data[0] = 0x53;
	dbbus_tx_data[1] = 0x00;
	if(curr_ic_type == CTP_ID_MSG21XXA) {
		dbbus_tx_data[2] = 0x2A;
		TP_DEBUG("   =====  %s  =====",__func__);
	} else if (curr_ic_type == CTP_ID_MSG21XX) {
		dbbus_tx_data[2] = 0x74;
	} else {
		TP_DEBUG_ERR("***ic_type = %d ***\n", dbbus_tx_data[2]);
		dbbus_tx_data[2] = 0x2A;
	}
	ret_w = HalTscrCDevWriteI2CSeq(FW_ADDR_MSG21XX_TP, &dbbus_tx_data[0], 3);
	TP_DEBUG("get %s ret_w=%d \n",__func__,ret_w);
	ret_f = HalTscrCReadI2CSeq(FW_ADDR_MSG21XX_TP, &dbbus_rx_data[0], 4);

	TP_DEBUG("get %s ret_f=%d \n",__func__,ret_f);
	if (ret_f < 0 || ret_w < 0) {
		curr_ic_major = 0xffff;
		curr_ic_minor = 0xffff;
	}
	curr_ic_major = (dbbus_rx_data[1] << 8)+dbbus_rx_data[0];
	curr_ic_minor = (dbbus_rx_data[3] << 8)+dbbus_rx_data[2];

	TP_DEBUG("***FW Version major = %d ***\n", curr_ic_major);
	TP_DEBUG("***FW Version minor = %d ***\n", curr_ic_minor);
	_HalTscrHWReset();
	enable_irq(rda_ts_irq);
	return 0;
}

static ssize_t firmware_version_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	TP_DEBUG("*** firmware_version_show fw_version = %s***\n", fw_version);
	return sprintf(buf, "%s\n", fw_version);
}

static ssize_t firmware_version_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
	unsigned char dbbus_tx_data[3];
	unsigned char dbbus_rx_data[4];
	unsigned short major = 0, minor = 0;

	dbbus_tx_data[0] = 0x53;
	dbbus_tx_data[1] = 0x00;
	if (curr_ic_type == CTP_ID_MSG21XXA) {
		dbbus_tx_data[2] = 0x2A;
	} else if (curr_ic_type == CTP_ID_MSG21XX) {
		dbbus_tx_data[2] = 0x74;
	} else {
		TP_DEBUG_ERR("***ic_type = %d ***\n", dbbus_tx_data[2]);
		dbbus_tx_data[2] = 0x2A;
	}
	HalTscrCDevWriteI2CSeq(FW_ADDR_MSG21XX_TP, &dbbus_tx_data[0], 3);
	HalTscrCReadI2CSeq(FW_ADDR_MSG21XX_TP, &dbbus_rx_data[0], 4);

	major = (dbbus_rx_data[1] << 8) + dbbus_rx_data[0];
	minor = (dbbus_rx_data[3] << 8) + dbbus_rx_data[2];
	curr_ic_major = major;
	curr_ic_minor = minor;

	TP_DEBUG_ERR("***major = %d ***\n", major);
	TP_DEBUG_ERR("***minor = %d ***\n", minor);
	sprintf(fw_version,"%03d%03d", major, minor);
	TP_DEBUG("***fw_version = %s ***\n", fw_version);

	return size;
}

static DEVICE_ATTR(version, CTP_AUTHORITY, firmware_version_show, firmware_version_store);

static ssize_t firmware_data_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return FwDataCnt;
}

static ssize_t firmware_data_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
	TP_DEBUG("***FwDataCnt = %d ***\n", FwDataCnt);

	memcpy(handle_crc[FwDataCnt], buf, 1024);
	FwDataCnt++;
	return size;
}

static DEVICE_ATTR(data, CTP_AUTHORITY, firmware_data_show, firmware_data_store);

/* add your attr in here */
static struct attribute *msg2133_attributes[] = {
	&dev_attr_update.attr,
	&dev_attr_version.attr,
	&dev_attr_data.attr,
	NULL
};

static struct attribute_group msg2133_attribute_group = {
	.attrs = msg2133_attributes
};

static struct mutex g_device_mutex;

int msg2133_create_sysfs(struct i2c_client *client)
{
	int err;
	i2c_client = client;
	_HalTscrHWReset();
	err = sysfs_create_group(&client->dev.kobj, &msg2133_attribute_group);
	if (0 != err) {
		dev_err(&client->dev, "%s() - ERROR: sysfs_create_group() failed.\n",
			__func__);
		sysfs_remove_group(&client->dev.kobj, &msg2133_attribute_group);
		return -EIO;
	} else {
		mutex_init(&g_device_mutex);
		dev_info(&client->dev, "msg2133:%s() - sysfs_create_group() succeeded.\n",
			__func__);
	}
	TP_DEBUG(" *******= %s************* \n", __func__);
	firmware_class = class_create(THIS_MODULE, "ms-touchscreen-msg20xx");
	if (IS_ERR(firmware_class))
		dev_err(&client->dev, "Failed to create class(firmware)!\n");
	firmware_cmd_dev = device_create(firmware_class, NULL, 0, NULL, "device");
	if (IS_ERR(firmware_cmd_dev))
		dev_err(&client->dev, "Failed to create device(firmware_cmd_dev)!\n");

	/* version */
	if (device_create_file(firmware_cmd_dev, &dev_attr_version) < 0)
		dev_err(&client->dev, "Failed to create device file(%s)!\n",
			dev_attr_version.attr.name);
	/* update */
	if (device_create_file(firmware_cmd_dev, &dev_attr_update) < 0)
		dev_err(&client->dev, "Failed to create device file(%s)!\n",
			dev_attr_update.attr.name);
	/* data */
	if (device_create_file(firmware_cmd_dev, &dev_attr_data) < 0)
		dev_err(&client->dev, "Failed to create device file(%s)!\n",
			dev_attr_data.attr.name);

	rda_ts_irq = get_ts_irq(client);
	dev_set_drvdata(firmware_cmd_dev, NULL);
	curr_ic_type = getchipType();
	getMSG21XXFWVersion(curr_ic_type);
	printk("[TP] check auto updata ==== curr_ic_type=%d curr_ic_major=%d curr_ic_minor=%d\n",
		curr_ic_type,curr_ic_major,curr_ic_minor); /* IC中的固件版本 */

	if (curr_ic_type == CTP_ID_MSG21XXA) {
		update_bin_major = MSG21XX_update_bin[0x7f4f] << 8|MSG21XX_update_bin[0x7f4e];
		update_bin_minor = MSG21XX_update_bin[0x7f51] << 8|MSG21XX_update_bin[0x7f50];
		TP_DEBUG("bin_major = %d \n",update_bin_major); /* 升级的固件本版 */
		TP_DEBUG("bin_minor = %d \n",update_bin_minor);

		if (curr_ic_minor != 1) { /* 版本比较 */
			int i = 0;
			for (i = 0; i < 33; i++) {
				firmware_data_store(NULL, NULL, &(MSG21XX_update_bin[i*1024]), 0); /* 固件拷贝 */
				TP_DEBUG(" %s ==========\n",__func__);
			}
			firmware_update_store(NULL,NULL,NULL,0);
		}
	} else if (curr_ic_type == CTP_ID_MSG21XX) {
		update_bin_major = MSG21XX_update_bin[0x3076] << 8|MSG21XX_update_bin[0x3077];
		update_bin_minor = MSG21XX_update_bin[0x3074] << 8|MSG21XX_update_bin[0x3075];
		TP_DEBUG("bin_major = %d \n",update_bin_major);
		TP_DEBUG("bin_minor = %d \n",update_bin_minor);

		if ((update_bin_major == curr_ic_major && update_bin_minor > curr_ic_minor)
			|| (update_bin_major > curr_ic_major)
			|| (curr_ic_major == 0xffff && curr_ic_minor == 0xffff)) {
			int i = 0;
			for (i = 0; i < 94; i++) {
				firmware_data_store(NULL, NULL, &(MSG21XX_update_bin[i*1024]), 0);
				TP_DEBUG(" %s ==========\n",__func__);
			}
			kthread_run(fwAutoUpdate, 0, "MSG21XX_fw_auto_update");
			firmware_update_store(NULL,NULL,NULL,0);
		}
	}

	return err;
}

void msg2133_release_sysfs(struct i2c_client *client)
{
	sysfs_remove_group(&client->dev.kobj, &msg2133_attribute_group);
	mutex_destroy(&g_device_mutex);
}
