/*
 * ch346 application demo
 *
 * Copyright (C) 2025 Nanjing Qinheng Microelectronics Co., Ltd.
 * Web:      http://wch.cn
 * Author:   WCH <tech@wch.cn>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Cross-compile with cross-gcc -I /path/to/cross-kernel/include
 *
 * V1.0 - initial version
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>
#include <endian.h>
#include <linux/hidraw.h>
#include <signal.h>
#include <pthread.h>

#include "ch346_lib.h"

typedef enum _CH346FUNCTYPE {
	FUNC_FIFO = 0,
	FUNC_SPI,
} CH346FUNCTYPE;

struct ch346 {
	int fd;
	char version[100];
	CHIP_TYPE chiptype;
	uint32_t dev_id;
	CH346FUNCTYPE functype;
};

static struct ch346 ch346device;
pthread_t sid, rid;

/**
 * get_file_size - get file size
 * @path: file name
 * The function return file size
 */
unsigned int get_file_size(const char *path)
{
	unsigned int filesize = -1;
	struct stat statbuff;

	if (stat(path, &statbuff) < 0) {
		return filesize;
	} else {
		filesize = statbuff.st_size;
	}
	return filesize;
}

bool Flash_File_Read_Test()
{
	uint32_t DataLen, i;
	uint8_t ioBuffer[8192] = { 0 };
	unsigned char buf_cmp[8192];
	int nread;
	int ret, rv;
	FILE *fp = NULL;
	unsigned long long filesize, total;

	static int count = 0;

	DataLen = 0x500;

	fp = fopen("flash.txt", "r");
	if (fp == NULL) {
		printf("open file failed.\n");
		return false;
	}

	filesize = get_file_size("flash.txt");
	if (filesize < 0) {
		printf("get_file_size error\n");
		return false;
	}

	while (1) {
		ret = fread(buf_cmp, 1, DataLen, fp);
		if (ret == 0) {
			goto recheck;
		} else if (ret < 0) {
			goto exit;
		}

		if (ret > 0) {
			rv = memcmp(ioBuffer, buf_cmp, ret);
			if (rv) {
				printf("flash content compare error.\n");
				printf("read data:\n");
				for (i = 0; i < ret; i++)
					printf("%02x ", ioBuffer[i]);
				printf("\n\n");
				printf("raw file data:\n");
				for (i = 0; i < ret; i++)
					printf("%02x ", buf_cmp[i]);
				printf("\n\n");
				goto exit;
			}
		} else {
			printf("flash file read error, Datalen: %d, ret: %d\n",
			       DataLen, ret);
			goto exit;
		}

recheck:
		if (total >= filesize) {
			printf("******************** flash Test Passed--> count: %d *************\n",
			       count++);
			break;
		}
	}

	fclose(fp);
	return true;

exit:
	fclose(fp);
	return false;
}

#define MB (1 * 1024 * 1024)

#define PACKLEN 8192

