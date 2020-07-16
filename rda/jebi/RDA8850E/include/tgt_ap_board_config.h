#ifndef _TGT_AP_BOARD_CFG_H_
#define _TGT_AP_BOARD_CFG_H_

#define EXTRA_BOOTCOMMAND "selinux=1 androidboot.selinux=permissive"

/* MEM Size, in MBytes */
#define _TGT_AP_MEM_SIZE		512
/* GSM memory size must be 4M due to gsm modem memory map. */
#define _TGT_MODEM_GSM_MEM_SIZE         4
/* if support wcdma, mem size set 10, or if gsm only, set 0 */
#define _TGT_MODEM_WCDMA_MEM_SIZE       0
#define _TGT_MODEM_MEM_SIZE       (_TGT_MODEM_GSM_MEM_SIZE + _TGT_MODEM_WCDMA_MEM_SIZE)
#define _TGT_AP_VPU_VIDEO_SIZE          6
#define _TGT_AP_VPU_MEM_SIZE		48
#define _TGT_AP_CAM_MEM_SIZE		4
#define _TGT_AP_GFX_MEM_SIZE		0
#define _TGT_AP_OS_MEM_SIZE		450

#define _TGT_AP_GFX_MEM_BASE    (_TGT_AP_OS_MEM_SIZE) /*224*/
#define _TGT_AP_VPU_MEM_BASE    (_TGT_AP_GFX_MEM_BASE + _TGT_AP_GFX_MEM_SIZE) /*224*/
#define _TGT_MODEM_MEM_BASE     (_TGT_AP_VPU_MEM_BASE + _TGT_AP_VPU_MEM_SIZE) /*240*/
#define _TGT_AP_CAM_MEM_BASE    (_TGT_MODEM_MEM_BASE + _TGT_MODEM_MEM_SIZE) /*244*/
#define _TGT_AP_VPU_VIDEO_BASE  (_TGT_AP_CAM_MEM_BASE + _TGT_AP_CAM_MEM_SIZE) /*250*/
#if ((_TGT_AP_VPU_VIDEO_BASE + _TGT_AP_VPU_VIDEO_SIZE) != _TGT_AP_MEM_SIZE)
#error "Invalid memory size configuration"
#endif
/* usb otg function */
/* #define _TGT_AP_USB_OTG_ENABLE */

/* NAND disable HEC for MLC NAND */
#define _TGT_AP_NAND_DISABLE_HEC

/* NAND clock in MHz */
#define _TGT_AP_NAND_CLOCK		(20000000)

/* SPI NAND clock in MHz: 64000000/80000000/100000000*/
#define _TGT_AP_SPI_NAND_CLOCK		(80000000)
/* SPI NAND read delay from flash to controller */
/* need change according different HW board */
#define _TGT_AP_SPI_NAND_READDELAY (4)

/* ADMMC clock */
#define _TGT_AP_SDMMC1_MAX_FREQ		(30000000)
#define _TGT_AP_SDMMC1_MCLK_INV		(1)
#define _TGT_AP_SDMMC1_MCLK_ADJ		(1)
#define _TGT_AP_SDMMC2_MAX_FREQ		(20000000)
#define _TGT_AP_SDMMC2_MCLK_INV		(1)
#define _TGT_AP_SDMMC2_MCLK_ADJ		(3)
#define _TGT_AP_SDMMC3_MAX_FREQ		(50000000)
#define _TGT_AP_SDMMC3_HIGH_SPEED       (1)
/*#define _TGT_AP_SDMMC3_UHS_DDR50        (1)*/
/* You can set different manufacture  emmc different
mclk adj and clokc inv at the same time. If you don't set,
 the driver will use default value(mclk_adj = 0; clk_inv = 1) */
/* #define _TGT_AP_EMMC_MCLK_ADJ_TOSHIBA    0 */
/* #define _TGT_AP_EMMC_CLK_INV_TOSHIBA     0 */
/* #define _TGT_AP_EMMC_MCLK_ADJ_GIGADEVICE 0 */
/* #define _TGT_AP_EMMC_CLK_INV_GIGADEVICE  1 */
/* #define _TGT_AP_EMMC_MCLK_ADJ_SAMSUNG    0 */
/* #define _TGT_AP_EMMC_CLK_INV_SAMSUNG     0 */
/* #define _TGT_AP_EMMC_MCLK_ADJ_SANDISK    0 */
/* #define _TGT_AP_EMMC_CLK_INV_SANDISK     0 */
/* #define _TGT_AP_EMMC_MCLK_ADJ_HYNIX      0 */
/* #define _TGT_AP_EMMC_CLK_INV_HYNIX       0 */
/* #define _TGT_AP_EMMC_MCLK_ADJ_MICRON     0 */
/* #define _TGT_AP_EMMC_CLK_INV_MICRON      0 */
/* #define _TGT_AP_EMMC_MCLK_ADJ_MICRON1    0 */
/* #define _TGT_AP_EMMC_CLK_INV_MICRON1     0 */


