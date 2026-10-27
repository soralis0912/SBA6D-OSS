#!/bin/sh

# this scripts will write ${product} to rawdata for DA check when download
# don't call this script directly! 

# get product info from software
# It seems more reliable to use ATCMD to query the version
#cur_product=`mipc_wan_cli --at_cmd ATI | grep RG* | awk 'NR==1' | tr -d '-' | sed 's/\n//g' | sed 's/\r//g' `
cur_product=`cat /etc/quectel-project-version | grep "Project Name" | awk '{print $3}' | tr -d '-'` 
cur_endc=`cat /etc/quectel-project-version | grep ENDC | awk '{print $3}'` 

echo "[rawdata] cur product_info: [${cur_product}] [${cur_endc}]" > /dev/kmsg

if [ -z "${cur_endc}" ] || [ -z "${cur_product}" ];then
    echo "[rawdata] invalid para: [${cur_product}] [${cur_endc}]" > /dev/kmsg
    exit 1   
fi

# get product info from rawdata
raw_product=`/usr/bin/rawdata_test -g 5 | grep product | awk '{print $2}'` 
raw_endc=`/usr/bin/rawdata_test -g 5 | grep data | awk '{print $2}'` 
echo "[rawdata] raw product_info: ${raw_product} ${raw_endc}" > /dev/kmsg

if [ ${raw_product} == "[${cur_product}]" ] && [ ${raw_endc} == "[${cur_endc}]" ];then
    echo "[rawdata]No Need to update product_info" > /dev/kmsg
else
    echo "[rawdata]product_info need update" > /dev/kmsg
    state=`/usr/bin/rawdata_test -s 5 -l ${cur_product} -m ${cur_endc}` 
    if [ $? -ne 0 ];then
        echo "[rawdata]update product_info failed" > /dev/kmsg
    else
        echo "[rawdata]update product_info successful" > /dev/kmsg
    fi
fi
