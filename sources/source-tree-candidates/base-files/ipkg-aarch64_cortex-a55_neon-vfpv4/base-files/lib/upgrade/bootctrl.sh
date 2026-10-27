
PARTITION_PRELOADER_A=preloader_a
PARTITION_PRELOADER_B=preloader_b
PARTITION_PRELOADER_BACKUP=preloader_backup
PARTITION_MCF_1_A=mcf1_a
PARTITION_MCF_1_B=mcf1_b
PARTITION_MCF_2_A=mcf2_a
PARTITION_MCF_2_B=mcf2_b
PARTITION_MODEM_A=md1img_a
PARTITION_MODEM_B=md1img_b
PARTITION_DSP_A=md1dsp_a
PARTITION_DSP_B=md1dsp_b
PARTITION_SPMFW_A=spmfw_a
PARTITION_SPMFW_B=spmfw_b
PARTITION_PI_A=pi_img_a
PARTITION_PI_B=pi_img_b
PARTITION_DPM_A=dpm_a
PARTITION_DPM_B=dpm_b
PARTITION_MEDMCU_A=medmcu_a
PARTITION_MEDMCU_B=medmcu_b
PARTITION_SSPM_A=sspm_a
PARTITION_SSPM_B=sspm_b
PARTITION_MCUPM_A=mcupm_a
PARTITION_MCUPM_B=mcupm_b
PARTITION_LK_A=lk_a
PARTITION_LK_B=lk_b
PARTITION_TEE_A=tee_a
PARTITION_TEE_B=tee_b
PARTITION_BOOT_A=boot_a
PARTITION_BOOT_B=boot_b
PARTITION_ROOTFS_A=rootfs_a
PARTITION_ROOTFS_B=rootfs_b
PARTITION_CONNSYS_GNSS_A=connsys_gnss_a
PARTITION_CONNSYS_GNSS_B=connsys_gnss_b

PARTITION_LIST_A="$PARTITION_PRELOADER_A $PARTITION_MCF_1_A $PARTITION_MCF_2_A $PARTITION_MODEM_A $PARTITION_DSP_A $PARTITION_SPMFW_A $PARTITION_PI_A $PARTITION_DPM_A $PARTITION_MEDMCU_A $PARTITION_SSPM_A $PARTITION_MCUPM_A $PARTITION_LK_A $PARTITION_TEE_A $PARTITION_BOOT_A $PARTITION_ROOTFS_A $PARTITION_CONNSYS_GNSS_A"
PARTITION_LIST_B="$PARTITION_PRELOADER_B $PARTITION_MCF_1_B $PARTITION_MCF_2_B $PARTITION_MODEM_B $PARTITION_DSP_B $PARTITION_SPMFW_B $PARTITION_PI_B $PARTITION_DPM_B $PARTITION_MEDMCU_B $PARTITION_SSPM_B $PARTITION_MCUPM_B $PARTITION_LK_B $PARTITION_TEE_B $PARTITION_BOOT_B $PARTITION_ROOTFS_B $PARTITION_CONNSYS_GNSS_B"
PARTITION_LIST_UPDATED=

DEV_PRELOADER=/dev/mmcblk0boot0
DEV_PRELOADER_A=/dev/mmcblk0boot0
DEV_PRELOADER_B=/dev/mmcblk0boot1

BOOTCTRL_DEV=
BOOTCTRL_OFFSET=2048
BOOTCTRL_OFFSET_MAGIC=2052
BOOTCTRL_OFFSET_PRIORITY_A=$((BOOTCTRL_OFFSET+12))
BOOTCTRL_OFFSET_TRY_A=$((BOOTCTRL_OFFSET+13))
BOOTCTRL_OFFSET_SUCCESS_A=$((BOOTCTRL_OFFSET+14))
BOOTCTRL_OFFSET_UP_A=$((BOOTCTRL_OFFSET+15))
BOOTCTRL_OFFSET_PRIORITY_B=$((BOOTCTRL_OFFSET+17))
BOOTCTRL_OFFSET_TRY_B=$((BOOTCTRL_OFFSET+18))
BOOTCTRL_OFFSET_SUCCESS_B=$((BOOTCTRL_OFFSET+19))
BOOTCTRL_OFFSET_UP_B=$((BOOTCTRL_OFFSET+20))
BOOTCTRL_READ_SIZE=2080

