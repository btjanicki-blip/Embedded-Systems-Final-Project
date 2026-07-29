/*
 * main.c
 *
 *  Created on: Nov 20, 2025
 *      Author: btjanicki
 */

#include "qpn_port.h"
#include "bsp.h"
#include "game.h"
#include "xil_cache.h"

static QEvent l_gameQueue[128];

QActiveCB const Q_ROM Q_ROM_VAR QF_active[] = {
    { (QActive *)0,           (QEvent *)0,       0                   },
    { (QActive *)&AO_GameObj, l_gameQueue,       Q_DIM(l_gameQueue)  }
};

Q_ASSERT_COMPILE(QF_MAX_ACTIVE == Q_DIM(QF_active) - 1);

int main(void) {

    Xil_ICacheInvalidate();
    Xil_ICacheEnable();
    Xil_DCacheInvalidate();
    Xil_DCacheEnable();

    Game_ctor();
    BSP_init();
    QF_run();

    return 0;
}
