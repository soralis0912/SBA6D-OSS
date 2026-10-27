// SPDX-License-Identifier: GPL-2.0+
/*
 * u_serial.h - interface to USB gadget "serial port"/TTY utilities
 *
 * Copyright (C) 2008 David Brownell
 * Copyright (C) 2008 by Nokia Corporation
 */

#ifndef __U_SERIAL_H
#define __U_SERIAL_H

#include <linux/usb/composite.h>
#include <linux/usb/cdc.h>

#define MAX_U_SERIAL_PORTS	4

struct f_serial_opts {
	struct usb_function_instance func_inst;
	u8 port_num;
};

enum gserial_suspend_state {
	GSERIAL_STATE_WAKEUP,
	GSERIAL_STATE_SUSPEND,
};

enum gs_port_low_power_state {
	GS_PORT_STATE_NORMAL,
	GS_PORT_STATE_LOW_POWER,
};

enum gs_port_wakeup_enabled {
	GS_PORT_WAKEUP_DISABLED,
	GS_PORT_WAKEUP_ENABLED,
};

/*
 * One non-multiplexed "serial" I/O port ... there can be several of these
 * on any given USB peripheral device, if it provides enough endpoints.
 *
 * The "u_serial" utility component exists to do one thing:  manage TTY
 * style I/O using the USB peripheral endpoints listed here, including
 * hookups to sysfs and /dev for each logical "tty" device.
 *
 * REVISIT at least ACM could support tiocmget() if needed.
 *
 * REVISIT someday, allow multiplexing several TTYs over these endpoints.
 */
struct gserial {
	struct usb_function		func;

	/* port is managed by gserial_{connect,disconnect} */
	struct gs_port			*ioport;

	struct usb_ep			*in;
	struct usb_ep			*out;

	/* REVISIT avoid this CDC-ACM support harder ... */
	struct usb_cdc_line_coding port_line_coding;	/* 9600-8-N-1 etc */

	enum gserial_suspend_state gser_suspend_state;
	u8  ctrl_id;

	/* notification callbacks */
	void (*connect)(struct gserial *p);
	void (*disconnect)(struct gserial *p);
	int (*send_break)(struct gserial *p, int duration);
};

/* utilities to allocate/free request and buffer */
struct usb_request *gs_alloc_req(struct usb_ep *ep, unsigned len, gfp_t flags);
void gs_free_req(struct usb_ep *, struct usb_request *req);

/* management of individual TTY ports */
int gserial_alloc_line(unsigned char *port_line);
void gserial_free_line(unsigned char port_line);

/* connect/disconnect is handled by individual functions */
int gserial_connect(struct gserial *, u8 port_num);
void gserial_disconnect(struct gserial *);

/* functions are bound to configurations by a config or gadget driver */
int gser_bind_config(struct usb_configuration *c, u8 port_num);
int obex_bind_config(struct usb_configuration *c, u8 port_num);

void gserial_suspend(struct gserial *gser);
void gserial_resume(struct gserial *gser);
int gs_port_get_status(struct gserial *gser);
int gs_port_update_suspend(struct gserial *gser, u8 suspend_opt);

#endif /* __U_SERIAL_H */
