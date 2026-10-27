
FLASH_TYPE=
IS_AB_PARTITION=

IMAGE_PRELOADER=preloader_*.bin
IMAGE_MCF_1=MCF_OTA_1.img
IMAGE_MCF_2=MCF_OTA_2.img
IMAGE_MODEM=modem*.img
IMAGE_DSP=dsp*.bin
IMAGE_SPMFW=spmfw*.img
IMAGE_PI=pi_img*.img
IMAGE_DPM=dpm*.img
IMAGE_MEDMCU=medmcu*.img
IMAGE_SSPM=sspm*.img
IMAGE_MCUPM=mcupm*.img
IMAGE_LK=lk*.img
IMAGE_TEE=tee*.img
IMAGE_BOOT=boot*.img
IMAGE_ROOTFS=*.squashfs
IMAGE_CONNSYS_GNSS=connsys_gnss*.img

PARTITION_PRELOADER=preloader
PARTITION_MCF_1=mcf1
PARTITION_MCF_2=mcf2
PARTITION_MODEM=md1img
PARTITION_DSP=md1dsp
PARTITION_SPMFW=spmfw
PARTITION_PI=pi_img
PARTITION_DPM=dpm
PARTITION_MEDMCU_1=medmcu_1
PARTITION_SSPM=sspm
PARTITION_MCUPM=mcupm
PARTITION_LK=lk
PARTITION_TEE=tee
PARTITION_BOOT=boot
PARTITION_ROOTFS=rootfs
PARTITION_CONNSYS_GNSS=connsys_gnss

PARTITION_BOOT_PARA=boot_para
PARTITION_USER_DATA=user_data

OFFSET_PRELOADER=2048

get_flash_type() {
	if [ -n "$(cat /proc/partitions 2>/dev/null | grep mtd)" ]; then
		echo "nand"
	elif [ -n "$(cat /proc/partitions 2>/dev/null | grep mmcblk)" ]; then
		echo "emmc"
	fi
}

get_partition_dev() {
	local devtype=
	if [ "$FLASH_TYPE" = "nand" ]; then
		devtype=mtd
	elif [ "$FLASH_TYPE" = "emmc" ]; then
		devtype=block
	fi
	local dev=/dev/$devtype/$1
	if [ "$1" = "$PARTITION_PRELOADER" ] || [ "$1" = "$PARTITION_PRELOADER_A" ] || [ "$1" = "$PARTITION_PRELOADER_B" ]; then
		dev=$(get_preloader_dev $1)
	fi
	echo "$dev"
}

clear_partition_adapter() {
	if [ "$FLASH_TYPE" = "nand" ]; then
		echo "Clearing $1 partition"
		mtd erase $1
	elif [ "$FLASH_TYPE" = "emmc" ]; then
		local partition_dev=$(get_partition_dev $1)
		echo "Clearing $1 partition ($partition_dev)"
		dd if=/dev/zero of=$partition_dev
	fi
}

flash_image_adapter() {
	v "flash_image_adapter, enter"
	if [ "$FLASH_TYPE" = "nand" ]; then
		local offset=${4:-0}
		local partition="$3"
		if [ "$3" = "$PARTITION_PRELOADER" ] || [ "$3" = "$PARTITION_PRELOADER_A" ] || [ "$3" = "$PARTITION_PRELOADER_B" ]; then
			# 256KB
			offset=262144
			partition="$PARTITION_PRELOADER"
			echo "Flashing $2 to $partition partition (offset=$offset)"
			tar -xzf $1 $2 -O | mtd -p $offset write - $partition
		else
			echo "Erasing $partition partition"
			mtd erase $partition
			echo "Flashing $2 to $partition partition (offset=$offset)"
			tar -xzf $1 $2 -O | mtd -p $offset write - $partition
		fi
	elif [ "$FLASH_TYPE" = "emmc" ]; then
		local partition_dev=$(get_partition_dev $3)
		if [ "$3" = "$PARTITION_PRELOADER" ] || [ "$3" = "$PARTITION_PRELOADER_A" ] || [ "$3" = "$PARTITION_PRELOADER_B" ]; then
			if [ "$IS_AB_PARTITION" -eq 1 ]; then
				flash_preloader_header $3
			fi
			echo 0 > /sys/block/mmcblk0boot0/force_ro
			echo "Flashing $2 to $3 partition ($partition_dev, offset=${4:-0})"
			tar -xzf $1 $2 -O | dd of=$partition_dev seek=$4 bs=1
		else
			echo "Flashing $2 to $3 partition ($partition_dev, offset=${4:-0})"
			tar -xzf $1 $2 -O | dd of=$partition_dev
		fi
	fi
	PARTITION_LIST_UPDATED="$PARTITION_LIST_UPDATED $3"
	v "flash_image_adapter, leave"
}

