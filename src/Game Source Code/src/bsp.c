/*
 * bsp.c
 *
 *  Created on: Nov 20, 2025
 *      Author: btjanicki
 */

#include "qpn_port.h"
#include "bsp.h"
#include "game.h"
#include "lcd.h"

#include "xil_printf.h"
#include "xintc.h"
#include "xgpio.h"
#include "xtmrctr.h"
#include "xparameters.h"
#include "xil_exception.h"
#include "xspi.h"
#include "mb_interface.h"
#include <stdint.h>
// debuggin
#define DEBUG_ISR 0

static XIntc Intc;
static XGpio encGpio;
static XTmrCtr tmr;
static XGpio BtnGpio;
static XSpi lcdSPI;
static XGpio lcdDC;
// rotation table for the rotary encoder
static const int8_t quad_table[16] = {
    0,  // 00->00
    1,  // 00->01
   -1,  // 00->10
    0,  // 00->11
   -1,  // 01->00
    0,  // 01->01
    0,  // 01->10
    1,  // 01->11
    1,  // 10->00
    0,  // 10->01
    0,  // 10->10
   -1,  // 10->11
    0,  // 11->00
   -1,  // 11->01
    1,  // 11->10
    0   // 11->11
};

static int lastAB = 3; // idle state
// debounce stuff
static uint8_t btn_raw = 0;
static uint8_t btn_stable = 0;
static uint8_t btn_lockout = 0;    // only one button at a time
// encoder click debounce:
static uint8_t enc_btn_raw = 0;
static uint8_t enc_btn_stable = 0;
static uint8_t enc_db_cnt = 0;

static uint8_t db_cnt[4] = {0,0,0,0};  // debounce counters
#define DB_THRESHOLD 3                // 30ms of debounce

static void processQuadrature_full(int ab) {
    int idx = (lastAB << 2) | (ab & 0x3);
    int8_t delta = quad_table[idx & 0x0F];
    if (delta > 0) {
        QActive_postISR((QActive*)&AO_GameObj, ENCODER_RIGHT_SIG);
    } else if (delta < 0) {
        QActive_postISR((QActive*)&AO_GameObj, ENCODER_LEFT_SIG);
    }
    lastAB = ab & 0x3;
}

static void encoderISR(void *CallbackRef) {
    XGpio *g = (XGpio*)CallbackRef;
    int raw = XGpio_DiscreteRead(g, ENCODER_CHANNEL);
    int ab  = raw & 0x03;
    int btn = (raw >> ENC_BTN_BIT) & 1;
    enc_btn_raw = btn; //for the click debounce
    XGpio_InterruptClear(g, ENCODER_CHANNEL);
    processQuadrature_full(ab);
}
// buttons
void GpioHandler(void *CallbackRef) {
    uint32_t btns = XGpio_DiscreteRead(&BtnGpio, BUTTONS_CHANNEL);
    XGpio_InterruptClear(&BtnGpio, BUTTONS_CHANNEL);
    btn_raw = btns & 0x0F;
}

static void timerISR(void *CallBackRef, u8 TmrCtrNumber) {

    QActive_postISR((QActive*)&AO_GameObj, GAME_TICK_SIG);

    // debounce yippy
    uint8_t raw = btn_raw;
    uint8_t changed = raw ^ btn_stable;

    for (int i = 0; i < 4; i++) {
        uint8_t mask = 1 << i;

        if (changed & mask) {
            // bit changed -> count until stable
            if (++db_cnt[i] >= DB_THRESHOLD) {
                db_cnt[i] = 0;
                btn_stable ^= mask;       // toggle stable bit

                // one button at a time to stop mashers
                if (btn_stable & mask) {  // button pressed
                    if (btn_lockout == 0) {
                        btn_lockout = mask;  // lock the button

                        switch (mask) {
                        case 0x01: QActive_postISR((QActive*)&AO_GameObj, BTN1_PRESSED); break;
                        case 0x02: QActive_postISR((QActive*)&AO_GameObj, BTN2_PRESSED); break;
                        case 0x04: QActive_postISR((QActive*)&AO_GameObj, BTN3_PRESSED); break;
                        case 0x08: QActive_postISR((QActive*)&AO_GameObj, BTN4_PRESSED); break;
                        }
                    }
                } else { // button release
                    if (btn_lockout == mask)
                        btn_lockout = 0;
                }
            }
        } else {
            db_cnt[i] = 0;   // reset counter
        }
    }

    // encoder click debounce
    uint8_t e_changed = enc_btn_raw ^ enc_btn_stable;

    if (e_changed) {
        if (++enc_db_cnt >= DB_THRESHOLD) {
            enc_db_cnt = 0;
            enc_btn_stable = enc_btn_raw;

            if (enc_btn_stable) {
                QActive_postISR((QActive*)&AO_GameObj, ENCODER_CLICK_SIG);
            }
        }
    } else {
        enc_db_cnt = 0;
    }

    XTmrCtr_Reset(&tmr, 0);
}

