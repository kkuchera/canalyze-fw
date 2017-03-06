/**
 * Set up USB configuration.
 *
 * Responsible for setting up USB class configuration such as the configuration
 * descriptor, endpoints, etc according to the Linux 8dev device driver
 * (https://github.com/torvalds/linux/blob/master/drivers/net/can/usb/usb_8dev.c)
 * When a relevant action occurs, a callback is done via a USBD_8DEV_ItfTypeDef
 * interface. This does not handle protocol specifics, for this @see
 * usbd_8dev_if.c
 *
 * There are 4 endpoints (in addition to the default EP0)
 *  - EP1-IN, bulk, send data
 *  - EP2-OUT, bulk, receive data
 *  - EP3-IN, bulk, send command
 *  - EP4-OUT, bulk, receive command
 */
#include "usbd_8dev.h"
#include "usbd_desc.h"
#include "usbd_ctlreq.h"
#include "usbd_ioreq.h"

#define USB_8DEV_CONFIG_DESC_SIZE   46 /*< Length of the configuration descriptor */

typedef struct {
    uint8_t *buf_cmdrx;
    uint8_t *buf_cmdtx;
    uint8_t *buf_datarx;
    uint8_t *buf_datatx;
    uint8_t buf_cmdrxlen;
    uint8_t buf_cmdtxlen;
    uint8_t buf_datarxlen;
    uint8_t buf_datatxlen;

    __IO uint8_t cmdtxstate;
    __IO uint8_t cmdrxstate;
    __IO uint8_t datatxstate;
    __IO uint8_t datarxstate;
} USBD_8DEV_HandleTypeDef;

static uint8_t usbd_8dev_init(USBD_HandleTypeDef *pdev, uint8_t cfgidx);
static uint8_t usbd_8dev_deinit(USBD_HandleTypeDef *pdev, uint8_t cfgidx);
static uint8_t usbd_8dev_setup(USBD_HandleTypeDef *pdev, USBD_SetupReqTypedef *req);
static uint8_t usbd_8dev_datain(USBD_HandleTypeDef *pdev, uint8_t epnum);
static uint8_t usbd_8dev_dataout(USBD_HandleTypeDef *pdev, uint8_t epnum);
static uint8_t *usbd_8dev_getfscfgdesc(uint16_t *length);

/**
 * USB class callbacks.
 *
 * Callbacks from USB core to implement custom classes.
 */
USBD_ClassTypeDef usbd_8dev = {
    usbd_8dev_init,
    usbd_8dev_deinit,
    usbd_8dev_setup,
    /* Control endpoints */
    NULL,                           /* EP0_TxSent, */
    NULL,                           /* EP0_RxReady */
    /* Class specific endpoints */
    usbd_8dev_datain,
    usbd_8dev_dataout,
    NULL,                           /* SOF */
    NULL,                           /* IsoINIncomplete */
    NULL,                           /* IsoOUTIncomplete */
    NULL,                           /* GetHSConfigDescriptor */
    usbd_8dev_getfscfgdesc,
    NULL,                           /* GetOtherSpeedConfigDescriptor */
    NULL,                           /* GetDeviceQualifierDescriptor */
};

