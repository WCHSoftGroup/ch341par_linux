/*
 * CH341/CH346/CH347/CH9341 USB to multiple interfaces driver
 *
 * Copyright (C) 2026 Nanjing Qinheng Microelectronics Co., Ltd.
 * Web:      http://wch.cn
 * Author:   WCH <tech@wch.cn>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Update Log:
 * V1.0 - initial version
 * V1.1 - fix ioctl bugs when copy data from user space
 * V1.2 - fix write & read & ioctl bugs
 * V1.3 - add support of CH347T
 * V1.4 - add support of gpio interrupt function, spi slave mode, etc.
 *      - add support of CH347F
 * V1.5 - add support of CH339W
 *		- add support of CH346C
 * V1.6 - add support of CH9341C
 *      - add support of CH9346W
 */

#define DEBUG
#define VERBOSE_DEBUG

#undef DEBUG
#undef VERBOSE_DEBUG

#include <linux/errno.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/kref.h>
#include <linux/mm.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/usb.h>
#include <linux/version.h>
#include <linux/kfifo.h>
#include <linux/vmalloc.h>

#define DRIVER_AUTHOR "WCH"
#define DRIVER_DESC \
	"USB to multiple interface driver for CH341/CH347/CH339/CH346/CH9341, etc."
#define VERSION_DESC "V1.7 On 2026.11"

#define CH34x_MINOR_BASE 200
#define CH341_PACKET_LENGTH 32
#define MAX_BUFFER_LENGTH 0x10000
#define MAX_SLAVE_LENGTH 0x100000
#define MAX_TRANSFER 1024
#define CH34X_KFIFO_LENGTH (1 * 1024 * 1024)

#define CH347_MPSI_GPIOS 8
#define CH9341_MPSI_GPIOS 10
#define CH34X_NR 4
#define CH34X_NW 4
#define MAXCH 3

#define DEFAULT_TIMEOUT 1000

/* ioctl commands for interaction between driver and application */
#define IOCTL_MAGIC 'W'

#define CH34x_GET_DRV_VERSION _IOR(IOCTL_MAGIC, 0x80, u16)
#define CH34x_CHIP_TYPE _IOR(IOCTL_MAGIC, 0x81, u16)
#define CH34x_DEVICE_MSG _IOR(IOCTL_MAGIC, 0x82, u16)

#define CH34x_DEVICE_CTRLIN _IOWR(IOCTL_MAGIC, 0x87, u16)
#define CH34x_DEVICE_CTRLOUT _IOW(IOCTL_MAGIC, 0x88, u16)

#define CH34x_SET_TIMEOUT _IOW(IOCTL_MAGIC, 0x90, u16)
#define CH34x_USB_RESET _IOW(IOCTL_MAGIC, 0x91, u16)

#define CH34x_PIPE_DATA_READ _IOWR(IOCTL_MAGIC, 0xb0, u16)
#define CH34x_PIPE_DATA_WRITE _IOWR(IOCTL_MAGIC, 0xb1, u16)

#define CH34x_START_BUFFERED_UPLOAD _IOW(IOCTL_MAGIC, 0xb4, u16)
#define CH34x_STOP_BUFFERED_UPLOAD _IOW(IOCTL_MAGIC, 0xb5, u16)
#define CH34x_QUERY_SLAVE_FIFO _IOR(IOCTL_MAGIC, 0xb6, u16)
#define CH34x_RESET_SLAVE_FIFO _IOW(IOCTL_MAGIC, 0xb7, u16)
#define CH34x_READ_SLAVE_FIFO _IOR(IOCTL_MAGIC, 0xb8, u16)

#define CH34x_START_IRQ_TASK _IOW(IOCTL_MAGIC, 0xc0, u16)
#define CH34x_STOP_IRQ_TASK _IOW(IOCTL_MAGIC, 0xc1, u16)

/* Define these values to match your devices */

/* table of devices that work with this driver */
static const struct usb_device_id ch34x_usb_ids[] = {
	{ USB_DEVICE(0x1a86, 0x5512) }, /* CH341A/B/T/F */
	{ USB_DEVICE(0x1a86, 0x55ee) }, /* CH9341C */
	{ USB_DEVICE_INTERFACE_NUMBER(
		0x1a86, 0x55db, 0x02) }, /* CH347T Mode1 SPI+IIC+UART */
	{ USB_DEVICE_INTERFACE_NUMBER(0x1a86, 0x55dd,
				      0x02) }, /* CH347T Mode3 JTAG+UART */
	{ USB_DEVICE_INTERFACE_NUMBER(0x1a86, 0x55de, 0x04) }, /* CH347F */
	{ USB_DEVICE_INTERFACE_NUMBER(0x1a86, 0x55e7, 0x02) }, /* CH339W */
	{ USB_DEVICE_INTERFACE_NUMBER(0x1a86, 0x55eb, 0x02) }, /* CH346C */
	{ USB_DEVICE(0x1a86, 0xe100) }, /* CH9346W */
	{} /* Terminating entry */
};

MODULE_DEVICE_TABLE(usb, ch34x_usb_ids);

typedef enum _CHIP_TYPE {
	CHIP_CH341_VENDOR = 0,
	CHIP_CH347T_VENDOR,
	CHIP_CH347T_HID,
	CHIP_CH347F_VENDOR,
	CHIP_CH339W_VENDOR,
	CHIP_CH346C_VENDOR,
	CHIP_CH9341C_VENDOR,
	CHIP_CH9346W_VENDOR,
} CHIP_TYPE;

struct ch34x_rb {
	int size;
	unsigned char *base;
	dma_addr_t dma;
	int index;
	struct ch34x_pis *instance;
	u8 ch;
};

struct ch34x_wb {
	struct ch34x_pis *instance;
	unsigned char *buf;
	dma_addr_t dma;
	int len;
	int use;
	struct urb *urb;
};

struct ch34x_pis {
	struct usb_device *udev; /* the usb device for this device*/
	struct usb_interface *interface; /* the interface for this device*/
	struct usb_endpoint_descriptor *interrupt_in_endpoint;

	struct usb_device_descriptor device_desc;
	char manufacturer[32];
	char product[32];
	char serial[32];
	char location[32];
	char pathname[32];

	size_t interrupt_in_size;
	unsigned char *interrupt_in_buffer;
	struct urb *interrupt_in_urb;

	u8 bulk_in_endpointAddr[MAXCH];
	u8 bulk_out_endpointAddr[MAXCH];

	u8 bulkin_num;
	u8 bulkout_num;

	int readsize[MAXCH];
	int rx_endpoint[MAXCH];
	unsigned long read_urbs_free[MAXCH];
	struct urb *read_urbs[MAXCH][CH34X_NR];
	struct ch34x_rb read_buffers[MAXCH][CH34X_NR];
	int rx_buflimit;

	wait_queue_head_t wait[MAXCH]; /* wait queue */
	struct kfifo rfifo[MAXCH];
	bool buffered_mode[MAXCH];
	spinlock_t read_lock[MAXCH];

	int tx_endpoint[MAXCH];
	wait_queue_head_t wq_send; /* for usb write */
	int writesize;
	struct ch34x_wb wb[CH34X_NW];
	int transmitting;
	spinlock_t write_lock;

	bool irq_enable;

	int readtimeout;
	int writetimeout;

	struct mutex io_mutex; /* synchronize I/O with disconnect */
	bool disconnected;
	CHIP_TYPE chiptype;
	u8 schannel;
	int errors;
	spinlock_t err_lock;
	struct kref kref;

	int open_count;
	struct mutex open_mutex;

	unsigned int susp_count;
	struct fasync_struct *fasync;
};

