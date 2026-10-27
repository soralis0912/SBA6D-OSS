#!/bin/sh

rm -f /tmp/mtk_dut.txt
killall mtk_dut

wifi_detect=5
all_system_detect=30
count=0
inf1_up=0
inf2_up=0
inf3_up=0
while (true); do
	if [ $count -ge $all_system_detect ]; then
		echo "Detect too long time for wifi ready ! Stop!" >> /tmp/mtk_dut.txt
		break
	fi

	main_inf=`ifconfig -a | grep ra0 | awk '{print $1}' | sed -n 1p`
	if [ $count -ge $wifi_detect ] && [ -z $main_inf ]; then
		echo "Can't detect wifi up! Stop!" >> /tmp/mtk_dut.txt
		break
	fi

	if [ "$main_inf" = "ra0" ]; then
		echo "$main_inf is up!" >> /tmp/mtk_dut.txt
		inf1_up=1
	fi

	inf2=`ifconfig | grep rai0 | awk '{print $1}' | sed -n 1p`
	if [ "$inf2" = "rai0" ]; then
		echo "$inf2 is up!" >> /tmp/mtk_dut.txt
		inf2_up=1
	fi

	inf3=`ifconfig | grep rax0 | awk '{print $1}' | sed -n 1p`
	if [ "$inf3" = "rax0" ]; then
		echo "$inf3 is up!" >> /tmp/mtk_dut.txt
		inf3_up=1
	fi

	hostapd_status=`hostapd_cli status | grep "state" | cut -d '=' -f2`
	if [ $inf1_up -eq 1 ] && [ $inf2_up -eq 1 ] && [ $inf3_up -eq 1 ] && [ "$hostapd_status" = "ENABLED" ]; then
		echo "All ready, run mtk_dut!" >> /tmp/mtk_dut.txt
		mtk_dut ap br-lan 9000 -s hostapd &
		break
	fi

	sleep 1
	count=$((count + 1))
	echo "Wait $count seconds for hostapd WIFI status up!" >> /tmp/mtk_dut.txt
done
