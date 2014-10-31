PCCMute
=======

The repository of the PCCMute v2.2 board

##LINUX KERNEL 3.7.4+
####Description
This linux kernel 3.7.4+ is modified to return allways that the board is a revC4 board. 

It also allows to enable SPI just by writting inside uEnv.txt:
~~~~~~
buddy=spidev
~~~~~~
Or by executting in u-boot:
~~~~~~
$ setenv buddy spidev
~~~~~~

You can find the compiled uImage in beagle-kernel-3.7/kernel/arch/arm/boot/uImage

####How to compile it
========================================================
$ cd beagle-kernel-3.7/kernel

The first time you compile the kernel run: (just the first time!!)
~~~~~~
$ make CROSS_COMPILE=arm-linux-gnueabi- mrproper
~~~~~~

And after:
~~~~~~
$ cp ../configs/beagleboard .config
$ make ARCH=arm CROSS_COMPILE=arm-linux-gnueabi- uImage dtbs
~~~~~~

##u-boot-2012.10
========================================================
####Description
========================================================
This u-boot is modified to return allways that the board is a revC4 board. 

You can find the compiled boot.img in u-boot-2012.10/u-boot.img
How to compile it

####How to compile it
========================================================
The first time you compile the kernel run: (just the first time!!)
~~~~~~
$ make CROSS_COMPILE=arm-linux-gnueabi- mrproper
~~~~~~

And after:
~~~~~~
$ make CROSS_COMPILE=arm-linux-gnueabi- omap3_beagle_config
$ make CROSS_COMPILE=arm-linux-gnueabi-
~~~~~~




