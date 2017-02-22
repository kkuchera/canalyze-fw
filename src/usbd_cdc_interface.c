#include "usbd_cdc_interface.h" 
#include "led.h"

#define APP_RX_DATA_SIZE  256 //TODO set to max size of packet received over usb
#define APP_TX_DATA_SIZE  256 //TODO set to max size of packet  over usb

USBD_CDC_LineCodingTypeDef LineCoding =
{
    115200, /* baud rate*/
    0x00,   /* stop bits-1*/
    0x00,   /* parity - none*/
    0x08    /* nb. of bits 8*/
};

/* A circular ring buffer is only needed for data received over CAN and sent
 * over USB. Because the USB interface is much faster than CAN, when data is
 * received over USB, it must wait until the packet is processed and
 * transmitted over CAN before sending a new packet (new USB packets will be
 * NACKed). When data is received over CAN, it buffers it and periodically
 * sends it over USB.
 *
 * Buffer is    empty: buf_read == buf_write
 *              full: buf_write ==  buf_read - 1 
 * Note: there is 1 wasted entry in buffer but otherwise when buf_write ==
 * buf_read, you can't distinguish between empty and full so need extra
 * variable. Alternative: use buf_read + buf_size. This requires both read and
 * writing to change buf_size.
 */
uint8_t buf_rx[APP_RX_DATA_SIZE]; // Buffer for data received over USB.
// Circular ring buffer for data received over CAN.
uint8_t buf_tx[APP_TX_DATA_SIZE];
uint32_t buf_read = 0; // Points to first data entry in buffer.
uint32_t buf_write = 0; // Points to the first free entry in buffer.

CAN_HandleTypeDef can_handle;
TIM_HandleTypeDef tim_handle;
extern USBD_HandleTypeDef USBD_Device;

static int8_t CDC_Itf_Init     (void);
static int8_t CDC_Itf_DeInit   (void);
static int8_t CDC_Itf_Control  (uint8_t cmd, uint8_t* pbuf, uint16_t length);
static int8_t CDC_Itf_Receive  (uint8_t* pbuf, uint32_t *len);

static void Error_Handler(void);
static void CAN_Config(void);
static void TIM_Config(void);

USBD_CDC_ItfTypeDef USBD_CDC_fops = 
{
    CDC_Itf_Init,
    CDC_Itf_DeInit,
    CDC_Itf_Control,
    CDC_Itf_Receive
};

void print_char(const char c) {
    buf_tx[buf_write++] = c;
    buf_write %= APP_TX_DATA_SIZE;
}

void print(const char* s) {
    while(*s) print_char(*s++);
    print_char('\n');
}

/**
 * Initializes the CDC interface.
 *
 * @return USBD_OK if all operations are OK else USBD_FAIL
 */
static int8_t CDC_Itf_Init(void) {
    CAN_Config();
    if (HAL_CAN_Receive_IT(&can_handle, CAN_FIFO0) != HAL_OK) {
        Error_Handler();
    }

    TIM_Config();
    if (HAL_TIM_Base_Start_IT(&tim_handle) != HAL_OK) {
        Error_Handler();
    }

    USBD_CDC_SetTxBuffer(&USBD_Device, buf_tx, 0);
    USBD_CDC_SetRxBuffer(&USBD_Device, buf_rx);
    return (USBD_OK);
}

/**
 * Deinitializes the CDC interface.
 *
 * @return USBD_OK if all operations are OK else USBD_FAIL
 */
static int8_t CDC_Itf_DeInit(void) {
    if (HAL_TIM_Base_DeInit(&tim_handle) != HAL_OK) {
        Error_Handler();
    }
    if (HAL_CAN_DeInit(&can_handle) != HAL_OK) {
        Error_Handler();
    }
    return (USBD_OK);
}

/**
 * Manage CDC class requests
 *
 * @param  cmd Command code            
 * @param  pbuf Buffer containing command data (request parameters)
 * @param  length Number of data to be sent (in bytes)
 * @return USBD_OK if all operations are OK else USBD_FAIL
 */