__ALIGN_BEGIN uint8_t usbd_8dev_cfgfsdesc[USB_8DEV_CONFIG_DESC_SIZE] __ALIGN_END = {
    /* Configuration descriptor */
    USB_LEN_CFG_DESC,               /* bLength */
    USB_DESC_TYPE_CONFIGURATION,    /* bDescriptorType */
    USB_8DEV_CONFIG_DESC_SIZE,      /* wTotalLength */
    0x00,                           /* wTotalLength */
    0x01,                           /* bNumInterfaces */
    0x01,                           /* bConfigurationValue */
    0x00,                           /* iConfiguration */
    0x00,                           /* bmAttributes */
    0x32,                           /* MaxPower: 100mA */

    /* Interface descriptor */
    USB_LEN_IF_DESC,                /* bLength */
    USB_DESC_TYPE_INTERFACE,        /* bDescriptorType */
    0x00,                           /* bInterfaceNumber */
    0x00,                           /* bAlternateSetting */
    0x04,                           /* bNumEndpoints */
    0xff,                           /* bInterfaceClass: vendor specific */
    0xff,                           /* bInterfaceSubClass: vendor specific  */
    0xff,                           /* bInterfaceProtocol: vendor specific */
    0x00,                           /* iInterface: */

    /* Endpoint 1 descriptor: data IN */
    USB_LEN_EP_DESC,                /* bLength */
    USB_DESC_TYPE_ENDPOINT,         /* bDescriptorType */
    USBD_8DEV_DATA_IN_EP,           /* bEndpointAddress */
    USBD_EP_TYPE_BULK,              /* bmAttributes */
    LOBYTE(USBD_8DEV_DATA_FS_IN_PACKET_SIZE),   /* wMaxPacketSize */
    HIBYTE(USBD_8DEV_DATA_FS_IN_PACKET_SIZE),   /* wMaxPacketSize */
    0x00,                           /* bInterval */

    /* Endpoint 2 descriptor: data OUT */
    USB_LEN_EP_DESC,                /* bLength */
    USB_DESC_TYPE_ENDPOINT,         /* bDescriptorType */
    USBD_8DEV_DATA_OUT_EP,          /* bEndpointAddress */
    USBD_EP_TYPE_BULK,              /* bmAttributes */
    LOBYTE(USBD_8DEV_DATA_FS_OUT_PACKET_SIZE),  /* wMaxPacketSize: */
    HIBYTE(USBD_8DEV_DATA_FS_OUT_PACKET_SIZE),  /* wMaxPacketSize: */
    0x00,                           /* bInterval: */

    /* Endpoint 3 descriptor: command IN */
    USB_LEN_EP_DESC,                /* bLength */
    USB_DESC_TYPE_ENDPOINT,         /* bDescriptorType */
    USBD_8DEV_CMD_IN_EP,            /* bEndpointAddress */
    USBD_EP_TYPE_BULK,              /* bmAttributes */
    LOBYTE(USBD_8DEV_CMD_FS_IN_PACKET_SIZE),    /* wMaxPacketSize */
    HIBYTE(USBD_8DEV_CMD_FS_IN_PACKET_SIZE),    /* wMaxPacketSize */
    0x00,                           /* bInterval */

    /* Endpoint 4 descriptor: command OUT */
    USB_LEN_EP_DESC,                /* bLength */
    USB_DESC_TYPE_ENDPOINT,         /* bDescriptorType */
    USBD_8DEV_CMD_OUT_EP,           /* bEndpointAddress */
    USBD_EP_TYPE_BULK,              /* bmAttributes */
    LOBYTE(USBD_8DEV_CMD_FS_OUT_PACKET_SIZE),   /* wMaxPacketSize: */
    HIBYTE(USBD_8DEV_CMD_FS_OUT_PACKET_SIZE),   /* wMaxPacketSize: */
    0x00,                           /* bInterval: */
};

uint8_t usbd_8dev_registerinterface(USBD_HandleTypeDef *pdev, USBD_8DEV_ItfTypeDef *fops) {
    if (fops) {
        pdev->pUserData = fops;
        return USBD_OK;
    } else {
        return USBD_FAIL;
    }
}

uint8_t usbd_8dev_set_cmd_txbuf(USBD_HandleTypeDef *pdev, uint8_t *pbuff, uint16_t len) {
    USBD_8DEV_HandleTypeDef *h8dev;
    if ((h8dev = (USBD_8DEV_HandleTypeDef*) pdev->pClassData)) {
        h8dev->buf_cmdtx = pbuff;
        h8dev->buf_cmdtxlen = len;
        return USBD_OK;
    } else {
        return USBD_FAIL;
    }
}

uint8_t usbd_8dev_set_cmd_rxbuf(USBD_HandleTypeDef *pdev, uint8_t *pbuff) {
    USBD_8DEV_HandleTypeDef *h8dev;
    if ((h8dev = (USBD_8DEV_HandleTypeDef*) pdev->pClassData)) {
        h8dev->buf_cmdrx = pbuff;
        return USBD_OK;
    } else {
        return USBD_FAIL;
    }
}

uint8_t usbd_8dev_set_data_txbuf(USBD_HandleTypeDef *pdev, uint8_t *pbuff, uint16_t len) {
    USBD_8DEV_HandleTypeDef *h8dev;
    if ((h8dev = (USBD_8DEV_HandleTypeDef*) pdev->pClassData)) {
        h8dev->buf_datatx = pbuff;
        h8dev->buf_datatxlen = len;
        return USBD_OK;
    } else {
        return USBD_FAIL;
    }
}

