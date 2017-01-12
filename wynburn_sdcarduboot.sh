#!/bin/sh

#if [ $# -lt 1 ] ; then 
#echo "input arguments"
#return
#fi

if [ -e /dev/sdb ];then
sudo dd if=SPL of=/dev/sdb bs=512 seek=2
sudo dd if=u-boot.img of=/dev/sdb bs=1024 seek=64
sudo sync
fi
