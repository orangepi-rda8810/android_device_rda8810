#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <Windows.h>
#include <WinBase.h>
#include "rda_firmware.h"
#define ADD_PATH(FILE) ( (sprintf(path_file, "./%s/%s", str_path, FILE)) ? (path_file) : ("error") )//complete file path automatically

//#define debug_file //output useful input_data to "debug.log"

/*macro*/
#define Input_file_size 10//The largest number of input data file
#define Name_bufsize 80//The maximum length of the file and variables name
#define Line_size 400//The maximum length of each line
#define Data_size 10000//The maximum length of each data(counted by byte)


//fields which will be written to "bin_file"
rda_device_firmware_head rda_wland_firmware_head;//firmware information
rda_firmware_data_type rda_wland_firmeware_data_type;//data_type infomation
u8 rda_wland_firmware_data[Data_size];//store data temporarily
u16 crc_ret = 0;

struct file_node{
	char file_type[Name_bufsize];
	char cfg_file[Name_bufsize];
	char out_file[Name_bufsize];
};

file_node file_support[] = {//every 'bin_file" supported by the program
	{
		"rda_wland",
		"rda_wland_cfg.ini",
		"rda_wland.bin",
	},
	{
		"rda_combo",
		"rda_combo_cfg.ini",
		"rda_combo.bin",
	},
};

/***********crc resource*************/
u16 const crc16_table[256] = {
	0x0000, 0xC0C1, 0xC181, 0x0140, 0xC301, 0x03C0, 0x0280, 0xC241,
	0xC601, 0x06C0, 0x0780, 0xC741, 0x0500, 0xC5C1, 0xC481, 0x0440,
	0xCC01, 0x0CC0, 0x0D80, 0xCD41, 0x0F00, 0xCFC1, 0xCE81, 0x0E40,
	0x0A00, 0xCAC1, 0xCB81, 0x0B40, 0xC901, 0x09C0, 0x0880, 0xC841,
	0xD801, 0x18C0, 0x1980, 0xD941, 0x1B00, 0xDBC1, 0xDA81, 0x1A40,
	0x1E00, 0xDEC1, 0xDF81, 0x1F40, 0xDD01, 0x1DC0, 0x1C80, 0xDC41,
	0x1400, 0xD4C1, 0xD581, 0x1540, 0xD701, 0x17C0, 0x1680, 0xD641,
	0xD201, 0x12C0, 0x1380, 0xD341, 0x1100, 0xD1C1, 0xD081, 0x1040,
	0xF001, 0x30C0, 0x3180, 0xF141, 0x3300, 0xF3C1, 0xF281, 0x3240,
	0x3600, 0xF6C1, 0xF781, 0x3740, 0xF501, 0x35C0, 0x3480, 0xF441,
	0x3C00, 0xFCC1, 0xFD81, 0x3D40, 0xFF01, 0x3FC0, 0x3E80, 0xFE41,
	0xFA01, 0x3AC0, 0x3B80, 0xFB41, 0x3900, 0xF9C1, 0xF881, 0x3840,
	0x2800, 0xE8C1, 0xE981, 0x2940, 0xEB01, 0x2BC0, 0x2A80, 0xEA41,
	0xEE01, 0x2EC0, 0x2F80, 0xEF41, 0x2D00, 0xEDC1, 0xEC81, 0x2C40,
	0xE401, 0x24C0, 0x2580, 0xE541, 0x2700, 0xE7C1, 0xE681, 0x2640,
	0x2200, 0xE2C1, 0xE381, 0x2340, 0xE101, 0x21C0, 0x2080, 0xE041,
	0xA001, 0x60C0, 0x6180, 0xA141, 0x6300, 0xA3C1, 0xA281, 0x6240,
	0x6600, 0xA6C1, 0xA781, 0x6740, 0xA501, 0x65C0, 0x6480, 0xA441,
	0x6C00, 0xACC1, 0xAD81, 0x6D40, 0xAF01, 0x6FC0, 0x6E80, 0xAE41,
	0xAA01, 0x6AC0, 0x6B80, 0xAB41, 0x6900, 0xA9C1, 0xA881, 0x6840,
	0x7800, 0xB8C1, 0xB981, 0x7940, 0xBB01, 0x7BC0, 0x7A80, 0xBA41,
	0xBE01, 0x7EC0, 0x7F80, 0xBF41, 0x7D00, 0xBDC1, 0xBC81, 0x7C40,
	0xB401, 0x74C0, 0x7580, 0xB541, 0x7700, 0xB7C1, 0xB681, 0x7640,
	0x7200, 0xB2C1, 0xB381, 0x7340, 0xB101, 0x71C0, 0x7080, 0xB041,
	0x5000, 0x90C1, 0x9181, 0x5140, 0x9301, 0x53C0, 0x5280, 0x9241,
	0x9601, 0x56C0, 0x5780, 0x9741, 0x5500, 0x95C1, 0x9481, 0x5440,
	0x9C01, 0x5CC0, 0x5D80, 0x9D41, 0x5F00, 0x9FC1, 0x9E81, 0x5E40,
	0x5A00, 0x9AC1, 0x9B81, 0x5B40, 0x9901, 0x59C0, 0x5880, 0x9841,
	0x8801, 0x48C0, 0x4980, 0x8941, 0x4B00, 0x8BC1, 0x8A81, 0x4A40,
	0x4E00, 0x8EC1, 0x8F81, 0x4F40, 0x8D01, 0x4DC0, 0x4C80, 0x8C41,
	0x4400, 0x84C1, 0x8581, 0x4540, 0x8701, 0x47C0, 0x4680, 0x8641,
	0x8201, 0x42C0, 0x4380, 0x8341, 0x4100, 0x81C1, 0x8081, 0x4040
};
static inline u16 crc16_byte(u16 crc, const u8 data)
{
	return (crc >> 8) ^ crc16_table[(crc ^ data) & 0xff];
}

