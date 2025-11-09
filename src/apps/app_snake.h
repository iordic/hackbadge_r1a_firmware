#ifndef APP_SNAKE_H_
#define APP_SNAKE_H_

bool onSnakeBody(int x,int y);
void logic();
void johnBox(U8G2 *u8g2, int x, int y);
void johnDisc(U8G2 *u8g2, int x,int y);
void restartGame();
#endif