#ifndef _REQUESTS_H_
#define _REQUESTS_H_

#define REQ_VER             0x01
#define REQ_CAN_OPEN        0x02
#define REQ_CAN_CLOSE       0x04
#define REQ_CAN_TX          0x08
#define REQ_CAN_ERR         0x10

#include <stdint.h>

extern volatile uint8_t requests;

#endif
