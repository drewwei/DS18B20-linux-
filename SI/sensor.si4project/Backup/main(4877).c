#include <stdio.h>
#include "serial.h"
#include "i2c.h"
#include "gpio_spi.h"
#include "oled.h"
#include "s3c24xx.h"
#include "ds18b20.h"
#include "spi_flash.h"
#include "udelay.h"
#include "string.h"
unsigned char at24cxx_read(unsigned char address);
void at24cxx_write(unsigned char address, unsigned char data);

extern unsigned char crc_caculate(unsigned char *pval,int length);

int main()
{
    char c;
    char str[200];
    int i;
	int address;
	int data;
    unsigned char PMID;
	unsigned char PDID;
	unsigned char *string;
	unsigned char *dat = (unsigned char *)0x32000000;  //ä»flashä¸­è¯»åˆ°å†…å­˜åœ°å€ä¸º0x32000000å¤„
    uart0_init();   // ²¨ÌØÂÊ115200£¬8N1(8¸öÊı¾İÎ»£¬ÎŞĞ£ÑéÎ»£¬1¸öÍ£Ö¹Î»)
	printf("uart0_init");
	
	timer0_init();
    //SPIInit();
	 GPIO_SPI_Init();
   // OLEDInit();
    InitSpiFlash();
   
    ReadID(&PMID, &PDID);
	printf("MID is %x;DID is %x\n\r",PMID,PDID);
	sprintf(string,"MID:%x PID:%x",PMID,PDID);
    OLED_Init();
    OLED_Print(string, 4, 0);
	string = "In vertical addressing mode, after the display RAM is read/writtenthe, page address pointer is increased automatically by 1.  If the page address pointer reaches the page end address, the page address pointer is reset to page start address and column address pointer is increased by 1. The sequence of movement of the page and column address point for vertical addressing mode is shown in Figure 10-4. When both column and page address pointers reach the end address, the pointers are reset to column start address and page start address";
	unsigned int size = strlen(string);
	printf("size is:%d \n\r",size);
	
	WriteData(0x2000, string, size);	
	ReadData(0x2000, dat, size);
	dat[size] = '\0';  //ç»™å­—ç¬¦ä¸²åŠ ä¸Šç»“å°¾æ ‡å¿—
	printf("%s\n\r", dat);
	OLED_PrintHoriAddrModeMove(dat);
	
   //OLED_Print("I love linux ! 1677812132@qq.com jay", 2, 0);
	//OLED_PrintHoriAddrModeMove("In vertical addressing mode, after the display RAM is read/written, the page address pointer is increased automatically by 1.  If the page address pointer reaches the page end address, the page address pointer is reset to page start address and column address pointer is increased by 1. The sequence of movement of the page and column address point for vertical addressing mode is shown in Figure 10-4. When both column and page address pointers reach the end address, the pointers are reset to column start address and page start address ");
	//OledPutChar('w', 3, 65);
	
	
	while(1);

    int u =3;
    while(u--)
    	{
    		printf("test timer%d\n",u);
			mdelay(1000);
		}
	struct temp temp;
	while(1)
	{
		
		temp = read_tempareture();
		printf("current temperature: %d.%04d \n\r", temp.temp_h, temp.temp_l);
		mdelay(1000);
	}
	



	
    i2c_init();
    
    while (1)
    {
        printf("\r\n##### AT24CXX Menu #####\r\n");
        printf("[R] Read AT24CXX\n\r");
        printf("[W] Write AT24CXX\n\r");
        printf("Enter your selection: ");

        c = getc();
        printf("%c\n\r", c);
        switch (c)
        {
            case 'r':
            case 'R':
            {
                printf("Enter address: ");
                i = 0;
                do
                {
                    c = getc();
                    str[i++] = c;
                    putc(c);
                } while(c != '\n' && c != '\r');
                str[i] = '\0';

                while(--i >= 0)
                {
                    if (str[i] < '0' || str[i] > '9')
                        str[i] = ' ';
                }

                sscanf(str, "%d", &address);
				printf("\r\nread address = %d\r\n", address);
				data = at24cxx_read(address);
				printf("data = %d\r\n", data);
                    
                break;
            }
            
            case 'w':
            case 'W':
            {
                printf("Enter address: ");
                i = 0;
                do
                {
                    c = getc();
                    str[i++] = c;
                    putc(c);
                } while(c != '\n' && c != '\r');
                str[i] = '\0';
				printf("\r\n");

                while(--i >= 0)
                {
                    if (str[i] < '0' || str[i] > '9')
                        str[i] = ' ';
                }

                sscanf(str, "%d", &address);
				//printf("get str %s\r\n", str);

                printf("Enter data: ");
                i = 0;
                do
                {
                    c = getc();
                    str[i++] = c;
                    putc(c);
                } while(c != '\n' && c != '\r');
                str[i] = '\0';
				printf("\r\n");
				//printf("get str %s\r\n", str);

                while(--i >= 0)
                {
                    if (str[i] < '0' || str[i] > '9')
                        str[i] = ' ';
                }

                sscanf(str, "%d", &data);
				//address = 12;
				//data = 13;
				printf("write address %d with data %d\r\n", address, data);
				
				at24cxx_write(address, data);

                break;
            }
        }
        
    }
    
    return 0;
}
