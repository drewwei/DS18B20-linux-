#include "gpio_spi.h"
#include "s3c24xx.h"
#include "udelay.h"
/*flash片选*/
static void SPI_FlashCS(int select)
{
	if(select == 0) 
	{
		GPGDAT |= (1<<2); /*为高电平时取消选择*/
	}else
	{
		GPGDAT &= ~(1<<2);
	}
}
/*发地址*/
static void SendAddress(unsigned int addr)
{
	SPI_SendByte((addr>>16)&0xff);
	SPI_SendByte((addr>>8)&0xff);
	SPI_SendByte(addr&0xff);
}
/*读ID*/
void ReadID(unsigned char *PMID, unsigned char *PDID)
{
	SPI_FlashCS(1);   //选中芯片
	SPI_SendByte(0x90); //发命令
	SendAddress(0);
	*PMID = SPI_RevByte();
	*PDID = SPI_RevByte();
	
	SPI_FlashCS(0);	
}
/*写使能*/
static void WriteEnable()
{
	SPI_FlashCS(1);   //选中芯片
	SPI_SendByte(0x06);
	SPI_FlashCS(0);	//取消片选
}
/*写禁止*/
static void WriteDisable()
{
	SPI_FlashCS(1);   //选中芯片
	SPI_SendByte(0x04);
	SPI_FlashCS(0);	//取消片选
}
/* 写状态寄存器是能 */
static void WriteEnableForStatusReg()
{
	SPI_FlashCS(1);   //选中芯片
	SPI_SendByte(0x50);
	SPI_FlashCS(0);	//取消片选
}
/* 读状态寄存器1 */
static unsigned char ReadStatusReg1()
{
	unsigned char ret;
	SPI_FlashCS(1);   //选中芯片
	SPI_SendByte(0x05);	
	ret = SPI_RevByte();
	
	SPI_FlashCS(0);	//取消片选
	return ret;
}
/* 读状态寄存器2 */
static unsigned char ReadStatusReg2()
{
	unsigned char ret;
	SPI_FlashCS(1);   //选中芯片
	SPI_SendByte(0x35);	
	ret = SPI_RevByte();
	
	SPI_FlashCS(0);	//取消片选
	return ret;
}
/* 写状态寄存器 */
static void WriteStatusReg(unsigned char reg1, unsigned char reg2)
{
	WriteEnableForStatusReg(); //写使能
	SPI_FlashCS(1);   //选中芯片
	SPI_SendByte(0x01);
	SPI_SendByte(reg1);
	SPI_SendByte(reg2);	
	SPI_FlashCS(0);	//取消片选
	WriteDisable(); //写禁止
}
/*判断忙*/
static int BusyStatus()
{
	return ReadStatusReg1()&1;
}
/*擦除整个芯片*/
void ChipErase()
{
	WriteEnable(); //写使能
	SPI_FlashCS(1);   //选中芯片
	SPI_SendByte(0xc7);
	SPI_FlashCS(0);	//取消片选
	WriteDisable(); //写禁止
}
/*扇区擦除,addr为扇区地址*/
void SectorErase(unsigned int sector)
{
	WriteEnable(); //写使能
	SPI_FlashCS(1);   //选中芯片
	SPI_SendByte(0x20);
	SendAddress(sector); //发地址
	SPI_FlashCS(0);	//取消片选
}
/*块擦除,addr为块地址*/
void BlockErase(unsigned int block)
{
	WriteEnable();  //写使能
	SPI_FlashCS(1);   //选中芯片
	SPI_SendByte(0xd8);
	SendAddress(block); //发地址
	SPI_FlashCS(0);	//取消片选
}
static void Unprotect()
{
	while(BusyStatus());
	unsigned char reg1 = ReadStatusReg1();
	unsigned char reg2 = ReadStatusReg2();
	reg1 &= ~(1<<7); 
	reg2 &= ~(1<<0); 
	while(BusyStatus());
	WriteEnableForStatusReg();
	while(BusyStatus());
	WriteStatusReg(reg1, reg2);
}


/*页编程*/
unsigned char * PageProgram(unsigned int addr, unsigned char *data, unsigned int size)
{
	int i, col = 0;
	WriteEnable();   //写是能
	
	SPI_FlashCS(1);   //选中芯片
	SPI_SendByte(0x02);  //发指令
	SendAddress(addr); //发地址
	if(size > 256)
		size = 256;
	col = addr%256;
	if(col)
	{
		if((size+col) >= 256 )
		{
			size = 256 - col;
		}
	}	
	for(i = 0; i < size; i++) 
	{
		SPI_SendByte(*data);  //发数据
		data++;
	}
	
	SPI_FlashCS(0);	//取消片选
	
	return data;
}
/*读数据*/
void ReadData(unsigned int addr, unsigned char *data, int size)
{
	int i = 0;
	while(BusyStatus()); //循环判断忙
	SPI_FlashCS(1);   //选中芯片
	SPI_SendByte(0x03);  //发指令
	SendAddress(addr); //发地址
	
	for(i = 0; i < size; i++)
	{
		data[i] = SPI_RevByte();
	}
	
	SPI_FlashCS(0);	//取消片选
}

void WriteData(unsigned int addr, unsigned char *pdata, int size)
{

	int page, blcok , i, col;
	unsigned char *data = pdata;
	page = (col + size)/256 + 1;
	blcok = addr/64/1024;
	while(BusyStatus()); //循环判断忙
	BlockErase(blcok); //擦除块		
	col = size%256;
	for(i = 0; i < page; i++)
	{
		while(BusyStatus()); //循环判断忙
		data = PageProgram(addr, data, size);//写数据
		if(col)
		{
			addr += (256 - col); 
			size -= (256 - col);
			col = 0;
		}
		else{
			addr += 256;
			size -= 256;
		}	
	}
	



/*
//会绕道页头覆盖之前写的东西.
	int i,block;
	block = addr/64/1024;
	while(BusyStatus()); //循环判断忙
	BlockErase(block); //擦除块	

	while(BusyStatus()); //循环判断忙
	WriteEnable();   //写是能
	SPI_FlashCS(1);   //选中芯片
	SPI_SendByte(0x02);  //发指令
	SendAddress(addr); //发地址
	for(i = 0; i < size; i++) 
	{
		SPI_SendByte(*pdata);  //发数据
		pdata++;
	}
	SPI_FlashCS(0);	//取消片选
*/
}

void InitSpiFlash()
{
	Unprotect();
	while(BusyStatus()); //循环判断忙
	ChipErase();
	while(BusyStatus()); //循环判断忙
}