BOOTCTRL_MAX_PRI=0f
BOOTCTRL_MAX_PRI_MINUS_ONE=0e
BOOTCTRL_MAX_RETRY=03

get_bootctrl() {
	[ -z "$BOOTCTRL_DEV" ] && BOOTCTRL_DEV=$(get_partition_dev misc)
	if [ "$FLASH_TYPE" = "nand" ]; then
		nanddump --skip-bad-blocks-to-start -l $BOOTCTRL_READ_SIZE -f /tmp/miscout $BOOTCTRL_DEV 2>/dev/null
		echo $(hexdump -v -s $1 -n $2 -e '1/1 "%02x"' /tmp/miscout)
		rm /tmp/miscout 2>/dev/null
	elif [ "$FLASH_TYPE" = "emmc" ]; then
		echo $(hexdump -v -s $1 -n $2 -e '1/1 "%02x"' $BOOTCTRL_DEV)
	fi
}

set_bootctrl() {
	[ -z "$BOOTCTRL_DEV" ] && BOOTCTRL_DEV=$(get_partition_dev misc)
	if [ "$FLASH_TYPE" = "nand" ]; then
		echo -n -e $2 | dd-nand of=$BOOTCTRL_DEV seek=$1 bs=1 count=1 2>/dev/null
	elif [ "$FLASH_TYPE" = "emmc" ]; then
		echo -n -e $2 | dd of=$BOOTCTRL_DEV seek=$1 bs=1 count=1 2>/dev/null
	fi
}

get_bootctrl_string() {
	#gets 10 important bytes of bootctrl
	[ -z "$BOOTCTRL_DEV" ] && BOOTCTRL_DEV=$(get_partition_dev misc)
	if [ "$FLASH_TYPE" = "nand" ]; then
		nanddump --skip-bad-blocks-to-start -l $BOOTCTRL_READ_SIZE -f /tmp/miscout $BOOTCTRL_DEV 2>/dev/null
		echo $(hexdump -v -s $BOOTCTRL_OFFSET_PRIORITY_A -n 10 -e '1/1 "%02x"' /tmp/miscout)
		rm /tmp/miscout 2>/dev/null
	elif [ "$FLASH_TYPE" = "emmc" ]; then
		echo $(hexdump -v -s $BOOTCTRL_OFFSET_PRIORITY_A -n 10 -e '1/1 "%02x"' $BOOTCTRL_DEV)
	fi
}

write_bootctrl_string() {
	[ -z "$BOOTCTRL_DEV" ] && BOOTCTRL_DEV=$(get_partition_dev misc)
	if [ "$FLASH_TYPE" = "nand" ]; then
		echo -n -e $1 | dd-nand of=$BOOTCTRL_DEV seek=$BOOTCTRL_OFFSET_PRIORITY_A bs=1 count=10 > /dev/null 2>&1
		#2>&1
	elif [ "$FLASH_TYPE" = "emmc" ]; then
		echo -n -e $1 | dd of=$BOOTCTRL_DEV seek=$BOOTCTRL_OFFSET_PRIORITY_A bs=1 count=10 > /dev/null 2>&1
	fi
	return $?
}

update_bootctrl_string() {
	# $1 is input string
	# $2 is offset within string to set
	# $3 is set/not-set
	# $4 is value to set
	if [ "$3" -eq 1 ]; then
		myboot=$(echo "$1" | sed 's/\(.\{'"$2"'\}\)\([0-9a-f]\{2\}\)\(.*\)/\1'"$4"'\3/')
		echo "$myboot"
	else
		echo "$myboot"
	fi
}

