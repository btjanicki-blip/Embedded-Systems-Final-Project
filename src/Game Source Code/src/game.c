/*
 * game.c
 *
 *  Created on: Nov 20, 2025
 *      Author: btjanicki
 */

#include "game.h"
#include "lcd.h"
#include <string.h>
#include "xil_printf.h"

Game AO_GameObj;

#define SCREEN_W 240
#define SCREEN_H 320

#define PADDLE_W 40
#define PADDLE_H 6
#define PADDLE_Y (SCREEN_H-10)

#define BALL_SIZE 6
#define ROWS 6
#define COLS 8
#define BRICK_W (SCREEN_W/COLS)
#define BRICK_H 15

static QState Game_initial(Game *me);
static QState Game_playing(Game *me);
static QState Game_paused(Game *me);
static QState Game_menu(Game *me);
static QState Game_gameOver(Game *me);
static QState Game_victory(Game *me);

static int lives=3; // default lives set to 3
static int numBricks=48; // initial amount of bricks (used for victory condition)
static int score;
static int paddleX;
static int ballX, ballY;
static int dx, dy;
static uint8_t bricks[ROWS][COLS];
static uint8_t needsReset = 0; // gets set to 1 if its time to reset the game
static uint32_t gameTimeMs = 0;     // total milliseconds spent playing (divided by 60 before printing out)


static void drawPaddle(void) {
    setColor(190,212,247);
    fillRect(paddleX, PADDLE_Y,
             paddleX+PADDLE_W, PADDLE_Y+PADDLE_H);
}

static void erasePaddle(void) {
    setColor(0,0,0);
    fillRect(paddleX, PADDLE_Y,
             paddleX+PADDLE_W, PADDLE_Y+PADDLE_H);
}

static void drawBall(void) {
    setColor(255,182,78);
    fillRect(ballX, ballY, ballX+BALL_SIZE, ballY+BALL_SIZE);
}

static void eraseBall(void) {
    setColor(0,0,0);
    fillRect(ballX, ballY, ballX+BALL_SIZE, ballY+BALL_SIZE);
}

static void drawBricks(void) {
    for(int r=0;r<ROWS;r++){
        for(int c=0;c<COLS;c++){
            if(bricks[r][c]){
                int x1=c*BRICK_W;
                int y1=r*BRICK_H;
                int x2=x1+BRICK_W-2;
                int y2=y1+BRICK_H-2;
                switch(r){
                    case 0: setColor(31,33,77); break;
                    case 1: setColor(80,54,111); break;
                    case 2: setColor(191,52,117); break;
                    case 3: setColor(238,108,69); break;
                    case 4: setColor(255,206,97); break;
                    case 5: setColor(255,229,138); break;
                }
                fillRect(x1,y1,x2,y2);

                // adding an outline to make bricks easier to see:
                setColor(190,212,247);
                fillRect(x1,y1,x2,y1+1);
                fillRect(x1,y2-1,x2,y2);
                fillRect(x1,y1,x1+1,y2);
                fillRect(x2-1,y1,x2,y2);
            }
        }
    }
}
static void drawMenuBricks(void){
	for(int r=0;r<14;r++){
	        for(int c=0;c<COLS;c++){
	                int x1=c*BRICK_W;
	                int y1=r*BRICK_H;
	                int x2=x1+BRICK_W-2;
	                int y2=y1+BRICK_H-2;
	                switch(r){
	                    case 0: setColor(255,255,255); break;
	                    case 1: setColor(250,248,160); break;
	                    case 2: setColor(255,255,0); break;
	                    case 3: setColor(255,196,0); break;
	                    case 4: setColor(255,162,0); break;
	                    case 5: setColor(255,89,0); break;
	                    case 6: setColor(255,30,0); break;
	                    case 7: setColor(143,6,15); break;
	                    case 8: setColor(163,5,73); break;
	                    case 9: setColor(172,12,131); break;
	                    case 10: setColor(125,12,150); break;
	                    case 11: setColor(115,29,191); break;
	                    case 12: setColor(63,4,135); break;
	                    case 13: setColor(28,3,82); break;
	                }
	                fillRect(x1,y1,x2,y2);

	        }
	    }
}

static void drawVictoryBricks(void){
	for(int r=0;r<22;r++){
	        for(int c=0;c<COLS;c++){
	                int x1=c*BRICK_W;
	                int y1=r*BRICK_H;
	                int x2=x1+BRICK_W-2;
	                int y2=y1+BRICK_H-2;
	                switch(r){
	                    case 0: setColor(255,0,0); break;
	                    case 1: setColor(255,31,0); break;
	                    case 2: setColor(255,63,0); break;
	                    case 3: setColor(255,95,0); break;
	                    case 4: setColor(255,127,0); break;
	                    case 5: setColor(255,159,0); break;
	                    case 6: setColor(255,191,0); break;
	                    case 7: setColor(255,223,0); break;
	                    case 8: setColor(255,255,0); break;
	                    case 9: setColor(191,255,0); break;
	                    case 10: setColor(127,255,0); break;
	                    case 11: setColor(63,255,0); break;
	                    case 12: setColor(0,255,0); break;
	                    case 13: setColor(0,255,63); break;
	                    case 14: setColor(0,255,127); break;
	                    case 15: setColor(0,255,191); break;
	                    case 16: setColor(0,255,255); break;
	                    case 17: setColor(0,191,255); break;
	                    case 18: setColor(0,127,255); break;
	                    case 19: setColor(0,63,255); break;
	                    case 20: setColor(0,0,255); break;
	                    case 21: setColor(63,0,255); break;
	                }
	                fillRect(x1,y1,x2,y2);

	        }
	    }
}

