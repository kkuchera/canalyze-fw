#include "can.h"
#include "led.h"
#include "stm32f0xx_hal.h"
// CAN control modes
//#define CAN_CTRLMODE_NORMAL             0x00
#define USB_8DEV_CAN_MODE_SILENT        0x01
#define USB_8DEV_CAN_MODE_LOOPBACK      0x02
#define USB_8DEV_MODE_ONESHOT           0x04

// Not supported
//#define CAN_CTRLMODE_3_SAMPLES          0x04
//#define CAN_CTRLMODE_BERR_REPORTING     0x10
//#define CAN_CTRLMODE_FD                 0x20
//#define CAN_CTRLMODE_PRESUME_ACK        0x40
//#define CAN_CTRLMODE_FD_NON_ISO         0x80

CAN_HandleTypeDef can_handle;

/**
 * Initialize CAN interface.
 *
 * @return 0 if OK
 */
uint8_t can_init() {
    return 0;
}

/**
 * Open the CAN interface.
 *
 * Starts monitoring the can interface and sends the data over USB.
 *
 * @param can_bittiming CAN bit timings to configure CAN.
 * @param ctrlmode flag setting CAN control modes e.g. @see
 * USB_8DEV_CAN_MODE_SILENT
 */
uint8_t can_open(Can_BitTimingTypeDef* can_bittiming, uint8_t ctrlmode) {
    /* See datasheet p836 on bit timings for SJW, BS1 and BS2
     * tq = (BRP+1).tpclk
     * tsjw = tq.(SJW+1)
     * tbs1 = tq.(TS1+1)
     * tbs2 = tq.(TS2+1)
     * baud = 1/(tsjw+tbs1+tbs2) = 1/(tq.((SJW+1)+(TS1+1)+(TS2+1)))
     */
    CAN_FilterConfTypeDef sFilterConfig;
    static CanTxMsgTypeDef TxMessage;
    static CanRxMsgTypeDef RxMessage;

    // Configure the CAN peripheral
    can_handle.Instance = CANx;
    can_handle.pTxMsg = &TxMessage;
    can_handle.pRxMsg = &RxMessage;

    can_handle.Init.TTCM = DISABLE;
    can_handle.Init.ABOM = DISABLE;
    can_handle.Init.AWUM = DISABLE;
    can_handle.Init.NART = DISABLE;
    if (ctrlmode & USB_8DEV_MODE_ONESHOT) {
        can_handle.Init.NART = ENABLE;
    }
    can_handle.Init.RFLM = DISABLE;
    can_handle.Init.TXFP = DISABLE;
    can_handle.Init.Mode = CAN_MODE_NORMAL;
    if (ctrlmode & USB_8DEV_CAN_MODE_SILENT) {
        can_handle.Init.Mode |= CAN_MODE_SILENT;
    }
    if (ctrlmode & USB_8DEV_CAN_MODE_LOOPBACK) {
        can_handle.Init.Mode |= CAN_MODE_LOOPBACK;
    }
    // The shift is needed because that's how CAN_SJW_xTQ,CAN_BS1_xTQ and
    // CAN_BS2_xTQ are defined
    can_handle.Init.SJW = can_bittiming->sjw << 6*4;
    can_handle.Init.BS1 = can_bittiming->ts1 << 4*4;
    can_handle.Init.BS2 = can_bittiming->ts2 << 5*4;
    can_handle.Init.Prescaler = can_bittiming->brp;
    if (HAL_CAN_Init(&can_handle)) {
        return 1;
    }

    // Configure the CAN Filter, needed to receive CAN data
    sFilterConfig.FilterNumber = 0;
    sFilterConfig.FilterMode = CAN_FILTERMODE_IDMASK;
    sFilterConfig.FilterScale = CAN_FILTERSCALE_32BIT;
    sFilterConfig.FilterIdHigh = 0x0000;
    sFilterConfig.FilterIdLow = 0x0000;
    sFilterConfig.FilterMaskIdHigh = 0x0000;
    sFilterConfig.FilterMaskIdLow = 0x0000;
    sFilterConfig.FilterFIFOAssignment = CAN_FIFO0;
    sFilterConfig.FilterActivation = ENABLE;
    sFilterConfig.BankNumber = 14;
    if (HAL_CAN_ConfigFilter(&can_handle, &sFilterConfig)) {
        return 2;
    }
// Reason to not do it with interrupts is because when a CAN message
// is received, RxCplt is called. In this function the CAN controller
// is asked to receive a new packet. However if this function returns
// non zero, when do you ask the controller to receive a new CAN packet? This
// will happen because receive and send are different threads. You are only
// allowed to call HAL from the main thread. See
// https://community.st.com/thread/13989

    if (HAL_CAN_Receive_IT(&can_handle, CAN_FIFO0)) {
        return 3;
    }

    return 0;
}

/**
 * Close the CAN interface
 */
uint8_t can_close() {
    if (HAL_CAN_DeInit(&can_handle)) {
        return 1;
    }
    return 0;
}
