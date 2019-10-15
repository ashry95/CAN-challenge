#include <stdint.h>
#include <stdbool.h>

#include "driverlib/sysctl.h"
#include "driverlib/debug.h"

#include "inc/hw_types.h"
#include "inc/hw_memmap.h"
#include "driverlib/pin_map.h"

#include "inc/hw_gpio.h"
#include "driverlib/gpio.h"

#include "inc/hw_uart.h"
#include "driverlib/uart.h"
#include "utils/uartstdio.h"

#include "inc/hw_can.h"
#include "driverlib/can.h"

static uint8_t pui8BufferIn[4] = { 0 };
static uint8_t pui8BufferOut[4] = { 4, 5, 6, 7 };
static uint8_t threshold[4] = { 50, 50, 50, 50 };

uint8_t faultFlags = 0;

tCANBitClkParms CANBitClk = { .ui32SyncPropPhase1Seg = 11, .ui32Phase2Seg = 3,
                              .ui32SJW = 1, .ui32QuantumPrescaler = 32 };

static tCANMsgObject sMsgObjectRx = { .ui32MsgID = 0x01, .ui32MsgIDMask = 0,
                               .ui32MsgLen = 4, .ui32Flags =
                               MSG_OBJ_USE_ID_FILTER,
                               .pui8MsgData = pui8BufferIn };

static tCANMsgObject sMsgObjectTx = { .ui32MsgID = 0x01, .ui32Flags = 0, .ui32MsgLen =
                                       4,
                               .pui8MsgData = pui8BufferOut };

int main(void)
{

    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);
    while (!SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOA))
        ;
    GPIOPinTypeUART(GPIO_PORTA_BASE, GPIO_PIN_0 | GPIO_PIN_1);
    UARTStdioConfig(0, 115200, 16000000);

    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);
    while (!SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOF))
        ;
    GPIOPinTypeGPIOOutput(GPIO_PORTF_BASE,
    GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3);

    UARTprintf("************\n");
    UARTprintf(" CAN Challenge\n");
    UARTprintf("************\n");

    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOB);
    while (!SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOB));
    GPIOPinConfigure(GPIO_PB4_CAN0RX);
    GPIOPinConfigure(GPIO_PB5_CAN0TX);

    GPIOPinTypeCAN(GPIO_PORTB_BASE, GPIO_PIN_4 | GPIO_PIN_5);

    SysCtlPeripheralEnable(SYSCTL_PERIPH_CAN0);
    while (!SysCtlPeripheralReady(SYSCTL_PERIPH_CAN0))
        ;
    SysCtlPeripheralEnable(SYSCTL_PERIPH_CAN1);
    while (!SysCtlPeripheralReady(SYSCTL_PERIPH_CAN1))
        ;

    CANInit(CAN0_BASE);

    CANBitRateSet(CAN0_BASE, SysCtlClockGet(), 500000);

    CANMessageSet(CAN0_BASE, 3, &sMsgObjectTx, MSG_OBJ_TYPE_RX);

    CANEnable(CAN0_BASE);

    GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_1, 0xFF);
    while (1)
    {
        sMsgObjectTx.ui32MsgLen = 4;
        sMsgObjectTx.pui8MsgData = pui8BufferOut;
        CANMessageSet(CAN0_BASE, 1, &sMsgObjectTx, MSG_OBJ_TYPE_TX);
        SysCtlDelay(16000000 / 10);

        sMsgObjectTx.ui32MsgLen = 1;
        sMsgObjectTx.pui8MsgData = &faultFlags;
        faultFlags = 0;
        if (pui8BufferOut[0] > threshold[0])
        {
            faultFlags |= 0x01;
        }
        if (pui8BufferOut[1] > threshold[1])
        {
            faultFlags |= 0x02;
        }
        if (pui8BufferOut[2] > threshold[2])
        {
            faultFlags |= 0x04;
        }
        if (pui8BufferOut[3] > threshold[3])
        {
            faultFlags |= 0x08;
        }
        if (faultFlags != 0)
        {
            CANMessageSet(CAN0_BASE, 2, &sMsgObjectTx, MSG_OBJ_TYPE_TX);
        }

        if ((CANStatusGet(CAN0_BASE, CAN_STS_NEWDAT) & 1) == 1)
        {
            CANMessageGet(CAN0_BASE, 3, &sMsgObjectRx, true);
            if (pui8BufferIn[0] == 0xF2 && pui8BufferIn[1] == 0xFE)
            {
                switch (pui8BufferIn[3])
                {
                case 0x00: // temp
                    threshold[0] = pui8BufferIn[3];
                    break;
                case  0x01: // dest
                    threshold[1] = pui8BufferIn[3];
                    break;
                case  0x03: // oil
                    threshold[2] = pui8BufferIn[3];
                    break;
                case  0x04: // water
                    threshold[3] = pui8BufferIn[3];
                    break;
                default:
                    break;
                }
            }
        }

    }
    return 0;
}