set_bootctrl_string() {
	# set must be 0 or 1. value should be two chars 0-9a-f hex. 00 - ff
	# $1 is set A priority $2 is A priority value
	# $3 is set A retry    $4 is A retry value
	# $5 is set A success  $6 is A success value
	# $7 is set A uptype   $8 is A uptype value
	# $9 is set B priority $10 is B priority value
	# $11 is set B retry    $12 is B retry value
	# $13 is set B success  $14 is B success value
	# $15 is set B uptype   $16 is B uptype value
	[ -z "$BOOTCTRL_DEV" ] && BOOTCTRL_DEV=$(get_partition_dev misc)
	local bootctrl=$(get_bootctrl_string)
	echo "current $bootctrl"
	# first replace bytes that we want to modify
	# then insert the \x before each byte
	local myboot=$(echo "$bootctrl")
	for i in 1 2 3 4 5 6 7 8; do {
		case "$i" in
			1)	offset=0
				set=$1
				val=$2;;
			2)	offset=2
				set=$3
				val=$4;;
			3)	offset=4
				set=$5
				val=$6;;
			4)	offset=6
				set=$7
				val=$8;;
			5)	offset=10
				set=$9
				val=${10};;
			6)	offset=12
				set=${11}
				val=${12};;
			7)	offset=14
				set=${13}
				val=${14};;
			8)	offset=16
				set=${15}
				val=${16};;
		esac
		myboot=$(update_bootctrl_string $myboot $offset $set $val)
	}; done
	echo "newstrg $myboot"

	for offset in 18 16 14 12 10 8 6 4 2 0; do {
		myboot=$(echo "$myboot" | sed -e 's#\(.\{'"$offset"'\}\)\(.*\)#\1\\x\2#g')
	}; done

	write_bootctrl_string $myboot
	writerc=$?
	if [ $writerc -ne 0 ]; then
		echo "writerc $writerc is not 0"
	fi
}

is_ab_partition() {
	local magic=$(get_bootctrl $BOOTCTRL_OFFSET_MAGIC 4)
	if [ $((0x${magic})) -eq $((0x42434142)) ]; then
		echo "1"
	else
		echo "0"
	fi
}