void ch346_demo_slave_read()
{
	bool ret = false;
	uint8_t oBuffer[SLAVE_MAX_LENGTH];
	uint8_t cBuffer[8192];
	uint32_t oLength;
	uint64_t bytes_read = 0;
	struct timeval t1, t2;
	int delta_sec = 0;
	int delta_usec = 0;
	float speed_read = 0.0;
	int MB_Num = 10;
	int i, j, k;
	uint32_t packindex = 0;
	uint32_t len, perlen;
	time_t time_start, time_end;

	ret = CH346_Slave_Control(ch346device.fd, true);
	if (!ret)
		return;
	printf("Begin read spi/fifo data from master...\n");

	sleep(1);

	ret = CH346_Slave_QweryData(ch346device.fd, &oLength);
	if (!ret) {
		printf("CH346SPI_Slave_QweryData failed.\n");
		goto exit;
	}
	printf("buffered oLength: %d\n", oLength);

	ret = CH346_Slave_FIFOReset(ch346device.fd);
	if (ret == false) {
		printf("Failed to reset slave fifo.\n");
		return;
	}

	time_start = time(NULL);

	while (1) {
		bytes_read = 0;
		gettimeofday(&t1, NULL);
		while (bytes_read < MB_Num * MB) {
			ret = CH346_Slave_QweryData(ch346device.fd,
						    &oLength);
			if (!ret) {
				printf("CH346SPI_Slave_QweryData failed.\n");
				goto exit;
			}
			if (oLength >= SLAVE_MAX_LENGTH) {
				perlen = SLAVE_MAX_LENGTH;
				ret = CH346_Slave_ReadData(
					ch346device.fd, oBuffer, &perlen);
				if (!ret) {
					printf("CH346_Slave_ReadData failed.\n");
					goto exit;
				}
				// printf("oBuffer[0] : 0x%2x\n", oBuffer[0]);

				for (i = 0; i < perlen; i += PACKLEN) {
					// if (packindex == 0) {
					// 	packindex++;
					// 	continue;
					// }
					len = *(uint32_t *)(oBuffer + i);
					// printf("i=%d, pack number: %x, packindex:%d \n",
					//        i, le32toh(len), packindex);
					if (le32toh(len) != packindex) {
						time_end = time(NULL);
						printf("Bad packet: %d, last packet: %d\n",
						       le32toh(len),
						       packindex);
						printf("Stop time: %s\n",
						       ctime(&time_end));
						printf("The program continues %f secs...\n",
						       difftime(
							       time_end,
							       time_start));
						goto exit;
					}
					packindex = le32toh(len) + 1;
					// packindex &= 0x7f;
					memcpy(cBuffer, oBuffer + i, PACKLEN);
					// printf("read %d bytes this time.\n", oLength);
					for (j = 0; j < PACKLEN; j++) {
						if (j % 512 < 5)
							continue;
						if (((cBuffer[j] - cBuffer[j - 1]) != 1) && (cBuffer[j] != 0)) {
							printf("Error packet: %d\n", packindex);
							printf("Data Error, j = %d, buf[j] = 0x%2x, buf[j-1]: 0x%2x\n",
							       j, cBuffer[j], cBuffer[j - 1]);
							for (k = 0; k < oLength; k++)
								printf("0x%2x ", cBuffer[k]);
							printf("\n\n");
							time_end = time(NULL);
							printf("Stop time: %s\n", ctime(&time_end));
							printf("The program continues %f secs...\n",
							       difftime(time_end, time_start));
							goto exit;
						}
#ifdef LOOPBACK
						if (((cBuffer[j] -
						      cBuffer[j + 1]) !=
						     1) &&
						    (cBuffer[j] != 0)) {
							printf("Error packet: %d\n",
							       packindex);
							printf("Data Error, j = %d, buf[j] = 0x%2x, buf[j-1]: 0x%2x\n",
							       j,
							       cBuffer[j],
							       cBuffer[j -
								       1]);
							for (k = 0;
							     k < oLength;
							     k++)
								printf("0x%2x ",
								       cBuffer[k]);
							printf("\n\n");
							time_end =
								time(NULL);
							printf("Stop time: %s\n",
							       ctime(&time_end));
							printf("The program continues %f secs...\n",
							       difftime(
								       time_end,
								       time_start));
							goto exit;
						}
#endif
					}
				}
				bytes_read += perlen;
			} else {
				usleep(100);
				continue;
			}
		}
		gettimeofday(&t2, NULL);

		delta_sec = t2.tv_sec - t1.tv_sec;
		delta_usec = t2.tv_usec - t1.tv_usec;

		if (delta_usec < 0) {
			delta_sec--;
			delta_usec = 1000000 + delta_usec;
		}
		speed_read =
			(float)(MB_Num) /
			(float)(delta_sec + (float)delta_usec / 1000000);

		printf("RX %d MB takes: %2d sec %2d us, up speed: %4.2f MB/s\n",
		       MB_Num, delta_sec, delta_usec, speed_read);
	}

	ret = CH346_Slave_Control(ch346device.fd, false);
	if (!ret)
		return;

	pthread_exit(NULL);

	exit(0);

exit:

	gettimeofday(&t1, NULL);

	while (1) {
		gettimeofday(&t2, NULL);
		delta_sec = t2.tv_sec - t1.tv_sec;
		delta_usec = t2.tv_usec - t1.tv_usec;
		if (delta_usec < 0) {
			delta_sec--;
			delta_usec = 1000000 + delta_usec;
		}

		if (delta_sec >= 10) {
			printf("Time eclaped 10 seconds since error!");
			break;
		}

		perlen = SLAVE_MAX_LENGTH;
		ret = CH346_Slave_ReadData(ch346device.fd, oBuffer,
					   &perlen);
	}

	ret = CH346_Slave_Control(ch346device.fd, false);
	if (!ret)
		return;

	pthread_exit(NULL);

	exit(-1);
}

void ch346_demo_slave_write()
{
	bool ret = false;
	uint8_t iBuffer[SLAVE_MAX_LENGTH];
	uint32_t iLength;
	uint64_t bytes_write = 0;
	struct timeval t1, t2;
	int delta_sec = 0;
	int delta_usec = 0;
	float speed_write = 0.0;
	int MB_Num = 10;
	int i, j;
	uint32_t packindex = 1;
	uint32_t len, perlen;
	time_t time_start, time_end;

	sleep(2);

	printf("Begin write spi/fifo to from master...\n");

	time_start = time(NULL);

	for (i = 0; i < SLAVE_MAX_LENGTH; i++)
		iBuffer[i] = i;

	for (i = 0; i < SLAVE_MAX_LENGTH; i++)
		iBuffer[i] = 0xff - (uint8_t)i;

	// 	for (i = 0; i < SLAVE_MAX_LENGTH; i += 8192) {
	// //		packindex = htobe32(i / 8192);
	// 		packindex = i / 8192;
	// 		memcpy(iBuffer + i, &packindex, 4);
	// 	}

	while (1) {
		bytes_write = 0;
		gettimeofday(&t1, NULL);
		while (bytes_write < MB_Num * MB) {
			perlen = 8192;
			memcpy(iBuffer + bytes_write % MB, &packindex, 4);
			packindex++;
			ret = CH346_Slave_WriteData(
				ch346device.fd, iBuffer + bytes_write % MB,
				&perlen);
			if (!ret || perlen != 8192) {
				printf("CH346_Slave_WriteData failed.\n");
				goto exit;
			}
			bytes_write += 8192;
		}
		gettimeofday(&t2, NULL);

		delta_sec = t2.tv_sec - t1.tv_sec;
		delta_usec = t2.tv_usec - t1.tv_usec;

		if (delta_usec < 0) {
			delta_sec--;
			delta_usec = 1000000 + delta_usec;
		}
		speed_write =
			(float)(MB_Num) /
			(float)(delta_sec + (float)delta_usec / 1000000);

		printf("TX %d MB takes: %2d sec %2d us, up speed: %4.2f MB/s\n",
		       MB_Num, delta_sec, delta_usec, speed_write);
	}
	pthread_exit(NULL);

exit:
	pthread_exit(NULL);
}