static struct usb_driver ch34x_pis_driver;
static void ch34x_delete(struct kref *kref);
static void stop_data_traffic(struct ch34x_pis *ch34x_dev);
static int ch34x_submit_read_urbs(struct ch34x_pis *ch34x_dev, u8 ch,
				  gfp_t mem_flags);
static void ch34x_usb_free_device(struct ch34x_pis *ch34x_dev);

/* USB control transfer in */
static int ch34x_control_transfer_in(struct ch34x_pis *ch34x_dev,
				     u8 request, u8 request_type,
				     u16 value, u16 index,
				     void __user *data, u16 size)
{
	int retval;
	char *buffer = NULL;

	if (size > 0) {
		buffer = kmalloc(size, GFP_KERNEL);
		if (!buffer)
			return -ENOMEM;
	}

	retval = usb_control_msg(ch34x_dev->udev,
				 usb_rcvctrlpipe(ch34x_dev->udev, 0),
				 request, request_type, value, index,
				 buffer, size, ch34x_dev->readtimeout);
	if (retval > 0 && buffer) {
		if (copy_to_user(data, buffer, retval))
			retval = -EFAULT;
	}

	kfree(buffer);
	return retval;
}

/* USB control transfer out */
static int ch34x_control_transfer_out(struct ch34x_pis *ch34x_dev,
				      u8 request, u8 request_type,
				      u16 value, u16 index,
				      const void __user *data, u16 size)
{
	int retval;
	char *buffer = NULL;

	if (size > 0) {
		buffer = memdup_user(data, size);
		if (IS_ERR(buffer))
			return PTR_ERR(buffer);
	}

	retval = usb_control_msg(ch34x_dev->udev,
				 usb_sndctrlpipe(ch34x_dev->udev, 0),
				 request, request_type, value, index,
				 buffer, size, ch34x_dev->writetimeout);

	kfree(buffer);
	return retval;
}

static int ch34x_wb_alloc(struct ch34x_pis *ch34x_dev)
{
	int i, wbn;
	struct ch34x_wb *wb;

	wbn = 0;
	i = 0;
	for (;;) {
		wb = &ch34x_dev->wb[wbn];
		if (!wb->use) {
			wb->use = 1;
			return wbn;
		}
		wbn = (wbn + 1) % CH34X_NW;
		if (++i >= CH34X_NW)
			return -1;
	}
}

static int ch34x_wb_is_avail(struct ch34x_pis *ch34x_dev)
{
	int i, n;
	unsigned long flags;

	n = CH34X_NW;
	spin_lock_irqsave(&ch34x_dev->write_lock, flags);
	for (i = 0; i < CH34X_NW; i++)
		n -= ch34x_dev->wb[i].use;
	spin_unlock_irqrestore(&ch34x_dev->write_lock, flags);

	return n;
}

static void ch34x_write_done(struct ch34x_pis *ch34x_dev,
			     struct ch34x_wb *wb)
{
	wb->use = 0;
	if (ch34x_dev->transmitting > 0)
		ch34x_dev->transmitting--;
}

static void ch34x_write_bulk_callback(struct urb *urb)
{
	struct ch34x_wb *wb = urb->context;
	struct ch34x_pis *ch34x_dev = wb->instance;
	unsigned long flags;

	if (urb->status) {
		if (!(urb->status == -ENOENT ||
		      urb->status == -ECONNRESET ||
		      urb->status == -ESHUTDOWN))
			dev_err(&ch34x_dev->interface->dev,
				"%s - nonzero write bulk status received: %d\n",
				__func__, urb->status);
		spin_lock(&ch34x_dev->err_lock);
		ch34x_dev->errors = urb->status;
		spin_unlock(&ch34x_dev->err_lock);
	}

	spin_lock_irqsave(&ch34x_dev->write_lock, flags);
	ch34x_write_done(ch34x_dev, wb);
	wake_up_interruptible(&ch34x_dev->wq_send);
	spin_unlock_irqrestore(&ch34x_dev->write_lock, flags);
}

static int ch34x_start_wb(struct ch34x_pis *ch34x_dev, struct ch34x_wb *wb,
			  int pipe)
{
	int rc;
	unsigned long flags;

	spin_lock_irqsave(&ch34x_dev->write_lock, flags);
	ch34x_dev->transmitting++;
	spin_unlock_irqrestore(&ch34x_dev->write_lock, flags);

	wb->urb->dev = ch34x_dev->udev;
	wb->urb->pipe = pipe;
	wb->urb->transfer_buffer = wb->buf;
	wb->urb->transfer_dma = wb->dma;
	wb->urb->transfer_buffer_length = wb->len;

	rc = usb_submit_urb(wb->urb, GFP_ATOMIC);
	if (rc < 0) {
		dev_err(&ch34x_dev->interface->dev,
			"%s - usb_submit_urb(write bulk) failed: %d\n",
			__func__, rc);
		spin_lock_irqsave(&ch34x_dev->write_lock, flags);
		ch34x_write_done(ch34x_dev, wb);
		wake_up_interruptible(&ch34x_dev->wq_send);
		spin_unlock_irqrestore(&ch34x_dev->write_lock, flags);
	}
	return rc;
}

static int ch34x_data_read(struct ch34x_pis *ch34x_dev,
			   void __user *obuffer, u8 ch, u32 bytes_to_read)
{
	int bytes_read;
	int totallen = 0;
	unsigned char *obuf;
	int retval = 0;

	if ((bytes_to_read > MAX_BUFFER_LENGTH) || (bytes_to_read == 0))
		return -EINVAL;

	spin_lock_irq(&ch34x_dev->err_lock);
	retval = ch34x_dev->errors;
	if (retval < 0) {
		ch34x_dev->errors = 0;
		retval = (retval == -EPIPE) ? retval : -EIO;
	}
	spin_unlock_irq(&ch34x_dev->err_lock);
	if (retval < 0)
		return retval;

	obuf = kvmalloc(bytes_to_read, GFP_KERNEL);
	if (!obuf)
		return -ENOMEM;

	while (totallen < bytes_to_read) {
		mutex_lock(&ch34x_dev->io_mutex);
		if (ch34x_dev->disconnected) {
			mutex_unlock(&ch34x_dev->io_mutex);
			retval = -ENODEV;
			goto error;
		}

		retval = usb_bulk_msg(
			ch34x_dev->udev,
			usb_rcvbulkpipe(
				ch34x_dev->udev,
				ch34x_dev->bulk_in_endpointAddr[ch]),
			obuf + totallen, bytes_to_read - totallen,
			&bytes_read, ch34x_dev->readtimeout);
		mutex_unlock(&ch34x_dev->io_mutex);

		if (retval)
			goto error;

		if (bytes_read == 0)
			break;

		totallen += bytes_read;
	}

	if (copy_to_user(obuffer, obuf, totallen)) {
		retval = -EFAULT;
		goto error;
	}

	kvfree(obuf);
	return totallen;

error:
	kvfree(obuf);
	return retval;
}