change_slot_info() {
	if [ "$1" = "_a" ]; then
		local uptype_a=$(get_bootctrl $BOOTCTRL_OFFSET_UP_A 1)
		if [ $((0x${uptype_a})) -eq $((0x04)) ]; then
			if [ "$2" -eq 0 ]; then
				set_bootctrl_string 1 $BOOTCTRL_MAX_PRI_MINUS_ONE 0 00 0 00 1 01 1 $BOOTCTRL_MAX_PRI 1 00 1 00 1 00
			else
				set_bootctrl_string 1 $BOOTCTRL_MAX_PRI_MINUS_ONE 0 00 0 00 1 01 1 $BOOTCTRL_MAX_PRI 1 00 1 00 1 01
			fi
		elif [ $((0x${uptype_a})) -eq $((0x02)) ]; then
			if [ "$2" -eq 0 ]; then
				set_bootctrl_string 1 $BOOTCTRL_MAX_PRI_MINUS_ONE 0 00 0 00 1 00 1 $BOOTCTRL_MAX_PRI 1 00 1 00 1 00
			else
				set_bootctrl_string 1 $BOOTCTRL_MAX_PRI_MINUS_ONE 0 00 0 00 1 00 1 $BOOTCTRL_MAX_PRI 1 00 1 00 1 01
			fi
		else
			if [ "$2" -eq 0 ]; then
				set_bootctrl_string 1 $BOOTCTRL_MAX_PRI_MINUS_ONE 0 00 0 00 0 00 1 $BOOTCTRL_MAX_PRI 1 00 1 00 1 00
			else
				set_bootctrl_string 1 $BOOTCTRL_MAX_PRI_MINUS_ONE 0 00 0 00 0 00 1 $BOOTCTRL_MAX_PRI 1 00 1 00 1 01
			fi
		fi
		#set_bootctrl $BOOTCTRL_OFFSET_PRIORITY_A \\x0e
		#set_bootctrl $BOOTCTRL_OFFSET_PRIORITY_B \\x0f
		#set_bootctrl $BOOTCTRL_OFFSET_TRY_B \\x03
		#set_bootctrl $BOOTCTRL_OFFSET_SUCCESS_B \\x00
		#if [ "$2" -eq 0 ]; then
		#	set_bootctrl $BOOTCTRL_OFFSET_UP_B \\x00
		#else
		#	set_bootctrl $BOOTCTRL_OFFSET_UP_B \\x01
		#fi
		#if [ $((0x${uptype_a})) -eq $((0x04)) ]; then
		#	set_bootctrl $BOOTCTRL_OFFSET_UP_A \\x01
		#elif [ $((0x${uptype_a})) -eq $((0x02)) ]; then
		#	set_bootctrl $BOOTCTRL_OFFSET_UP_A \\x00
		#fi
	elif [ "$1" = "_b" ]; then
		local uptype_b=$(get_bootctrl $BOOTCTRL_OFFSET_UP_B 1)
		if [ $((0x${uptype_b})) -eq $((0x04)) ]; then
			if [ "$2" -eq 0 ]; then
				set_bootctrl_string 1 $BOOTCTRL_MAX_PRI 1 00 1 00 1 00 1 $BOOTCTRL_MAX_PRI_MINUS_ONE 0 00 0 00 1 01
			else
				set_bootctrl_string 1 $BOOTCTRL_MAX_PRI 1 00 1 00 1 01 1 $BOOTCTRL_MAX_PRI_MINUS_ONE 0 00 0 00 1 01
			fi
		elif [ $((0x${uptype_b})) -eq $((0x02)) ]; then
			if [ "$2" -eq 0 ]; then
				set_bootctrl_string 1 $BOOTCTRL_MAX_PRI 1 00 1 00 1 00 1 $BOOTCTRL_MAX_PRI_MINUS_ONE 0 00 0 00 1 00
			else
				set_bootctrl_string 1 $BOOTCTRL_MAX_PRI 1 00 1 00 1 01 1 $BOOTCTRL_MAX_PRI_MINUS_ONE 0 00 0 00 1 00
			fi
		else
			if [ "$2" -eq 0 ]; then
				set_bootctrl_string 1 $BOOTCTRL_MAX_PRI 1 00 1 00 1 00 1 $BOOTCTRL_MAX_PRI_MINUS_ONE 0 00 0 00 0 00
			else
				set_bootctrl_string 1 $BOOTCTRL_MAX_PRI 1 00 1 00 1 01 1 $BOOTCTRL_MAX_PRI_MINUS_ONE 0 00 0 00 0 00
			fi
		fi
		#set_bootctrl $BOOTCTRL_OFFSET_PRIORITY_A \\x0f
		#set_bootctrl $BOOTCTRL_OFFSET_PRIORITY_B \\x0e
		#set_bootctrl $BOOTCTRL_OFFSET_TRY_A \\x03
		#set_bootctrl $BOOTCTRL_OFFSET_SUCCESS_A \\x00
		#if [ "$2" -eq 0 ]; then
		#	set_bootctrl $BOOTCTRL_OFFSET_UP_A \\x00
		#else
		#	set_bootctrl $BOOTCTRL_OFFSET_UP_A \\x01
		#fi
		#if [ $((0x${uptype_b})) -eq $((0x04)) ]; then
		#	set_bootctrl $BOOTCTRL_OFFSET_UP_B \\x01
		#elif [ $((0x${uptype_b})) -eq $((0x02)) ]; then
		#	set_bootctrl $BOOTCTRL_OFFSET_UP_B \\x00
		#fi
	else
		echo "change_slot_info error: $1"
	fi
}

get_current_slot() {
	local priority_a=$(get_bootctrl $BOOTCTRL_OFFSET_PRIORITY_A 1)
	local priority_b=$(get_bootctrl $BOOTCTRL_OFFSET_PRIORITY_B 1)
	if [ $((0x${priority_a})) -gt $((0x${priority_b})) ]; then
		echo "_a"
	elif [ $((0x${priority_b})) -gt $((0x${priority_a})) ]; then
		echo "_b"
	elif [ $((0x${priority_a})) -eq $((0x${priority_b})) ]; then
		echo "SAME A/B PRIORITY"
	fi
}

get_preloader_dev() {
	if [ "$1" = "$PARTITION_PRELOADER" ]; then
		echo "$DEV_PRELOADER"
	elif [ "$1" = "$PARTITION_PRELOADER_A" ]; then
		echo "$DEV_PRELOADER_A"
	elif [ "$1" = "$PARTITION_PRELOADER_B" ]; then
		echo "$DEV_PRELOADER_B"
	else
		echo "Error preloader partition"
	fi
}

