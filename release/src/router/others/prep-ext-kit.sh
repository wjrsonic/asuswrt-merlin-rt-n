#!/bin/sh

DLSRV1="78.25.8.242"
DLSRV2="82.207.89.70"
DLSRV3="85.198.188.40"
DLSRV4="37.229.9.129"


case $1 in
	asusware)
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

if [ "$DLSRV" != "" ]
		then
ASUS_BASE_APPS="http://dlcdnet.asus.com/pub/ASUS/LiveUpdate/Release/Wireless/asus_base_apps.tgz"
SOURCE_DIR="ftp://"$DLSRV"/pub/ASUS/ASUSWRT_MODDED"
MOD_VERSION="mod"`nvram get extendno | sed -n -e 's/^.*-m//p' | sed 's/-webkit.*//'`
FILENAME=`nvram get productid`"_"`nvram get firmver`"_"`nvram get buildno`"_"`nvram get extendno`"-ext.tgz"


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
			cd "/tmp/mnt/"$MOUNTED_PATH
			mkdir -p ./asusware
			cd ./asusware
			wget $ASUS_BASE_APPS
			tar -xvzf asus_base_apps.tgz
			rm -f asus_base_apps.tgz
			wget $SOURCE_DIR/$MOD_VERSION/$FILENAME
			tar -xvzf $FILENAME
			rm -f $FILENAME
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

if [ "$DLSRV" != "" ]
		then
SOURCE_DIR="ftp://"$DLSRV"/pub/ASUS/ASUSWRT_MODDED"
MOD_VERSION="mod7c"
FILENAME=`nvram get productid`"_"`nvram get firmver`"_"`nvram get buildno`"_"`nvram get extendno`"-ext.tgz"


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
			entPartition="/tmp/mnt/"$MOUNTED_PATH
			entFolder=$entPartition"/entware"

			if [ -d $entFolder ]
					then
					echo "Found previous installation, deleting..."
					logger "Found previous installation, deleting..."
					rm -fr $entFolder
					fi

			mkdir $entFolder

			if [ -d /tmp/opt ]
					then
					echo "Deleting old /tmp/opt symlink..."
					logger "Deleting old /tmp/opt symlink..."
					rm /tmp/opt
			fi

			ln -s $entFolder /tmp/opt
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

#			echo "" >> /jffs/scripts/post-mount
#			echo "" >> /jffs/scripts/post-mount
#			echo "if [ \$1 = "__Partition__" ]" >> /jffs/scripts/post-mount
#			echo "then" >> /jffs/scripts/post-mount
#			echo "  ln -sf \$1/entware /tmp/opt" >> /jffs/scripts/post-mount
#			echo "fi" >> /jffs/scripts/post-mount
#			eval sed -i 's,__Partition__,$entPartition,g' /jffs/scripts/post-mount
			wget http://entware.wl500g.info/binaries/entware/installer/entware_install.sh
			sh ./entware_install.sh			
			rm -f ./entware_install.sh	
			wget $SOURCE_DIR/$MOD_VERSION/$FILENAME
			tar -xvzf $FILENAME
			rm -f $FILENAME
			cd "/tmp/mnt/"$MOUNTED_PATH
			ln -sf ./entware ./asusware
			cp -f /rom/.asusrouter /opt
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
	echo "Please use prep-ext-kit.sh asusware or prep-ext-kit.sh entware and try again"
	logger "Please use prep-ext-kit.sh asusware or prep-ext-kit.sh entware and try again"

esac
