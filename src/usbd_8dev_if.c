/**
 * Handle device driver protocol.
 *
 * Responsible for sending the correct message format, responding to commands
 * etc. All USB communication is piped through @see usbd_8dev.c
 * This file contains the main working of the device and is mostly event
 * driven through callbacks @see USBD_8DEV_ItfTypeDef received from @see
 * usbd_8dev.c
 *
 * How it works
 *  Commands are exchanged in @see usb_8dev_cmd_msg format.
 *  Commands:
 *      open: start monitoring the CAN bus and USB. Relay the data between
 *      interfaces
 *      close: disable the can bus
 *      version: send the current hardware and firmware version
 *
 *  Data is sent in @see usb_8dev_tx_msg and received in @see usb_8dev_rx_msg
 *  format and mostly consists of a CAN frame and starts/stops once open/close
 *  command is called respectively.
 */
#include "usbd_8dev_if.h"
#include "stm32f0xx_hal.h"
#include "led.h"
#include "can.h"
#include "requests.h"

#define FIRMWARE_VER    0x0010  /* bcd v0.1 */
#define HARDWARE_VER    0x0010  /* bcd v0.1 */

// Command message constants
#define USB_8DEV_CMD_START      0x11
#define USB_8DEV_CMD_END        0x22
#define USB_8DEV_CMD_SUCCESS    0
#define USB_8DEV_CMD_ERROR      255

// Data message constants
#define USB_8DEV_DATA_START     0x55
#define USB_8DEV_DATA_END       0xAA

// Command type
#define USB_8DEV_TYPE_CAN_FRAME     0
#define USB_8DEV_TYPE_ERROR_FRAME   3

// usb_8dev_tx_msg flags
//#define USB_8DEV_STDID          0x00
#define USB_8DEV_EXTID          0x01
#define USB_8DEV_RTR            0x02
#define USB_8DEV_ERR            0x04

#define USB_8DEV_ERROR_OK       0x00
//#define USB_8DEV_ERROR_OVERRUN  0x01
#define USB_8DEV_ERROR_EWG      0x02
#define USB_8DEV_ERROR_EPV      0x03
#define USB_8DEV_ERROR_BOF      0x04
#define USB_8DEV_ERROR_STF      0x20
#define USB_8DEV_ERROR_FOR      0x21
#define USB_8DEV_ERROR_ACK      0x23
#define USB_8DEV_ERROR_BR       0x24
#define USB_8DEV_ERROR_BD       0x25
#define USB_8DEV_ERROR_CRC      0x27
#define USB_8DEV_ERROR_UNK      0xff

// Needed because 8dev works at 32MHz, and this device at 48MHz
#define TQ_SCALE    (1.5)       /* 48MHz/32MHz = 1.5 */

enum usb_8dev_cmd {
    USB_8DEV_RESET = 1,             /* not used */
    USB_8DEV_OPEN,
    USB_8DEV_CLOSE,
    USB_8DEV_SET_SPEED,             /* not used */
    USB_8DEV_SET_MASK_FILTER,       /* not used */
    USB_8DEV_GET_STATUS,            /* not used */
    USB_8DEV_GET_STATISTICS,        /* not used */
    USB_8DEV_GET_SERIAL,            /* not used */
    USB_8DEV_GET_SOFTW_VER,         /* not used */
    USB_8DEV_GET_HARDW_VER,         /* not used */
    USB_8DEV_RESET_TIMESTAMP,       /* not used */
    USB_8DEV_GET_SOFTW_HARDW_VER
};

/* Format of transmitted USB data messages. */
typedef struct __packed usb_8dev_tx_msg {
    uint8_t start;      // start of message byte
    uint8_t type;       // frame type
    uint8_t flags;      // RTR and EXT_ID flag
    uint32_t id;        // upper 3 bits not used
    uint8_t dlc;        // data length code 0-8 bytes
    uint8_t data[8];    // 64-bit data
    uint32_t timestamp; // 32-bit timestamp
    uint8_t end;        // end of message byte
} Msg_TxTypeDef;