do_upgrade() {
	v "do_upgrade, enter"
	local image_list=$(tar -tzf $1 | awk 1 ORS=' ')
	echo "$1 has image: $image_list"
	for image_name in $image_list; do {
		case "$image_name" in
			$IMAGE_PRELOADER)
				flash_image_adapter $1 $image_name $PARTITION_PRELOADER $OFFSET_PRELOADER;;
			$IMAGE_MCF_1)
				flash_image_adapter $1 $image_name $PARTITION_MCF_1;;
			$IMAGE_MCF_2)
				flash_image_adapter $1 $image_name $PARTITION_MCF_2;;
			$IMAGE_MODEM)
				flash_image_adapter $1 $image_name $PARTITION_MODEM;;
			$IMAGE_DSP)
				flash_image_adapter $1 $image_name $PARTITION_DSP;;
			$IMAGE_SPMFW)
				flash_image_adapter $1 $image_name $PARTITION_SPMFW;;
			$IMAGE_PI)
				flash_image_adapter $1 $image_name $PARTITION_PI;;
			$IMAGE_DPM)
				flash_image_adapter $1 $image_name $PARTITION_DPM;;
			$IMAGE_MEDMCU)
				flash_image_adapter $1 $image_name $PARTITION_MEDMCU_1;;
			$IMAGE_SSPM)
				flash_image_adapter $1 $image_name $PARTITION_SSPM;;
			$IMAGE_MCUPM)
				flash_image_adapter $1 $image_name $PARTITION_MCUPM;;
			$IMAGE_LK)
				flash_image_adapter $1 $image_name $PARTITION_LK;;
			$IMAGE_TEE)
				flash_image_adapter $1 $image_name $PARTITION_TEE;;
			$IMAGE_BOOT)
				flash_image_adapter $1 $image_name $PARTITION_BOOT;;
			$IMAGE_ROOTFS)
				flash_image_adapter $1 $image_name $PARTITION_ROOTFS;;
			$IMAGE_CONNSYS_GNSS)
				flash_image_adapter $1 $image_name $PARTITION_CONNSYS_GNSS;;
			*) echo "Invalid image name: $image_name";;
		esac
	}; done
	v "do_upgrade, leave"
	return 0
}

platform_do_upgrade() {
	v "platform_do_upgrade, enter"
	FLASH_TYPE=$(get_flash_type)
	local bootctrl=$(get_bootctrl $BOOTCTRL_OFFSET 32)
	IS_AB_PARTITION=$(is_ab_partition)
	echo "FLASH_TYPE=$FLASH_TYPE, bootctrl=$bootctrl, IS_AB_PARTITION=$IS_AB_PARTITION"

	if [ "$IS_AB_PARTITION" -eq 1 ]; then
		do_upgrade_ab $1
	else
		do_upgrade $1
	fi
	clear_partition_adapter $PARTITION_BOOT_PARA
	bootctrl=$(get_bootctrl $BOOTCTRL_OFFSET 32)
	echo "bootctrl=$bootctrl"
	v "platform_do_upgrade, leave"
	return 0
}

platform_check_image() {
	v "platform_check_image"
	case "$(get_magic_word $1 cat)" in
		# .gz files
		1f8b) ;;
		*)
			echo "Invalid sysupgrade file."
			return 1
			;;
	esac
	return 0
}

platform_pre_upgrade() {
	v "platform_pre_upgrade"
}

platform_copy_config() {
	v "platform_copy_config, enter"
	echo "UPGRADE_BACKUP=$UPGRADE_BACKUP, BACKUP_FILE=$BACKUP_FILE"
	local backup_list=$(tar -tzf $UPGRADE_BACKUP | awk 1 ORS=' ')
	echo "$UPGRADE_BACKUP: $backup_list"
	if [ "$FLASH_TYPE" = "nand" ]; then
		if [ ! -d /data ]; then
			mkdir -p /data
			mount_user_data
		fi
	elif [ "$FLASH_TYPE" = "emmc" ]; then
		if [ ! -d /data ]; then
			local partition_dev=$(get_partition_dev $PARTITION_USER_DATA)
			mkdir -p /data
			mount -o rw,noatime $partition_dev /data
		fi
	fi
	cp -af "$UPGRADE_BACKUP" "/data/$BACKUP_FILE"
	sync
	if [ ! -f /data/$BACKUP_FILE ]; then
		echo "Fail: copy $UPGRADE_BACKUP to /data/$BACKUP_FILE"
	fi
	v "platform_copy_config, leave"
}