static int ch34x_data_write(struct ch34x_pis *ch34x_dev,
			    const void __user *ibuffer, u8 ch, u32 count)
{
	int retval = 0;
	int wbn;
	struct ch34x_wb *wb;
	unsigned long flags;
	int timeout;

	if (count > ch34x_dev->writesize || count == 0)
		return -EINVAL;

	spin_lock_irq(&ch34x_dev->err_lock);
	retval = ch34x_dev->errors;
	if (retval < 0) {
		ch34x_dev->errors = 0;
		retval = (retval == -EPIPE) ? retval : -EIO;
	}
	spin_unlock_irq(&ch34x_dev->err_lock);
	if (retval < 0)
		return retval;

retry:
	spin_lock_irqsave(&ch34x_dev->write_lock, flags);
	wbn = ch34x_wb_alloc(ch34x_dev);
	if (wbn < 0) {
		spin_unlock_irqrestore(&ch34x_dev->write_lock, flags);
		timeout = wait_event_interruptible_timeout(
			ch34x_dev->wq_send, ch34x_wb_is_avail(ch34x_dev),
			msecs_to_jiffies(DEFAULT_TIMEOUT));
		if (timeout <= 0)
			return (timeout == 0) ? -ETIMEDOUT : timeout;
		goto retry;
	}
	wb = &ch34x_dev->wb[wbn];
	spin_unlock_irqrestore(&ch34x_dev->write_lock, flags);

	if (copy_from_user(wb->buf, ibuffer, count)) {
		spin_lock_irqsave(&ch34x_dev->write_lock, flags);
		wb->use = 0;
		wake_up_interruptible(&ch34x_dev->wq_send);
		spin_unlock_irqrestore(&ch34x_dev->write_lock, flags);
		return -EFAULT;
	}
	wb->len = count;

	mutex_lock(&ch34x_dev->io_mutex);
	if (ch34x_dev->disconnected) {
		spin_lock_irqsave(&ch34x_dev->write_lock, flags);
		wb->use = 0;
		wake_up_interruptible(&ch34x_dev->wq_send);
		spin_unlock_irqrestore(&ch34x_dev->write_lock, flags);
		mutex_unlock(&ch34x_dev->io_mutex);
		return -ENODEV;
	}

	if (ch34x_dev->susp_count) {
		spin_lock_irqsave(&ch34x_dev->write_lock, flags);
		wb->use = 0;
		wake_up_interruptible(&ch34x_dev->wq_send);
		spin_unlock_irqrestore(&ch34x_dev->write_lock, flags);
		mutex_unlock(&ch34x_dev->io_mutex);
		return -EIO;
	}

	retval = ch34x_start_wb(ch34x_dev, wb, ch34x_dev->tx_endpoint[ch]);
	mutex_unlock(&ch34x_dev->io_mutex);

	if (retval < 0)
		return retval;

	return count;
}

static int ch34x_enable_buffered_mode(struct ch34x_pis *ch34x_dev, u8 ch)
{
	int retval = 0;
	int i;

	if (ch >= ch34x_dev->schannel ||
	    !kfifo_initialized(&ch34x_dev->rfifo[ch]))
		return -EOPNOTSUPP;

	mutex_lock(&ch34x_dev->io_mutex);
	if (ch34x_dev->disconnected) {
		retval = -ENODEV;
		goto out;
	}

	if (ch34x_dev->buffered_mode[ch]) {
		retval = -EBUSY;
		goto out;
	}

	retval = ch34x_submit_read_urbs(ch34x_dev, ch, GFP_KERNEL);
	if (retval) {
		for (i = 0; i < ch34x_dev->rx_buflimit; i++)
			usb_kill_urb(ch34x_dev->read_urbs[ch][i]);
		goto out;
	}

	ch34x_dev->buffered_mode[ch] = true;

out:
	mutex_unlock(&ch34x_dev->io_mutex);
	return retval;
}

static int ch34x_disable_buffered_mode(struct ch34x_pis *ch34x_dev, u8 ch)
{
	int retval = 0;
	int i;

	if (ch >= ch34x_dev->schannel)
		return -EINVAL;

	mutex_lock(&ch34x_dev->io_mutex);
	if (!ch34x_dev->buffered_mode[ch]) {
		mutex_unlock(&ch34x_dev->io_mutex);
		return 0;
	}

	ch34x_dev->buffered_mode[ch] = false;

	for (i = 0; i < ch34x_dev->rx_buflimit; i++)
		usb_kill_urb(ch34x_dev->read_urbs[ch][i]);

	wake_up_interruptible(&ch34x_dev->wait[ch]);

	mutex_unlock(&ch34x_dev->io_mutex);
	return retval;
}

static u32 ch34x_query_slave_fifo(struct ch34x_pis *ch34x_dev, u8 ch)
{
	unsigned long flags;
	u32 fifolen;

	spin_lock_irqsave(&ch34x_dev->read_lock[ch], flags);
	fifolen = kfifo_len(&ch34x_dev->rfifo[ch]);
	spin_unlock_irqrestore(&ch34x_dev->read_lock[ch], flags);

	return fifolen;
}

static void ch34x_reset_slave_fifo(struct ch34x_pis *ch34x_dev, u8 ch)
{
	unsigned long flags;

	spin_lock_irqsave(&ch34x_dev->read_lock[ch], flags);
	kfifo_reset(&ch34x_dev->rfifo[ch]);
	spin_unlock_irqrestore(&ch34x_dev->read_lock[ch], flags);
}

static int ch34x_slave_fifo_read(struct ch34x_pis *ch34x_dev, u8 ch,
				 void __user *obuffer, u32 bytes_to_read)
{
	int bytes_read;
	unsigned char *obuf;
	int retval = 0;
	int fifolen;
	unsigned long flags;

	if ((bytes_to_read > MAX_SLAVE_LENGTH) || (bytes_to_read == 0))
		return -EINVAL;

	spin_lock_irq(&ch34x_dev->err_lock);
	retval = ch34x_dev->errors;
	if (retval < 0) {
		ch34x_dev->errors = 0;
		retval = (retval == -EPIPE) ? retval : -EIO;
	}
	spin_unlock_irq(&ch34x_dev->err_lock);
	if (retval < 0)
		return retval;

	spin_lock_irqsave(&ch34x_dev->read_lock[ch], flags);
	fifolen = kfifo_len(&ch34x_dev->rfifo[ch]);
	spin_unlock_irqrestore(&ch34x_dev->read_lock[ch], flags);

	if (fifolen == 0) {
		retval = wait_event_interruptible_timeout(
			ch34x_dev->wait[ch],
			!kfifo_is_empty(&ch34x_dev->rfifo[ch]) ||
				!ch34x_dev->buffered_mode[ch] ||
				ch34x_dev->disconnected,
			msecs_to_jiffies(ch34x_dev->readtimeout));
		if (retval < 0)
			return retval;
		if (retval == 0)
			return -ETIMEDOUT;
		if (ch34x_dev->disconnected)
			return -ENODEV;
		if (!ch34x_dev->buffered_mode[ch] &&
		    kfifo_is_empty(&ch34x_dev->rfifo[ch]))
			return -EIO;
	}

	obuf = kvmalloc(bytes_to_read, GFP_KERNEL);
	if (!obuf)
		return -ENOMEM;

	spin_lock_irqsave(&ch34x_dev->read_lock[ch], flags);
	fifolen = kfifo_len(&ch34x_dev->rfifo[ch]);
	bytes_read = (bytes_to_read > fifolen) ? fifolen : bytes_to_read;
	retval = kfifo_out(&ch34x_dev->rfifo[ch], obuf, bytes_read);
	spin_unlock_irqrestore(&ch34x_dev->read_lock[ch], flags);

	if (retval != bytes_read) {
		kvfree(obuf);
		return -EFAULT;
	}

	if (copy_to_user(obuffer, obuf, bytes_read)) {
		kvfree(obuf);
		return -EFAULT;
	}

	kvfree(obuf);
	return bytes_read;
}

static int ch34x_start_irq_task(struct ch34x_pis *ch34x_dev)
{
	int retval = -ENODEV;

	mutex_lock(&ch34x_dev->io_mutex);
	if (ch34x_dev->disconnected || !ch34x_dev->interrupt_in_urb) {
		mutex_unlock(&ch34x_dev->io_mutex);
		return -ENODEV;
	}

	retval = usb_submit_urb(ch34x_dev->interrupt_in_urb, GFP_ATOMIC);
	if (retval)
		goto error_submit_urb;

	mutex_unlock(&ch34x_dev->io_mutex);
	return 0;

error_submit_urb:
	mutex_unlock(&ch34x_dev->io_mutex);
	return usb_translate_errors(retval);
}

