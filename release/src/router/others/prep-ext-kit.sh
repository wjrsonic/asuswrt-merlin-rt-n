#!/bin/sh

DLSRV1="78.25.8.242"
DLSRV2="82.207.89.70"
DLSRV3="85.198.188.40"
DLSRV4="37.229.9.129"

stop_all_usb_apps () {
for k in vsftpd nmbd smbd snmpd openvpn u2ec lpd Tor webdav_client sambaclient dropbox_client ftpclient asuswebstorage inotify mDNSResponder minidlna mt-daapd nfsd disk_monitor
do
killall $k
killall -SIG $k
killall -SIGTERM $k
done
}

check_download_server () {
if [ "`ping $DLSRV1 -w 2 -q | grep -o "100%"`" == "" ]
						then
						DLSRV=$DLSRV1
	elif [ "`ping $DLSRV2 -w 2 -q | grep -o "100%"`" == "" ]
						then
						DLSRV=$DLSRV2
	elif [ "`ping $DLSRV3 -w 2 -q | grep -o "100%"`" == "" ]
						then
						DLSRV=$DLSRV3
	elif [ "`ping $DLSRV4 -w 2 -q | grep -o "100%"`" == "" ]
						then
						DLSRV=$DLSRV4
	else
		echo "All download server down or no Internet connection"
		logger "All download server down or no Internet connection"
		DLSRV=""
fi
}

MOD_VERSION="mod"`nvram get extendno | sed -n -e 's/^.*-m//p' | sed 's/-webkit.*//'`
FILENAME=`nvram get productid`"_"`nvram get firmver`"_"`nvram get buildno`"_"`nvram get extendno`"-ext.tgz"
ASUS_BASE_APPS="http://dlcdnet.asus.com/pub/ASUS/LiveUpdate/Release/Wireless/asus_base_apps.tgz"

ASUSWARE_MOUNT="/tmp/mnt/ASUSWARE"
ENTWARE_MOUNT="/tmp/mnt/ENTWARE"
DATA_MOUNT="/tmp/mnt/DATA"
ASUSWARE_FOLDER=$ASUSWARE_MOUNT"/asusware"
ENTWARE_FOLDER=$ENTWARE_MOUNT"/entware"

if [ "$3" == "" ]
		then
		APP_PARTITION_SIZE="+256M"
		else
		APP_PARTITION_SIZE="+"$3"M"
fi


if [ "$2" == "install" ]
			then

case $1 in
	asusware)

check_download_server

if [ "$DLSRV" != "" ]
		then

SOURCE_DIR="ftp://"$DLSRV"/pub/ASUS/ASUSWRT_MODDED"

