KERN_DIR = /work/linux-4.19-rc3
all:
	make -C $(KERN_DIR) M=`pwd` modules 

clean:
	make -C $(KERN_DIR) M=`pwd` modules clean
	rm -rf modules.order

obj-m	+= ds18d20_dri.o
