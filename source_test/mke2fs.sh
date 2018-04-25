#!/bin/bash

#mke2fs -T ext4 -L root2 /dev/mmcblk0p7
if mke2fs -T ext4 -L root2 /dev/mmcblk0p3 1>/dev/null 2>&1; then
		echo done
	else
		echo failed!
	fi
