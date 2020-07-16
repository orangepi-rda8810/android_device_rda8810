typedef unsigned int u32;
typedef int s32;
typedef unsigned short u16;
typedef short s16;
typedef unsigned char u8;
typedef char s8;


#define RDA_FIRMWARE_TYPE_SIZE 16		//firmware type size
#define RDA_FIRMWARE_DATA_NAME_SIZE 50	//firmware data_type size
#define RDA_FIRMWARE_VERSION 3

#pragma pack (push)
#pragma pack(1)
struct rda_device_firmware_head {
	char firmware_type[RDA_FIRMWARE_TYPE_SIZE];
	u32 version;
	u32 data_num;
};
struct rda_firmware_data_type {
	char data_name[RDA_FIRMWARE_DATA_NAME_SIZE];
	u16 crc;
	s8 chip_version;
	u32 size;
};
#pragma pack (pop)

#define WLAN_VERSION_90_D (1)
#define WLAN_VERSION_90_E (2)
#define WLAN_VERSION_91   (3)
#define WLAN_VERSION_91_E (4)
#define WLAN_VERSION_91_F (5)
#define WLAN_VERSION_91_G (6)