static int ch34x_stop_irq_task(struct ch34x_pis *ch34x_dev)
{
	if (ch34x_dev->disconnected || !ch34x_dev->interrupt_in_urb)
		return -ENODEV;

	usb_kill_urb(ch34x_dev->interrupt_in_urb);
	return 0;
}

static int ch34x_fops_open(struct inode *inode, struct file *file)
{
	struct ch34x_pis *ch34x_dev;
	struct usb_interface *interface;
	int retval = 0;
	unsigned int subminor;
	int i;

#if (LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 35))
	subminor = iminor(inode);
#else
	subminor = iminor(file->f_path.dentry->d_inode);
#endif

	interface = usb_find_interface(&ch34x_pis_driver, subminor);
	if (!interface)
		return -ENODEV;

	ch34x_dev = usb_get_intfdata(interface);
	if (!ch34x_dev)
		return -ENODEV;

	retval = usb_autopm_get_interface(interface);
	if (retval)
		return retval;

	mutex_lock(&ch34x_dev->io_mutex);
	if (ch34x_dev->disconnected) {
		mutex_unlock(&ch34x_dev->io_mutex);
		usb_autopm_put_interface(interface);
		return -ENODEV;
	}
	mutex_unlock(&ch34x_dev->io_mutex);

	kref_get(&ch34x_dev->kref);
	file->private_data = ch34x_dev;

	mutex_lock(&ch34x_dev->open_mutex);
	if (ch34x_dev->open_count == 0) {
		ch34x_dev->errors = 0;
		for (i = 0; i < ch34x_dev->schannel; i++)
			ch34x_reset_slave_fifo(ch34x_dev, i);
	}
	ch34x_dev->open_count++;
	mutex_unlock(&ch34x_dev->open_mutex);

	return 0;
}

static int ch34x_fops_release(struct inode *inode, struct file *file)
{
	struct ch34x_pis *ch34x_dev;
	u8 i;

	ch34x_dev = (struct ch34x_pis *)file->private_data;
	if (ch34x_dev == NULL)
		return -ENODEV;

	mutex_lock(&ch34x_dev->open_mutex);
	ch34x_dev->open_count--;
	if (ch34x_dev->open_count <= 0) {
		ch34x_dev->open_count = 0;
		stop_data_traffic(ch34x_dev);
		for (i = 0; i < ch34x_dev->schannel; i++)
			ch34x_dev->buffered_mode[i] = false;
		ch34x_dev->irq_enable = false;
	}
	mutex_unlock(&ch34x_dev->open_mutex);

	mutex_lock(&ch34x_dev->io_mutex);
	if (ch34x_dev->interface)
		usb_autopm_put_interface(ch34x_dev->interface);
	mutex_unlock(&ch34x_dev->io_mutex);

	kref_put(&ch34x_dev->kref, ch34x_delete);
	return 0;
}

#if (LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 35))
static int ch34x_fops_ioctl(struct inode *inode, struct file *file,
			    unsigned int ch34x_cmd,
			    unsigned long ch34x_arg)
#else
static long ch34x_fops_ioctl(struct file *file, unsigned int ch34x_cmd,
			     unsigned long ch34x_arg)
#endif
{
	int retval = 0;
	int readtimeout = 0;
	int writetimeout = 0;
	u32 bytes_to_read;
	u32 bytes_write;
	u32 ch;
	u32 index;
	u32 datalen;
	char *drv_version = VERSION_DESC;
	struct ch34x_pis *ch34x_dev;
	unsigned long arg1, arg2, arg3, arg4, arg5;
	void __user *user_buf;

	ch34x_dev = (struct ch34x_pis *)file->private_data;
	if (ch34x_dev == NULL)
		return -ENODEV;

	switch (ch34x_cmd) {
	case CH34x_GET_DRV_VERSION:
		if (copy_to_user((char __user *)ch34x_arg, drv_version,
				 strlen(VERSION_DESC) + 1))
			retval = -EFAULT;
		break;
	case CH34x_CHIP_TYPE:
		retval = put_user(ch34x_dev->chiptype,
				  (u8 __user *)ch34x_arg);
		break;
	case CH34x_DEVICE_MSG: {
		char msg_buf[32 * 5];

		memset(msg_buf, 0, sizeof(msg_buf));
		memcpy(msg_buf, ch34x_dev->manufacturer,
		       sizeof(ch34x_dev->manufacturer));
		memcpy(msg_buf + 32, ch34x_dev->product,
		       sizeof(ch34x_dev->product));
		memcpy(msg_buf + 64, ch34x_dev->serial,
		       sizeof(ch34x_dev->serial));
		memcpy(msg_buf + 96, ch34x_dev->location,
		       sizeof(ch34x_dev->location));
		memcpy(msg_buf + 128, ch34x_dev->pathname,
		       sizeof(ch34x_dev->pathname));

		if (copy_to_user((void __user *)ch34x_arg, msg_buf,
				 sizeof(msg_buf)))
			return -EFAULT;

		if (copy_to_user((void __user *)ch34x_arg +
					 sizeof(msg_buf),
				 &ch34x_dev->device_desc,
				 sizeof(struct usb_device_descriptor)))
			return -EFAULT;
		break;
	}
	case CH34x_DEVICE_CTRLIN:
		if (get_user(arg1, (u8 __user *)ch34x_arg) ||
		    get_user(arg2, ((u8 __user *)ch34x_arg + 1)) ||
		    get_user(arg3,
			     (u16 __user *)((u8 __user *)ch34x_arg + 2)) ||
		    get_user(arg4,
			     (u16 __user *)((u8 __user *)ch34x_arg + 4)) ||
		    get_user(arg5,
			     (u16 __user *)((u8 __user *)ch34x_arg + 6)))
			return -EFAULT;

		user_buf = (void __user *)((u8 __user *)ch34x_arg + 8);
		retval = ch34x_control_transfer_in(ch34x_dev, (u8)arg1,
						   (u8)arg2, (u16)arg3,
						   (u16)arg4, user_buf,
						   (u16)arg5);
		break;
	case CH34x_DEVICE_CTRLOUT:
		if (get_user(arg1, (u8 __user *)ch34x_arg) ||
		    get_user(arg2, ((u8 __user *)ch34x_arg + 1)) ||
		    get_user(arg3,
			     (u16 __user *)((u8 __user *)ch34x_arg + 2)) ||
		    get_user(arg4,
			     (u16 __user *)((u8 __user *)ch34x_arg + 4)) ||
		    get_user(arg5,
			     (u16 __user *)((u8 __user *)ch34x_arg + 6)))
			return -EFAULT;

		user_buf = (void __user *)((u8 __user *)ch34x_arg + 8);
		retval = ch34x_control_transfer_out(ch34x_dev, (u8)arg1,
						    (u8)arg2, (u16)arg3,
						    (u16)arg4, user_buf,
						    (u16)arg5);
		if (retval < 0)
			return retval;
		if (retval != (u16)arg5)
			return -EIO;
		break;
	case CH34x_SET_TIMEOUT:
		if (get_user(readtimeout, (int __user *)ch34x_arg) ||
		    get_user(writetimeout, ((int __user *)ch34x_arg + 1)))
			return -EFAULT;
		if (readtimeout < 0 || writetimeout < 0)
			return -EINVAL;
		ch34x_dev->readtimeout = readtimeout;
		ch34x_dev->writetimeout = writetimeout;
		break;
	case CH34x_USB_RESET:
		retval = usb_reset_device(ch34x_dev->udev);
		break;
	case CH34x_PIPE_DATA_READ:
		if (get_user(bytes_to_read, (u32 __user *)ch34x_arg) ||
		    get_user(ch, (u32 __user *)ch34x_arg + 1))
			return -EFAULT;
		if (ch >= ch34x_dev->bulkin_num)
			return -EINVAL;
		if (ch34x_dev->buffered_mode[ch])
			return -EBUSY;
		user_buf = (void __user *)((u32 __user *)ch34x_arg + 2);
		retval = ch34x_data_read(ch34x_dev, user_buf, (u8)ch,
					 bytes_to_read);
		if (retval < 0)
			return retval;
		retval = put_user(retval, (u32 __user *)ch34x_arg);
		break;
	case CH34x_PIPE_DATA_WRITE:
		if (get_user(bytes_write, (u32 __user *)ch34x_arg) ||
		    get_user(ch, (u32 __user *)ch34x_arg + 1))
			return -EFAULT;
		if (ch >= ch34x_dev->bulkout_num)
			return -EINVAL;
		user_buf = (void __user *)((u32 __user *)ch34x_arg + 2);
		retval = ch34x_data_write(ch34x_dev, user_buf, (u8)ch,
					  bytes_write);
		if (retval < 0)
			return retval;
		retval = put_user(retval, (u32 __user *)ch34x_arg);
		break;
	case CH34x_START_BUFFERED_UPLOAD:
		if (get_user(index, (u32 __user *)ch34x_arg))
			return -EFAULT;
		retval = ch34x_enable_buffered_mode(ch34x_dev, (u8)index);
		break;
	case CH34x_STOP_BUFFERED_UPLOAD:
		if (get_user(index, (u32 __user *)ch34x_arg))
			return -EFAULT;
		retval = ch34x_disable_buffered_mode(ch34x_dev, (u8)index);
		break;
	case CH34x_QUERY_SLAVE_FIFO:
		if (get_user(index, (u32 __user *)ch34x_arg))
			return -EFAULT;
		if (index >= ch34x_dev->schannel)
			return -EINVAL;
		if (!ch34x_dev->buffered_mode[index])
			return -EINVAL;
		datalen = ch34x_query_slave_fifo(ch34x_dev, (u8)index);
		retval = put_user(datalen, (u32 __user *)ch34x_arg + 1);
		break;
	case CH34x_RESET_SLAVE_FIFO:
		if (get_user(index, (u32 __user *)ch34x_arg))
			return -EFAULT;
		if (index >= ch34x_dev->schannel)
			return -EINVAL;
		if (!ch34x_dev->buffered_mode[index])
			return -EINVAL;
		ch34x_reset_slave_fifo(ch34x_dev, (u8)index);
		break;
	case CH34x_READ_SLAVE_FIFO:
		if (get_user(bytes_to_read, (u32 __user *)ch34x_arg) ||
		    get_user(index, (u32 __user *)ch34x_arg + 1))
			return -EFAULT;
		if (index >= ch34x_dev->schannel)
			return -EINVAL;
		if (!ch34x_dev->buffered_mode[index])
			return -EINVAL;
		user_buf = (void __user *)((u32 __user *)ch34x_arg + 2);
		retval = ch34x_slave_fifo_read(ch34x_dev, (u8)index,
					       user_buf, bytes_to_read);
		if (retval < 0)
			return retval;
		retval = put_user(retval, (u32 __user *)ch34x_arg);
		break;
	case CH34x_START_IRQ_TASK:
		retval = ch34x_start_irq_task(ch34x_dev);
		if (!retval)
			ch34x_dev->irq_enable = true;
		break;
	case CH34x_STOP_IRQ_TASK:
		retval = ch34x_stop_irq_task(ch34x_dev);
		if (!retval)
			ch34x_dev->irq_enable = false;
		break;
	default:
		retval = -ENOTTY;
		break;
	}

	return retval;
}