static void drawGameOverBricks(void){
	for(int r=0;r<22;r++){
	        for(int c=0;c<COLS;c++){
	                int x1=c*BRICK_W;
	                int y1=r*BRICK_H;
	                int x2=x1+BRICK_W-2;
	                int y2=y1+BRICK_H-2;
	                setColor(255,0,0);
	                fillRect(x1,y1,x2,y2);
	        }
	    }
}

static void resetGame(void) {
    memset(bricks,1,sizeof(bricks));
    paddleX = SCREEN_W/2 - PADDLE_W/2;
    ballX   = SCREEN_W/2;
    ballY   = 300;
    dx = 2; dy = -2;
    clrScr();
    drawBricks();
    drawPaddle();
    drawBall();
}

static void resetBall(void) {
	// made it so that ball always moves away from player on reset,
	// because during playtesting users got angry when they didn't
	// react fast enough
    eraseBall();
    ballX   = SCREEN_W/2;
    ballY   = 250;
    dx = 2;
    dy = -2;

    drawPaddle();
    drawBall();
}

static void updateBall(void) {
    eraseBall();
    ballX += dx;
    ballY += dy;

    // wall bounce
    if(ballX < 0 || ballX+BALL_SIZE >= SCREEN_W) dx = -dx;
    if(ballY < 0) dy = -dy;

    // reset ball if it misses paddle
    if(ballY > SCREEN_H-10){
        needsReset = 1;
        return;
    }

    // added the -2 so that the paddle has a slightly bigger y hitbox that way the ball
    // doesnt take chunks out of the paddle, and we dont need to update the paddle as often
    // also now the ball doesnt get stuck inside the paddle because the ball is snapped above the paddle
    // if it ever gets below it.
    if(ballY + BALL_SIZE >= (PADDLE_Y - 2) &&
       ballY <= PADDLE_Y + PADDLE_H &&
       ballX + BALL_SIZE >= paddleX &&
       ballX <= paddleX + PADDLE_W) {

        // Snap ball just above paddle
        ballY = PADDLE_Y - BALL_SIZE - 1;

        dy = -dy;
    }


    // collision
    int r = (ballY)/BRICK_H;
    int c = ballX/BRICK_W;
    if(r>=0 && r<ROWS && c>=0 && c<COLS){
        if(bricks[r][c]){
            bricks[r][c] = 0;
            int x1=c*BRICK_W;
            int y1=r*BRICK_H;
            setColor(0,0,0);
            fillRect(x1,y1,x1+BRICK_W-2,y1+BRICK_H-2);
            dy=-dy;
            score+=6000/(r+1);
            numBricks--;
        }
    }
    drawBall();
}

// time for some states
static QState Game_initial(Game *me) {

    return Q_TRAN(&Game_menu);
}

static QState Game_menu(Game *me) {
    switch(Q_SIG(me)){
        case Q_ENTRY_SIG:
        	gameTimeMs = 0;
        	lives = 3;
        	score=0;
        	numBricks=48;
        	setColor(11,7,54);
        	fillRect(0, 0, 240, 320);
        	drawMenuBricks();
        	setFont(BigFont);
        	setColor(255, 255, 255);
        	setColorBg(40, 3, 79);
        	lcdPrint(" BREAKOUT ", 40, 80);
        	setFont(SmallFont);
        	setColor(255, 255, 255);
        	setColorBg(40, 3, 79);
        	lcdPrint(" Brian Janicki ", 59, 95);
        	setColor(255, 255, 255);
        	setColorBg(40, 3, 79);
        	lcdPrint("         HOW TO PLAY:        ", 4, 200);
        	lcdPrint("Press one of the 4 buttons   ", 4, 212);
        	lcdPrint("to select your number of     ", 4, 224);
        	lcdPrint("lives. To control the paddle,", 4, 236);
        	lcdPrint("turn the encoder left and    ", 4, 248);
        	lcdPrint("right. The bricks in the back", 4, 260);
        	lcdPrint("are worth more points! To    ", 4, 272);
        	lcdPrint("start, click on the encoder! ", 4, 284);
            return Q_HANDLED();

        case BTN1_PRESSED:
        	lives = 4;
        	setFont(SmallFont);
        	setColor(255, 255, 255);
        	setColorBg(40, 3, 79);
        	lcdPrint(" 4 Lives? No sweat champ", 24, 160);
        	return Q_HANDLED();

        case BTN2_PRESSED:
        	lives = 3;
        	setFont(SmallFont);
        	setColor(255, 255, 255);
        	setColorBg(40, 3, 79);
        	lcdPrint("  3 Lives: the classic  ", 24, 160);
        	return Q_HANDLED();

        case BTN3_PRESSED:
        	lives = 2;
        	setFont(SmallFont);
        	setColor(255, 255, 255);
        	setColorBg(40, 3, 79);
        	lcdPrint(" 2 Lives, feeling brave?", 24, 160);
        	return Q_HANDLED();

        case BTN4_PRESSED:
        	lives = 1;
        	setFont(SmallFont);
        	setColor(255, 255, 255);
        	setColorBg(40, 3, 79);
        	lcdPrint("   1 Life and a dream   ", 24, 160);
        	return Q_HANDLED();

        case ENCODER_CLICK_SIG:
        	resetGame();
            return Q_TRAN(&Game_playing);

        case GAME_TICK_SIG:
            return Q_HANDLED();
    }
    return Q_SUPER(&QHsm_top);
}