void BSP_init(void) {
    xil_printf("BSP_init\r\n");
    if (XGpio_Initialize(&encGpio, ENCODER_GPIO_ID) != XST_SUCCESS) {
        xil_printf("Encoder GPIO init failed (id=%d)\r\n", ENCODER_GPIO_ID);
    } else {
        XGpio_SetDataDirection(&encGpio, ENCODER_CHANNEL, 0xFF);
        XGpio_InterruptEnable(&encGpio, ENCODER_CHANNEL);
        XGpio_InterruptGlobalEnable(&encGpio);
    }
    // button
    XGpio_Initialize(&BtnGpio, PUSHBUTTONS_DEVICE_ID);
    XGpio_SetDataDirection(&BtnGpio, BUTTONS_CHANNEL, 0xFF);
    XGpio_InterruptEnable(&BtnGpio, 0xFF);
    XGpio_InterruptGlobalEnable(&BtnGpio);

    // timer
    if (XTmrCtr_Initialize(&tmr, XPAR_AXI_TIMER_0_DEVICE_ID) != XST_SUCCESS) {
        xil_printf("Timer init failed (id=%d)\r\n", XPAR_AXI_TIMER_0_DEVICE_ID);
    } else {
        XTmrCtr_SetHandler(&tmr, timerISR, &tmr);

        XTmrCtr_SetOptions(&tmr, 0, XTC_INT_MODE_OPTION | XTC_AUTO_RELOAD_OPTION);

        uint32_t clk = XPAR_AXI_TIMER_0_CLOCK_FREQ_HZ;
        uint32_t load = clk / GAME_TICK_HZ;
        XTmrCtr_SetResetValue(&tmr, 0, 0xFFFFFFFF - load);

        XTmrCtr_Start(&tmr, 0);
    }

    XSpi_Config *spiConfig = XSpi_LookupConfig(XPAR_SPI_DEVICE_ID);
    if (!spiConfig) {
        xil_printf("SPI config lookup failed!\n");
        return;
    }
    if (XSpi_CfgInitialize(&lcdSPI, spiConfig, spiConfig->BaseAddress) != XST_SUCCESS) {
        xil_printf("SPI init failed!\n");
        return;
    }
    XSpi_Reset(&lcdSPI);
    XSpi_SetControlReg(&lcdSPI,
        (XSpi_GetControlReg(&lcdSPI) | XSP_CR_ENABLE_MASK | XSP_CR_MASTER_MODE_MASK)
        & (~XSP_CR_TRANS_INHIBIT_MASK));
    XSpi_SetSlaveSelectReg(&lcdSPI, ~0x01);

    if (XGpio_Initialize(&lcdDC, XPAR_SPI_DC_DEVICE_ID) != XST_SUCCESS) {
        xil_printf("LCD DC GPIO init failed (id=%d)\r\n", XPAR_SPI_DC_DEVICE_ID);
    } else {
        XGpio_SetDataDirection(&lcdDC, 1, 0x0);
    }

    initLCD();
    clrScr();
}

void QF_onStartup(void) {
    xil_printf("QF_onStartup\r\n");

    if (XIntc_Initialize(&Intc, XPAR_INTC_0_DEVICE_ID) != XST_SUCCESS) {
        xil_printf("XIntc init failed (id=%d)\r\n", XPAR_INTC_0_DEVICE_ID);
        return;
    }

    if (XIntc_Connect(&Intc,
            XPAR_MICROBLAZE_0_AXI_INTC_AXI_GPIO_ENCODER_IP2INTC_IRPT_INTR,
            (XInterruptHandler)encoderISR, &encGpio) != XST_SUCCESS) {
        xil_printf("XIntc_Connect encoder failed\n");
    }

    if (XIntc_Connect(&Intc,
            XPAR_MICROBLAZE_0_AXI_INTC_AXI_TIMER_0_INTERRUPT_INTR,
            (XInterruptHandler)timerISR, &tmr) != XST_SUCCESS) {
        xil_printf("XIntc_Connect timer failed\n");
    }

    XIntc_Connect(&Intc, PUSHBUTTONS_INTERRUPT_ID,
                      (XInterruptHandler)GpioHandler, &BtnGpio);

    if (XIntc_Start(&Intc, XIN_REAL_MODE) != XST_SUCCESS) {
        xil_printf("XIntc_Start failed\n");
        return;
    }

    XIntc_Enable(&Intc, XPAR_MICROBLAZE_0_AXI_INTC_AXI_GPIO_ENCODER_IP2INTC_IRPT_INTR);
    XIntc_Enable(&Intc, XPAR_MICROBLAZE_0_AXI_INTC_AXI_TIMER_0_INTERRUPT_INTR);
    XIntc_Enable(&Intc, PUSHBUTTONS_INTERRUPT_ID);

    Xil_ExceptionInit();
    Xil_ExceptionRegisterHandler(XIL_EXCEPTION_ID_INT,
                                 (Xil_ExceptionHandler)XIntc_DeviceInterruptHandler,
                                 (void *)XPAR_INTC_0_DEVICE_ID);
    Xil_ExceptionEnable();

    microblaze_enable_interrupts();

    xil_printf("BSP startup done\r\n");
}


void QF_onIdle(void) {
    QF_INT_UNLOCK();
}

void Q_onAssert(char const *file, int line) {
    xil_printf("ASSERT: %s line %d\r\n", file, line);
    for(;;);
}
