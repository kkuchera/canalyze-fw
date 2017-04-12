#ifndef _USBD_8DEV_IF_H_
#define _USBD_8DEV_IF_H_

#include "usbd_8dev.h"

extern USBD_8DEV_ItfTypeDef usbd_8dev_fops;

void usbd_8dev_send_cmd_rsp(uint8_t error);
void usbd_8dev_transmit_can_frame();
void usbd_8dev_transmit_can_error();
void usbd_8dev_receive();

#endif
