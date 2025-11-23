/* Taken & adapted from: https://github.com/johnpathe/arduinoSnake */
#include "devices/display.h"
#include "app.h"
#include "app_menu.h"
#include "app_snake.h"

#define HEAD_STOP 0
#define HEAD_UP 1
#define HEAD_DOWN 2
#define HEAD_LEFT 3
#define HEAD_RIGHT 4
int heading = HEAD_STOP; // 0-stopped,1-up,2-down,3-left,4-right

extern App app_menu;

int snakeHead[2] = {5,6}; // starting spot

int apple[2]= {-1,-1}; // apple starts out of view
//15x15 grid of game positions, 2 coords for each
int snakeBody[225][2]= { //initial starting positions
  {1,6},
  {2,6},
  {3,6},
  {4,6},
  {5,6}
};
int snakeLength = 5; //starts at 5
char scoreText[8];
char lastScore;
bool gameOver = false;
int gs = 4; //gs is GRID SIZE, 4 works well on my duinotech 128x64 OLED
unsigned long currentMillis = 0;
unsigned long previousMillis = 0;
const long interval = 300; // interval to take game turns, 300 is pretty nice
bool inputLock = false;

void snake_onStart() {
  restartGame();
}
void snake_onStop() {}

void snake_onEvent(int evt) {
    // buttons change heading, up button for heading=up and so on
  if ( inputLock == false && heading!=HEAD_DOWN && evt == BTN_UP) {
    heading = HEAD_UP;
    inputLock = true; // stop accepting input presses, prevents snake being able to head back on itself by changing heading rapidly
    gameOver = false;
  }
  if ( inputLock == false && heading!=HEAD_UP && evt == BTN_DOWN) {
    heading = HEAD_DOWN;
    inputLock = true;
    gameOver = false;
  }
  if ( inputLock == false && heading!=HEAD_RIGHT && evt == BTN_LEFT) {
    heading = HEAD_LEFT;
    inputLock = true;
    gameOver = false;
  }
  if ( inputLock == false && heading!=HEAD_LEFT && evt == BTN_RIGHT) {
    heading = HEAD_RIGHT;
    inputLock = true;
    gameOver = false;
  }
  // reset game to starting state
  if (evt == BTN_OK) {
    restartGame();
  }
  if (evt == BTN_BACK) {
        extern App *currentApp;
        currentApp = &app_menu;
        currentApp->onStart();
  }
}

bool onSnakeBody(int x,int y) {
  for (int i=0; i< snakeLength; i++) {
    if (snakeBody[i][0]==x && snakeBody[i][1] == y) {
      return true;
    }
  }
  return false;
}

void logic() {
  // if apple is out of view, spawn it on the game grid at a random location where the snake doesn't occupy
  if (apple[0]== -1 && apple[1] == -1) {
    do {
      apple[0]=random(15);
      apple[1]=random(15);
      //keep picking apple spawinputs();n pos so long as...
    } while ( onSnakeBody(apple[0],apple[1]) );
  }

  // this controls game speed to make the game "tick" along at a set interval
  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;

    inputLock = false; //release input lock, accepting direction presses again

    // detect snake eating apple
    if (apple[0] == snakeHead[0] && apple[1] == snakeHead[1]) {
      //Serial.println("ate apple. yum!");
      snakeLength++;
      apple[0]= -1;
      apple[1]= -1;
    }
    
    // if snake moving update last body postion with head position
    if (heading!=HEAD_STOP) {
  
      //the front of body becomes becomes head position
      snakeBody[snakeLength-1][0] = snakeHead[0];
      snakeBody[snakeLength-1][1] = snakeHead[1];
  
      // the other segments of the body update, the tail[0] becomes segment[1], [1] becomes [2] and so on...
      for (int i=0; i< snakeLength-1; i++) {
        snakeBody[i][0] = snakeBody[i+1][0];
        snakeBody[i][1] = snakeBody[i+1][1];
      }
    }
    // move snake head if heading == HEAD_UP/DOWN/LEFT/RIGHT (nothing here for HEAD_STOP)
    if (heading==HEAD_UP) {
      snakeHead[1]--;
    }
    if (heading==HEAD_DOWN) {
      snakeHead[1]++;
    }
    if (heading==HEAD_LEFT) {
      snakeHead[0]--;
    }
    if (heading==HEAD_RIGHT) {
      snakeHead[0]++;
    }

    //detect if snake head touched snake body
    if ( onSnakeBody(snakeHead[0],snakeHead[1]) ) {
      heading=HEAD_STOP;
      restartGame();
      gameOver = true;
    }
    
    //detect if snake head left game area
    if ( snakeHead[0] < 0 || snakeHead[0] > 15 || snakeHead[1] < 0 || snakeHead[1] > 15 ) {
      heading=HEAD_STOP;
      restartGame();
      gameOver = true;
    }
  }
}

