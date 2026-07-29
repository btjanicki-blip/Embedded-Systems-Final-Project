/*
 * game.h
 *
 *  Created on: Nov 20, 2025
 *      Author: btjanicki
 */

#ifndef GAME_H
#define GAME_H

#include "qpn_port.h"

enum GameSignals {
    GAME_TICK_SIG = Q_USER_SIG,
    ENCODER_LEFT_SIG,
    ENCODER_RIGHT_SIG,
    ENCODER_CLICK_SIG,
	GAME_RESET_SIG,
	BTN1_PRESSED,
	BTN2_PRESSED,
	BTN3_PRESSED,
	BTN4_PRESSED
};

typedef struct {
    QActive super;
} Game;

extern Game AO_GameObj;

void Game_ctor(void);
void GpioHandler(void *CallbackRef);
#endif