static int8_t CDC_Itf_Control(uint8_t cmd, uint8_t* pbuf, uint16_t length) { 
    UNUSED(length);
    switch (cmd) {
        case CDC_SEND_ENCAPSULATED_COMMAND:
            break;
        case CDC_GET_ENCAPSULATED_RESPONSE:
            break;
        case CDC_SET_COMM_FEATURE:
            break;
        case CDC_GET_COMM_FEATURE:
            break;
        case CDC_CLEAR_COMM_FEATURE:
            break;
        case CDC_SET_LINE_CODING:
            break;
        case CDC_GET_LINE_CODING:
            pbuf[0] = (uint8_t)(LineCoding.bitrate);
            pbuf[1] = (uint8_t)(LineCoding.bitrate >> 8);
            pbuf[2] = (uint8_t)(LineCoding.bitrate >> 16);
            pbuf[3] = (uint8_t)(LineCoding.bitrate >> 24);
            pbuf[4] = LineCoding.format;
            pbuf[5] = LineCoding.paritytype;
            pbuf[6] = LineCoding.datatype;     
            break;
        case CDC_SET_CONTROL_LINE_STATE:
            break;
        case CDC_SEND_BREAK:
            break;    
        default:
            break;
    }
    return (USBD_OK);
}

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim) {
    UNUSED(htim);
    uint16_t len;
    // Check that buffer isn't empty
    if (buf_read != buf_write) {
        // If there is a wrap around, only send the data until end of buffer
        if (buf_read > buf_write) {
            len = APP_RX_DATA_SIZE - buf_read;                            
        } else {
            len = buf_write - buf_read;                              
        }

        USBD_CDC_SetTxBuffer(&USBD_Device, buf_tx + buf_read, len);

        if (USBD_CDC_TransmitPacket(&USBD_Device) == USBD_OK) {
            buf_read += len;
            buf_read %= APP_TX_DATA_SIZE;
        }
    }
}

void HAL_CAN_RxCpltCallback(CAN_HandleTypeDef *hcan) {
    uint8_t i;
    uint16_t bufsize;
    uint8_t len;
    //char len_ascii[] = "000";
    // Doesn't handle negative modulo, so add APP_TX_DATA_SIZE.
    bufsize = (APP_TX_DATA_SIZE + buf_write - buf_read) % APP_TX_DATA_SIZE;

    //len_ascii[0] = bufsize/100 + 48;
    //len_ascii[1] = (bufsize%100)/10 + 48;
    //len_ascii[2] = bufsize%10 + 48;
    //print(len_ascii);
    
    // TODO buffer overflow due to adding packet (size 8) to buffer
    // Check for buffer overflow. Leave one empty space in buffer to
    // distinguish full and empty (hence >=)
    len = hcan->pRxMsg->DLC;
    if (bufsize + len >= APP_TX_DATA_SIZE) {
        Error_Handler();
    }

    //if ((can_handle->pRxMsg->StdId == 0x321) && (can_handle->pRxMsg->IDE == CAN_ID_STD) && (can_handle->pRxMsg->DLC == 8)) { 
    for (i=0; i<len; i++) {
        buf_tx[buf_write++] = hcan->pRxMsg->Data[i];
        buf_write %= APP_TX_DATA_SIZE;
    }

    if (HAL_CAN_Receive_IT(hcan, CAN_FIFO0) != HAL_OK) {
        Error_Handler();
    }
}

/**
 * Handles data received over USB OUT endpoint.
 *
 * @param  buf Buffer containing data to be transmitted
 * @param  len Number of elements in buffer
 * @return USBD_OK if all operations are OK else USBD_FAIL
 */
static int8_t CDC_Itf_Receive(uint8_t* buf, uint32_t *len) {
    UNUSED(buf);
    uint32_t i;
    if (*len > 0 && *len <= 8) {
        // TODO build CAN message
        // Replay received data over CAN
        can_handle.pTxMsg->DLC = *len;
        for (i=0; i<*len; i++) {
            can_handle.pTxMsg->Data[i] = buf[i];
        }
    }

    HAL_CAN_Transmit_IT(&can_handle);
    return (USBD_OK);
}

void HAL_CAN_TxCpltCallback(CAN_HandleTypeDef *hcan) {
    UNUSED(hcan);
    // Initiate next USB packet transfer once CAN completes transfer
    USBD_CDC_ReceivePacket(&USBD_Device);
}