/* Format of received USB data messages. */
typedef struct __packed usb_8dev_rx_msg {
    uint8_t start;      // start of message byte
    uint8_t flags;      // RTR and EXT_ID flag
    uint32_t id;        // upper 3 bits not used
    uint8_t dlc;        // data length code 0-8 bytes
    uint8_t data[8];    // 64-bit data
    uint8_t end;        // end of message byte
} Msg_RxTypeDef;

/* Format of both received and transmitted USB command messages. */
typedef struct __packed usb_8dev_cmd_msg {
    uint8_t start;       // start of message byte
    uint8_t channel;     // unknown - always 0
    uint8_t command;     // command to execute
    uint8_t opt1;        // optional parameter / return value
    uint8_t opt2;        // optional parameter 2
    uint8_t data[10];    // optional parameter and data
    uint8_t end;         // end of message byte
} Msg_CmdTypeDef;

Msg_TxTypeDef buf_datatx;// = {USB_8DEV_CMD_START, 0, 0, 0, 0, {0}, 0, USB_8DEV_CMD_END};
Msg_RxTypeDef buf_datarx;
Msg_CmdTypeDef buf_cmdtx;// = {USB_8DEV_CMD_START, 0, 0, 0, 0, {0}, USB_8DEV_CMD_END};
Msg_CmdTypeDef buf_cmdrx;

extern USBD_HandleTypeDef usbd_handle;

static uint8_t usbd_8dev_itf_init(void);
static uint8_t usbd_8dev_itf_deinit(void);
static uint8_t usbd_8dev_itf_rcv_cmd(uint8_t* pbuf, uint8_t *len);
static uint8_t usbd_8dev_itf_rcv_data(uint8_t* pbuf, uint8_t *len);

static void Error_Handler(void);

/**
 * USB 8dev class callbacks.
 *
 * Callback from 8dev class for protocol specifics.
 */
USBD_8DEV_ItfTypeDef usbd_8dev_fops = {
    usbd_8dev_itf_init,
    usbd_8dev_itf_deinit,
    usbd_8dev_itf_rcv_cmd,
    usbd_8dev_itf_rcv_data
};

void usbd_8dev_send_cmd_rsp(uint8_t error) {
    UNUSED(error);
    buf_cmdtx.opt1 = error ? USB_8DEV_CMD_ERROR : USB_8DEV_CMD_SUCCESS;
    if (usbd_8dev_transmit_cmd_packet(&usbd_handle)) {
        Error_Handler();
    }
    if (usbd_8dev_receive_cmd_packet(&usbd_handle)) {
        Error_Handler();
    }
}

uint8_t usbd_8dev_transmit(CanRxMsgTypeDef *buf_canrx) {
    uint8_t i;
    buf_datatx.start = USB_8DEV_DATA_START;
    buf_datatx.type = USB_8DEV_TYPE_CAN_FRAME;
    buf_datatx.flags = 0; // Default is STD ID.
    if (buf_canrx->IDE == CAN_ID_EXT) {
        buf_datatx.flags |= USB_8DEV_EXTID;
        buf_datatx.id = __builtin_bswap32(buf_canrx->ExtId & 0x1ffffff);
    } else {
        buf_datatx.id = __builtin_bswap32(buf_canrx->StdId & 0x00007ff);
    }
    if (buf_canrx->RTR) buf_datatx.flags |= USB_8DEV_RTR;
    buf_datatx.dlc = buf_canrx->DLC;
    for (i=0; i<buf_canrx->DLC; i++) {
        buf_datatx.data[i] = buf_canrx->Data[i];
    }
    buf_datatx.timestamp = HAL_GetTick();
    buf_datatx.end = USB_8DEV_DATA_END;
    return usbd_8dev_transmit_data_packet(&usbd_handle);
}

uint8_t usbd_8dev_receive() {
    return usbd_8dev_receive_data_packet(&usbd_handle);
}