bool Show_DevMsg(char *pathname)
{
	unsigned char buf[256];
	int ret;
	int i;
	struct hidraw_devinfo info;
	uint16_t vendor, product;
	CHIP_TYPE chiptype;
	uint8_t mode;

	if (strstr(pathname, "ch34x_pis")) {
		/* Get Driver Version */
		ret = CH34x_GetDriverVersion(ch346device.fd,
					     ch346device.version);
		if (ret == false) {
			printf("CH34x_GetDriverVersion error.\n");
			goto exit;
		}
		printf("Driver version: %s\n", ch346device.version);

		/* Get Chip Type */
		ret = CH34x_GetChipType(ch346device.fd,
					&ch346device.chiptype);
		if (ret == false) {
			printf("CH34x_GetChipType error.\n");
			goto exit;
		}
		if (ch346device.chiptype != CHIP_CH346C) {
			printf("Current chip operating is CH341/CH347, please use other demos.\n");
			goto exit;
		}

		/* Get Device ID */
		ret = CH34X_GetDeviceID(ch346device.fd,
					&ch346device.dev_id);
		if (ret == false) {
			printf("CH34X_GetDeviceID error.\n");
			goto exit;
		}
		vendor = ch346device.dev_id;
		product = ch346device.dev_id >> 16;
		printf("Vendor ID: 0x%4x, Product ID: 0x%4x\n", vendor,
		       product);

		/* Get Device Mode */
		ret = CH346_GetMode(ch346device.fd, &mode);
		if (ret == false) {
			printf("CH346_GetMode error.\n");
			goto exit;
		}

		if (mode == 0x00) {
			ch346device.functype = FUNC_FIFO;
			printf("Device operating has function [FIFO].\n");
		} else {
			ch346device.functype = FUNC_SPI;
			printf("Device operating has function [SPI].\n");
		}
	}
	return true;

exit:
	return false;
}

static void sig_handler(int signo)
{
	int ret;

	printf("capture sign no:%d\n", signo);
	if (ch346device.fd) {
		ret = CH346_Slave_Control(ch346device.fd, false);
		if (!ret)
			printf("CH346_Slave_Control disable error.\n");
	}
	exit(0);
}

int main(int argc, char *argv[])
{
	bool ret;
	char choice, ch;

	if (argc != 2) {
		printf("Usage: sudo %s [device]\n", argv[0]);
		return -1;
	}

	/* open device */
	ch346device.fd = CH346OpenDevice(argv[1]);
	if (ch346device.fd < 0) {
		printf("CH346OpenDevice false.\n");
		return -1;
	}
	printf("Open device %s succeed, fd: %d\n", argv[1],
	       ch346device.fd);

	signal(SIGINT, sig_handler);

	ret = Show_DevMsg(argv[1]);
	if (ret == false)
		return -1;

	sleep(1);

	ret = CH346_SlaveInit(ch346device.fd);
	if (ret == false)
		return -1;

	if (strstr(argv[1], "ch34x_pis")) {
		ret = CH34xSetTimeout(ch346device.fd, 2000, 2000);
		if (ret == false) {
			printf("CH34xSetTimeout false.\n");
			return -1;
		}
	}

	while (1) {
		printf("\npress r to read spi/fifo data from master,\n"
		       "w to write spi/fifo data to master, q to quit.\n");

		scanf("%c", &choice);
		while ((ch = getchar()) != EOF && ch != '\n')
			;
		if (choice == 'q')
			break;

		switch (choice) {
		case 'w':
			ret = pthread_create(
				&sid, NULL, (void *)ch346_demo_slave_write,
				NULL);
			if (ret != 0) {
				printf("pthread-write create error.\n");
				goto exit;
			}
		case 'r':
			ret = pthread_create(&rid, NULL,
					     (void *)ch346_demo_slave_read,
					     NULL);
			if (ret != 0) {
				printf("pthread-read create error.\n");
				goto exit;
			}
			break;
		default:
			printf("Bad choice, please input again.\n");
			break;
		}
	}

	pthread_join(sid, NULL);
	pthread_join(rid, NULL);

	/* close the device */
	if (CH346CloseDevice(ch346device.fd)) {
		printf("Close device succeed.\n");
	}

exit:
	return 0;
}