static int ch34x_fops_fasync(int fd, struct file *file, int on)
{
	struct ch34x_pis *ch34x_dev;

	ch34x_dev = (struct ch34x_pis *)file->private_data;
	if (ch34x_dev == NULL)
		return -ENODEV;

	return fasync_helper(fd, file, on, &ch34x_dev->fasync);
}

#ifdef CONFIG_COMPAT
static long ch34x_fops_compat_ioctl(struct file *filp, unsigned int cmd,
				    unsigned long ch34x_arg)
{
	return ch34x_fops_ioctl(filp, cmd, ch34x_arg);
}
#endif

static const struct file_operations ch34x_fops_driver = {
	.owner = THIS_MODULE,
	.open = ch34x_fops_open,
	.release = ch34x_fops_release,
#if (LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 35))
	.ioctl = ch34x_fops_ioctl,
#else
#ifdef CONFIG_COMPAT
	.compat_ioctl = ch34x_fops_compat_ioctl,
#endif
	.unlocked_ioctl = ch34x_fops_ioctl,
#endif
	.fasync = ch34x_fops_fasync,
};

static void ch34x_usb_complete_intr_urb(struct urb *urb)
{
	struct ch34x_pis *ch34x_dev = urb->context;
	int status = urb->status;
	bool triggered, irq_enabled;
	int i;
	int retval;

	switch (status) {
	case 0:
		break;
	case -ECONNRESET:
	case -ENOENT:
	case -ESHUTDOWN:
		/* This urb is terminated, clean up */
		dev_dbg(&ch34x_dev->interface->dev,
			"%s - urb shutting down with status: %d\n",
			__func__, status);
		return;
	default:
		dev_dbg(&ch34x_dev->interface->dev,
			"%s - nonzero urb status received: %d\n", __func__,
			status);
		goto exit;
	}

	if (ch34x_dev->chiptype == CHIP_CH347T_VENDOR ||
	    ch34x_dev->chiptype == CHIP_CH347F_VENDOR) {
		if (urb->actual_length != 11) {
			dev_err(&ch34x_dev->interface->dev,
				"%s - bad interrupt urb length: %d\n",
				__func__, urb->actual_length);
			goto exit;
		}
		for (i = 0; i < CH347_MPSI_GPIOS; i++) {
			irq_enabled =
				ch34x_dev->interrupt_in_buffer[i + 3] &
				BIT(5);
			triggered = ch34x_dev->interrupt_in_buffer[i + 3] &
				    BIT(3);
			if (irq_enabled && triggered) {
				kill_fasync(&ch34x_dev->fasync, SIGIO,
					    POLL_IN);
			}
		}
	} else if (ch34x_dev->chiptype == CHIP_CH9341C_VENDOR) {
		if (urb->actual_length != 12) {
			dev_err(&ch34x_dev->interface->dev,
				"%s - bad interrupt urb length: %d\n",
				__func__, urb->actual_length);
			goto exit;
		} else {
			u8 *buf = ch34x_dev->interrupt_in_buffer;
			u16 irq_enable_status = (*(buf + 5) << 8) |
						*(buf + 4);
			u16 irq_trigger_status = (*(buf + 7) << 8) |
						 *(buf + 6);
			if ((*buf != 0xCC) || (*(buf + 1) != 0x33))
				goto exit;
			for (i = 0; i < CH9341_MPSI_GPIOS; i++) {
				irq_enabled = irq_enable_status & BIT(i);
				triggered = irq_trigger_status & BIT(i);
				if (irq_enabled && triggered) {
					kill_fasync(&ch34x_dev->fasync,
						    SIGIO, POLL_IN);
				}
			}
		}
	}

exit:
	if (ch34x_dev->disconnected || !ch34x_dev->irq_enable)
		return;
	retval = usb_submit_urb(urb, GFP_ATOMIC);
	if (retval && retval != -EPERM)
		dev_err(&ch34x_dev->interface->dev,
			"%s - usb_submit_urb failed: %d\n", __func__,
			retval);
}

