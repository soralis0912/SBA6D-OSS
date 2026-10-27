/*************************************************************************
	> File Name: ql_spi.c
	> Author: 
	> Mail: 
	> Created Time: 2019年04月02日 星期二 21时43分09秒
 ************************************************************************/

#if 0
#include <stdint.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/types.h>
#include <linux/spi/spidev.h>

#include "ql_spi.h"

static uint8_t spi_bits = 8;
static uint32_t  spi_speed = 10000000;
static uint16_t spi_delay;

/**************************************************************************
dev_name:need to insmod spidev.ko or quec_spi_chn.ko
mode:the default is mode0
    CPOL:when spi work in idles,the level of sclk, 1:HIGH,0:LOW
    CHPA:sample on the first few edges of sclk     0:the first edge 1:the second edge
bits:bits of word  
speed:the default support spi frequency  
**************************************************************************/

int Ql_SPI_Init(char *dev_name,SPI_MODE mode,uint8_t bits,SPI_SPEED speed)
{
    int ret = -1;
    int fd_spi = open(dev_name, O_RDWR);
    
    //printf("< open(%s, O_RDWR)=%d >\n", dev_name, fd_spi);
    if (fd_spi < 0)  
    {
        printf("< Fail to open spi >\n");
        return -1;
    }  
    /*
    * spi mode
    */
    ret = ioctl(fd_spi, SPI_IOC_WR_MODE, &mode);
    if (ret == -1)
    {
        perror("can't set spi mode");
        close(fd_spi);
        return ret;
    }
    /*
     * bits per word
     */
    spi_bits = bits;
    ret = ioctl(fd_spi, SPI_IOC_WR_BITS_PER_WORD, &spi_bits);
    if (ret == -1)
    {
        perror("can't set bits per word");
        close(fd_spi);
        return ret;
    }

    /*
    * speed
    */
    spi_speed = (uint32_t)speed;
    ret = ioctl(fd_spi, SPI_IOC_WR_MAX_SPEED_HZ, &spi_speed);
    if (ret == -1)
    {
        perror("can't set max speed hz");
        close(fd_spi);
        return ret;
    }
    //printf("spi mode:0x%x\n", mode);
    //printf("bits per word: %d\n", spi_bits);
    //printf("max speed    : %d Hz (%d KHz)\n", spi_speed, spi_speed/1000);

    return fd_spi;
}

int Ql_SPI_Write_Read(int fd,uint8_t* write_buf,uint8_t* read_buf,uint32_t len)
{
    int ret;
    struct spi_ioc_transfer tr = {
    .tx_buf = (unsigned long)write_buf,
    .rx_buf = (unsigned long)read_buf,
    .len = len,
    .delay_usecs = spi_delay,
    .speed_hz = spi_speed,
    .bits_per_word = spi_bits,
    };
    ret = ioctl(fd, SPI_IOC_MESSAGE(1), &tr);
    if (ret < 0)
    {
        perror("can't send spi message");
        return ret;
    }

    return ret;
}

int Ql_SPI_Read(int fd,uint8_t* read_buf,uint32_t len)
{
    int ret;
    struct spi_ioc_transfer tr = {
    .rx_buf = (unsigned long)read_buf,
    .len = len,
    .delay_usecs = spi_delay,
    .speed_hz = spi_speed,
    .bits_per_word = spi_bits,
    };
    ret = ioctl(fd, SPI_IOC_MESSAGE(1), &tr);
    if (ret < 0)
    {
        perror("can't read spi message");
        return ret;
    }

    return ret;
}

int Ql_SPI_Write(int fd,uint8_t* write_buf,uint32_t len)
{
    int ret;
    struct spi_ioc_transfer tr = {
    .tx_buf = (unsigned long)write_buf,
    .len = len,
    .delay_usecs = spi_delay,
    .speed_hz = spi_speed,
    .bits_per_word = spi_bits,
    };
    ret = ioctl(fd, SPI_IOC_MESSAGE(1), &tr);
    if (ret < 0)
    {
        perror("can't write spi message");
        return ret;
    }

    return ret;
}

int Ql_SPI_DeInit(int fd)
{
    close(fd);
    return 0;
}

#endif
