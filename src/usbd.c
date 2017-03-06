#include "usbd.h"
#include "usbd_8dev.h"
#include "usbd_8dev_if.h"
#include "usbd_core.h"
#include "usbd_desc.h"

USBD_HandleTypeDef usbd_handle;

void usb_init() {
    // Init Device Library
    USBD_Init(&usbd_handle, &usbd_8dev_desc, 0);

    // Add Supported Class
    USBD_RegisterClass(&usbd_handle, &usbd_8dev);

    // Add CDC Interface Class
    usbd_8dev_registerinterface(&usbd_handle, &usbd_8dev_fops);

    // Start Device Process
    USBD_Start(&usbd_handle);
}