void snake_onDraw(U8G2 *u8g2) {
  u8g2->clearBuffer();
  u8g2->setDrawColor(1);
  u8g2->setFont(u8g2_font_crox1t_tf);
  currentMillis = millis();
  logic();
  u8g2->firstPage(); // required for forst part of display stuff
  do {
    //draw apple
    johnDisc(u8g2, apple[0],apple[1]);

    //draw snake head
    johnBox(u8g2, snakeHead[0],snakeHead[1]);
    //johnHead(snakeHead[0],snakeHead[1]);

    //draw snake body  
    for (int i=0; i< snakeLength-1; i++) {
    johnBox(u8g2, snakeBody[i][0],snakeBody[i][1]);
    }

    //draw SNAKE logo with blank space
    u8g2->drawStr(70,10,  "S");
    u8g2->drawStr(75,12,  "N");
    u8g2->drawStr(83,10,  "A");
    u8g2->drawStr(90,12,  "K");
    u8g2->drawStr(98,10,  "E");
    u8g2->drawStr(107,10,  "!");
    u8g2->drawStr(113,12,  "!");
    u8g2->drawStr(119,10,  "!");

    u8g2->drawStr(70,24, "by johnpathe"); // score stuff
  
    u8g2->drawFrame(0,0,64,64); // divider line, pretty

    u8g2->drawStr(70,40, "Score: "); // score stuff
    itoa(snakeLength-5, scoreText, 10);
    u8g2->drawStr(98,40,  scoreText);

  if (gameOver) {
    u8g2->drawStr(70,50, "You Lose!"); // show 'you lose'
  }
  } while ( u8g2->nextPage() );
}

void johnBox(U8G2 *u8g2, int x,int y) {
  x = x * gs;
  y = y * gs;
  u8g2->drawBox(x, y, gs, gs);
}
void johnDisc(U8G2 *u8g2, int x,int y) {
  x = x * gs+(gs/2);
  y = y * gs+(gs/2);
  int rad = (gs/2);
  u8g2->drawDisc(x,y,rad,U8G2_DRAW_ALL);
}

void restartGame()  {
  snakeHead[0] = 5; 
  snakeHead[1] = 6; 
  heading = HEAD_STOP;
  apple[0]= -1;
  apple[1]= -1;
  snakeBody[0][0]=1;
  snakeBody[0][1]=6;
  snakeBody[1][0]=2;
  snakeBody[1][1]=6;
  snakeBody[2][0]=3;
  snakeBody[2][1]=6;
  snakeBody[3][0]=4;
  snakeBody[3][1]=6;
  snakeBody[4][0]=-1;
  snakeBody[4][1]=-1;
  snakeLength = 5;
  gameOver = false;
}

App app_snake = {
  .name = "Snake",
  .onStart = snake_onStart,
  .onEvent = snake_onEvent,
  .onDraw = snake_onDraw,
  .onStop = snake_onStop
};