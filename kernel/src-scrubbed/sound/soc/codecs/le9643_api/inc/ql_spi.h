/*************************************************************************
	> File Name: ql_spi.h
	> Author: 
	> Mail: 
	> Created Time: 2019年04月02日 星期二 21时42分29秒
 ************************************************************************/
#ifndef __QL_SPI_H__
#define __QL_SPI_H__

#include "vp_api_types.h"
#if 1
#define SPI_CPHA        0x01
#define SPI_CPOL        0x02
#define SPI_MODE_0      (0|0)
#define SPI_MODE_1      (0|SPI_CPHA)
#define SPI_MODE_2      (SPI_CPOL|0)
#define SPI_MODE_3      (SPI_CPOL|SPI_CPHA)
#endif
/* Read / Write of the SPI mode field */ 
#define SPI_IOC_RD_MODE32       _IOR(SPI_IOC_MAGIC, 5, __u32)
#define SPI_IOC_WR_MODE32       _IOW(SPI_IOC_MAGIC, 5, __u32) 

typedef enum
{
    SPIMODE0  =   SPI_MODE_0,
    SPIMODE1  =   SPI_MODE_1,
    SPIMODE2  =   SPI_MODE_2,
    SPIMODE3  =   SPI_MODE_3,
}SPI_MODE;

/**************************************************************************

SPI default support frequency

**************************************************************************/
typedef enum
{
    S_960K      =   960000,
    S_4_8M      =   4800000,
    S_9_6M      =   9600000,
    S_16M       =   16000000,
    S_19_2M     =   19200000,
    S_25M       =   25000000,
    S_50M       =   50000000,
}SPI_SPEED;

/**************************************************************************
dev_name:need to insmod spidev.ko or quec_spi_chn.ko, The corresponding device can be found in the "/dev/" directory.
mode:the default is mode0
    CPOL:when spi work in idles,the level of sclk, 1:HIGH,0:LOW
    CHPA:sample on the first few edges of sclk     0:the first edge 1:the second edge
bits:bits of word, Supports 4 to 32 bits per word of transfer
speed:the max spi speed
return:
    spi_fd when successfully
    -1 when failed
**************************************************************************/
int Ql_SPI_Init(char *dev_name,SPI_MODE mode,uint8 bits,SPI_SPEED speed);


/**************************************************************************
function: write and read spi data
para:
    fd:spi file description
    write_buf: the pointer to write data
    read_buf: the pointer to read data
    len:the length of data
return:
    0 when successfully
    negative value when failed
**************************************************************************/
int Ql_SPI_Write_Read(int fd,uint8* write_buf,uint8* read_buf,uint32 len);

/**************************************************************************
close spi device fd
**************************************************************************/
int Ql_SPI_DeInit(int fd);

#endif