static QState Game_playing(Game *me) {
    switch(Q_SIG(me)){
        case Q_ENTRY_SIG:
        	return Q_HANDLED();

        case GAME_TICK_SIG:
        	gameTimeMs++; //keeping time
            updateBall();
            if(needsReset){
                resetBall();
                needsReset = 0;
                lives--;
                if (lives <= 0) {
                	return Q_TRAN(&Game_gameOver);
                }
            }
            if(numBricks==0){
            	return Q_TRAN(&Game_victory);
            }
            return Q_HANDLED();

        case ENCODER_LEFT_SIG:
            erasePaddle();
            paddleX -= 10; if(paddleX<0)paddleX=0;
            drawPaddle();
            return Q_HANDLED();

        case ENCODER_RIGHT_SIG:
            erasePaddle();
            paddleX +=10; if(paddleX>SCREEN_W-PADDLE_W)paddleX=SCREEN_W-PADDLE_W;
            drawPaddle();
            return Q_HANDLED();

        case ENCODER_CLICK_SIG:
            return Q_TRAN(&Game_paused);
    }
    return Q_SUPER(&QHsm_top);
}

static QState Game_paused(Game *me) {
    switch(Q_SIG(me)){
        case Q_ENTRY_SIG:
            xil_printf("Game paused\n");
            return Q_HANDLED();

        case ENCODER_CLICK_SIG:
            return Q_TRAN(&Game_playing);

        case GAME_TICK_SIG:
            return Q_HANDLED();
    }
    return Q_SUPER(&QHsm_top);
}

static QState Game_gameOver(Game *me) {
    switch (Q_SIG(me)) {
        case Q_ENTRY_SIG:
            clrScr();
            drawGameOverBricks();
            setFont(BigFont);
            setColor(255, 0, 0);
            setColorBg(0, 0, 0);
            lcdPrint(" GAME OVER ", 32, 120);
            char buf[32];
            snprintf(buf, sizeof(buf), "Score: %d", score);
            setFont(SmallFont);
            setColor(255, 255, 255);
            setColorBg(0, 0, 0);
            lcdPrint(buf, 60, 150);
            lcdPrint("Click any button to return", 16, 200);
            return Q_HANDLED();

        case BTN1_PRESSED:
        case BTN2_PRESSED:
        case BTN3_PRESSED:
        case BTN4_PRESSED:
            return Q_TRAN(&Game_menu);
    }
    return Q_SUPER(&QHsm_top);
}

static QState Game_victory(Game *me) {
    switch (Q_SIG(me)) {
        case Q_ENTRY_SIG:
            clrScr();
            drawVictoryBricks();
            setFont(BigFont);
            setColor(0, 255, 0);
            setColorBg(0, 0, 0);
            lcdPrint(" VICTORY! ", 34, 120);
            char buf[32];
            snprintf(buf, sizeof(buf), "Score: %d", score);
            setFont(SmallFont);
            setColor(255, 255, 255);
            setColorBg(0, 0, 0);
            lcdPrint(buf, 60, 150);
            lcdPrint("Click any button to return", 16, 200);
            // time:
            uint32_t timeSeconds = gameTimeMs / 60;
            char timeBuf[32];
            snprintf(timeBuf, sizeof(timeBuf), "Time: %u s", timeSeconds);
            lcdPrint(timeBuf, 60, 166);

            return Q_HANDLED();

        case BTN1_PRESSED:
        case BTN2_PRESSED:
        case BTN3_PRESSED:
        case BTN4_PRESSED:
            return Q_TRAN(&Game_menu);
    }
    return Q_SUPER(&QHsm_top);
}

void Game_ctor(void) {
    Game *me = &AO_GameObj;
    QActive_ctor(&me->super, (QStateHandler)&Game_initial);
}
