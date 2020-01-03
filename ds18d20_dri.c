#include <linux/module.h>
#include <linux/sched.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/irq.h>
#include <asm/uaccess.h>
#include <asm/irq.h>
#include <asm/io.h>
//#include <asm/arch/regs-gpio.h>
//#include <asm/hardware.h>
#include <linux/device.h>
//#include <mach/gpio.h>
#include <linux/interrupt.h>
#include <linux/poll.h>
#include <mach/regs-gpio.h>
#include <mach/gpio-samsung.h>
#include <plat/gpio-cfg.h>

#include <linux/delay.h>

#include <linux/of_gpio.h>
#include <linux/gpio.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/of_platform.h>
#include <linux/platform_device.h>
#include <linux/cdev.h>

#define CONVERT 		 	0X44

#define READ_SCRATCHPAD		0XBE
#define WRITE_SCRATCHPAD 	0X4E
#define COPY_SCRATCHPAD 	0X48
#define RECALL				0XB8
#define READ_PS				0XB4	
#define Skip_ROM            0xCC
#define READ_ROM            0x33


struct temp {
	int temp_l;
	int temp_h;
};

static struct {
    int flag;
    spinlock_t lock;
    struct cdev *cdev;
    struct class *ds18b20_class;
    unsigned int pin;
}*ds18b20_dev;

static void ds18b20_release(void)
{
	//GPGDAT = (1<<6); //释放总线，设置为高电平
	//s3c_gpio_cfgpin(pin, INPUT); //进入输入模式
	gpio_direction_input(ds18b20_dev->pin);
}

static void ds18b20_set_pin_val_time(int val, int time)
{
	//s3c_gpio_cfgpin(pin, OUTPUT);

	if(val)
		{
			//gpio_set_value(pin, 1);
				gpio_direction_output(ds18b20_dev->pin, 1);
		}
	else
		{
			//gpio_set_value(pin, 0);
				gpio_direction_output(ds18b20_dev->pin, 0);
		}
	
	udelay(time);
	
}

static void ds18b20_send_byte(char c)
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

static void ds18b20_send_bytes(char *p, unsigned int len)
{
	char c;
	int i;
	for(i = 0; i < len; i++)
	{
		c = *p++;
		ds18b20_send_byte(c);
	}
}

static int ds180b20_rev_bit(void)
{
	int ret;
	ds18b20_set_pin_val_time(0, 2); //拉低2us
	ds18b20_release();  //释放总线，设置为输入
	udelay(10);         //延迟10us后读
	ret = gpio_get_value(ds18b20_dev->pin);
	
	return ret;
}