static int ch34x_write_buffers_alloc(struct ch34x_pis *ch34x_dev)
{
	int i;
	struct ch34x_wb *wb;

	for (wb = &ch34x_dev->wb[0], i = 0; i < CH34X_NW; i++, wb++) {
		wb->buf = usb_alloc_coherent(ch34x_dev->udev,
					     ch34x_dev->writesize,
					     GFP_KERNEL, &wb->dma);
		if (!wb->buf) {
			while (i != 0) {
				--i;
				--wb;
				usb_free_coherent(ch34x_dev->udev,
						  ch34x_dev->writesize,
						  wb->buf, wb->dma);
				wb->buf = NULL;
			}
			return -ENOMEM;
		}
	}
	return 0;
}

static void ch34x_write_buffers_free(struct ch34x_pis *ch34x_dev)
{
	int i;
	struct ch34x_wb *wb;

	for (wb = &ch34x_dev->wb[0], i = 0; i < CH34X_NW; i++, wb++) {
		if (wb->buf) {
			usb_free_coherent(ch34x_dev->udev,
					  ch34x_dev->writesize, wb->buf,
					  wb->dma);
			wb->buf = NULL;
		}
	}
}

static void ch34x_read_buffers_free(struct ch34x_pis *ch34x_dev, u8 ch)
{
	int i;

	for (i = 0; i < ch34x_dev->rx_buflimit; i++) {
		if (ch34x_dev->read_buffers[ch][i].base) {
			usb_free_coherent(
				ch34x_dev->udev, ch34x_dev->readsize[ch],
				ch34x_dev->read_buffers[ch][i].base,
				ch34x_dev->read_buffers[ch][i].dma);
			ch34x_dev->read_buffers[ch][i].base = NULL;
		}
	}
	kfifo_free(&ch34x_dev->rfifo[ch]);
}

static int ch34x_submit_read_urb(struct ch34x_pis *ch34x_dev, u8 ch,
				 int index, gfp_t mem_flags)
{
	int res;

	if (!test_and_clear_bit(index, &ch34x_dev->read_urbs_free[ch]))
		return 0;

	res = usb_submit_urb(ch34x_dev->read_urbs[ch][index], mem_flags);
	if (res) {
		if (res != -EPERM) {
			dev_err(&ch34x_dev->interface->dev,
				"%s - usb_submit_urb failed: %d\n",
				__func__, res);
		}
		set_bit(index, &ch34x_dev->read_urbs_free[ch]);
		return res;
	}

	return 0;
}

static int ch34x_submit_read_urbs(struct ch34x_pis *ch34x_dev, u8 ch,
				  gfp_t mem_flags)
{
	int res;
	int i;

	for (i = 0; i < ch34x_dev->rx_buflimit; ++i) {
		res = ch34x_submit_read_urb(ch34x_dev, ch, i, mem_flags);
		if (res)
			return res;
	}

	return 0;
}

static void ch34x_process_read_urb(struct ch34x_pis *ch34x_dev, u8 ch,
				   struct urb *urb)
{
	unsigned long flags;

	if (!urb->actual_length)
		return;

	if (ch34x_dev->chiptype == CHIP_CH346C_VENDOR ||
	    ch34x_dev->chiptype == CHIP_CH9341C_VENDOR ||
	    ch34x_dev->chiptype == CHIP_CH9346W_VENDOR) {
		spin_lock_irqsave(&ch34x_dev->read_lock[ch], flags);
		if ((CH34X_KFIFO_LENGTH -
		     kfifo_len(&ch34x_dev->rfifo[ch])) <
		    urb->actual_length) {
			dev_err(&ch34x_dev->interface->dev,
				"%s - kfifo overflow.\n", __func__);
		}
		int copied = kfifo_in(&ch34x_dev->rfifo[ch],
				      urb->transfer_buffer,
				      urb->actual_length);
		if (copied < urb->actual_length)
			dev_err(&ch34x_dev->interface->dev,
				"kfifo_in partial: %d/%d\n", copied,
				urb->actual_length);
		spin_unlock_irqrestore(&ch34x_dev->read_lock[ch], flags);
	}
	wake_up_interruptible(&ch34x_dev->wait[ch]);
}

static void ch34x_read_bulk_callback(struct urb *urb)
{
	struct ch34x_rb *rb = urb->context;
	struct ch34x_pis *ch34x_dev = rb->instance;
	int status = urb->status;

	if (ch34x_dev->disconnected || !ch34x_dev->buffered_mode[rb->ch]) {
		set_bit(rb->index, &ch34x_dev->read_urbs_free[rb->ch]);
		return;
	}

	if (status) {
		set_bit(rb->index, &ch34x_dev->read_urbs_free[rb->ch]);
		dev_dbg(&ch34x_dev->interface->dev,
			"%s - non-zero urb status: %d\n", __func__,
			status);
		if (status == -ECONNRESET || status == -ENOENT ||
		    status == -ESHUTDOWN)
			return;
		else
			goto resubmit;
	}

	usb_mark_last_busy(ch34x_dev->udev);
	ch34x_process_read_urb(ch34x_dev, rb->ch, urb);
	set_bit(rb->index, &ch34x_dev->read_urbs_free[rb->ch]);
resubmit:
	ch34x_submit_read_urb(ch34x_dev, rb->ch, rb->index, GFP_ATOMIC);
}

/*
 * usb class driver info in order to get a minor number from the usb core
 * and to have the device registered with the driver core
 */
static struct usb_class_driver ch34x_class = {
	.name = "ch34x_pis%d",
	.fops = &ch34x_fops_driver,
	.minor_base = CH34x_MINOR_BASE,
};

static void get_usb_device_info(struct ch34x_pis *ch34x_dev,
				struct usb_device *udev)
{
	int ret;

	if (usb_make_path(udev, ch34x_dev->location,
			  sizeof(ch34x_dev->location)) >= 0) {
		dev_info(&udev->dev, "USB Location: %s\n",
			 ch34x_dev->location);
	}

	ch34x_dev->device_desc = udev->descriptor;

	if (udev->descriptor.iManufacturer) {
		ret = usb_string(udev, udev->descriptor.iManufacturer,
				 ch34x_dev->manufacturer,
				 sizeof(ch34x_dev->manufacturer));
		if (ret > 0)
			dev_info(&udev->dev, "Manufacturer: %s\n",
				 ch34x_dev->manufacturer);
	}

	if (udev->descriptor.iProduct) {
		ret = usb_string(udev, udev->descriptor.iProduct,
				 ch34x_dev->product,
				 sizeof(ch34x_dev->product));
		if (ret > 0)
			dev_info(&udev->dev, "Product: %s\n",
				 ch34x_dev->product);
	}

	if (udev->descriptor.iSerialNumber) {
		ret = usb_string(udev, udev->descriptor.iSerialNumber,
				 ch34x_dev->serial,
				 sizeof(ch34x_dev->serial));
		if (ret > 0)
			dev_info(&udev->dev, "Serial Number: %s\n",
				 ch34x_dev->serial);
	}
}