uint8_t usbd_8dev_set_data_rxbuf(USBD_HandleTypeDef *pdev, uint8_t *pbuff) {
    USBD_8DEV_HandleTypeDef *h8dev;
    if ((h8dev = (USBD_8DEV_HandleTypeDef*) pdev->pClassData)) {
        h8dev->buf_datarx = pbuff;
        return USBD_OK;
    } else {
        return USBD_FAIL;
    }
}

uint8_t usbd_8dev_transmit_cmd_packet(USBD_HandleTypeDef *pdev) {
    USBD_8DEV_HandleTypeDef *h8dev;
    if ((h8dev = (USBD_8DEV_HandleTypeDef*) pdev->pClassData)) {
        if (h8dev->cmdtxstate == 0) {
            /* Tx Transfer in progress */
            h8dev->cmdtxstate = 1;
            USBD_LL_Transmit(pdev, USBD_8DEV_CMD_IN_EP, h8dev->buf_cmdtx,
                    h8dev->buf_cmdtxlen);
            return USBD_OK;
        }
        else {
            return USBD_BUSY;
        }
    }
    else {
        return USBD_FAIL;
    }
}

uint8_t usbd_8dev_transmit_data_packet(USBD_HandleTypeDef *pdev) {
    USBD_8DEV_HandleTypeDef *h8dev;
    if ((h8dev = (USBD_8DEV_HandleTypeDef*) pdev->pClassData)) {
        if (h8dev->datatxstate == 0) {
            /* Tx Transfer in progress */
            h8dev->datatxstate = 1;
            USBD_LL_Transmit(pdev, USBD_8DEV_DATA_IN_EP, h8dev->buf_datatx,
                    h8dev->buf_datatxlen);
            return USBD_OK;
        }
        else {
            return USBD_BUSY;
        }
    }
    else {
        return USBD_FAIL;
    }

}
uint8_t usbd_8dev_receive_cmd_packet(USBD_HandleTypeDef *pdev) {
    USBD_8DEV_HandleTypeDef *h8dev;
    if ((h8dev = (USBD_8DEV_HandleTypeDef*) pdev->pClassData)) {
        if (pdev->dev_speed == USBD_SPEED_FULL) {
            USBD_LL_PrepareReceive(pdev, USBD_8DEV_CMD_OUT_EP,
                    h8dev->buf_cmdrx, USBD_8DEV_CMD_FS_OUT_PACKET_SIZE);
        }
        return USBD_OK;
    }
    else {
        return USBD_FAIL;
    }
}
uint8_t usbd_8dev_receive_data_packet(USBD_HandleTypeDef *pdev) {
    USBD_8DEV_HandleTypeDef *h8dev;
    if ((h8dev = (USBD_8DEV_HandleTypeDef*) pdev->pClassData)) {
        if (pdev->dev_speed == USBD_SPEED_FULL) {
            USBD_LL_PrepareReceive(pdev, USBD_8DEV_DATA_OUT_EP,
                    h8dev->buf_datarx, USBD_8DEV_DATA_FS_OUT_PACKET_SIZE);
        }
        return USBD_OK;
    }
    else {
        return USBD_FAIL;
    }
}

/* Callback for class initialization */
static uint8_t usbd_8dev_init(USBD_HandleTypeDef *pdev, uint8_t cfgidx) {
    UNUSED(cfgidx);
    USBD_8DEV_HandleTypeDef *h8dev;
    pdev->pClassData = USBD_malloc(sizeof (USBD_8DEV_HandleTypeDef));
    if ((h8dev = (USBD_8DEV_HandleTypeDef*) pdev->pClassData)) {
        if (pdev->dev_speed == USBD_SPEED_FULL) {
            USBD_LL_OpenEP(pdev, USBD_8DEV_DATA_IN_EP, USBD_EP_TYPE_BULK,
                    USBD_8DEV_DATA_FS_IN_PACKET_SIZE);
            USBD_LL_OpenEP(pdev, USBD_8DEV_DATA_OUT_EP, USBD_EP_TYPE_BULK,
                    USBD_8DEV_DATA_FS_OUT_PACKET_SIZE);
            USBD_LL_OpenEP(pdev, USBD_8DEV_CMD_IN_EP, USBD_EP_TYPE_BULK,
                    USBD_8DEV_CMD_FS_IN_PACKET_SIZE);
            USBD_LL_OpenEP(pdev, USBD_8DEV_CMD_OUT_EP, USBD_EP_TYPE_BULK,
                    USBD_8DEV_CMD_FS_OUT_PACKET_SIZE);

            ((USBD_8DEV_ItfTypeDef *)pdev->pUserData)->init();

            h8dev->datatxstate = 0;
            h8dev->cmdtxstate = 0;
            h8dev->datarxstate = 0;
            h8dev->cmdrxstate = 0;

            USBD_LL_PrepareReceive(pdev, USBD_8DEV_DATA_OUT_EP,
                    h8dev->buf_datarx, USBD_8DEV_DATA_FS_OUT_PACKET_SIZE);
            USBD_LL_PrepareReceive(pdev, USBD_8DEV_CMD_OUT_EP,
                    h8dev->buf_cmdrx, USBD_8DEV_CMD_FS_OUT_PACKET_SIZE);
        }
        return USBD_OK;
    } else {
        return USBD_FAIL;
    }
}