if [ ! -d /jffs/scripts ]
		then
		echo "Please enable jffs on Administartion page and try again"
		logger "Please enable jffs on Administartion page and try again"
		else
		MOUNTED_PATH=`ls /tmp/mnt`
		if [ "$MOUNTED_PATH" == "" ]
			then
			echo "Please connect USB drive and try again"
			logger "Please connect USB drive and try again"
			else
			stop_all_usb_apps
			swapoff -a
			swapoff /opt/.swap
			rm -f /tmp/opt
			for i in /tmp/mnt/*
			do
			umount -l $i
			done
			sleep 2
			for i in /tmp/mnt/*
			do
			umount -f $i
			done
			sleep 2
			for i in /dev/sda*
			do
			umount -f $i
			done
			echo -e "d\n1\nd\n2\nd\n3\nd\n4\nw" | fdisk /dev/sda
			echo -e "n\np\n1\n\n$APP_PARTITION_SIZE\nn\np\n2\n\n\nt\n2\n7\nw" | fdisk /dev/sda
			mke2fs -t ext3 -m 0 -j -L ASUSWARE /dev/sda1
			sleep 2
			rm -f /tmp/opt
			for i in /dev/sda*
			do
			umount -f $i
			done
			mkdir -p $ASUSWARE_MOUNT
			mount -t ext3 /dev/sda1 $ASUSWARE_MOUNT
			mkdir -p $ASUSWARE_FOLDER
			cd $ASUSWARE_FOLDER
			wget $ASUS_BASE_APPS
			tar -xvzf asus_base_apps.tgz
			rm -f asus_base_apps.tgz
			wget $SOURCE_DIR/$MOD_VERSION/$FILENAME
			tar -xvzf $FILENAME
			rm -f $FILENAME
			ln -sf $ASUSWARE_FOLDER /tmp/opt
			if [ -d /opt/ext/usr/bin ]
			then
			mkntfs -L DATA /dev/sda2
			sleep 5
			mkdir -p $DATA_MOUNT
			mount -t tntfs /dev/sda2 $DATA_MOUNT
			mkdir -p $DATA_MOUNT"/swap"
			ln -sf $DATA_MOUNT"/swap" $ASUSWARE_FOLDER"/swap"
			ln -sf $DATA_MOUNT $ASUSWARE_FOLDER"/data"
			fi
			echo "Asusware and External web kit installed. please reboot router"
			logger "Asusware and External web kit installed. please reboot router"
		fi
fi
else
	echo "Please check Internet connection and try again" 
	logger "Please check Internet connection and try again" 
fi
	;;
	entware)

check_download_server

if [ "$DLSRV" != "" ]
		then
SOURCE_DIR="ftp://"$DLSRV"/pub/ASUS/ASUSWRT_MODDED"

if [ ! -d /jffs/scripts ]
		then
		echo "Please enable jffs on Administartion page and try again"
		logger "Please enable jffs on Administartion page and try again"
		else
		MOUNTED_PATH=`ls /tmp/mnt`
		if [ "$MOUNTED_PATH" == "" ]
			then
			echo "Please connect USB drive and try again"
			logger "Please connect USB drive and try again"
			else
			stop_all_usb_apps
			swapoff -a
			swapoff /opt/.swap
			rm -f /tmp/opt
			for i in /tmp/mnt/*
			do
			umount -l $i
			done
			sleep 2
			for i in /tmp/mnt/*
			do
			umount -f $i
			done
			sleep 2
			for i in /dev/sda*
			do
			umount -f $i
			done
			echo -e "d\n1\nd\n2\nd\n3\nd\n4\nw" | fdisk /dev/sda
			echo -e "n\np\n1\n\n$APP_PARTITION_SIZE\nn\np\n2\n\n\nt\n2\n7\nw" | fdisk /dev/sda
			mke2fs -t ext3 -m 0 -j -L ENTWARE /dev/sda1
			sleep 2
			rm -f /tmp/opt
			for i in /dev/sda*
			do
			umount -f $i
			done
			mkdir -p $ENTWARE_MOUNT
			mount -t ext3 /dev/sda1 $ENTWARE_MOUNT
			mkdir -p $ENTWARE_FOLDER
			ln -sf $ENTWARE_FOLDER /tmp/opt
			cd /opt
			sed -i "/rc.unslung start/d" /jffs/scripts/services-start
			echo "" >> /jffs/scripts/services-start
			echo "" >> /jffs/scripts/services-start
			echo "sleep 10" >> /jffs/scripts/services-start
			echo "/opt/etc/init.d/rc.unslung start" >> /jffs/scripts/services-start

			sed -i "/rc.unslung stop/d" /jffs/scripts/services-stop
			echo "" >> /jffs/scripts/services-stop
			echo "" >> /jffs/scripts/services-stop
			echo "/opt/etc/init.d/rc.unslung stop" >> /jffs/scripts/services-stop
			wget http://entware.wl500g.info/binaries/entware/installer/entware_install.sh
			sh ./entware_install.sh			
			rm -f ./entware_install.sh	
			wget $SOURCE_DIR/$MOD_VERSION/$FILENAME
			tar -xvzf $FILENAME
			rm -f $FILENAME
			cd $ENTWARE_MOUNT
			ln -sf ./entware ./asusware
			cp -f /rom/.asusrouter /opt
			if [ -d /opt/ext/usr/bin ]
			then
			mkntfs -L DATA /dev/sda2
			sleep 5
			mkdir -p $DATA_MOUNT
			mount -t tntfs /dev/sda2 $DATA_MOUNT
			mkdir -p $DATA_MOUNT"/swap"
			ln -sf $DATA_MOUNT"/swap" $ENTWARE_FOLDER"/swap"
			ln -sf $DATA_MOUNT $ENTWARE_FOLDER"/data"
			fi
			echo "Entware and External web kit installed. please reboot router"
			logger "Entware and External web kit installed. please reboot router"
		fi
fi
else
	echo "Please check Internet connection and try again" 
	logger "Please check Internet connection and try again" 
fi
	;;
	*)
	echo "Please use prep-ext-kit.sh asusware install(upgrade) or prep-ext-kit.sh entware install(upgrade) and try again"
	logger "Please use prep-ext-kit.sh asusware install(upgrade) or prep-ext-kit.sh entware install(upgrade) and try again"

esac

elif [ "$2" == "upgrade" ]
			then

case $1 in
	asusware)

check_download_server

if [ "$DLSRV" != "" ]
		then
SOURCE_DIR="ftp://"$DLSRV"/pub/ASUS/ASUSWRT_MODDED"


if [ ! -d /jffs/scripts ]
		then
		echo "Please enable jffs on Administartion page and try again"
		logger "Please enable jffs on Administartion page and try again"
		else
		if [ ! -d $ASUSWARE_FOLDER ]
			then
			echo "Please connect USB drive and try again or use prep-ext-kit.sh asusware install"
			logger "Please connect USB drive and try again or use prep-ext-kit.sh asusware install"
			else
			cd $ASUSWARE_FOLDER
			stop_all_usb_apps
			wget $SOURCE_DIR/$MOD_VERSION/$FILENAME
			tar -xvzf $FILENAME
			rm -f $FILENAME
			echo "External web kit for asusware upgraded. please reboot router"
			logger "External web kit for asusware upgraded. please reboot router"
		fi
fi
else
	echo "Please check Internet connection and try again" 
	logger "Please check Internet connection and try again" 
fi
	;;
	entware)

check_download_server()

if [ "$DLSRV" != "" ]
		then
SOURCE_DIR="ftp://"$DLSRV"/pub/ASUS/ASUSWRT_MODDED"


if [ ! -d /jffs/scripts ]
		then
		echo "Please enable jffs on Administartion page and try again"
		logger "Please enable jffs on Administartion page and try again"
		else
		if [ ! -d $ENTWARE_FOLDER ]
			then
			echo "Please connect USB drive and try again or use prep-ext-kit.sh entware install"
			logger "Please connect USB drive and try again or use prep-ext-kit.sh entware install"
			else
			cd $ENTWARE_FOLDER
			stop_all_usb_apps
			wget $SOURCE_DIR/$MOD_VERSION/$FILENAME
			tar -xvzf $FILENAME
			rm -f $FILENAME
			echo "External web kit for entware upgraded. please reboot router"
			logger "External web kit for entware upgraded. please reboot router"
		fi
fi
else
	echo "Please check Internet connection and try again" 
	logger "Please check Internet connection and try again" 
fi
	;;
	*)
	echo "Please use prep-ext-kit.sh asusware install(upgrade) or prep-ext-kit.sh entware install(upgrade) and try again"
	logger "Please use prep-ext-kit.sh asusware install(upgrade) or prep-ext-kit.sh entware install(upgrade) and try again"

esac


else
	echo "Please use:"
	echo "prep-ext-kit.sh asusware install    - partition and format drive and install asusware and external webkit"
	echo "prep-ext-kit.sh asusware install 128    - partition drive with size in MB and format drive and install asusware and external webkit"
	echo "prep-ext-kit.sh asusware upgrade    - upgrade external webkit with asusware"
	echo "prep-ext-kit.sh entware install    - partition and format drive and install entware and external webkit"
	echo "prep-ext-kit.sh entware install 128    - partition drive with size in MB and format drive and install entware and external webkit"
	echo "prep-ext-kit.sh entware upgrade    - upgrade external webkit with entware"
	logger "Please use:"
	logger "prep-ext-kit.sh asusware install    - partition and format drive and install asusware and external webkit"
	logger "prep-ext-kit.sh asusware install 128    - partition drive with size in MB and format drive and install asusware and external webkit"
	logger "prep-ext-kit.sh asusware upgrade    - upgrade external webkit with asusware"
	logger "prep-ext-kit.sh entware install    - partition and format drive and install entware and external webkit"
	logger "prep-ext-kit.sh entware install 128    - partition drive with size in MB and format drive and install entware and external webkit"
	logger "prep-ext-kit.sh entware upgrade    - upgrade external webkit with entware"
fi
