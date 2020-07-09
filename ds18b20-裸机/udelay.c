#include "s3c24xx.h"
#include <stdio.h>
void timer0_init()
{
	TCFG0 = 4;
	TCFG1 &= ~0xf;

	TCNTB0 = 50000;
	/* 加载初值*/
	TCON |= (1<<1);   /* Update from TCNTB0 & TCMPB0 */

	/* 设置为自动加载并启动 */
	TCON &= ~(1<<1);
	TCON |= (1<<0) | (1<<3);  /* bit0: start, bit3: auto reload */
}

void udelay(int n)
{
	int cnt = 5*n;
	int cur;
	int pre = TCNTO0;
	int len;
	while(cnt > 0)
	{	
		cur = TCNTO0;
		if(cur < pre)
			len = pre - cur;
		else
			len = (50000 - cur) + pre;
		pre = cur;
		cnt = cnt - len;
	}
	
}

void mdelay(int n)
{
	udelay(n*1000);
}