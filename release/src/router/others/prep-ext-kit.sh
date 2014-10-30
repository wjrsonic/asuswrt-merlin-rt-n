#!/bin/sh

DLSRV1="78.25.8.242"
DLSRV2="82.207.89.70"
DLSRV3="85.198.188.40"
DLSRV4="37.229.9.129"


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
		DLSRV=""
fi

if [ "$DLSRV" != "" ]
		then
ASUS_BASE_APPS="http://dlcdnet.asus.com/pub/ASUS/LiveUpdate/Release/Wireless/asus_base_apps.tgz"
SOURCE_DIR="ftp://"$DLSRV"/pub/ASUS/ASUSWRT_MODDED"
MOD_VERSION="mod7b1"
FILENAME=`nvram get productid`"_"`nvram get firmver`"_"`nvram get buildno`"_"`nvram get extendno`"-ext.tgz"

if [ ! -d /jffs/scripts ]
		then
		echo "Please enable jffs on Administartion page and try again"
		else
		MOUNTED_PATH=`ls /tmp/mnt`
		if [ "$MOUNTED_PATH" == "" ]
			then
			echo "Please connect USB drive and try again"
			else
			cd "/tmp/mnt/"$MOUNTED_PATH
			mkdir ./asusware
			cd ./asusware
			wget $ASUS_BASE_APPS
			tar -xvzf asus_base_apps.tgz
			rm -f asus_base_apps.tgz
			wget $SOURCE_DIR/$MOD_VERSION/$FILENAME
			tar -xvzf $FILENAME
			rm -f $FILENAME
			echo "Asusware and External web kit installed. please reboot router"
		fi
fi
else
	echo "Please check Internet connection and try again" 
fi
