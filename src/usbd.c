#include "usbd.h"
#include "usbd_cdc.h"
#include "usbd_cdc_interface.h"
#include "usbd_desc.h"

USBD_HandleTypeDef USBD_Device;

void usb_init() {
  // Init Device Library
  USBD_Init(&USBD_Device, &VCP_Desc, 0);
  
  // Add Supported Class
  USBD_RegisterClass(&USBD_Device, &USBD_CDC);
  
  // Add CDC Interface Class
  USBD_CDC_RegisterInterface(&USBD_Device, &USBD_CDC_fops);

  // Start Device Process
  USBD_Start(&USBD_Device);
}