static u16 crc16(u16 crc, u8 const *buffer, size_t len)
{
	while (len--)
		crc = crc16_byte(crc, *buffer++);
	return crc;
}
/******************************/



/***********macro define************/
struct rda_macro {
	char name[Name_bufsize];
	char value[Name_bufsize];
	int num;//add macro twice
	struct rda_macro *next;
};
struct rda_macro *Macro_home = NULL;

struct rda_macro* check_rda_macro(char *name)
{
	struct rda_macro * i;
	for (i = Macro_home; i != NULL; i = i->next) {
		if (strcmp(name, i->name) == 0)
			return i;
	}
	return NULL;
}
//return 0 if we can add the value,return -1 if [name] if defined.
int add_macro_value(char *name, char *value)
{
	struct rda_macro *ans = check_rda_macro(name);
	if ( ans == NULL){
		struct rda_macro * i = new rda_macro;
		strcpy(i->name, name);
		strcpy(i->value, value);
		i->num = 1;
		i->next = Macro_home;
		Macro_home = i;
		return 0;
	}
	else if (ans->num == 1){
		ans->num = 2;
		return 0;
	}
	else
		return -1;

}
//return 0 if we can find the value,return -1, else.
int find_macro_value(char *name, char *value)
{
	struct rda_macro * temp = check_rda_macro(name);
	if (temp == NULL)
		return 0;
	else{
		strcpy(value, temp->value);
		return 1;
	}
}
/******************************/


/*
 *Storage of data_name and data_type information
 *which is used to make sure wheth "data_name" is repetitive or not
 *does't write to "bin_file"
 */
struct Node {
	char name[40];
	int data_type;
	u8 *data;//temporary data pointer
	int size;//data length, which is used by "struct rda_wland_firmeware_data_type"
	struct Node * nest = NULL;
};
Node * Node_home = NULL;
struct Node * temp_Node = NULL;
/****************************/


/****************************/
char path_file[100];
char str_path[100];//store "rda_wland/"，which is used to complete file path
int data_num = 0;//the number of data_name
/****************************/




/****************************/
/*status of "#if #else"*/
int data_status[10] = { 1 };//1:valid data, 0:data is ignored by macro
int if_flag = 0;//0:there is no "#if", 1:this is in "#if", 2:this is in "#if #if",,,,,,,
int else_flag[10] = { 0 };//1:the "else data" can be valid, 0:the "#else data" must be ignored because the "#if" had been valid
/****************************/


char cfg_name[Name_bufsize];//configure file name
char output_file_name[Name_bufsize];//output file name
char file_type[Name_bufsize];//file type

/****************************/
char input_name[Line_size];
char data_flag[Line_size];
int len_input_name = sizeof(input_name);
char output_name[Name_bufsize];
/****************************/



int strcmp_string(char *str, char * name)
{
	char temp[Line_size];
	int i = 0;
	if (strlen(str) < strlen(name))
		return -1;
	for (i = 0; i < strlen(name); ++i)
		temp[i] = str[i];
	temp[i] = '\0';
	return strcmp(temp, name);
}

