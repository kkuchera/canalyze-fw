#ifndef _REQUESTS_H_
#define _REQUESTS_H_

#define REQ_VER             0x01
#define REQ_CAN_INIT        0x02
#define REQ_CAN_OPEN        0x04
#define REQ_CAN_RX_RDY      0x08
#define REQ_CAN_TX      0x10
#define REQ_CAN_CLOSE       0x20
#define REQ_USB_RX_CMD_RDY  0x40
#define REQ_USB_TX_CMD_RDY  0x80
#define REQ_USB_RX_DATA_RDY
#define REQ_USB_TX_DATA_RDY

#include <stdint.h>

extern volatile uint8_t requests;

#endif
