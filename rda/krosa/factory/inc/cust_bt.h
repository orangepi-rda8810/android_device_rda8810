
#ifndef FTM_CUST_BT_H
#define FTM_CUST_BT_H

#include <cutils/log.h>
#include "ftm.h"
#define RDA5990_SUPPORT
#define RDA_BT_IOCTL_MAGIC              'u'

#define RDA_BT_POWER_ON_IOCTL           _IO(RDA_BT_IOCTL_MAGIC ,0x01)
#define RDA_BT_RF_INIT_IOCTL            _IO(RDA_BT_IOCTL_MAGIC ,0x02)
#define RDA_BT_DC_CAL_IOCTL             _IO(RDA_BT_IOCTL_MAGIC ,0x03)
#define RDA_BT_RF_SWITCH_IOCTL          _IO(RDA_BT_IOCTL_MAGIC ,0x04)
#define RDA_BT_POWER_OFF_IOCTL          _IO(RDA_BT_IOCTL_MAGIC ,0x05)
#define RDA_BT_EN_CLK                   _IO(RDA_BT_IOCTL_MAGIC ,0x06)
#define RDA_BT_DC_DIG_RESET_IOCTL       _IO(RDA_BT_IOCTL_MAGIC ,0x07)
#define RDA_BT_GET_ADDRESS_IOCTL	    _IO(RDA_BT_IOCTL_MAGIC ,0x08)
#define RDA_WLAN_COMBO_VERSION              _IO(RDA_BT_IOCTL_MAGIC ,0x15)
#define RDA_BT_DC_CAL_IOCTL_FIX_5991_LNA_GAIN           _IO(RDA_BT_IOCTL_MAGIC ,0x26)

#define RDA_UART_IOCTL_MAGIC 'u'
#define RDA_UART_ENABLE_RX_BREAK_IOCTL          _IO(RDA_BT_IOCTL_MAGIC ,0x01)

#define RDABT_DRV_NAME                  "/dev/rdacombo"

/* SERIAL PORT */
#define CUST_BT_SERIAL_PORT             "/dev/ttyS1"

#define CUST_BT_BAUD_RATE               921600 /* use 4M but is not controlled by bt directly */

#define WLAN_VERSION_90_D (1)
#define WLAN_VERSION_90_E (2)
#define WLAN_VERSION_91   (3)
#define WLAN_VERSION_91_E (4)
#define WLAN_VERSION_91_F (5)
#define WLAN_VERSION_91_G (6)

#define HCI_COMMAND_PKT		            0x01
#define HCI_ACLDATA_PKT		            0x02
#define HCI_SCODATA_PKT		            0x03
#define HCI_EVENT_PKT		            0x04
#define HCI_VENDOR_PKT		            0xFF
#define HCI_EV_INQUIRY_RESULT		    0x02
#define HCI_EV_INQUIRY_RESULT_WITH_RSSI 0x22
#define HCI_EV_INQUIRY_COMPLETE         0x01


#define BT_FM_DEBUG                     1

#if BT_FM_DEBUG
#define ERR(f, ...)                     ALOGE("%s: " f, __func__, ##__VA_ARGS__)
#define WAN(f, ...)                     ALOGW("%s: " f, __func__, ##__VA_ARGS__)
#define DBG(f, ...)                     ALOGD("%s: " f, __func__, ##__VA_ARGS__)
#define TRC(f)                          ALOGW("%s #%d", __func__, __LINE__)
#else
#define DBG(...)                        ((void)0)
#define TRC(f)                          ((void)0)
#endif

#ifndef BT_DRV_MOD_NAME
#define BT_DRV_MOD_NAME                 "bluetooth"
#endif


typedef unsigned long  DWORD;
typedef unsigned long *PDWORD;
typedef unsigned long *LPDWORD;
typedef unsigned short USHORT;
typedef unsigned char  UCHAR;
typedef unsigned char  BYTE;
typedef unsigned long  HANDLE;
typedef unsigned char  BOOL;
typedef unsigned char  BOOLEAN;
typedef void           VOID;
typedef void          *LPCVOID;
typedef void          *LPVOID;
typedef void          *LPOVERLAPPED;
typedef unsigned char *PUCHAR;
typedef unsigned char *PBYTE;
typedef unsigned char *LPBYTE;

#define TRUE                            1
#define FALSE                           0


typedef struct 
{
	unsigned char    event;
	unsigned short	 handle;
	unsigned char    len;
	unsigned char    status;
	unsigned char    parms[256];
} BT_HCI_EVENT;

typedef struct 
{
	unsigned short   opcode;
	unsigned char    len;
	unsigned char    cmd[256];
} BT_HCI_CMD;

typedef enum 
{
    ITEM_PASS,
    ITEM_FAIL,
};

struct bt_factory 
{
    char  info[1024];
    bool  exit_thd;
    int   result;
    
    /* for UI display */
    text_t    title;
    text_t    text;
    
    pthread_t update_thd;
    
    struct ftm_module *mod;
    struct textview tv;
    struct itemview *iv;
};


#define mod_to_bf(p) (struct bt_factory*)((char*)(p) + sizeof(struct ftm_module))

#endif /* FTM_CUST_BT_H */