int first_word(char *des, char *sor)
{
	int i = 0;
	while (sor[i] != '\0' && sor[i] != ' ') {
		des[i] = sor[i];
		i++;
	}
	des[i] = '\0';
	return 0;
}

int second_word(char *des, char *sor)
{
	int i = 0, j = 0;
	while (sor[i] != '\0' && sor[i] != ' ') {
		i++;
	}
	if (sor[i] == '\0') {
		printf("second_word error:only one word!\n");
		return -1;
	}
	while (sor[i] != '\0' && sor[i] == ' ') {
		i++;
	}
	if (sor[i] == '\0'){
		printf("second_word error:only one word!\n");
		return -1;
	}
	while (sor[i] != '\0' && sor[i] != ' ') {
		des[j++] = sor[i++];
	}
	des[j] = '\0';
	return 0;
}
int del_string_char(char *str, int begin, int size)
{
	int i;
	if (begin + size > strlen(str))
		return -1;
	for (i = begin; i < strlen(str) - size; i++)
		str[i] = str[i + size];

	str[i] = '\0';
	return 0;
}
//return 1:this line is macro; return 2:this line is ignored by macro; return 0: this line is valid data; return -1:error
int del_macro(char *str_line, char *file_name, int line)
{
	char * str, *str1, *str2;
	char define_str[Line_size];//store data temporarily

	if (str_line[0] != '#')
		if (data_status[if_flag] == 0)
			return 2;
		else
			return 0;

	if (strcmp_string(str_line, "#define") == 0) {
		if (data_status[if_flag] == 0)
			return 2;

		str = strtok(str_line, " ");
		str1 = strtok(NULL, " ");
		str2 = strtok(NULL, " ");
		if (str2 != NULL) {//#define name value
			if (add_macro_value(str1, str2) == -1){
				printf("error:%s - %d macro error!\n", file_name, line);
				return -1;
			}

		}
		else {//#define name value
			if (add_macro_value(str1, "true") == -1){
				printf("error:%s - %d macro error!\n", file_name, line);
				return -1;
			}
		}
		return 1;
	}

	if (strcmp_string(str_line, "#endif") == 0) {
		if (if_flag != -1) {
			data_status[if_flag] = 1;
			if_flag -= 1;

		}
		else {
			printf("error:%s-%d \"#endif\" error\n", file_name, line);
			return -1;
		}
		return 1;
	}
	if (strcmp_string(str_line, "#if 1") == 0) {
		if (data_status[if_flag]) {//the status now is valid, program can goto nest "#if" normaly
			if_flag += 1;
			data_status[if_flag] = 1;
			else_flag[if_flag] = 0;
			return 1;
		}
		else {//the status now is ignored, nest "#if" is invalid
			if_flag++;
			data_status[if_flag] = 0;
			else_flag[if_flag] = 0;
			return 1;
		}


	}
	if (strcmp_string(str_line, "#if 0") == 0) {
		if (strcmp_string(str_line, "#if 0") == 0) {
			if (data_status[if_flag]){//the status now is valid, program can goto nest "#if" normaly
				if_flag += 1;
				data_status[if_flag] = 0;
				else_flag[if_flag] = 1;
				return 1;
			}
			else {//the status now is ignored, nest "#if" is invalid
				if_flag++;
				data_status[if_flag] = 0;
				else_flag[if_flag] = 0;
				return 1;
			}

		}

	}

	if (strcmp_string(str_line, "#ifdef") == 0) {
		if_flag += 1;

		str = strtok(str_line, " ");
		str1 = strtok(NULL, " ");
		if (str1 == NULL) {
			printf("error:%s-%d \"#ifdef\" error!\n", file_name, line);
			return -1;
		}
		str2 = strtok(NULL, " ");
		if (str2 != NULL) {
			printf("warning:%s-%d too many parameters!(ignored)\n", file_name, line);
		}
		if (find_macro_value(str1, define_str) == 0)//cound't find specified macro_define, #ifdef is invalid
		{
			if (data_status[if_flag - 1]){//the status now is valid, program can goto nest "#if" normaly
				data_status[if_flag] = 0;
				else_flag[if_flag] = 1;
			}
			else {//the status now is ignored, nest "#if" is invalid
				data_status[if_flag] = 0;
				else_flag[if_flag] = 0;
			}

		}
		else {//#ifdef is valid
			if (data_status[if_flag - 1]){//the status now is valid, program can goto nest "#if" normaly
				data_status[if_flag] = 1;
				else_flag[if_flag] = 0;
			}
			else {//the status now is ignored, nest "#if" is invalid
				data_status[if_flag] = 0;
				else_flag[if_flag] = 0;
			}
		}
		return 1;
	}

	if (strcmp_string(str_line, "#else") == 0) {

		if (else_flag[if_flag] == 0 || data_status[if_flag] == 1){
			data_status[if_flag] = 0;//the "#else data" must be ignored because the "#if" had been valid
		}
		else{
			data_status[if_flag] = 1;
			else_flag[if_flag] = 0;
		}
		return 1;
	}

	if (strcmp_string(str_line, "#elif") == 0) {
		str = strtok(str_line, " ");
		str1 = strtok(NULL, " ");
		if (str1 == NULL) {
			printf("error:%s-%d \"#elif\" error!\n", file_name, line);
			return -1;
		}
		if (strcmp(str1, "defined") == 0)
			str1 = strtok(NULL, "");
		str2 = strtok(NULL, " ");
		if (str2 != NULL) {
			printf("warning:%s-%d too many parameters!(ignored)\n", file_name, line);
		}

		if (else_flag[if_flag] == 0 || data_status[if_flag] == 1){
			data_status[if_flag] = 0;//the "#else data" must be ignored because the "#if" had been valid
		}
		else {
			if (find_macro_value(str1, define_str) == 0)//cound't find specified macro_define, #elif is invalid
				return 1;
			else{
				data_status[if_flag] = 1;
				else_flag[if_flag] = 0;
			}
		}
		return 1;
	}
	printf("error:%s-%d undefined macro!\n", file_name, line);
	return -1;

}
int is_data(char c)
{
	if ((c >= '0'&&c <= '9') || (c >= 'a'&& c <= 'z') || (c >= 'A'&& c <= 'Z'))
		return 1;
	else
		return 0;
}
int is_control_world(char c)
{
	if (c == '{' || c == '}' || c == ',' || c == ' ')
		return 1;
	else
		return 0;
}

