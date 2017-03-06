#ifndef _USBD_8DEV_H_
#define _USBD_8DEV_H_

#include "usbd_def.h"

// No idea why not EP1-IN (0x81), EP1-OUT (0x01), EP2-IN (0x82), EP2-OUT (0x02)
#define USBD_8DEV_DATA_IN_EP    0x81  /* EP1 for data IN */
#define USBD_8DEV_DATA_OUT_EP   0x02  /* EP2 for data OUT */
#define USBD_8DEV_CMD_IN_EP     0x83  /* EP3 for command IN */
#define USBD_8DEV_CMD_OUT_EP    0x04  /* EP4 for command OUT */

#define USBD_8DEV_FS_MAX_PACKET_SIZE        64  /* Endpoint max packet size (bytes) */
#define USBD_8DEV_DATA_FS_IN_PACKET_SIZE    USBD_8DEV_FS_MAX_PACKET_SIZE
#define USBD_8DEV_DATA_FS_OUT_PACKET_SIZE   USBD_8DEV_FS_MAX_PACKET_SIZE
#define USBD_8DEV_CMD_FS_IN_PACKET_SIZE     USBD_8DEV_FS_MAX_PACKET_SIZE
#define USBD_8DEV_CMD_FS_OUT_PACKET_SIZE    USBD_8DEV_FS_MAX_PACKET_SIZE

typedef struct _usbd_8dev_itf {
    uint8_t (*init)(void);
    uint8_t (*deinit)(void);
    uint8_t (*receive_command)(uint8_t *pbuf, uint8_t *len);
    uint8_t (*receive_data)(uint8_t *pbuf, uint8_t *len);
} USBD_8DEV_ItfTypeDef;

extern USBD_ClassTypeDef usbd_8dev;

uint8_t usbd_8dev_registerinterface(USBD_HandleTypeDef *pdev, USBD_8DEV_ItfTypeDef *fops);
uint8_t usbd_8dev_set_cmd_txbuf(USBD_HandleTypeDef *pdev, uint8_t *pbuff, uint16_t len);
uint8_t usbd_8dev_set_cmd_rxbuf(USBD_HandleTypeDef *pdev, uint8_t *pbuff);
uint8_t usbd_8dev_set_data_txbuf(USBD_HandleTypeDef *pdev, uint8_t *pbuff, uint16_t len);
uint8_t usbd_8dev_set_data_rxbuf(USBD_HandleTypeDef *pdev, uint8_t *pbuff);
uint8_t usbd_8dev_transmit_cmd_packet(USBD_HandleTypeDef *pdev);
uint8_t usbd_8dev_transmit_data_packet(USBD_HandleTypeDef *pdev);
uint8_t usbd_8dev_receive_cmd_packet(USBD_HandleTypeDef *pdev);
uint8_t usbd_8dev_receive_data_packet(USBD_HandleTypeDef *pdev);

#endif