static uint8_t usbd_8dev_itf_init(void) {
    usbd_8dev_set_cmd_txbuf(&usbd_handle, (uint8_t*) &buf_cmdtx, sizeof(Msg_CmdTypeDef));
    usbd_8dev_set_cmd_rxbuf(&usbd_handle, (uint8_t*)  &buf_cmdrx);
    usbd_8dev_set_data_txbuf(&usbd_handle, (uint8_t*)  &buf_datatx, sizeof(Msg_TxTypeDef));
    usbd_8dev_set_data_rxbuf(&usbd_handle, (uint8_t*)  &buf_datarx);
    return (USBD_OK);
}

static uint8_t usbd_8dev_itf_deinit(void) {
    requests |= REQ_CAN_CLOSE;
    return (USBD_OK);
}

// buf == buf_cmdrx
static uint8_t usbd_8dev_itf_rcv_cmd(uint8_t* buf, uint8_t *len) {
    UNUSED(buf);
    Can_BitTimingTypeDef can_bittiming;
    if (*len == sizeof(buf_cmdrx) && buf_cmdrx.start == USB_8DEV_CMD_START && buf_cmdrx.end == USB_8DEV_CMD_END) {
        buf_cmdtx.start = USB_8DEV_CMD_START;
        buf_cmdtx.channel = buf_cmdrx.channel;
        buf_cmdtx.command = buf_cmdrx.command;
        buf_cmdtx.opt1 = buf_cmdrx.opt1;
        buf_cmdtx.opt2 = buf_cmdrx.opt2;
        buf_cmdtx.end = USB_8DEV_CMD_END;
        switch (buf_cmdrx.command) {
            case USB_8DEV_GET_SOFTW_HARDW_VER:
                buf_cmdtx.data[0] = (FIRMWARE_VER & 0xff00) >> 8; // major
                buf_cmdtx.data[1] = (FIRMWARE_VER & 0x00f0) >> 4; // minor
                buf_cmdtx.data[2] = (HARDWARE_VER & 0xff00) >> 8; // major
                buf_cmdtx.data[3] = (HARDWARE_VER & 0x00f0) >> 4; // minor
                buf_cmdtx.opt1 = USB_8DEV_CMD_SUCCESS;
                requests |= REQ_VER;
                break;
            case USB_8DEV_OPEN:
                can_bittiming.ts1 = buf_cmdrx.data[0] - 1;
                can_bittiming.ts2 = buf_cmdrx.data[1] - 1;
                can_bittiming.sjw = buf_cmdrx.data[2] - 1;
                can_bittiming.brp = (buf_cmdrx.data[3] << 8) | buf_cmdrx.data[4];
                can_bittiming.brp *= TQ_SCALE;
                /*  Ctrl mode is be32 stored in data[5]..data[8], only 1 byte
                 *  is used. If multiple bytes are used, ctrlmode =
                 *  bswap((uint32_t) data[5]..data[8])
                 */
                can_open_req(&can_bittiming, buf_cmdrx.data[8]);
                requests |= REQ_CAN_OPEN;
                break;
            case USB_8DEV_CLOSE:
                requests |= REQ_CAN_CLOSE;
                break;
            default:
                Error_Handler();
        }
    } else {
        Error_Handler();
    }
    return USBD_OK;
}

// buf == buf_datarx
static uint8_t usbd_8dev_itf_rcv_data(uint8_t* buf, uint8_t *len) {
    UNUSED(buf);
    uint8_t i;
    if (*len == sizeof(buf_datarx) && buf_datarx.start == USB_8DEV_DATA_START && buf_datarx.end == USB_8DEV_DATA_END) {
        // Extended frame format
        if (buf_datarx.flags & USB_8DEV_EXTID) {
            can_handle.pTxMsg->IDE = CAN_ID_EXT;
            can_handle.pTxMsg->ExtId = __builtin_bswap32(buf_datarx.id) & 0x1fffffff;
        } else { // Base frame format
            can_handle.pTxMsg->IDE = CAN_ID_STD;
            can_handle.pTxMsg->StdId = __builtin_bswap32(buf_datarx.id) & 0x00007ff;
        }
        can_handle.pTxMsg->DLC = buf_datarx.dlc;
        for (i=0; i<buf_datarx.dlc; i++) {
            can_handle.pTxMsg->Data[i] = buf_datarx.data[i];
        }
        requests |= REQ_CAN_TX;
    } else {
        Error_Handler();
    }
    return (USBD_OK);
}

