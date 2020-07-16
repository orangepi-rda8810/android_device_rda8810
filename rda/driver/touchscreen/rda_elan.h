#ifndef _LINUX_ELAN_TS_H
#define _LINUX_ELAN_TS_H

/****************************customer info****************************/
/* definition of EKT2527 touch screen */
#define ELAN_RES_X   779
#define ELAN_RES_Y   1280


/****************************elan data info****************************/

#define ELAN_7BITS_ADDR 0x15

//sleep  mod
#define PWR_STATE_DEEP_SLEEP    0
#define PWR_STATE_NORMAL        1
#define PWR_STATE_MASK      BIT(3)

//cmd or paket head
#define CMD_S_PKT           0x52
#define CMD_R_PKT           0x53
#define CMD_W_PKT           0x54
#define HELLO_PKT           0x55
#define RAM_PKT             0xcc
#define BUFF_PKT            0x63

//elan IC series(only choose one)
#define ELAN_2K_XX
//#define ELAN_3K_XX
//#define ELAN_RAM_XX

/**********************fingers number macro switch**********************/
#define TWO_FINGERS
//#define FIVE_FINGERS
//#define TEN_FINGERS

#define RECV_MORE_THAN_EIGHT_BYTES

#ifdef TWO_FINGERS
    #define FINGERS_PKT             0x5A
    #define PACKET_SIZE             8
    #define FINGERS_NUM             2
#endif

#ifdef FIVE_FINGERS
    #define FINGERS_PKT             0x5D
    #define PACKET_SIZE             18
    #define FINGERS_NUM             5
#endif

#ifdef TEN_FINGERS
    #define FINGERS_PKT             0x62
#ifdef ELAN_BUFFER_MODE
    #define PACKET_SIZE             55
#else
    #define PACKET_SIZE             35
#endif
    #define FINGERS_NUM             10
#endif


/************************* button macro *********************/
#define ONE_LAYER
#ifndef ONE_LAYER
#define ELAN_KEY_BACK 0x81
#define ELAN_KEY_HOME 0x41
#define ELAN_KEY_MENU 0x21
#else
#define ELAN_KEY_BACK 0x04
#define ELAN_KEY_HOME 0x08
#define ELAN_KEY_MENU 0x10
#endif


#define VIR_KEY_BACK_X          100 
#define VIR_KEY_BACK_Y          964
#define VIR_KEY_BACK_DX         60
#define VIR_KEY_BACK_DY         30

#define VIR_KEY_HOMEPAGE_X      200
#define VIR_KEY_HOMEPAGE_Y      964
#define VIR_KEY_HOMEPAGE_DX     60
#define VIR_KEY_HOMEPAGE_DY     30

#define VIR_KEY_MENU_X          300
#define VIR_KEY_MENU_Y          964
#define VIR_KEY_MENU_DX         60
#define VIR_KEY_MENU_DY         30

#endif /* _LINUX_ELAN_TS_H */