/* Callback for class deinitialization */
static uint8_t usbd_8dev_deinit(USBD_HandleTypeDef *pdev, uint8_t cfgidx) {
    UNUSED(cfgidx);
    USBD_LL_CloseEP(pdev, USBD_8DEV_DATA_IN_EP);
    USBD_LL_CloseEP(pdev, USBD_8DEV_DATA_OUT_EP);
    USBD_LL_CloseEP(pdev, USBD_8DEV_CMD_IN_EP);
    USBD_LL_CloseEP(pdev, USBD_8DEV_CMD_OUT_EP);
    if (pdev->pClassData) {
        ((USBD_8DEV_ItfTypeDef *)pdev->pUserData)->deinit();
        USBD_free(pdev->pClassData);
        pdev->pClassData = NULL;
    }
    return USBD_OK;
}

/* Callback for when setup commands are received on EP0 */
static uint8_t usbd_8dev_setup(USBD_HandleTypeDef *pdev, USBD_SetupReqTypedef *req) {
    static uint8_t ifalt = 0;
    switch (req->bmRequest & USB_REQ_TYPE_MASK) {
        case USB_REQ_TYPE_CLASS:
            break;
        case USB_REQ_TYPE_STANDARD:
            switch (req->bRequest) {
                case USB_REQ_GET_INTERFACE:
                    USBD_CtlSendData(pdev, &ifalt, 1);
                    break;
                case USB_REQ_SET_INTERFACE:
                default:
                    break;
            }
        default:
            break;
    }
    return USBD_OK;
}

/* Callback when data sent to EP<epnum>-IN has been completed. */
static uint8_t usbd_8dev_datain(USBD_HandleTypeDef *pdev, uint8_t epnum) {
    USBD_8DEV_HandleTypeDef *h8dev;
    if ((h8dev = (USBD_8DEV_HandleTypeDef*) pdev->pClassData)) {
        /* Data in */
        if ((USBD_8DEV_DATA_IN_EP & 0x0f) == epnum) {
            h8dev->datatxstate = 0;
        } else { /* Command in */
            h8dev->cmdtxstate = 0;
        }
        return USBD_OK;
    } else {
        return USBD_FAIL;
    }
}

/* Callback when data received on EP<epnum>-OUT has been completed. */
static uint8_t usbd_8dev_dataout(USBD_HandleTypeDef *pdev, uint8_t epnum) {
    USBD_8DEV_HandleTypeDef *h8dev;
    if ((h8dev = (USBD_8DEV_HandleTypeDef*) pdev->pClassData)) {
        /* Data out */
        if ((USBD_8DEV_DATA_OUT_EP & 0x0f) == epnum) {
            h8dev->buf_datarxlen = USBD_LL_GetRxDataSize(pdev, epnum);
            ((USBD_8DEV_ItfTypeDef*) pdev->pUserData)->receive_data(
                h8dev->buf_datarx, &h8dev->buf_datarxlen);
        } else { /* Command out */
            h8dev->buf_cmdrxlen = USBD_LL_GetRxDataSize(pdev, epnum);
            ((USBD_8DEV_ItfTypeDef*) pdev->pUserData)->receive_command(
                h8dev->buf_cmdrx, &h8dev->buf_cmdrxlen);
        }
        return USBD_OK;
    } else {
        return USBD_FAIL;
    }
}

static uint8_t *usbd_8dev_getfscfgdesc(uint16_t *length) {
    *length = sizeof(usbd_8dev_cfgfsdesc);
    return usbd_8dev_cfgfsdesc;
}