/*delete comment marked by '//'
 *if this line is empty return -1
 */
int del_comment(char *str)
{
	int i = 0;
	if (str == NULL)
		return -1;

	int len = strlen(str);
	for (i = 0; i < len; ++i)
		if (str[i] == '\n' || str[i] == '\t')
			str[i] = ' ';
	//del space
	i = 0;
	while (str[i] == ' ')
		i++;
	int j;
	for (j = 0; i < len; ++i) {
		str[j] = str[i];
		j++;
	}
	str[j] = '\0';

	//del comment
	len = strlen(str);
	for (i = 0; i < len - 1; ++i) {
		if (str[i] == '/' && str[i + 1] == '/') {
			str[i] = 0;
			break;
		}
	}

	//del continuous space
	for (i = 0; i < strlen(str); ++i) {
		if (str[i] == ' ') {
			int j = i + 1;
			while (str[j] == ' ' && j < strlen(str))
				j++;
			if (i + 1 != j)
				if (del_string_char(str, i + 1, j - i - 1) < 0){
					printf("error: del_string_char error\n");
					return -2;
				}

		}
	}

	i = strlen(str) - 1;
	while (i >= 0 && str[i] == ' ')
		i--;
	if (i == -1)
		return -1;
	else
		str[i + 1] = '\0';
	return 0;
}
int check_data_flag(char *str)
{
	int i = 0;
	if (str == NULL)
		return -1;
	while (i < strlen(str)) {
		if (str[i]<'0' || str[i]>'9')
			return -2;
		i++;
	}
	return 0;
}
int chack_data_name(char *str)
{
	struct Node *i;
	for (i = Node_home; i != NULL; i = i->nest) {
		if (strcmp(str, i->name) == 0)
			return -1;
	}
	return 0;
}
int add_node(struct Node * node)
{
	node->nest = Node_home;
	Node_home = node;
	return 0;
}
int check_open_bace(char *str)
{
	int i = 1;
	while (i < strlen(str))
		if (str[i++] != ' ')
			return -1;
	return 0;
}
int check_close_bace(char *str)
{
	int i = 1;
	if (str[1] == ';')
		i = 2;
	while (i < strlen(str))
		if (str[i++] != ' ')
			return -1;
	return 0;
}