static int ch34x_pis_probe(struct usb_interface *intf,
			   const struct usb_device_id *id)
{
	struct usb_host_interface *iface_desc;
	struct usb_endpoint_descriptor *endpoint;
	struct ch34x_pis *ch34x_dev;
	size_t ep_size;
	u16 epsize_intr = 0;
	int retval = -ENOMEM;
	int i;
	u8 ch;

	/* Allocate memory for our device state and initialize it */
	ch34x_dev = kzalloc(sizeof(struct ch34x_pis), GFP_KERNEL);
	if (!ch34x_dev) {
		dev_err(&intf->dev, "Out of memory\n");
		return -ENOMEM;
	}

	kref_init(&ch34x_dev->kref);
	spin_lock_init(&ch34x_dev->err_lock);
	mutex_init(&ch34x_dev->io_mutex);
	mutex_init(&ch34x_dev->open_mutex);
	ch34x_dev->open_count = 0;

	ch34x_dev->udev = usb_get_dev(interface_to_usbdev(intf));
	ch34x_dev->interface = usb_get_intf(intf);

	get_usb_device_info(ch34x_dev, ch34x_dev->udev);

	iface_desc = intf->cur_altsetting;
	ch34x_dev->rx_buflimit = CH34X_NR;

	ch34x_dev->readtimeout = DEFAULT_TIMEOUT;
	ch34x_dev->writetimeout = DEFAULT_TIMEOUT;

	spin_lock_init(&ch34x_dev->write_lock);
	init_waitqueue_head(&ch34x_dev->wq_send);
	ch34x_dev->writesize = MAX_BUFFER_LENGTH;

	/* Chip type determination */
	if (id->idProduct == 0x5512)
		ch34x_dev->chiptype = CHIP_CH341_VENDOR;
	else if (id->idProduct == 0x55db)
		ch34x_dev->chiptype = CHIP_CH347T_VENDOR;
	else if (id->idProduct == 0x55de)
		ch34x_dev->chiptype = CHIP_CH347F_VENDOR;
	else if (id->idProduct == 0x55e7)
		ch34x_dev->chiptype = CHIP_CH339W_VENDOR;
	else if (id->idProduct == 0x55eb) {
		ch34x_dev->schannel = 1;
		ch34x_dev->chiptype = CHIP_CH346C_VENDOR;
	} else if (id->idProduct == 0x55ee) {
		ch34x_dev->schannel = 2;
		ch34x_dev->chiptype = CHIP_CH9341C_VENDOR;
	} else if (id->idProduct == 0xe100) {
		ch34x_dev->schannel = 1;
		ch34x_dev->chiptype = CHIP_CH9346W_VENDOR;
	}

	/* Traverse endpoints */
	for (i = 0; i < iface_desc->desc.bNumEndpoints; ++i) {
		endpoint = &iface_desc->endpoint[i].desc;

		if (usb_endpoint_is_bulk_in(endpoint)) {
			if (ch34x_dev->bulkin_num >= MAXCH) {
				retval = -EINVAL;
				goto error;
			}
			ep_size = usb_endpoint_maxp(endpoint);
			if (!ep_size) {
				retval = -EINVAL;
				goto error;
			}
			ch34x_dev->bulk_in_endpointAddr
				[ch34x_dev->bulkin_num] =
				endpoint->bEndpointAddress;

			ch34x_dev->rx_endpoint[ch34x_dev->bulkin_num] =
				usb_rcvbulkpipe(
					ch34x_dev->udev,
					endpoint->bEndpointAddress);
			ch34x_dev->readsize[ch34x_dev->bulkin_num] =
				ep_size * 8;
			ch34x_dev->bulkin_num++;
		}

		if (usb_endpoint_is_bulk_out(endpoint)) {
			if (ch34x_dev->bulkout_num >= MAXCH) {
				retval = -EINVAL;
				goto error;
			}
			ch34x_dev->bulk_out_endpointAddr
				[ch34x_dev->bulkout_num] =
				endpoint->bEndpointAddress;
			ch34x_dev->tx_endpoint[ch34x_dev->bulkout_num] =
				usb_sndbulkpipe(
					ch34x_dev->udev,
					endpoint->bEndpointAddress);
			ch34x_dev->bulkout_num++;
		}

		if (usb_endpoint_is_int_in(endpoint)) {
			ch34x_dev->interrupt_in_endpoint = endpoint;
			epsize_intr = usb_endpoint_maxp(endpoint);
		}
	}

	if (ch34x_dev->interrupt_in_endpoint && epsize_intr > 0) {
		ch34x_dev->interrupt_in_buffer =
			kmalloc(epsize_intr, GFP_KERNEL);
		/* Create URBs for handling interrupts */
		ch34x_dev->interrupt_in_urb = usb_alloc_urb(0, GFP_KERNEL);
		if (!ch34x_dev->interrupt_in_urb ||
		    !ch34x_dev->interrupt_in_buffer) {
			retval = -ENOMEM;
			goto error;
		}
		usb_fill_int_urb(
			ch34x_dev->interrupt_in_urb, ch34x_dev->udev,
			usb_rcvintpipe(
				ch34x_dev->udev,
				usb_endpoint_num(
					ch34x_dev->interrupt_in_endpoint)),
			ch34x_dev->interrupt_in_buffer, epsize_intr,
			ch34x_usb_complete_intr_urb, ch34x_dev,
			ch34x_dev->interrupt_in_endpoint->bInterval);
	}

	if (ch34x_dev->chiptype == CHIP_CH346C_VENDOR ||
	    ch34x_dev->chiptype == CHIP_CH9341C_VENDOR ||
	    ch34x_dev->chiptype == CHIP_CH9346W_VENDOR) {
		if (ch34x_dev->bulkin_num < ch34x_dev->schannel) {
			dev_err(&intf->dev,
				"Insufficient bulk-in endpoints for schannel: %d < %d\n",
				ch34x_dev->bulkin_num,
				ch34x_dev->schannel);
			retval = -ENODEV;
			goto error;
		}

		for (ch = 0; ch < ch34x_dev->schannel; ch++) {
			spin_lock_init(&ch34x_dev->read_lock[ch]);
			init_waitqueue_head(&ch34x_dev->wait[ch]);
			for (i = 0; i < ch34x_dev->rx_buflimit; i++) {
				struct ch34x_rb *rb =
					&(ch34x_dev->read_buffers[ch][i]);
				struct urb *urb;

				rb->base = usb_alloc_coherent(
					ch34x_dev->udev,
					ch34x_dev->readsize[ch],
					GFP_KERNEL, &rb->dma);
				if (!rb->base) {
					retval = -ENOMEM;
					goto error;
				}
				rb->index = i;
				rb->instance = ch34x_dev;
				rb->ch = ch;

				urb = usb_alloc_urb(0, GFP_KERNEL);
				if (!urb) {
					retval = -ENOMEM;
					goto error;
				}

				urb->transfer_flags |=
					URB_NO_TRANSFER_DMA_MAP;
				urb->transfer_dma = rb->dma;

				usb_fill_bulk_urb(
					urb, ch34x_dev->udev,
					ch34x_dev->rx_endpoint[ch],
					rb->base, ch34x_dev->readsize[ch],
					ch34x_read_bulk_callback, rb);

				ch34x_dev->read_urbs[ch][i] = urb;
				__set_bit(i,
					  &ch34x_dev->read_urbs_free[ch]);
			}
			retval = kfifo_alloc(&ch34x_dev->rfifo[ch],
					     CH34X_KFIFO_LENGTH,
					     GFP_KERNEL);
			if (retval) {
				dev_err(&intf->dev,
					"%s - kfifo_alloc failed\n",
					__func__);
				goto error;
			}
		}
	}

	retval = ch34x_write_buffers_alloc(ch34x_dev);
	if (retval)
		goto error;

	for (i = 0; i < CH34X_NW; i++) {
		struct ch34x_wb *wb = &(ch34x_dev->wb[i]);

		wb->urb = usb_alloc_urb(0, GFP_KERNEL);
		if (wb->urb == NULL) {
			retval = -ENOMEM;
			goto error;
		}

		usb_fill_bulk_urb(wb->urb, ch34x_dev->udev, 0, NULL,
				  ch34x_dev->writesize,
				  ch34x_write_bulk_callback, wb);
		wb->urb->transfer_flags |= URB_NO_TRANSFER_DMA_MAP;
		wb->instance = ch34x_dev;
	}

	/* Save our data point in this interface device */
	usb_set_intfdata(intf, ch34x_dev);

	retval = usb_register_dev(intf, &ch34x_class);
	if (retval) {
		/* Something prevented us from registering this driver */
		dev_err(&intf->dev,
			"Not able to get a minor for this device.\n");
		usb_set_intfdata(intf, NULL);
		goto error;
	}

	snprintf(ch34x_dev->pathname, sizeof(ch34x_dev->pathname),
		 "/dev/%s", dev_name(intf->usb_dev));

	/* Let the user know what node this device is now attached to */
	dev_info(&intf->dev, "USB device ch34x_pis #%d now attached",
		 intf->minor);

	return 0;

error:
	if (ch34x_dev)
		kref_put(&ch34x_dev->kref, ch34x_delete);
	return retval;
}

