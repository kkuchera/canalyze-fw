#include "stm32f0xx_hal.h"
#include "usbd_core.h"
#include "usbd_8dev.h"

PCD_HandleTypeDef hpcd;

/******************************************************************************
 * PCD BSP Routines
 *****************************************************************************/

void HAL_PCD_MspInit(PCD_HandleTypeDef *hpcd) {
    UNUSED(hpcd);
    GPIO_InitTypeDef  GPIO_InitStruct;

    __HAL_RCC_GPIOA_CLK_ENABLE();

    /* Configure USB DM and DP pins. (optional) */
    GPIO_InitStruct.Pin = (GPIO_PIN_11 | GPIO_PIN_12);
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF2_USB;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    /* Enable USB FS Clock */
    __HAL_RCC_USB_CLK_ENABLE();

    /* Enable USB FS Interrupt */
    HAL_NVIC_SetPriority(USB_IRQn, 3, 0);
    HAL_NVIC_EnableIRQ(USB_IRQn);
}

void HAL_PCD_MspDeInit(PCD_HandleTypeDef *hpcd) {
    UNUSED(hpcd);
    HAL_NVIC_DisableIRQ(USB_IRQn);
    __HAL_RCC_USB_CLK_DISABLE();
}

/******************************************************************************
 * LL Driver Callbacks (PCD -> USB Device Library)
 *****************************************************************************/

void HAL_PCD_SetupStageCallback(PCD_HandleTypeDef *hpcd) {
    USBD_LL_SetupStage((USBD_HandleTypeDef*)hpcd->pData, (uint8_t *)hpcd->Setup);
}

void HAL_PCD_DataOutStageCallback(PCD_HandleTypeDef *hpcd, uint8_t epnum) {
    USBD_LL_DataOutStage((USBD_HandleTypeDef*)hpcd->pData, epnum, hpcd->OUT_ep[epnum].xfer_buff);
}

void HAL_PCD_DataInStageCallback(PCD_HandleTypeDef *hpcd, uint8_t epnum) {
    USBD_LL_DataInStage((USBD_HandleTypeDef*)hpcd->pData, epnum, hpcd->IN_ep[epnum].xfer_buff);
}

void HAL_PCD_SOFCallback(PCD_HandleTypeDef *hpcd) {
    USBD_LL_SOF((USBD_HandleTypeDef*)hpcd->pData);
}

void HAL_PCD_ResetCallback(PCD_HandleTypeDef *hpcd) {
    USBD_LL_SetSpeed((USBD_HandleTypeDef*)hpcd->pData, USBD_SPEED_FULL);
    USBD_LL_Reset((USBD_HandleTypeDef*)hpcd->pData);
}

void HAL_PCD_SuspendCallback(PCD_HandleTypeDef *hpcd) {
    UNUSED(hpcd);
}

void HAL_PCD_ResumeCallback(PCD_HandleTypeDef *hpcd) {
    UNUSED(hpcd);
}

void HAL_PCD_ISOOUTIncompleteCallback(PCD_HandleTypeDef *hpcd, uint8_t epnum) {
    USBD_LL_IsoOUTIncomplete((USBD_HandleTypeDef*)hpcd->pData, epnum);
}

void HAL_PCD_ISOINIncompleteCallback(PCD_HandleTypeDef *hpcd, uint8_t epnum) {
    USBD_LL_IsoINIncomplete((USBD_HandleTypeDef*)hpcd->pData, epnum);
}

void HAL_PCD_ConnectCallback(PCD_HandleTypeDef *hpcd) {
    USBD_LL_DevConnected((USBD_HandleTypeDef*)hpcd->pData);
}

void HAL_PCD_DisconnectCallback(PCD_HandleTypeDef *hpcd) {
    USBD_LL_DevDisconnected((USBD_HandleTypeDef*)hpcd->pData);
}

/******************************************************************************
 * LL Driver Interface (USB Device Library --> PCD)
 *****************************************************************************/

USBD_StatusTypeDef USBD_LL_Init(USBD_HandleTypeDef *pdev) {
    /* Set LL Driver parameters */
    hpcd.Instance = USB;
    hpcd.Init.dev_endpoints = 8;
    hpcd.Init.ep0_mps = 0x40;
    hpcd.Init.phy_itface = PCD_PHY_EMBEDDED;
    hpcd.Init.speed = PCD_SPEED_FULL;
    hpcd.Init.low_power_enable = DISABLE;
    hpcd.Init.battery_charging_enable = DISABLE;
    /* Link The driver to the stack */
    hpcd.pData = pdev;
    pdev->pData = &hpcd;
    /* Initialize LL Driver */
    HAL_PCD_Init(&hpcd);

    // Why start at 0x18?
    HAL_PCDEx_PMAConfig(pdev->pData, 0x00, PCD_SNG_BUF, 0x40+0*USBD_8DEV_FS_MAX_PACKET_SIZE);
    HAL_PCDEx_PMAConfig(pdev->pData, 0x80, PCD_SNG_BUF, 0x40+1*USBD_8DEV_FS_MAX_PACKET_SIZE);
    HAL_PCDEx_PMAConfig(pdev->pData, USBD_8DEV_DATA_IN_EP, PCD_SNG_BUF, 0x40+2*USBD_8DEV_FS_MAX_PACKET_SIZE);
    HAL_PCDEx_PMAConfig(pdev->pData, USBD_8DEV_DATA_OUT_EP, PCD_SNG_BUF, 0x40+3*USBD_8DEV_FS_MAX_PACKET_SIZE);
    HAL_PCDEx_PMAConfig(pdev->pData, USBD_8DEV_CMD_IN_EP, PCD_SNG_BUF, 0x40+4*USBD_8DEV_FS_MAX_PACKET_SIZE);
    HAL_PCDEx_PMAConfig(pdev->pData, USBD_8DEV_CMD_OUT_EP, PCD_SNG_BUF, 0x40+5*USBD_8DEV_FS_MAX_PACKET_SIZE);

    return USBD_OK;
}