void replace_bace(char *str)
{
	int i;
	for (i = 0; i < strlen(str); ++i)
		if (is_control_world(str[i]))
			str[i] = ' ';
}
void replace_plus(char *str)
{
	int i;
	for (i = 0; i < strlen(str); ++i)
		if (str[i] == '+') {
			int j = strlen(str) + 2;
			for (j; j > i + 2; --j)
				str[j] = str[j - 2];
			str[i] = ' ';
			str[i + 1] = '+';
			str[i + 2] = ' ';
		}
}
int check_bace(char *str)
{
	int i = 0;
	while (i < strlen(str) && str[i] != '{')
		i++;
	while (i < strlen(str) && str[i] != ',')
		i++;
	while (i < strlen(str) && str[i] != '}')
		i++;
	if (i == strlen(str))
		return -1;
	return 0;
}
int check_data_string(char *str)
{
	return 0;
}

int check_plus(char *str)
{
	int i = 0;
	for (i = 0; i < strlen(str); ++i)
		if (str[i] == '+')
			return 1;
	return 0;
}
int handle_plus(char *str, unsigned * data1, unsigned *data2)
{
	char temp_str[Line_size];
	char macro_str[Line_size];
	strcpy(temp_str, str);
	char *s;
	*str = '\0';
	int flag_data1 = 0;
	int flag_data2 = 0;
	char *s1 = NULL, *s2 = NULL, *s3 = NULL;
	for (s = strtok(temp_str, " "); s != NULL; s = strtok(NULL, " ")) {
		if (strcmp(s, "+") == 0){
			s2 = strtok(NULL, " ");
			if (s1 == NULL || s2 == NULL)
				return -1;
			if (!flag_data1) {
				*data1 = (unsigned int)strtoul(s1, NULL, 0) + (unsigned int)strtoul(s2, NULL, 0);
				flag_data1 = 1;
			}
			else if (!flag_data2) {
				*data2 = (unsigned int)strtoul(s1, NULL, 0) + (unsigned int)strtoul(s2, NULL, 0);
				flag_data2 = 1;
			}
			else
				return -1;
		}
		s1 = s;
	}
	if (!flag_data1)
		return -1;
	if (!flag_data2)
		if (s1 != NULL)
			*data2 = (unsigned int)strtoul(s1, NULL, 0);
		else
			return -1;

}
//find mcaro and replace is by it's real message
int check_macro(char *str)
{
	char temp_str[Line_size];
	char macro_str[Line_size];
	strcpy(temp_str, str);
	char *s;
	*str = '\0';
	int ret = 0;
	for (s = strtok(temp_str, " "); s != NULL; s = strtok(NULL, " ")) {
		if (find_macro_value(s, macro_str) == 1){//this is macro define
			if (strcmp(macro_str, "true")) {//this macro define has actual mean
				strcat(str, macro_str);
				strcat(str, " ");
				ret = 1;
				continue;
			}
		}
		strcat(str, s);
		strcat(str, " ");
	}
	return ret;
}

int get_two_data(char **str1, char **str2, char *str)
{
	int i = 0;
	while (i < strlen(str)){
		if (!(is_data(str[i]) || is_control_world(str[i])))
			return -1;
		else if (is_control_world(str[i]))
			str[i] = ' ';
		i++;
	}
	*str1 = strtok(str, " ");
	if (*str1 == NULL)
		return -2;
	*str2 = strtok(NULL, " ");
	if (*str2 == NULL)
		return -2;
	if (strtok(NULL, " ") != NULL)
		return -3;


	if (check_data_string(*str1) < 0)
		return -4;
	if (check_data_string(*str2) < 0)
		return -5;

	return 0;
}

int check_data_name(char *str, int *flag)
{
	char *c8[] = { "static const u8", "static const unsigned char", "const u8", "const unsigned char", "u8", "unsigned char" };
	char *c16[] = { "static const u16", "static const unsigned short", "const u16", "const unsigned short", "u16", "unsigned short" };
	char *c32[] = { "static const u32", "static const unsigned int", "static const unsigned", "const u32", "const unsigned int", "const unsigned", "u32", "unsigned int", "unsigned" };
	int i;
	for (i = 0; i < sizeof(c8) / 4; ++i) {
		if (!strcmp_string(str, c8[i])) {
			*flag = 8;
			return strlen(c8[i]);
		}
	}
	for (i = 0; i < sizeof(c16) / 4; ++i) {
		if (!strcmp_string(str, c16[i])) {
			*flag = 16;
			return strlen(c16[i]);
		}
	}
	for (i = 0; i < sizeof(c32) / 4; ++i) {
		if (!strcmp_string(str, c32[i])) {
			*flag = 32;
			return strlen(c32[i]);
		}
	}
	return -1;
}