static void CAN_Config(void) {
    /* See datasheet p836 on bit timings for SJW, BS1 and BS2
     * tq = (BRP+1).tpclk
     * tsjw = tq.1 
     * tbs1 = tq.(TS1+1)
     * tbs2 = tq.(TS2+1)
     * baud = 1/(tsjw+tbs1+tbs2) = 1/(tq.(1+(TS1+1)+(TS2+1)))
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
    can_handle.Init.RFLM = DISABLE;
    can_handle.Init.TXFP = DISABLE;
    can_handle.Init.Mode = CAN_MODE_NORMAL;
    can_handle.Init.SJW = CAN_SJW_1TQ;
    can_handle.Init.BS1 = CAN_BS1_4TQ;
    can_handle.Init.BS2 = CAN_BS2_3TQ;
    can_handle.Init.Prescaler = 600;
    if (HAL_CAN_Init(&can_handle) != HAL_OK) {
        Error_Handler();
    }

    // Configure the CAN Filter, needed to receive CAN data 
    sFilterConfig.FilterNumber = 0;
    sFilterConfig.FilterMode = CAN_FILTERMODE_IDMASK;
    sFilterConfig.FilterScale = CAN_FILTERSCALE_32BIT;
    sFilterConfig.FilterIdHigh = 0x0000;
    sFilterConfig.FilterIdLow = 0x0000;
    sFilterConfig.FilterMaskIdHigh = 0x0000;
    sFilterConfig.FilterMaskIdLow = 0x0000;
    sFilterConfig.FilterFIFOAssignment = 0;
    sFilterConfig.FilterActivation = ENABLE;
    sFilterConfig.BankNumber = 14;
    if (HAL_CAN_ConfigFilter(&can_handle, &sFilterConfig) != HAL_OK) {
        Error_Handler();
    }

    // Configure Transmission process
    can_handle.pTxMsg->StdId = 0x321;
    can_handle.pTxMsg->ExtId = 0x01;
    can_handle.pTxMsg->RTR = CAN_RTR_DATA;
    can_handle.pTxMsg->IDE = CAN_ID_STD;
    can_handle.pTxMsg->DLC = 8;
}

static void TIM_Config(void) { 
    tim_handle.Instance = TIMx;
    tim_handle.Init.Period = (CDC_POLLING_INTERVAL*1000) - 1;
    tim_handle.Init.Prescaler = 48-1; // Uses pclk, increment every ms
    tim_handle.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    tim_handle.Init.CounterMode = TIM_COUNTERMODE_UP;
    tim_handle.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
    if (HAL_TIM_Base_Init(&tim_handle) != HAL_OK) {
        Error_Handler();
    }
}

void HAL_CAN_ErrorCallback(CAN_HandleTypeDef *hcan) {
    const char error_none[] = "Error: none";
    const char error_ewg[] = "Error: ewg";
    const char error_epv[] = "Error: epv";
    const char error_bof[] = "Error: bof";
    const char error_stf[] = "Error: stf";
    const char error_for[] = "Error: for";
    const char error_ack[] = "Error: ack";
    const char error_br[] = "Error: br";
    const char error_bd[] = "Error: bd";
    const char error_crc[] = "Error: crc";
    const char error_default[] = "Error: unknown";
    switch (hcan->ErrorCode) {
        case HAL_CAN_ERROR_NONE:
            print(error_none);
            break;
        case HAL_CAN_ERROR_EWG:
            print(error_ewg);
            break;
        case HAL_CAN_ERROR_EPV:
            print(error_epv);
            break;
        case HAL_CAN_ERROR_BOF:
            print(error_bof);
            break;
        case HAL_CAN_ERROR_STF:
            print(error_stf);
            break;
        case HAL_CAN_ERROR_FOR:
            print(error_for);
            break;
        case HAL_CAN_ERROR_ACK:
            print(error_ack);
            break;
        case HAL_CAN_ERROR_BR:
            print(error_br);
            break;
        case HAL_CAN_ERROR_BD:
            print(error_bd);
            break;
        case HAL_CAN_ERROR_CRC:
            print(error_crc);
            break;
        default:
            print(error_default);
    }
    Error_Handler();
}

static void Error_Handler(void) {
    led_on(LED_RED);
}