#ifdef CONFIG_PM
static int ch34x_pis_suspend(struct usb_interface *intf,
			     pm_message_t message)
{
	struct ch34x_pis *ch34x_dev = usb_get_intfdata(intf);
	int cnt;

	spin_lock_irq(&ch34x_dev->write_lock);
	if (PMSG_IS_AUTO(message)) {
		if (ch34x_dev->transmitting) {
			spin_unlock_irq(&ch34x_dev->write_lock);
			return -EBUSY;
		}
	}
	cnt = ch34x_dev->susp_count++;
	spin_unlock_irq(&ch34x_dev->write_lock);
	if (cnt)
		return 0;
	stop_data_traffic(ch34x_dev);

	return 0;
}

static int ch34x_pis_resume(struct usb_interface *intf)
{
	struct ch34x_pis *ch34x_dev = usb_get_intfdata(intf);
	int rv = 0;
	int ch;

	spin_lock_irq(&ch34x_dev->write_lock);
	if (--ch34x_dev->susp_count)
		goto out;

	if (ch34x_dev->irq_enable && ch34x_dev->interrupt_in_urb)
		rv = usb_submit_urb(ch34x_dev->interrupt_in_urb,
				    GFP_ATOMIC);
	for (ch = 0; ch < ch34x_dev->schannel; ch++) {
		if (ch34x_dev->buffered_mode[ch])
			ch34x_submit_read_urbs(ch34x_dev, ch, GFP_ATOMIC);
	}

out:
	spin_unlock_irq(&ch34x_dev->write_lock);
	return rv;
}
#endif

static void stop_data_traffic(struct ch34x_pis *ch34x_dev)
{
	int i;
	u8 ch;

	if (ch34x_dev->interrupt_in_urb)
		usb_kill_urb(ch34x_dev->interrupt_in_urb);

	for (i = 0; i < CH34X_NW; i++) {
		if (ch34x_dev->wb[i].urb)
			usb_kill_urb(ch34x_dev->wb[i].urb);
	}

	for (ch = 0; ch < ch34x_dev->schannel; ch++) {
		for (i = 0; i < ch34x_dev->rx_buflimit; i++) {
			if (ch34x_dev->read_urbs[ch][i])
				usb_kill_urb(ch34x_dev->read_urbs[ch][i]);
		}
	}
}

static void ch34x_usb_free_device(struct ch34x_pis *ch34x_dev)
{
	int i;
	u8 ch;

	if (ch34x_dev->interrupt_in_urb)
		usb_free_urb(ch34x_dev->interrupt_in_urb);

	if (ch34x_dev->interrupt_in_buffer)
		kfree(ch34x_dev->interrupt_in_buffer);

	if (ch34x_dev->chiptype == CHIP_CH346C_VENDOR ||
	    ch34x_dev->chiptype == CHIP_CH9341C_VENDOR ||
	    ch34x_dev->chiptype == CHIP_CH9346W_VENDOR) {
		for (ch = 0; ch < ch34x_dev->schannel; ch++) {
			for (i = 0; i < ch34x_dev->rx_buflimit; i++) {
				if (ch34x_dev->read_urbs[ch][i])
					usb_free_urb(
						ch34x_dev->read_urbs[ch]
								    [i]);
			}
			ch34x_read_buffers_free(ch34x_dev, ch);
		}
	}

	for (i = 0; i < CH34X_NW; i++) {
		if (ch34x_dev->wb[i].urb)
			usb_free_urb(ch34x_dev->wb[i].urb);
	}

	ch34x_write_buffers_free(ch34x_dev);
}

static void ch34x_delete(struct kref *kref)
{
	struct ch34x_pis *ch34x_dev =
		container_of(kref, struct ch34x_pis, kref);

	usb_put_intf(ch34x_dev->interface);
	usb_put_dev(ch34x_dev->udev);
	ch34x_usb_free_device(ch34x_dev);
	kfree(ch34x_dev);
}

static void ch34x_pis_disconnect(struct usb_interface *intf)
{
	struct ch34x_pis *ch34x_dev = usb_get_intfdata(intf);
	int minor = intf->minor;
	u8 ch;

	if (!ch34x_dev)
		return;

	/* Give back our minor */
	usb_deregister_dev(intf, &ch34x_class);

	/* Prevent more I/O from starting */
	mutex_lock(&ch34x_dev->io_mutex);
	ch34x_dev->disconnected = true;
	wake_up_all(&ch34x_dev->wq_send);
	for (ch = 0; ch < ch34x_dev->schannel; ch++)
		wake_up_interruptible(&ch34x_dev->wait[ch]);
	kill_fasync(&ch34x_dev->fasync, SIGIO, POLL_HUP);
	usb_set_intfdata(intf, NULL);
	mutex_unlock(&ch34x_dev->io_mutex);

	stop_data_traffic(ch34x_dev);

	/* Decrement our usage count*/
	kref_put(&ch34x_dev->kref, ch34x_delete);

	pr_info("ch34x_pis-%d now disconnected.\n", minor);
}

static int ch34x_pre_reset(struct usb_interface *intf)
{
	struct ch34x_pis *ch34x_dev = usb_get_intfdata(intf);

	mutex_lock(&ch34x_dev->io_mutex);
	stop_data_traffic(ch34x_dev);

	return 0;
}

static int ch34x_post_reset(struct usb_interface *intf)
{
	struct ch34x_pis *ch34x_dev = usb_get_intfdata(intf);

	ch34x_dev->errors = -EPIPE;
	mutex_unlock(&ch34x_dev->io_mutex);

	return 0;
}

/* usb driver Interface */
static struct usb_driver ch34x_pis_driver = {
	.name = "ch34x_pis",
	.probe = ch34x_pis_probe,
	.disconnect = ch34x_pis_disconnect,
	.suspend = ch34x_pis_suspend,
	.resume = ch34x_pis_resume,
	.pre_reset = ch34x_pre_reset,
	.post_reset = ch34x_post_reset,
	.id_table = ch34x_usb_ids,
	.supports_autosuspend = 1,
};

static int __init ch34x_pis_init(void)
{
	int retval;

	pr_info(KBUILD_MODNAME ": " DRIVER_DESC "\n");
	pr_info(KBUILD_MODNAME ": " VERSION_DESC "\n");
	retval = usb_register(&ch34x_pis_driver);
	if (retval)
		pr_err("CH34X driver registration failed.\n");
	return retval;
}

static void __exit ch34x_pis_exit(void)
{
	pr_info(KBUILD_MODNAME ": CH34X driver has been deregistered.\n");
	usb_deregister(&ch34x_pis_driver);
}

module_init(ch34x_pis_init);
module_exit(ch34x_pis_exit);

MODULE_AUTHOR(DRIVER_AUTHOR);
MODULE_DESCRIPTION(DRIVER_DESC);
MODULE_VERSION(VERSION_DESC);
MODULE_LICENSE("GPL");
