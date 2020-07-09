/*
* 温度传感器
*/
#include <stdio.h>

#include "s3c24xx.h"
#include "ds18b20.h"


#define CONVERT 		 	0X44

#define READ_SCRATCHPAD		0XBE
#define WRITE_SCRATCHPAD 	0X4E
#define COPY_SCRATCHPAD 	0X48
#define RECALL				0XB8
#define READ_PS				0XB4	
#define Skip_ROM            0xCC
#define READ_ROM            0x33

extern void udelay(int n);
extern unsigned char crc_check2(unsigned char *pval,int length);

static void set_gpg6_output()
{
	GPGCON &= ~(3<<12);
	GPGCON |= (1<<12);
}
static void set_gpg6_input()
{
	GPGCON &= ~(3<<12);
	GPGCON |= (0<<12);
}

static void ds18b20_release()
{
	//GPGDAT = (1<<6); //释放总线，设置为高电平
	set_gpg6_input(); //进入输入模式
}


static void ds18b20_set_pin_val_time(int val, int time)
{
	set_gpg6_output();
	if(val)
		{
			GPGDAT = (1<<6);
		}
	else
		{
			GPGDAT &= ~(1<<6);
		}
	
	udelay(time);
	
}


static unsigned int ds18b20_get_pin_val()
{
	
	if(GPGDAT & (1<<6))
		return 1;
	else
		return 0;
}

void ds18b20_init()
{
	unsigned int ret;
	ds18b20_set_pin_val_time(0, 500);
	ds18b20_release();
	udelay(120);
	
	ret = ds18b20_get_pin_val();
	if(ret)
		{
			printf("ds18b20_init fail.\n");
		}
	else
		{
			udelay(180);
		}
	ds18b20_release();
	udelay(200);
}

void ds18b20_send_byte(char        c)
{
	int i;
	for(i = 0; i < 8; i++)
	{
		if((c>>i) & 1)
		{
			ds18b20_set_pin_val_time(0, 2);
			ds18b20_release();
			udelay(65);
			
		}
		else{
			ds18b20_set_pin_val_time(0, 65);
			ds18b20_release();
		}
		udelay(1);
	}
		
}

void ds18b20_send_bytes(char *p, unsigned int len)
{
	char c;
	int i;
	for(i = 0; i < len; i++)
	{
		c = *p++;
		ds18b20_send_byte(c);
	}
}
static int ds180b20_rev_bit()
{
	int ret;
	ds18b20_set_pin_val_time(0, 2); //拉低2us
	ds18b20_release();  //释放总线，设置为输入
	udelay(10);         //延迟10us后读
	ret = ds18b20_get_pin_val();
	
	return ret;
}

static unsigned char ds180b20_rev_byte()
{
	int i;
	char c = 0;
	for(i = 0; i < 8; i++)
	{
		ds18b20_set_pin_val_time(0, 2); //拉低2us
		ds18b20_release();  //释放总线，设置为输入
		udelay(10);         //延迟10us后读
		if (ds18b20_get_pin_val())
			c |= (1 << i);

		udelay(55);
	}

	return c;
}

void ds180b20_rev_bytes(char *buff, int count)
{
	int i;
	for(i = 0; i < count; i++)
	{
		buff[i] = ds180b20_rev_byte();
	}

}

int read_rom(char *rom_buff, int count)
{
	//INITIALIZATION
	ds18b20_init();

	// SEQUENCE33h READ ROM COMMAND
	ds18b20_send_byte(READ_ROM);
	
	//rcv rom 
	ds180b20_rev_bytes(rom_buff, count);
	return 0;
}
int read_ram(char *ram_buff, int count)
{
	

	//PULLUPYNRETURN TO INITIALIZATION
	ds18b20_init();
	//发rom cmd
	ds18b20_send_byte(Skip_ROM);
	//发ram cmd
	ds18b20_send_byte(READ_SCRATCHPAD);
	//接收数据
	ds180b20_rev_bytes(ram_buff, count); 
	return 0;
}

int convert()
{
	//PULLUPYNRETURN TO INITIALIZATION
	ds18b20_init();
	//发rom cmd
	ds18b20_send_byte(Skip_ROM);
	//Read Power Supply [B4h]  parasite powered DS18B20s will pull the bus low, and externally powered DS18B20s will let the bus remain high
	ds18b20_send_byte(CONVERT);
	udelay(50);
}
struct temp read_tempareture()
{
	int ret;
	char rom_buff[8] = {0};
	char ram_buff[9] = {0};
	//读rom
	read_rom(rom_buff, 8);
	printf("DS18B20’s 1-Wire family code:%x \n \r", rom_buff[0]);
	
	convert();
	
	read_ram(ram_buff, 9);
	
	int buff[11] = {625, 1250, 2500, 5000, 1, 2, 4, 8, 16, 32, 64};
	int i;
	
	struct temp temp = {
		.temp_l = 0,
		.temp_h = 0,
	};
	

	//小数部分
	for(i = 0; i < 4; i++)
		{
			if(ram_buff[0]	& (1 << i))
				temp.temp_l += buff[i];		
		}
	//整数部分
	for(i = 0; i < 4; i++)
		{
			if(ram_buff[0]	& (1 << (i+4)))
				temp.temp_h += buff[4+i];		
		}
	
	for(i = 0; i < 3; i++)
		{
			if(ram_buff[1]	& (1 << i))
				temp.temp_h += buff[8+i];
		}
	
	if(ram_buff[1]	& (1 << 3))
		temp.temp_h = -temp.temp_h;

	ret = 1;

	for(i = 0; i<8; i++)
	{
		rom_buff[i]   = rom_buff[i] + rom_buff[7-i];
		rom_buff[7-i] = rom_buff[i] - rom_buff[7-i];
		rom_buff[i]   = rom_buff[i] - rom_buff[7-i];
	}
	ret = crc_check2(rom_buff, 7);
	
	if(ret == rom_buff[7])
		printf("crc check pass! \n\r");
	else
		printf("crc check fail! \n\r");
	
	return temp;
}




















