#ifndef __RTL8251B_H__
#define __RTL8251B_H__

#define QLOGD(f,...)        do{\
            printk("[%s]%s():%d " f " ",NET_TAG,__FUNCTION__,__LINE__,## __VA_ARGS__);}while(0);

#endif