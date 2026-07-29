/*****************************************************************************

* bsp.h for Lab2A of ECE 153a at UCSB, modified by me today

* Date of the Last Update:  November 20,2025

*****************************************************************************/

#ifndef BSP_H

#define BSP_H



#include "qpn_port.h"



/* Xilinx driver headers — MUST be included here */

#include "xgpio.h"

#include "xintc.h"

#include "xtmrctr.h"



/* Timer tick rate */

#define GAME_TICK_HZ   60



/* Encoder GPIO definitions */

#define ENCODER_GPIO_ID      XPAR_AXI_GPIO_ENCODER_DEVICE_ID

#define ENCODER_CHANNEL      1

#define PUSHBUTTONS_DEVICE_ID   XPAR_AXI_GPIO_BTN_DEVICE_ID

#define PUSHBUTTONS_INTERRUPT_ID XPAR_MICROBLAZE_0_AXI_INTC_AXI_GPIO_BTN_IP2INTC_IRPT_INTR

#define BUTTONS_CHANNEL         1



/* Bit positions */

#define ENC_A_BIT            0

#define ENC_B_BIT            1

#define ENC_BTN_BIT          2



/* BSP initialization */

void BSP_init(void);



/* QP callbacks */

void QF_onStartup(void);

void QF_onIdle(void);



#endif /* BSP_H */



