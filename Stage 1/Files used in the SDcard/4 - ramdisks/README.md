##HOW TO CREATE THE RAMDISK
####Procedure from: http://processors.wiki.ti.com/index.php/Initrd
Substitute "count" by the number of bytes of your ramdisk:

~~~~~~
$ sudo dd if=/dev/zero of=/dev/ram0 bs=1k count="count"
$ sudo mke2fs -vm0 /dev/ram0 "count"
$ tune2fs -c 0 /dev/ram0
$ sudo dd if=/dev/ram0 bs=1k count="count" | gzip -v9 > ramdisk.gz
~~~~~~
~~~~~~
$ mkdir mnt
$ gunzip ramdisk.gz
$ sudo mount -o loop ramdisk mnt/
~~~~~~
Untar now into the mounted device your root file system.
After untar, run the following:
~~~~~~
$ sudo umount mnt
$ gzip -v9 ramdisk
~~~~~~
If you find any problem while unmount just run:
~~~~~~
$ sudo umount -a
~~~~~~
