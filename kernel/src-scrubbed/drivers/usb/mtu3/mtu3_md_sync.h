// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2016 MediaTek Inc.
 *
 * Author: Jeremy Chou <jeremy.chou@mediatek.com>
 */

#ifndef __MTU3_MD_SYNC_H__
#define __MTU3_MD_SYNC_H__

#define USB_MAX_CLASS_NUM 8
#define USB_CLASS_MAX_IF_NUM 4
#define UBS_IF_MAX_EP_NUM 4
#define CCCI_PAYLOAD_SIZE 1500

#define USB_EP_DIRECT_OUT 0
#define USB_EP_DIRECT_IN 1

typedef enum {
	NOTIFY_MSG_ID_UNKNOWN = 0,
	NOTIFY_EVT_ATTACHED,
	NOTIFY_EVT_DETACHED,
	NOTIFY_EVT_SUSPEND,
	NOTIFY_EVT_RESUME,
	NOTIFY_EVT_RESET,
	NOTIFY_EVT_CLASS_CTRL_REQ,
	NOTIFY_EVT_FEATURE_SC,
	NOTIFY_EVT_CTRL_DATA,
	NOTIFY_FUNCTION_RESUME,
	NOTIFY_EVT_ATTACHING,
	NOTIFY_MSG_ID_NUM
} mdusb_msg_id_e;

typedef enum {
	APUSB_MSG_ID_UNKNOWN = 0,
	APUSB_RSP_STATUS_SUCCESS,
	APUSB_RSP_STATUS_STALL,
	APUSB_RSP_SEND_DATA,
	APUSB_NOTIFY_EVT_REMOTE_WAKEUP,
	APUSB_NOTIFY_EVT_EP_HALT,
	APUSB_NOTIFY_EVT_RESET_DONE,
	APUSB_NOTIFY_EVT_SUSPEND_DONE,
	APUSB_NOTIFY_EVT_ATTACHED_DONE,
	APUSB_NOTIFY_EVT_DETACHED_DONE,
	APUSB_MSG_ID_NUM
} apusb_msg_id_e;

typedef enum {
	USB_SPEED_MIN = 0,
	USB_SPEED_USB11,
	USB_SPEED_USB20,
	USB_SPEED_USB30,
	USB_SPEED_NUM
} usb_speed_enum_e;

typedef enum {
	USB_CLASS_TYPE_UNKNOWN = 0,
	USB_CLASS_TYPE_CDC_ACM1,
	USB_CLASS_TYPE_CDC_ACM2,
	USB_CLASS_TYPE_CDC_ACM3,
	USB_CLASS_TYPE_ADB,
	USB_CLASS_TYPE_MS,
	USB_CLASS_TYPE_NUM
} usb_device_type_e;

typedef enum {
	USB_INTERFACE_TYPE_UNKNOWN = 0,
	USB_INTERFACE_TYPE_ACM_CTRL,
	USB_INTERFACE_TYPE_ACM_DATA,
	USB_INTERFACE_TYPE_MSD_DATA = 0x8,
	USB_INTERFACE_TYPE_ADB_DATA = 0xB,
	USB_INTERFACE_TYPE_MAX
} usb_interface_type_e;

typedef enum __attribute__ ((packed)){
	USB_EP_TYPE_UNKNOWN = 0,
	USB_EP_TYPE_CTRL,
	USB_EP_TYPE_ISOC,
	USB_EP_TYPE_BULK,
	USB_EP_TYPE_INTR,
	USB_EP_TYPE_NUM
} usb_ep_type_e;

typedef struct dual_usb_owner_msg_s{
  mdusb_msg_id_e msg_type;
  u32 msg_length;
  char msg_data[CCCI_PAYLOAD_SIZE];
} dual_usb_owner_msg_s_t;

typedef struct apusb_rsp_s {
  apusb_msg_id_e msg_id;
  u32 msg_length;
  char data[CCCI_PAYLOAD_SIZE];
} apusb_rsp_s_t;

typedef struct endpoint_info_s{
  u32 class_device_id;
  u32 ep_address;
  usb_ep_type_e ep_type;
  u32 max_packetsize;
} endpoint_info_s_t;

typedef struct interface_info_s{
  u32 interface_id;
  usb_interface_type_e interface_type;
} interface_info_s_t;

typedef struct class_device_info_s{
  u32 class_device_id;
  usb_device_type_e class_device_type;
  u32 total_interface_num;
  interface_info_s_t if_info[USB_CLASS_MAX_IF_NUM];
  endpoint_info_s_t ep_info[UBS_IF_MAX_EP_NUM];
} class_device_info_s_t;

typedef struct usb_device_info_s{
  u32 total_class_device_num;
  class_device_info_s_t class_device_info[USB_MAX_CLASS_NUM];
} usb_device_info_s_t;

typedef struct usb_endpoint_num_s{
  u8 endpoint_num;
  u8 endpoint_direct;
} usb_endpoint_num_s_t;
#endif