static unsigned char ds180b20_rev_byte(void)
{
	int i;
	char c = 0;
	for(i = 0; i < 8; i++)
	{
		ds18b20_set_pin_val_time(0, 2); //拉低2us
		ds18b20_release();  //释放总线，设置为输入
		udelay(10);         //延迟10us后读
		if (gpio_get_value(ds18b20_dev->pin))
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


static void ds18b20_init(void)
{
	unsigned int ret;

	ds18b20_set_pin_val_time(0, 500);

	ds18b20_release();
	udelay(120);
	
	ret = gpio_get_value(ds18b20_dev->pin);
	if(ret)
		{
			printk("ds18b20_init fail.\n");
		}
	else
		{
			udelay(180);
		}
	ds18b20_release();
	udelay(200);
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
static void convert(void)
{
	//PULLUPYNRETURN TO INITIALIZATION
	ds18b20_init();
	//发rom cmd
	ds18b20_send_byte(Skip_ROM);
	//Read Power Supply [B4h]  parasite powered DS18B20s will pull the bus low, and externally powered DS18B20s will let the bus remain high
	ds18b20_send_byte(CONVERT);
	mdelay(50);
}

static ssize_t ds18b20_read(struct file *file, char __user *buf, size_t size, loff_t *ppos)
{
	char rom_buff[8] = {0};
	char ram_buff[9] = {0};
	struct temp temp = {
                .temp_l = 0,
                .temp_h = 0,
        };
	int i;
	//读rom
	int buff[11] = {625, 1250, 2500, 5000, 1, 2, 4, 8, 16, 32, 64};
	spin_lock(&ds18b20_dev->lock);
    ds18b20_dev->flag = 1;
	read_rom(rom_buff, 8);
	printk(KERN_INFO "DS18B20's 1-Wire family code:%x \n \r", rom_buff[0]);
	
	convert();
	
	read_ram(ram_buff, 9);
	spin_unlock(&ds18b20_dev->lock);
	ds18b20_dev->flag = 0;
	 
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
	
	return copy_to_user(buf, &temp, sizeof(temp))? -EFAULT : 0;
	
}
static ssize_t ds18020_write (struct file *filp, const char __user *buf, size_t len, loff_t *off)
{
	
	return 0;
}
static int ds18b20_open(struct inode *inode, struct file *filp)
{
    if((filp->f_flags & O_NONBLOCK) && (ds18b20_dev->flag ==1)) {
        return -1;
    }
    else  {
        spin_lock(&ds18b20_dev->lock);
        ds18b20_dev->flag = 1;
	    ds18b20_init();
        spin_unlock(&ds18b20_dev->lock);
        ds18b20_dev->flag = 0;
    }
 
    
	return 0;
}

static int ds18b20_close(struct inode *inode, struct file *file)
{
	return 0;
}
static struct file_operations ds18b20_fops = {
	.owner = THIS_MODULE,
	.open  = ds18b20_open,
	.read  = ds18b20_read,
	.write = ds18020_write,
	.release = ds18b20_close,
};

static int ds18b20_remove(struct platform_device *pdev)

{
	dev_t *devp = platform_get_drvdata(pdev);
    dev_t dev = *devp;
	cdev_del(ds18b20_dev->cdev);
	unregister_chrdev_region(dev, 1);
	device_destroy(ds18b20_dev->ds18b20_class ,dev);
	class_destroy(ds18b20_dev->ds18b20_class);
	kfree(ds18b20_dev);
	return 0;
}

static int ds18b20_probe(struct platform_device *pdev)
{
    dev_t dev;
	int ret;
	struct device_node *dp_node = pdev->dev.of_node;

    ds18b20_dev = kzalloc(sizeof(*ds18b20_dev), GFP_KERNEL);
    if(!ds18b20_dev){
        printk(KERN_WARNING "kzalloc fail\n");
        return -ENOMEM;
    }
    
	ret = alloc_chrdev_region(&dev, 0, 1, "ds18b20");
	if(ret < 0) {
			printk(KERN_WARNING "ds18b20:fail to alloc_chrdev_region\n");
			return ret;
		}
	
	ds18b20_dev->cdev = cdev_alloc();
	ds18b20_dev->cdev->owner = THIS_MODULE;
	ds18b20_dev->cdev->ops   = &ds18b20_fops;

	ret = cdev_add(ds18b20_dev->cdev, dev, 1);
	if(ret) {
		printk(KERN_WARNING "ds18b20:fail to cdev_add\n");
		goto fail_cdev_add;
	}

	ds18b20_dev->ds18b20_class = class_create(THIS_MODULE, "ds18b20_class");
	device_create(ds18b20_dev->ds18b20_class, NULL, dev, NULL, "ds18b20");

	ds18b20_dev->pin = of_get_named_gpio(dp_node, "pins", 0);
	printk("pin = %d\n", ds18b20_dev->pin);
	
	platform_set_drvdata(pdev, &dev);
	spin_lock_init(&ds18b20_dev->lock);
	ds18b20_dev->flag = 0;

return 0;
	
fail_cdev_add:
	cdev_del(ds18b20_dev->cdev);
	unregister_chrdev_region(dev, 1);
	return -1;
		
}

static const struct of_device_id of_match_buttons[] = {
	{ .compatible = "ds18b20", .data = NULL },
	{ /* sentinel */ }
};


struct platform_driver ds18b20_drv = {
	.probe		= ds18b20_probe,
	.remove		= ds18b20_remove,
	.driver		= {
		.name	= "ds18b20",
		.of_match_table = of_match_buttons, /* 能支持哪些来自于dts的platform_device */
	},
};


static int __init ds18b20_driv_init(void)
{
	return platform_driver_register(&ds18b20_drv);
}


static void __exit ds18b20_driv_exit(void)
{
	platform_driver_unregister(&ds18b20_drv);
}


module_init(ds18b20_driv_init);
module_exit(ds18b20_driv_exit);

MODULE_LICENSE("GPL");