flash_preloader_header() {
	if [ "$1" = "$PARTITION_PRELOADER_A" ]; then
		echo 0 > /sys/block/mmcblk0boot0/force_ro
		dd if=$DEV_PRELOADER_B of=$DEV_PRELOADER_A count=$OFFSET_PRELOADER bs=1
	elif [ "$1" = "$PARTITION_PRELOADER_B" ]; then
		echo 0 > /sys/block/mmcblk0boot1/force_ro
		dd if=$DEV_PRELOADER_A of=$DEV_PRELOADER_B count=$OFFSET_PRELOADER bs=1
	fi
}

string_ab_switch() {
	local str=$1
	str=${str/_a/+b}
	str=${str/_b/+a}
	str=${str/+b/_b}
	str=${str/+a/_a}
	echo "$str"
}

clone_image_adapter() {
	v "clone_image_adapter, enter"
	local dst_partition=$1
	local src_partition=$(string_ab_switch $1)
	if [ "$FLASH_TYPE" = "nand" ]; then
		if [ "$dst_partition" = "$PARTITION_PRELOADER_A" ] || [ "$dst_partition" = "$PARTITION_PRELOADER_B" ]; then
			echo "Skip clone preloader"
		else
			local src_dev=$(get_partition_dev $src_partition)
			echo "Cloning $src_partition ($src_dev) to $dst_partition partition"
			nanddump --skip-bad-blocks-to-start $src_dev | mtd write - $dst_partition
		fi
	elif [ "$FLASH_TYPE" = "emmc" ]; then
		local dst_dev=$(get_partition_dev $dst_partition)
		local src_dev=$(get_partition_dev $src_partition)
		echo "Cloning $src_dev to $dst_dev"
		if [ "$dst_partition" = "$PARTITION_PRELOADER_A" ]; then
			echo 0 > /sys/block/mmcblk0boot0/force_ro
		elif [ "$dst_partition" = "$PARTITION_PRELOADER_B" ]; then
			echo 0 > /sys/block/mmcblk0boot1/force_ro
		fi
		dd if=$src_dev of=$dst_dev
	fi
	v "clone_image_adapter, leave"
}

clone_non_update_images() {
	v "clone_non_update_images, enter"
	local partition_list=
	if [ "$1" = "A" ]; then
		partition_list=$PARTITION_LIST_A
	elif [ "$1" = "B" ]; then
		partition_list=$PARTITION_LIST_B
	fi
	for partition in $partition_list; do {
		local updated=0
		for updated_partition in $PARTITION_LIST_UPDATED; do {
			if [ "$partition" = "$updated_partition" ]; then
				updated=1
				break
			fi
		}; done
		if [ "$updated" = "0" ]; then
			clone_image_adapter $partition
		fi
	}; done
	v "clone_non_update_images, leave"
}