s32 count_chip_version(char *data_name)
{
	int ret;
	char temp[Name_bufsize];
	ret = GetPrivateProfileString("data_chip_version", data_name, "no_define", temp, Name_bufsize, cfg_name);

	if (ret < 0) {
		printf("error:get data_chip_version from %s error!\n", cfg_name);
		return -2;
	}

	if (strcmp(temp, "no_define") == 0) {
		return -1;
	}

	switch (atoi(temp)) {
	case WLAN_VERSION_90_D:
	case WLAN_VERSION_90_E:
		return WLAN_VERSION_90_D;
		break;
	case WLAN_VERSION_91:
		return WLAN_VERSION_91;
		break;
	case WLAN_VERSION_91_E:
		return WLAN_VERSION_91_E;
		break;
	case WLAN_VERSION_91_F:
		return WLAN_VERSION_91_F;
		break;
	case WLAN_VERSION_91_G:
		return WLAN_VERSION_91_G;
		break;
	default:
		return -1;
	}


}
void print_message(Node * i)
{
	if (i == NULL){
		return;
	}

	print_message(i->nest);
	if (strlen(i->name) >= (32 - 5))
		printf("name:%s\tsize:%d\n", i->name, i->size);
	else if (strlen(i->name) >= (24 - 5))
		printf("name:%s\t\tsize:%d\n", i->name, i->size);
	else if (strlen(i->name) >= (16 - 5))
		printf("name:%s\t\t\tsize:%d\n", i->name, i->size);
	else if (strlen(i->name) >= (8 - 5))
		printf("name:%s\t\t\t\tsize:%d\n", i->name, i->size);
	else
		printf("name:%ssize:%d\n", i->name, i->size);

	return;
}

int init()
{
	int choise = -1;
	int i;
	int num = sizeof(file_support) / sizeof(file_node);
	printf("Please choose the file you want to generate:\n");
	for (i = 0; i < num; ++i){
		printf("%d:%s.bin\n", i + 1, file_support[i].file_type);
	}
	//choise = 2;
	scanf("%d", &choise);
	if (choise < 1 || choise > i){
		printf("input error:please input 1 to %d\n", i);
		return -1;
	}
	choise--;
	strcpy(str_path, file_support[choise].file_type);

	printf("config file:%s\n", ADD_PATH(file_support[choise].cfg_file) );
	printf("output file:%s\n", ADD_PATH(file_support[choise].out_file) );

	strcpy(cfg_name, ADD_PATH(file_support[choise].cfg_file) );

	FILE *f_init = fopen( cfg_name, "r");
	if (f_init == NULL){
		printf("Could not open config file: %s", cfg_name);
		return -1;
	}
	fclose(f_init);

	strcpy(output_file_name, file_support[choise].out_file);
	strcpy(file_type, file_support[choise].file_type );

	return 0;

}