void HAL_CAN_ErrorCallback(CAN_HandleTypeDef *hcan) {
    /* Multiple errors can happen simultaneously but there is no way to report
     * this to the device driver. */
    buf_datatx.start = USB_8DEV_DATA_START;
    buf_datatx.type = USB_8DEV_TYPE_ERROR_FRAME;
    buf_datatx.flags = USB_8DEV_ERR;
    buf_datatx.end = USB_8DEV_DATA_END;
    // hcan->ErrorCode is a flag. Can contain multiple errors.
#if 0
    if (hcan->ErrorCode & HAL_CAN_ERROR_NONE) {
            buf_datatx.data[0] = USB_8DEV_ERROR_OK;
    } else if (hcan->ErrorCode & HAL_CAN_ERROR_EWG) {
            buf_datatx.data[0] = USB_8DEV_ERROR_EWG;
    } else if (hcan->ErrorCode & HAL_CAN_ERROR_EPV) {
            buf_datatx.data[0] = USB_8DEV_ERROR_EPV;
    } else if (hcan->ErrorCode & HAL_CAN_ERROR_BOF) {
            buf_datatx.data[0] = USB_8DEV_ERROR_BOF;
    } else if (hcan->ErrorCode & HAL_CAN_ERROR_STF) {
            buf_datatx.data[0] = USB_8DEV_ERROR_STF;
    } else if (hcan->ErrorCode & HAL_CAN_ERROR_FOR) {
            buf_datatx.data[0] = USB_8DEV_ERROR_FOR;
    } else if (hcan->ErrorCode & HAL_CAN_ERROR_ACK) {
            buf_datatx.data[0] = USB_8DEV_ERROR_ACK;
    } else if (hcan->ErrorCode & HAL_CAN_ERROR_BR) {
            buf_datatx.data[0] = USB_8DEV_ERROR_BR;
    } else if (hcan->ErrorCode & HAL_CAN_ERROR_BD) {
            buf_datatx.data[0] = USB_8DEV_ERROR_BD;
    } else if (hcan->ErrorCode & HAL_CAN_ERROR_CRC) {
            buf_datatx.data[0] = USB_8DEV_ERROR_CRC;
    } else {
            buf_datatx.data[0] = USB_8DEV_ERROR_UNK;
    }
#else
    switch (hcan->ErrorCode) {
        case HAL_CAN_ERROR_NONE:
            buf_datatx.data[0] = USB_8DEV_ERROR_OK;
            break;
        case HAL_CAN_ERROR_EWG:
            buf_datatx.data[0] = USB_8DEV_ERROR_EWG;
            break;
        case HAL_CAN_ERROR_EPV:
            buf_datatx.data[0] = USB_8DEV_ERROR_EPV;
            break;
        case HAL_CAN_ERROR_BOF:
            buf_datatx.data[0] = USB_8DEV_ERROR_BOF;
            break;
        case HAL_CAN_ERROR_STF:
            buf_datatx.data[0] = USB_8DEV_ERROR_STF;
            break;
        case HAL_CAN_ERROR_FOR:
            buf_datatx.data[0] = USB_8DEV_ERROR_FOR;
            break;
        case HAL_CAN_ERROR_ACK:
            buf_datatx.data[0] = USB_8DEV_ERROR_ACK;
            break;
        case HAL_CAN_ERROR_BR:
            buf_datatx.data[0] = USB_8DEV_ERROR_BR;
            break;
        case HAL_CAN_ERROR_BD:
            buf_datatx.data[0] = USB_8DEV_ERROR_BD;
            break;
        case HAL_CAN_ERROR_CRC:
            buf_datatx.data[0] = USB_8DEV_ERROR_CRC;
            break;
        default:
            // Occurs when multiple errors happen simultaneously
            //buf_datatx.data[0] = USB_8DEV_ERROR_UNK;
            buf_datatx.data[0] = hcan->ErrorCode;
    }
#endif
    usbd_8dev_transmit_data_packet(&usbd_handle);
}

static void Error_Handler(void) {
    led_on(LED_RED);
}
