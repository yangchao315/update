#!/bin/sh

readonly size_1K=1024
readonly size_1M=$((1024*1024))

readonly ostype=$1
if test -e /dev/nandblk0; then
	devpath=/dev/nandblk0
	readonly sector_size=$(cat /sys/block/nandblk0/queue/hw_sector_size)
	echo "[check-and-make-partitions]nand boot media ..."
else
	if test -e /dev/mmcblk0; then
		devpath=/dev/mmcblk0
		readonly sector_size=$(cat /sys/block/mmcblk0/queue/hw_sector_size)
		echo "[check-and-make-partitions]mmc boot media ..."
	else
		echo "[check-and-make-partitions]unknow boot media"
		exit
	fi
fi

if [ -e /dev/sda1 ];then
echo "[check-and-make-partitions]/dev/sda1 exist"
mkdir -p /udisk
sleep 1
if [ -e /udisk ];then
mount -t vfat /dev/sda1	/udisk
fi
fi

readonly uboot_env_length=$((64*$size_1K))
# uboot environment start sector
readonly uboot_env_sector=$(((3*$size_1M-64*$size_1K)/$sector_size))

readonly rawdev=${devpath}p1 #raw partition
readonly raw_part_size=5
readonly bootdev=${devpath}p2 # boot partition
if [ "${ostype}" = "android" ]; then
	swapdev=${devpath}p3 # swap partition for hibernation
	rootdev=${devpath}p5 # rootfs partition (android)
	systdev=${devpath}p6 # system partition (android)
	datadev=${devpath}p7 # data partition (android)
	userdev=${devpath}p8 # user partition
	boot_part_size=30
	root_part_size=256
	syst_part_size=256
	data_part_size=1024
	swap_part_size=160
fi
if [ "${ostype}" = "linux" ]; then
	#swapdev=${devpath}p3 # swap partition for hibernation
	rootdev=${devpath}p3 # root partition
	datadev=${devpath}p5 # data partition
	userdev=${devpath}p6 # user partition
	boot_part_size=30
	#modify by ycw 20160720 to 350M
	root_part_size=350
	#end
	data_part_size=160
fi
rootfs_type=ext4

have_valid_files()
{
	if [ "${ostype}" = "linux" ]; then
		if [ -e /udisk/update/u-boot.csr ]; then
			echo "[check-and-make-partitions] u-boot.csr exits"			
			mkdir /tmp/uboot.flg
		fi 
		
		if [ -e /udisk/update/zImage ]; then
			echo "[check-and-make-partitions] zImage exits"
			mkdir /tmp/zImage.flg
		fi
		
		if [ -e /udisk/update/freertos.bin ]; then
			echo "[check-and-make-partitions] freertos.bin exits"
			mkdir /tmp/freertos.flg	
		fi
		
		if [ -e /udisk/update/root_flg ]; then
			echo "[check-and-make-partitions] root exits"
			mkdir /tmp/root.flg	
		fi
		
	fi
echo "have valid files done"
}

have_valid_partitions()
{
	if [ "${ostype}" = "android" ]; then
		for i in $rawdev $bootdev $rootdev $systdev $datadev $swapdev $userdev;  do
			if [ ! -e $i ]; then
				echo no
				return
			fi
		done
	fi

	if [ "${ostype}" = "linux" ]; then
		# added by ycw to check format partition or not
		have_valid_files
		if [ -e /udisk/update/rcv_fmt.flg ];then
			echo no
			umount /udisk
			return		
		fi
		#end
		
		# modify by ycw 20160903 to remove userdev check
		#for i in $rawdev $bootdev $rootdev $datadev $userdev;  do
		for i in $rawdev $bootdev $rootdev $datadev;  do
			if [ ! -e $i ]; then
				echo no
				return
			fi
		done
	fi

	echo yes
}

#
# create_partition(part_type, part_num, part_size)
#
create_partition()
{
	local part_type=$1
	local part_num=$2
	local part_size=$3

	if [ "$part_size" = "" ]; then
		show_size=+all
	else
		show_size=$part_size
	fi

	echo -n "create_partition($part_type, $part_num, $show_size)..."

	case $part_type in
	p|e|l);;
	*)
		echo $part_type not supported!;;
	esac

	case $part_type in
	p)
		fdisk $devpath << EOF
		n
		$part_type
		$part_num

		$part_size
		wq
EOF
		;;
	e)
		fdisk $devpath << EOF
		n
		$part_type


		wq
EOF
		;;
	l)
		fdisk $devpath << EOF
		n

		$part_size
		wq
EOF
		;;
	esac

	echo "done"
}

#
# erase_sector(start, count)
#
erase_sector()
{
	echo "erase_sector\($1, $2\)"
	dd seek=$1 count=$2 bs=$sector_size if=/dev/zero of=$devpath 1>/dev/null 2>&1
}

#
# format_partition(part_num, fs_type, label)
#
format_partition()
{
	local part_num=$1
	local fs_type=$2
	local label=$3

	echo -n "format_partition($part_num, $fs_type)..."

	for i in {1..5}; do
		if test -b ${devpath}p${part_num}; then
			break
		fi
		sleep 1
	done

	case $fs_type in
	fat32)
		if mkfs.vfat -F 32 ${devpath}p${part_num} -n $label 1>/dev/null 2>/dev/null; then
			echo done
		else
			echo failed!
		fi
		;;
	swap)
		if mkswap ${devpath}p${part_num}  1>/dev/null 2>/dev/null; then
			echo done
		else
			echo failed!
		fi
		;;
	*)
		if mke2fs -T $fs_type -L $label ${devpath}p${part_num} 1>/dev/null 2>&1; then
			echo done
		else
			echo failed!
		fi
		;;
	esac
}

if [ "`have_valid_partitions`" = "no" ]; then
	echo "remove existed partitions ..."
	fdisk $devpath 1>/dev/null 2>&1 << EOF
	d
	1
	d
	2
	d
	3
	d
	4
	wq
EOF
	echo "done"
	if [ "${ostype}" = "android" ]; then
		create_partition p 1 +${raw_part_size}M
		create_partition p 2 +${boot_part_size}M
		create_partition p 3 +${swap_part_size}M

		create_partition e 4
		create_partition l 5 +${root_part_size}M
		create_partition l 6 +${syst_part_size}M
		create_partition l 7 +${data_part_size}M
		create_partition l 8

		format_partition 2 $rootfs_type boot
		format_partition 3 swap
		format_partition 5 $rootfs_type rootfs
		format_partition 6 $rootfs_type system
		format_partition 7 $rootfs_type data
		format_partition 8 fat32 user
	fi

	if [ "${ostype}" = "linux" ]; then
		create_partition p 1 +${raw_part_size}M
		create_partition p 2 +${boot_part_size}M
		create_partition p 3 +${root_part_size}M

		create_partition e 4
		#modify by ycw 20160831
		create_partition l 5
		#create_partition l 5 +${data_part_size}M
		#create_partition l 6
		sleep 3
		erase_sector $uboot_env_sector $(($uboot_env_length/$sector_size))
		format_partition 2 $rootfs_type boot
		format_partition 3 $rootfs_type root
		format_partition 5 $rootfs_type data
		#format_partition 6 fat32 user
		#end
	fi
else
	
	echo "[check-and-make-partitions]partitions are existed"
	
fi

