mkdir dev
cd dev
sudo mknod -m 600 console c 5 1
sudo mknod -m 600 tty2 c 4 2
sudo mknod -m 600 tty3 c 4 3
sudo mknod -m 600 tty4 c 4 4
sudo mknod -m 666 null c 1 3
sudo mknod -m 666 zero c 1 5
sudo mknod -m 666 ptmx c 5 2
sudo mknod -m 666 random c 1 8
sudo mknod -m 666 urandom c 1 9
sudo mknod -m 666 fb0 c 29 0
mkdir pts
cd ..

mkdir etc
cd etc
mkdir dropbear
touch fstab
echo "proc	/proc	proc	defaults	0	0" >> fstab
echo "sysfs	/sys	sysfs	defaults	0	0" >> fstab
echo "devpts	/dev/pts	devpts	defaults	0	0" >> fstab
touch inittab
echo "::sysinit:/etc/init.d/rcS" >> inittab
echo "::respawn:/sbin/getty ttyS0 115200" >> inittab
echo "::restart:/sbin/init" >> inittab
echo "::shutdown:/bin/umount -a -r" >> inittab
#touch inetd.conf
#echo "23      stream  tcp     nowait  root    /usr/sbin/telnetd telnetd" >> inetd.conf
#user group and password shiznin
touch group
echo "root:x:0:" >> group
touch passwd
echo "root::0:0:root:/home/root:/bin/sh" >> passwd
touch shadow

mkdir init.d
cd init.d
touch rcS
echo '#!bin/sh' >> rcS
echo "mount -t proc /proc" >> rcS
echo "mount -t sysfs /sys" >> rcS
echo "mount -t devpts /dev/pts" >> rcS
#starting network suppose we have eth0
echo "/sbin/ifconfig eth0 up" >> rcS
echo "/sbin/udhcpc eth0" >> rcS
#done setting up network with dhcp
echo "/usr/sbin/httpd -h /www/ &" >> rcS
echo "telnetd -f /etc/banner" >> rcS
echo "dropbear -b /etc/banner" >> rcS
#doing dropbear key and starting dropbear
#we suppose dropbear and dropbearkey is in the path
echo -e "\n #Checking if dropbear has keys, if not we generate them" >> rcS
echo "if [ -f /etc/dropbear/dropbear_rsa_host_key ] ; then {" >> rcS
echo -e "\techo 'rsa key found'" >> rcS
echo "}" >> rcS
echo "else" >> rcS
echo -e "\tdropbearkey -t rsa -f /etc/dropbear/dropbear_rsa_host_key -s 1024" >> rcS
echo -e "fi\n" >> rcS

#adding sbin to path
echo -e "export PATH=$PATH:/sbin" >> rcS

sudo chmod 755 rcS
cd ..

touch banner
echo -e "####################################################################" >> banner
echo -e "####################################################################" >> banner
echo -e "####################################################################" >> banner
echo -e "###################### WELCOME TO THE BEAGLE #######################" >> banner
echo -e "####################################################################" >> banner
echo -e "####################################################################" >> banner
echo -e "####################################################################" >> banner
#exiting etc folder
cd ..

mkdir proc
mkdir sys

mkdir lib
#we add dinamically libraries
cd lib
#example of copying library
#cp -a /path_to_libs/ld-uClibc* .

cd ..


mkdir -p ./home/root

mkdir -p ./usr/share/udhcpc/
cd ./usr/share/udhcpc/
touch default.script
#this script is called by the udhcpc to set up resolv.conf and gateway and route remember to get one from the busybox src
#this has to be done on the device apparently
#login as root via telnetd and set a password
#then disable telnetd and use dropbear with the password