USBD_StatusTypeDef USBD_LL_DeInit(USBD_HandleTypeDef *pdev) {
    HAL_PCD_DeInit((PCD_HandleTypeDef*)pdev->pData);
    return USBD_OK;
}

USBD_StatusTypeDef USBD_LL_Start(USBD_HandleTypeDef *pdev) {
    HAL_PCD_Start((PCD_HandleTypeDef*)pdev->pData);
    return USBD_OK;
}

USBD_StatusTypeDef USBD_LL_Stop(USBD_HandleTypeDef *pdev) {
    HAL_PCD_Stop((PCD_HandleTypeDef*)pdev->pData);
    return USBD_OK;
}

/**
 * Open an endpoint of the Low Level Driver.
 *
 * @param  pdev: Device handle
 * @param  ep_addr: Endpoint Number
 * @param  ep_type: Endpoint Type
 * @param  ep_mps: Endpoint Max Packet Size
 * @retval USBD Status
 */
USBD_StatusTypeDef USBD_LL_OpenEP(USBD_HandleTypeDef *pdev, uint8_t ep_addr,
        uint8_t ep_type, uint16_t ep_mps) {
    HAL_PCD_EP_Open((PCD_HandleTypeDef*)pdev->pData, ep_addr, ep_mps, ep_type);
    return USBD_OK;
}

USBD_StatusTypeDef USBD_LL_CloseEP(USBD_HandleTypeDef *pdev, uint8_t ep_addr) {
    HAL_PCD_EP_Close((PCD_HandleTypeDef*)pdev->pData, ep_addr);
    return USBD_OK;
}

USBD_StatusTypeDef USBD_LL_FlushEP(USBD_HandleTypeDef *pdev, uint8_t ep_addr) {
    HAL_PCD_EP_Flush((PCD_HandleTypeDef*)pdev->pData, ep_addr);
    return USBD_OK;
}

USBD_StatusTypeDef USBD_LL_StallEP(USBD_HandleTypeDef *pdev, uint8_t ep_addr) {
    HAL_PCD_EP_SetStall((PCD_HandleTypeDef*)pdev->pData, ep_addr);
    return USBD_OK;
}

USBD_StatusTypeDef USBD_LL_ClearStallEP(USBD_HandleTypeDef *pdev, uint8_t ep_addr) {
    HAL_PCD_EP_ClrStall((PCD_HandleTypeDef*)pdev->pData, ep_addr);
    return USBD_OK;
}

/**
 * Return Stall condition.
 *
 * @param  pdev: Device handle
 * @param  ep_addr: Endpoint Number
 * @retval Stall (1: Yes, 0: No)
 */
uint8_t USBD_LL_IsStallEP(USBD_HandleTypeDef *pdev, uint8_t ep_addr) {
    PCD_HandleTypeDef *hpcd = (PCD_HandleTypeDef*)pdev->pData;

    if((ep_addr & 0x80) == 0x80) {
        return hpcd->IN_ep[ep_addr & 0x7F].is_stall;
    }
    else {
        return hpcd->OUT_ep[ep_addr & 0x7F].is_stall;
    }
}

USBD_StatusTypeDef USBD_LL_SetUSBAddress(USBD_HandleTypeDef *pdev, uint8_t dev_addr) {
    HAL_PCD_SetAddress((PCD_HandleTypeDef*)pdev->pData, dev_addr);
    return USBD_OK;
}

USBD_StatusTypeDef USBD_LL_Transmit(USBD_HandleTypeDef *pdev, uint8_t ep_addr,
        uint8_t *pbuf, uint16_t size) {
    HAL_PCD_EP_Transmit((PCD_HandleTypeDef*)pdev->pData, ep_addr, pbuf, size);
    return USBD_OK;
}

USBD_StatusTypeDef USBD_LL_PrepareReceive(USBD_HandleTypeDef *pdev, uint8_t ep_addr, uint8_t *pbuf, uint16_t size) {
    HAL_PCD_EP_Receive((PCD_HandleTypeDef*)pdev->pData, ep_addr, pbuf, size);
    return USBD_OK;
}

/**
 * Returns the last transfered packet size.
 *
 * @param  pdev: Device handle
 * @param  ep_addr: Endpoint Number
 * @retval Recived Data Size
 */
uint32_t USBD_LL_GetRxDataSize(USBD_HandleTypeDef *pdev, uint8_t ep_addr) {
    return HAL_PCD_EP_GetRxCount((PCD_HandleTypeDef*)pdev->pData, ep_addr);
}

void USBD_LL_Delay(uint32_t Delay) {
    HAL_Delay(Delay);
}

void *USBD_static_malloc(uint32_t size) {
    UNUSED(size);
    static uint32_t mem[MAX_STATIC_ALLOC_SIZE];
    return mem;
}

void USBD_static_free(void *p) {
    UNUSED(p);
}