/* I2C clocks */
#define _TGT_AP_I2C0_CLOCK		(100000)
#define _TGT_AP_I2C1_CLOCK		(400000)
#define _TGT_AP_I2C2_CLOCK		(200000)

/*
 * I2C addresses (Only keep non-RDA peripherals )
 */
#define _TGT_AP_I2C_BUS_ID_TS		(2)
#  define _DEF_I2C_ADDR_TS_GSL168X	(0x40)
#  define _DEF_I2C_ADDR_TS_MSG2133	(0x26)
#  define _DEF_I2C_ADDR_TS_FT6x06	(0x38)
#  define _DEF_I2C_ADDR_TS_IT7252       (0x46)
#  define _DEF_I2C_ADDR_TS_GTP868       (0x5d)
#  define _DEF_I2C_ADDR_TS_ICN831X      (0x40)
/* NO need to define ADDR, this is determined by customer driver */
/* #define _TGT_AP_I2C_ADDR_TS */

#define _TGT_AP_I2C_BUS_ID_GSENSOR	(2)
#  define _DEF_I2C_ADDR_GSENSOR_MMA7760	(0x4c)
#  define _DEF_I2C_ADDR_GSENSOR_STK8312	(0x3d)
#  define _DEF_I2C_ADDR_GSENSOR_MMA865X	(0x1D)
/* Notice : when SA0=1, the MMA8452 slave address is 0x1D, and when SA0 = 0,
    the address is 0x1C */
#  define _DEF_I2C_ADDR_GSENSOR_MMA845X (0x1D)
#  define _DEF_I2C_ADDR_GSENSOR_MXC622X (0x15)

/* gsensor position to lcd */
#define _TGT_AP_GSENSOR_POSITION (5)

/* NO need to define ADDR, this is determined by customer driver */
/* #define _TGT_AP_I2C_ADDR_GSENSOR */

#define _TGT_AP_I2C_BUS_ID_LSENSOR	(2)
#  define _DEF_I2C_ADDR_LSENSOR_STK3x1x	(0x48)
/* NO need to define ADDR, this is determined by customer driver */
/* #define _TGT_AP_I2C_ADDR_LSENSOR */

#define _TGT_AP_I2C_BUS_ID_WIFI		(0)
/* No Address defination, as rda5990 address is not changable */

#define _TGT_AP_I2C_BUS_ID_ATV		(0)
/* No Address defination, as rda5888 address is not changable */

#define _TGT_AP_I2C_BUS_ID_CAM		(0)

/* External pwml to control lcd backlight defination */
/* KRT R118 use a LDO to driver BL, GPO_2 to enable, no real PWM used */
#define _TGT_AP_BL_EXT_PWM 1
/*#define _TGT_AP_BL_EXT_PWM_INVERT 0*/
#define _TGT_AP_EXT_PWM_CLOCK		(100000)
/*#define _TGT_AP_BL_EXT_GPIO_ENABLE*/
/* LED defination */
/* #define _TGT_AP_LED_RED 		(1) */
/* # define _TGT_AP_LED_RED_KB		(1) */
/* # define _TGT_AP_LED_RED_FLASH	(1) */
/*#define _TGT_AP_LED_GREEN		(1)*/
/* # define _TGT_AP_LED_GREEN_KB 	(1) */
/*# define _TGT_AP_LED_GREEN_FLASH	(1)*/
/*#define _TGT_AP_LED_BLUE		(1)*/
/*# define _TGT_AP_LED_BLUE_KB		(1)*/
/* # define _TGT_AP_LED_BLUE_FLASH	(1) */

/*#define _TGT_AP_BOARD_HAS_ATV*/

#define _TGT_AP_VIBRATOR_POWER_ON	(1)
# define _TGT_AP_VIBRATOR_TIME		(300)

#endif /*_TGT_AP_BOARD_CFG_H_*/