int main()
{
	int ret;
	char * ret_char;
	FILE * fi, *fo;
	char str_line[Line_size];

#ifdef debug_file
	FILE * f_dbg = fopen("debug.log", "w");
#endif

	ret = init();
	if (ret < 0){
		printf("init error:please check you ini file!\n");
		goto err;
	}


	char file[Input_file_size][Name_bufsize];//input_file names
	int file_num = 0, file_num1 = 0;;
	char *file_name;


	//read input data file names
	ret = GetPrivateProfileString("file_name", "input_file_name", "no_define", input_name, Line_size, cfg_name);
	if (ret < 0) {
		printf("error:get input file name from %s error!\n", cfg_name);
		goto err;
	}
	if (strcmp(input_name, "no_define") == 0) {
		printf("error:there is no input files in %s!\n", cfg_name);
		goto err;
	}
	printf("\ninput file name: %s\n", input_name);

	for (file_name = strtok(input_name, " "); file_name != NULL; file_name = strtok(NULL, " ")) {
		strcpy(file[file_num++], ADD_PATH(file_name));
	}

	//read output file name
	ret = GetPrivateProfileString("file_name", "output_file_name", output_file_name, output_name, Name_bufsize, cfg_name);
	if (ret < 0) {
		printf("error:get output file name from %s error!\n", cfg_name);
		goto err;
	}
	printf("output file name: %s\n\n", ADD_PATH(output_name));

	//open output file
	fo = fopen(ADD_PATH(output_name), "wb");
	if (fo == NULL){
		printf("error:open or create output file %s error!\n", ADD_PATH(output_name));
		goto err;
	}
	strcpy(rda_wland_firmware_head.firmware_type, output_name);


	//traverse each input files, count the number of data_name
	for (int i = 0; i < file_num; ++i) {
		int line = 0;
		fi = fopen(file[i], "r");//open input data file
		if (fi == NULL){
			printf("error: open input file %s error!\n", file[i]);
			goto err;
		}

		while (fgets(str_line, Line_size, fi)) {//read each line
			line++;
			if (del_comment(str_line) == -1)//delete comment marked by '//'
				continue;
			ret = del_macro(str_line, file[i], line);//deal with macro
			if (ret > 0)
				continue;
			else if (ret == -1)
				goto err;

			int data_type;
			ret = check_data_name(str_line, &data_type);

			if (ret > 0) {//this line have data_name
				data_num++;
			}
		}
	}
	rda_wland_firmware_head.data_num = data_num;
	rda_wland_firmware_head.version = RDA_FIRMWARE_VERSION;
	fwrite(&rda_wland_firmware_head, sizeof(struct rda_device_firmware_head), 1, fo);
	crc_ret = crc16(crc_ret, (const u8 *)&rda_wland_firmware_head, sizeof(struct rda_device_firmware_head));

	//read input data file and write data to "bin file"
	for (int i = 0; i < file_num; ++i) {	//for each input data file
		int line = 0;

		char *str = NULL;
		char *str1 = NULL, *str2 = NULL, *str3 = NULL;
		char str_line[Line_size];

		fi = fopen(file[i], "r");//open
		if (fi == NULL){
			printf("error: open input file %s error!\n", file[i]);
			goto err;
		}
		printf("file_name%d:\t%s\n", i, file[i]);

		while (fgets(str_line, Line_size, fi)) {//read first line
			line++;

			if (del_comment(str_line) == -1)//delete comment marked by '//'
				continue;

			ret = del_macro(str_line, file[i], line);//deal with macro
			if (ret > 0)
				continue;
			else if (ret == -1)
				goto err;


#ifdef debug_file
			fputs(str_line, f_dbg);
			fprintf(f_dbg, "\n");
#endif
			char name[40];
			int data_type;
			char data_flag[Line_size];

			ret = check_data_name(str_line, &data_type);
			if (ret > 0) {
				int open_brace = 0;
				if (str_line[strlen(str_line) - 1] == '{')//open brace
					open_brace = 1;
				char *str_line_temp = str_line + ret + 1;
				for (int k = 0; k < strlen(str_line_temp); ++k)//replace '[' by ' ',
					if (str_line_temp[k] == ' ' || str_line_temp[k] == '['){
						str_line_temp[k] = ' ';
						break;
					}
				str = strtok(str_line_temp, " ");

				if (str == NULL) {//there is't a data_name
					printf("error:%s-%d data_name error!\n", file[i], line);
					goto err;
				}
				if (chack_data_name(str) < 0) {//data_name repeated
					printf("error:%s-%d %s has been defined!\n", file[i], line, str);
					goto err;
				}
				strcpy(name, str);

				temp_Node = new Node;//new data_node
				strcpy(temp_Node->name, name);
				temp_Node->data_type = data_type;
				temp_Node->size = 0;
				temp_Node->data = rda_wland_firmware_data;

				strcpy(rda_wland_firmeware_data_type.data_name, name);

				if (open_brace == 0) {//deal with open brace
					while (1) {
						ret_char = fgets(str_line, Line_size, fi);
						line++;
						if (ret_char == NULL)
							break;

						if (del_comment(str_line) == -1)
							continue;
#ifdef debug_file
						fputs(str_line, f_dbg);
						fprintf(f_dbg, "\n");
#endif
						ret = del_macro(str_line, file[i], line);
						if (ret > 0)
							continue;
						else if (ret == -1)
							goto err;
						else
							break;
					}
					if (ret_char == NULL) {//end of input data file
						printf("error:%s-%d %s has no data !\n", file[i], line, name);
						goto err;
					}
					if (str_line[0] != '{'){	//this is't a '{'
						printf("error:%s-%d here should be a '{' as the begin of %s's data\n", file[i], line, name);
						goto err;
					}
					else if (check_open_bace(str_line) < 0) {//there is needless message after ‘{’
						printf("error:%s-%d too many parameters after '{' (Some data may be ignored)!\n", file[i], line);
						goto err;
					}
				}

				//read data
				while (fgets(str_line, Line_size, fi)) {
					line++;
					if (del_comment(str_line) == -1)
						continue;
#ifdef debug_file
					fputs(str_line, f_dbg);
					fprintf(f_dbg, "\n");
#endif
					ret = del_macro(str_line, file[i], line);
					if (ret > 0)
						continue;
					else if (ret == -1)
						goto err;
					if (str_line[0] == '}') {//this is a '}'

						if (check_close_bace(str_line) < 0) {
							printf("warning:%s-%d too many parameters after '}' (ignored)!\n", file[i], line);
						}
						printf("write data:%s done!\n", name);
						add_node(temp_Node);//add data_node to list
						rda_wland_firmeware_data_type.size = temp_Node->size;
						rda_wland_firmeware_data_type.crc = crc16(0, (const u8*)rda_wland_firmware_data, rda_wland_firmeware_data_type.size);
						rda_wland_firmeware_data_type.chip_version = count_chip_version(rda_wland_firmeware_data_type.data_name);
						if (rda_wland_firmeware_data_type.chip_version == -2)
							goto err;
						fwrite(&rda_wland_firmeware_data_type, sizeof(struct rda_firmware_data_type), 1, fo);
						crc_ret = crc16(crc_ret, (const u8 *)&rda_wland_firmeware_data_type, sizeof(struct rda_firmware_data_type));
						fwrite(rda_wland_firmware_data, 1, rda_wland_firmeware_data_type.size, fo);
						crc_ret = crc16(crc_ret, (const u8 *)rda_wland_firmware_data, rda_wland_firmeware_data_type.size);
						break;
					}

					//data
					if (check_bace(str_line) < 0) {//check '{' ',' '}'
						printf("error:%s-%d there should be '{***,***}' for each data of %s\n", file[i], line, name);
						goto err;
					}
					replace_bace(str_line);//replace '{' ', '}' by ' '
					unsigned int data1, data2;
					check_macro(str_line);
					if (check_plus(str_line)) {//there is a '+'
						ret = handle_plus(str_line, &data1, &data2);
						if (ret < 0){
							printf("error:%s-%d data format error\n", file[i], line);
							goto err;
						}

					}
					else{//there is't a '+'
						ret = get_two_data(&str1, &str2, str_line);
						if (ret == -1){//illegal world
							printf("error:%s-%d data format error，illegal world!\n", file[i], line);
							goto err;
						}
						else if (ret == -2){//there is't data1 or data2
							printf("error:%s-%d data format error（there shoule be two data）\n", file[i], line);
							goto err;
						}
						else if (ret == -3){//more than to data in this line
							printf("error:%s-%d data format error, too many data\n", file[i], line);
							goto err;
						}
						else if (ret == -4){//error
							printf("error:%s-%d data format error!\n", file[i], line);
							goto err;
						}
						data1 = (unsigned int)strtoul(str1, NULL, 0);
						data2 = (unsigned int)strtoul(str2, NULL, 0);
					}

					if (temp_Node->data_type == 32){
						unsigned int * p = (unsigned int *)temp_Node->data;
						*p = data1;
						p++;
						*p = data2;
						temp_Node->data += 8;
						temp_Node->size += 8;
					}
					else if (temp_Node->data_type == 16){
						unsigned short * p = (unsigned short *)temp_Node->data;
						*p = data1;
						p++;
						*p = data2;
						temp_Node->data += 4;
						temp_Node->size += 4;
					}
					else{
						unsigned char * p = temp_Node->data;
						*p = (unsigned char)data1;
						p++;
						*p = (unsigned char)data2;
						temp_Node->size += 2;
						temp_Node->data += 2;
					}
				}

			}
			else {//the first line is't a data_name
				printf("error: %s-%d there  should be a data_name:\n", file[i], line);
				goto err;
			}
		}
		printf("read file_name%d:\t%s done!\n\n", i, file[i]);
	}
	fwrite(&crc_ret, 2, 1, fo);
	fclose(fo);
#ifdef debug_file
	fclose(f_dbg);
#endif
	fclose(fi);
	print_message(Node_home);

	printf("Generate %s successfully!\n", output_file_name);
	system("pause");
	return 0;
err:
	system("pause");
	return -1;
}

