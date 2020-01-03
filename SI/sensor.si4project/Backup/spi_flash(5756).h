
#ifndef __SPI_FLASH__
#define __SPI_FLASH__

extern void ReadID(unsigned char *PMID, unsigned char *PDID);

extern void ReadData(unsigned int addr, unsigned char *data, unsigned int size);

extern void WriteData(unsigned int addr, unsigned char *pdata, unsigned int size);
extern void ChipErase();
#endif