do_upgrade_ab() {
	v "do_upgrade_ab, enter"
	local image_list=$(tar -tzf $1 | awk 1 ORS=' ')
	echo "$1 has image: $image_list"
	local current_slot=$(get_current_slot)
	echo "current_slot=$current_slot"
	if [ "$current_slot" = "_a" ]; then
		set_bootctrl_string 0 00 0 00 0 00 1 02 0 00 0 00 1 00 0 00
		#set_bootctrl $BOOTCTRL_OFFSET_UP_A \\x02
		#set_bootctrl $BOOTCTRL_OFFSET_SUCCESS_B \\x00
		for image_name in $image_list; do {
			case "$image_name" in
				$IMAGE_PRELOADER) set_bootctrl $BOOTCTRL_OFFSET_UP_A \\x04
					flash_image_adapter $1 $image_name $PARTITION_PRELOADER_B $OFFSET_PRELOADER;;
				$IMAGE_MCF_1) flash_image_adapter $1 $image_name $PARTITION_MCF_1_B;;
				$IMAGE_MCF_2) flash_image_adapter $1 $image_name $PARTITION_MCF_2_B;;
				$IMAGE_MODEM) flash_image_adapter $1 $image_name $PARTITION_MODEM_B;;
				$IMAGE_DSP) flash_image_adapter $1 $image_name $PARTITION_DSP_B;;
				$IMAGE_SPMFW) flash_image_adapter $1 $image_name $PARTITION_SPMFW_B;;
				$IMAGE_PI) flash_image_adapter $1 $image_name $PARTITION_PI_B;;
				$IMAGE_DPM) flash_image_adapter $1 $image_name $PARTITION_DPM_B;;
				$IMAGE_MEDMCU) flash_image_adapter $1 $image_name $PARTITION_MEDMCU_B;;
				$IMAGE_SSPM) flash_image_adapter $1 $image_name $PARTITION_SSPM_B;;
				$IMAGE_MCUPM) flash_image_adapter $1 $image_name $PARTITION_MCUPM_B;;
				$IMAGE_LK) flash_image_adapter $1 $image_name $PARTITION_LK_B;;
				$IMAGE_TEE) flash_image_adapter $1 $image_name $PARTITION_TEE_B;;
				$IMAGE_BOOT) flash_image_adapter $1 $image_name $PARTITION_BOOT_B;;
				$IMAGE_ROOTFS) flash_image_adapter $1 $image_name $PARTITION_ROOTFS_B;;
				$IMAGE_CONNSYS_GNSS) flash_image_adapter $1 $image_name $PARTITION_CONNSYS_GNSS_B;;
				*) echo "Invalid image name: $image_name";;
			esac
		}; done
		clone_non_update_images B
	elif [ "$current_slot" = "_b" ]; then
		set_bootctrl_string 0 00 0 00 1 00 0 00 0 00 0 00 0 00 1 02
		#set_bootctrl $BOOTCTRL_OFFSET_UP_B \\x02;
		#set_bootctrl $BOOTCTRL_OFFSET_SUCCESS_A \\x00
		for image_name in $image_list; do {
			case "$image_name" in
				$IMAGE_PRELOADER) set_bootctrl $BOOTCTRL_OFFSET_UP_B \\x04;
					flash_image_adapter $1 $image_name $PARTITION_PRELOADER_A $OFFSET_PRELOADER;;
				$IMAGE_MCF_1) flash_image_adapter $1 $image_name $PARTITION_MCF_1_A;;
				$IMAGE_MCF_2) flash_image_adapter $1 $image_name $PARTITION_MCF_2_A;;
				$IMAGE_MODEM) flash_image_adapter $1 $image_name $PARTITION_MODEM_A;;
				$IMAGE_DSP) flash_image_adapter $1 $image_name $PARTITION_DSP_A;;
				$IMAGE_SPMFW) flash_image_adapter $1 $image_name $PARTITION_SPMFW_A;;
				$IMAGE_PI) flash_image_adapter $1 $image_name $PARTITION_PI_A;;
				$IMAGE_DPM) flash_image_adapter $1 $image_name $PARTITION_DPM_A;;
				$IMAGE_MEDMCU) flash_image_adapter $1 $image_name $PARTITION_MEDMCU_A;;
				$IMAGE_SSPM) flash_image_adapter $1 $image_name $PARTITION_SSPM_A;;
				$IMAGE_MCUPM) flash_image_adapter $1 $image_name $PARTITION_MCUPM_A;;
				$IMAGE_LK) flash_image_adapter $1 $image_name $PARTITION_LK_A;;
				$IMAGE_TEE) flash_image_adapter $1 $image_name $PARTITION_TEE_A;;
				$IMAGE_BOOT) flash_image_adapter $1 $image_name $PARTITION_BOOT_A;;
				$IMAGE_ROOTFS) flash_image_adapter $1 $image_name $PARTITION_ROOTFS_A;;
				$IMAGE_CONNSYS_GNSS) flash_image_adapter $1 $image_name $PARTITION_CONNSYS_GNSS_A;;
				*) echo "Invalid image name: $image_name";;
			esac
		}; done
		clone_non_update_images A
	else
		v "Slot Error: $current_slot"
		return 0
	fi
	local uppl=0
	for image_name in $image_list; do {
		case "$image_name" in
			$IMAGE_PRELOADER) uppl=1;;
		esac
	}; done
	change_slot_info $current_slot $uppl
	v "do_upgrade_ab, leave"
	return 0
}

