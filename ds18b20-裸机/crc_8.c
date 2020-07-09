
unsigned char crc_high_first(unsigned char val)
{
	unsigned char crc = val;
	int i;
	/*  因为crc是8位所以，valu向左移动8位*/
	for(i = 0; i < 8; i++) 
	{
			if(crc & 0x80)
			{
				crc = (crc << 1)^0x31; 
			/* 如果最高位为:1,则向左移1位，然后与0x31异或（因为多项式位数=crc+1 且最高位一定等于1,所以当val最高位为1时向左移动表示已经和多项式(0x131)的最高位异或成0了。）*/
			}
			else{
				
				crc <<= 1;
			}
	}

	
	return crc;
}
/*多个字节左crc校验：将每个字节的crc与下一个字节异或，然后再与多项式做8次异或，直到所有字节都校验完*/
unsigned char crc_caculate(unsigned char *pval,int length)
{
	int i;
	unsigned char crc = 0x0;
	/*  因为crc是8位所以，valu向左移动8位*/
	while(length--)
	{
		crc ^= *pval++;
		for(i = 0; i < 8; i++) 
		{
				if(crc & 0x80)
				{
					crc = (crc << 1)^0x31; 
				/* 如果最高位为:1,则向左移1位，然后与0x31异或（因为多项式位数=crc+1 且最高位一定等于1,所以当val最高位为1时向左移动表示已经和多项式(0x131)的最高位异或成0了。）*/
				}
				else{
					
					crc <<= 1;
				}
		}
	}

	return crc;
	
}


unsigned char crc_check1(unsigned char val,unsigned char crc)
{
	int i;
	for(i = 0; i < 8; i++) 
		{
				if(val & 0x80)
				{
					val = (val << 1)^0x31; 
				/* 如果最高位为:1,则向左移1位，然后与0x31异或（因为多项式位数=crc+1 且最高位一定等于1,所以当val最高位为1时向左移动表示已经和多项式(0x131)的最高位异或成0了。）*/
				}
				else{
					
					val <<= 1;
				}
		}
		val ^= crc;
	for(i = 0; i < 8; i++) 
		{
				if(val & 0x80)
				{
					val = (val << 1)^0x31; 
				/* 如果最高位为:1,则向左移1位，然后与0x31异或（因为多项式位数=crc+1 且最高位一定等于1,所以当val最高位为1时向左移动表示已经和多项式(0x131)的最高位异或成0了。）*/
				}
				else{
					
					val <<= 1;
				}
		}	
	return val;
}

unsigned char crc_check2(unsigned char *pval,int length)
{
	int i;
	unsigned char val = 0x0;
	while(length--)
	{	
		val ^= *pval++;
		for(i = 0; i < 8; i++) 
		{
				if(val & 0x80)
				{
					val = (val << 1)^0x31; 
				/* 如果最高位为:1,则向左移1位，然后与0x31异或（因为多项式位数=crc+1 且最高位一定等于1,所以当val最高位为1时向左移动表示已经和多项式(0x131)的最高位异或成0了。）*/
				}
				else{
					
					val <<= 1;
				}
		}
	}
	return val;
}
