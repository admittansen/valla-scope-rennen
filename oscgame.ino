#define TREE_TRUNK 3
#define TREE_WIDTH 5
#define TREE_HEIGHT 13

#define PLAYER_WIDTH 12
#define PLAYER_HEIGHT 20
#define PLAYER_LINE 220

#define CRASH_ANIM_CYCLE 200
#define CRASH_ANIM_TIME 2000
#define CRASH_SIZE 30

#define START_SPEED 4
#define MAX_SPEED 8
#define SPEED_INC_TIME 20000

#define CHANCE_BASE 10000
#define START_CHANCE 40
#define MAX_CHANCE 100
#define CHANCE_INC_TIME 2500

#define SIZE_SHIFT_TIME 5000

#define NTREES 10

#define TICK (1000 / 60)

#define LOVE_TIME 2000
#define HOLDOFF 1500

#define PLAYER_SPEED 8

#define CONTROL_POT A7

// Use 64x64 grid with inverted y-axis for text
#define l(X1, Y1, X2, Y2) line((X1)*4, 255-(Y1)*4, (X2)*4, 255-(Y2)*4)

// Invert y-axis
#define L(X1, Y1, X2, Y2) line(X1, 255-(Y1), X2, 255-(Y2))

enum GameState {
  love,     // Love screen
  title,    // Title screen
  holdoff,  // Before trees start spawning
  running,  // Game running
  crashed,  // Crash animation showing
  end       // End screen with time
};

enum TreeSize {
  small = 2,
  medium = 3,
  large = 4
};

typedef struct 
{
   int x;
   int y;
   TreeSize size;
   bool active;
} tree;

tree trees[NTREES];
unsigned long lastTick = 0;
unsigned long startTime;
unsigned long crashTime;
unsigned long speedIncTime;
unsigned long chanceIncTime;
unsigned long sizeShiftTime;
int speed = START_SPEED;

int crashDecMin;
int crashMin;
int crashDecSec;
int crashSec;

GameState gameState = love;

long treeChance = START_CHANCE;

int smallChance = 45;
int mediumChance = 30;
int largeChance = 25;

int playerPos = 127;
int playerSkew;

void setup() {
  // put your setup code here, to run once:
  DDRC = 0b00111111;
  DDRB = 0b00111111;
  DDRD = 0b11110010;
  
  for (int i = 0; i < NTREES; i++)
  {
    trees[i].active = false;
  }

  //Serial.begin(115200);
}

void loop() {
  if (millis() - lastTick >= TICK)
  {
    lastTick = millis();
    update();
  }
  
  draw();
}

void update()
{
  switch (gameState)
  {
    case love:
    {
      if (millis() > LOVE_TIME)
      {
        gameState = title;
      }
      break;
    }
    case title:
    {
      if (millis() > 4000)
      {
        gameState = holdoff;
        startTime = millis();
        speedIncTime = startTime;
        chanceIncTime = startTime;
        sizeShiftTime = startTime;
        randomSeed(analogRead(CONTROL_POT) ^ micros());
      }
      break;
    }
    case holdoff:
    {
      if (millis() - startTime > HOLDOFF)
      {
        gameState = running;
      }
    } // FALLTHROUGH
    case running:
    {
      int targetPos = PLAYER_WIDTH + ((255 - 2 * PLAYER_WIDTH) * (unsigned int)(analogRead(CONTROL_POT) >> 2)) / 255;
      
      int playerErr = targetPos - playerPos;
      int dir = abs(playerErr) <= PLAYER_WIDTH ? playerErr : PLAYER_WIDTH * (playerErr < 0 ? -1 : 1);
      playerSkew = dir/2;
      
      if (targetPos > playerPos + PLAYER_SPEED)
      {
        playerPos += PLAYER_SPEED;
      } 
      else if (targetPos < playerPos - PLAYER_SPEED)
      {
        playerPos -= PLAYER_SPEED;
      } 
      else 
      {
        playerPos = targetPos;
      }

      if (gameState == running)
      {
        // Increase speed over time
        if (millis() - speedIncTime >= SPEED_INC_TIME)
        {
          speedIncTime = millis();
          
          if (speed < MAX_SPEED)
          {
            speed++;
          }
        }

        // Increase probability of spawning trees over time
        if (millis() - chanceIncTime >= CHANCE_INC_TIME)
        {
          chanceIncTime = millis();
          
          if (treeChance < MAX_CHANCE)
          {
            treeChance++;
          }
        }

        // Shift probability of small trees to large trees over time
        if (millis() - sizeShiftTime >= SIZE_SHIFT_TIME)
        {
          sizeShiftTime = millis();
          
          if (smallChance > 0)
          {
            smallChance--;
            largeChance++;
          }
        }
        
        for (int i = 0; i < NTREES; i++)
        {
          if (trees[i].active)
          {
            trees[i].y += speed;
    
            if (checkPlayerTreeCollision(trees[i]))
            {
              gameState = crashed;
              crashTime = millis();
              
              unsigned long time = (crashTime - startTime) / 1000;
              int minutes = time / 60;
              int seconds = time % 60;
              
              crashDecMin = minutes / 10;
              crashMin = minutes % 10;
              crashDecSec = seconds / 10;
              crashSec = seconds % 10;
              return;
            }
      
            if (trees[i].y > 255)
            {
              trees[i].active = false;
            }
          }
          else
          {
            if (random(CHANCE_BASE) < treeChance)
            {
              trees[i].active = true;
  
              int size = random(smallChance + mediumChance + largeChance);
              if (size < smallChance)
              {
                trees[i].size = small;
              }
              else if (size < smallChance + mediumChance)
              {
                trees[i].size = medium;
              }
              else 
              {
                trees[i].size = large;
              }
    
              trees[i].y = -TREE_HEIGHT * trees[i].size;
    
              trees[i].x = random(TREE_WIDTH * trees[i].size, 255 - TREE_WIDTH * trees[i].size);
            }
          }
        }
      }
      break;
    }
    case crashed:
    {
      if (millis() - crashTime >= CRASH_ANIM_TIME)
      {
        gameState = end;
      }
      break;
    }
    case end:
    {
      break;
    }
  }
}

void draw()
{
  drawPerimeter();
  switch (gameState)
  {
    case love:
    {
      drawLove();
      break;
    }
    case title:
    {
      drawTitleScreen();
      break;
    }
    case holdoff: // FALLTHROUGH
    case running:
    {
      // Player
      line(playerPos + playerSkew, PLAYER_LINE, playerPos - PLAYER_WIDTH, PLAYER_LINE + PLAYER_HEIGHT);
      line(playerPos + playerSkew, PLAYER_LINE, playerPos + PLAYER_WIDTH, PLAYER_LINE + PLAYER_HEIGHT);
      line(playerPos + playerSkew, PLAYER_LINE, playerPos + playerSkew*2, PLAYER_LINE - PLAYER_HEIGHT);
      
    } // FALLTHROUGH
    case crashed:
    {
      if (gameState == crashed)
      {
        int length = (CRASH_SIZE * ((millis() - crashTime) % CRASH_ANIM_CYCLE)) / CRASH_ANIM_CYCLE;
        int diag = (length * 2) / 3;

        // Crash animation
        line(playerPos, PLAYER_LINE, playerPos + length, PLAYER_LINE);
        line(playerPos, PLAYER_LINE, playerPos - length, PLAYER_LINE);
        line(playerPos, PLAYER_LINE, playerPos, PLAYER_LINE + length);
        line(playerPos, PLAYER_LINE, playerPos, PLAYER_LINE - length);

        line(playerPos, PLAYER_LINE, playerPos + diag, PLAYER_LINE + diag);
        line(playerPos, PLAYER_LINE, playerPos - diag, PLAYER_LINE - diag);
        line(playerPos, PLAYER_LINE, playerPos + diag, PLAYER_LINE - diag);
        line(playerPos, PLAYER_LINE, playerPos - diag, PLAYER_LINE + diag);
      }
      
      // Trees    
      for (int i = 0; i < NTREES; i++)
      {
        if (trees[i].active)
        {
          drawTree(trees[i].x, trees[i].y, trees[i].size);
        }
      }
      break;
    }
    case end:
    {
      drawEndScreen();
      break;
    }
  }

  plot(0, 255);
}

void drawPerimeter()
{
  line(0, 255, 255, 255);
  line(255, 255, 255, 0);
  line(255, 0, 0, 0);
  line(0, 0, 0, 255);
}

void drawTree(int x, int y, byte size)
{  
  byte height = TREE_HEIGHT * size;
  byte width = TREE_WIDTH * size;
  
  line(x, y - TREE_TRUNK * size, x, y);
  line(x, y, x - width, y);
  line(x - width, y, x, y + height);
  line(x, y + height, x + width, y);
  line(x + width, y, x, y);
}

bool checkPlayerTreeCollision(tree t)
{
  byte height = TREE_HEIGHT * t.size;
  
  if (t.y + height < PLAYER_LINE - PLAYER_HEIGHT)
  {
    return false;
  }

  byte width = TREE_WIDTH * t.size;
  
  return
  pointInTriangle(
    playerPos - PLAYER_WIDTH, PLAYER_LINE + PLAYER_HEIGHT,
    t.x- width, t.y, 
    t.x + width, t.y, 
    t.x, t.y + height)
  ||
  pointInTriangle(
    playerPos + PLAYER_WIDTH, PLAYER_LINE + PLAYER_HEIGHT,
    t.x- width, t.y, 
    t.x + width, t.y, 
    t.x, t.y + height)
  ||
  pointInTriangle(
    playerPos + playerSkew, PLAYER_LINE,
    t.x- width, t.y, 
    t.x + width, t.y, 
    t.x, t.y + height)
  ||
  pointInTriangle(
    playerPos + playerSkew * 2, PLAYER_LINE - PLAYER_HEIGHT,
    t.x- width, t.y, 
    t.x + width, t.y, 
    t.x, t.y + height);
}

bool pointInTriangle (int px, int py, int x1, int y1, int x2, int y2, int x3, int y3)
{
    int px1 = px-x1;
    int py1 = py-y1;

    bool p12 = (x2-x1)*py1-(y2-y1)*px1 > 0;

    if((x3-x1)*py1-(y3-y1)*px1 > 0 == p12) return false;

    if((x3-x2)*(py-y2)-(y3-y2)*(px-x2) > 0 != p12) return false;

    return true;
}

void line(int x0, int y0, int x1, int y1) {     /// Bresenham's Line Algorithm  
  plot(x0, y0);
  
  int dx = abs(x1-x0), sx = x0<x1 ? 1 : -1;
  int dy = abs(y1-y0), sy = y0<y1 ? 1 : -1;
  int err = (dx>dy ? dx : -dy)/2, e2;
  
  while (x0!=x1 || y0!=y1) {
    e2 = err;
    if (e2 >-dx) { 
      err -= dy;
      x0 += sx;
    }    
    if (e2 < dy) { 
      err += dx;
      y0 += sy;
    }

    plot(x0, y0);
  }
}

void plot(int x, int y)
{
  // Don't go outside of screen
  //if (highByte(x)) return;
  //if (highByte(y)) return;
  
  if (x < 0) x = 0;
  else if (x > 255) x = 255;
  if (y < 0) y = 0;
  else if (y > 255) y = 255;
  
  PORTD = (lowByte(y) & 0b11000000) | ((0b11000000 & lowByte(x)) >> 2);
  PORTC = lowByte(x);
  PORTB = lowByte(y); 
}

void drawTitleScreen()
{
  #define HIGHLIGHT 4
  
  // V
  for (int i = 0; i < HIGHLIGHT; i++)
  {
    l(3,7, 3,15);
    l(3,15, 7,19);
    l(7,19, 11,15);
    l(11,15, 11,7);
  }

  // A
  l(14,19, 14,10);
  l(14,10, 17,7);
  l(17,7, 19,7);
  l(19,7, 22,10);
  l(22,10, 22,19);
  l(14,15, 22, 15);

  // L
  l(25,7, 25,19);
  l(25,19, 31,19);

  // L
  l(33,7, 33,19);
  l(33,19, 39,19);

  // A
  l(41,19, 41,11);
  l(41,11, 44,7);
  l(44,7, 46,7);
  l(46,7, 49,10);
  l(49,10, 49,19);
  l(49,15, 41,15);

  // S
  for (int i = 0; i < HIGHLIGHT; i++)
  {
    l(10,26, 9,25);
    l(9,25, 5,25);
    l(5,25, 3,27);
    l(3,27, 3,29);
    l(3,29, 5,31);
    l(5,31, 9,31);
    l(9,31, 11,33);
    l(11,33, 11,35);
    l(11,35, 9,37);
    l(9,37, 4,37);
    l(4,37, 3,36);
  }

  // C
  l(22,34, 22,35);
  l(22,35, 20,37);
  l(20,37, 16,37);
  l(16,37, 14,35);
  l(14,35, 14,27);
  l(14,27, 16,25);
  l(16,25, 20,25);
  l(20,25, 22,27);
  l(22,27, 22,28);

  // O
  l(25,27, 27,25);
  l(27,25, 31,25);
  l(31,25, 33,27);
  l(33,27, 33,35);
  l(33,35, 31,37);
  l(31,37, 27,37);
  l(27,37, 25,35);
  l(25,35, 25,27);

  // P
  l(36,25, 42,25);
  l(42,25, 44,27);
  l(44,27, 44,31);
  l(44,31, 42,33);
  l(42,33, 36,33);
  l(36,37, 36,25);

  // E
  l(47,25, 53,25);
  l(47,25, 47,37);
  l(47,37, 53,37);
  l(50,31, 47,31);

  // R
  for (int i = 0; i < HIGHLIGHT; i++)
  {
    l(9,51, 3,51);
    l(3,55, 3,43);
    l(3,43, 9,43);
    l(9,43, 11,45);
    l(11,45, 11,49);
    l(11,49, 9,51);
    l(9,51, 11,53);
    l(11,53, 11,55);
  }

  // E
  l(14,55, 14,43);
  l(14,43, 20,43);
  l(17,49, 14,49);
  l(14,55, 20,55);

  // N
  l(22,55, 22,43);
  l(22,43, 30,51);
  l(30,43, 30,55);

  // N
  l(33,55, 33,43);
  l(33,43, 41,51);
  l(41,43, 41,55);

  // E
  l(44,55, 44,43);
  l(44,43, 50,43);
  l(47,49, 44,49);
  l(44,55, 50,55);

  // N
  l(52,55, 52,43);
  l(52,43, 60,51);
  l(60,43, 60,55);
}

void drawEndScreen()
{
  // G
  l(17,8, 17,7);
  l(17,7, 14,4);
  l(14,4, 9,4);
  l(9,4, 6,7);
  l(6,7, 6,16);
  l(6,16, 9,19);
  l(9,19, 14,19);
  l(14,19, 17,16);
  l(17,16, 17,11);
  l(17,11, 14,11);

  // A
  l(20,19, 20,9);
  l(20,9, 25,4);
  l(25,4, 26,4);
  l(26,4, 31,9);
  l(31,9, 31,19);
  l(20,14, 31,14);

  // M
  l(34,19, 34,4);
  l(34,4, 39,9);
  l(39,9, 40,9);
  l(40,9, 45,4);
  l(45,4, 45,19);

  // E
  l(56,19, 48,19);
  l(48,19, 48,4);
  l(48,4, 56,4);
  l(53,11, 48,11);

  // O
  l(14,24, 9,24);
  l(9,24, 6,27);
  l(6,27, 6,36);
  l(6,36, 9,39);
  l(9,39, 14,39);
  l(14,39, 17,36);
  l(17,36, 17,27);
  l(17,27, 14,24);

  // V
  l(20,24, 20,34);
  l(20,34, 25,39);
  l(25,39, 26,39);
  l(26,39, 31,34);
  l(31,34, 31,24);

  // E
  l(42,24, 34,24);
  l(34,24, 34,39);
  l(34,39, 42,39);
  l(34,31, 39,31);

  // R
  l(45,39, 45,24);
  l(45,24, 53,24);
  l(53,24, 56,27);
  l(56,27, 56,31);
  l(56,31, 53,34);
  l(53,34, 56,37);
  l(56,37, 56,39);
  l(53,34, 45,34);

  drawTime();
}

void drawTime()
{  
  // Minutes
  drawDigit(crashDecMin, 60, 20);
  drawDigit(crashMin, 100, 20);

  // Separator dots
  line(126, 36, 126, 37);
  line(126, 55, 126, 56);

  // Seconds
  drawDigit(crashDecSec, 155, 20);
  drawDigit(crashSec, 195, 20);
}

void drawDigit(byte digit, byte x1, byte y1)
{
  switch (digit)
  {
    case 0:
      return draw0(x1,y1);
    case 1:
      return draw1(x1,y1);
    case 2:
      return draw2(x1,y1);
    case 3:
      return draw3(x1,y1);
    case 4:
      return draw4(x1,y1);
    case 5:
      return draw5(x1,y1);
    case 6:
      return draw6(x1,y1);
    case 7:
      return draw7(x1,y1);
    case 8:
      return draw8(x1,y1);
    case 9:
      return draw9(x1,y1);
  }
}

void draw0(byte x1, byte y1)
{
  int x = x1/4 - 4;
  int y = 64 - (y1/4 + 14);
  
  l(0+x,12+y, 2+x,14+y);
  l(2+x,14+y, 6+x,14+y);
  l(6+x,14+y, 8+x,12+y);
  l(8+x,12+y, 8+x,2+y);
  l(8+x,2+y, 6+x,0+y);
  l(6+x,0+y, 2+x,0+y);
  l(2+x,0+y, 0+x,2+y);
  l(0+x,2+y, 0+x,12+y);
  l(0+x,11+y, 8+x,3+y);
}

void draw1(byte x1, byte y1)
{
  int x = x1/4 - 2;
  int y = 64 - (y1/4 + 14);
  
  l(0+x,2+y, 2+x,0+y);
  l(2+x,0+y, 2+x,14+y);
  l(0+x,14+y, 4+x,14+y);
}

void draw2(byte x1, byte y1)
{
  int x = x1/4 - 4;
  int y = 64 - (y1/4 + 14);
  
  l(0+x,2+y, 2+x,0+y);
  l(2+x,0+y, 6+x,0+y);
  l(6+x,0+y, 8+x,2+y);
  l(8+x,2+y, 8+x,6+y);
  l(8+x,6+y, 0+x,14+y);
  l(0+x,14+y, 8+x,14+y);
}

void draw3(byte x1, byte y1)
{
  int x = x1/4 - 4;
  int y = 64 - (y1/4 + 14);
  
  l(0+x,2+y, 2+x,0+y);
  l(2+x,0+y, 6+x,0+y);
  l(6+x,0+y, 8+x,2+y);
  l(8+x,2+y, 8+x,5+y);
  l(8+x,5+y, 6+x,7+y);
  l(6+x,7+y, 8+x,9+y);
  l(8+x,9+y, 8+x,12+y);
  l(8+x,12+y, 6+x,14+y);
  l(6+x,14+y, 2+x,14+y);
  l(2+x,14+y, 0+x,12+y);
  l(3+x,7+y, 6+x,7+y);
}

void draw4(byte x1, byte y1)
{
  int x = x1/4 - 4;
  int y = 64 - (y1/4 + 14);
  
  l(6+x,0+y, 0+x,6+y);
  l(0+x,6+y, 0+x,11+y);
  l(0+x,11+y, 8+x,11+y);
  l(6+x,9+y, 6+x,14+y);
}

void draw5(byte x1, byte y1)
{
  int x = x1/4 - 4;
  int y = 64 - (y1/4 + 14);
  
  l(8+x,0+y, 0+x,0+y);
  l(0+x,0+y, 0+x,4+y);
  l(0+x,4+y, 1+x,5+y);
  l(1+x,5+y, 6+x,5+y);
  l(6+x,5+y, 8+x,7+y);
  l(8+x,7+y, 8+x,12+y);
  l(8+x,12+y, 6+x,14+y);
  l(6+x,14+y, 2+x,14+y);
  l(2+x,14+y, 0+x,12+y);
}

void draw6(byte x1, byte y1)
{
  int x = x1/4 - 4;
  int y = 64 - (y1/4 + 14);
  
  l(8+x,2+y, 6+x,0+y);
  l(6+x,0+y, 2+x,0+y);
  l(2+x,0+y, 0+x,2+y);
  l(0+x,2+y, 0+x,12+y);
  l(0+x,12+y, 2+x,14+y);
  l(2+x,14+y, 6+x,14+y);
  l(6+x,14+y, 8+x,12+y);
  l(8+x,12+y, 8+x,7+y);
  l(8+x,7+y, 6+x,5+y);
  l(6+x,5+y, 2+x,5+y);
  l(2+x,5+y, 0+x,7+y);
}

void draw7(byte x1, byte y1)
{
  int x = x1/4 - 4;
  int y = 64 - (y1/4 + 14);
  
  l(0+x,2+y, 0+x,0+y);
  l(0+x,0+y, 8+x,0+y);
  l(8+x,0+y, 8+x,5+y);
  l(8+x,5+y, 3+x,10+y);
  l(3+x,10+y, 3+x,14+y);
}

void draw8(byte x1, byte y1)
{
  int x = x1/4 - 4;
  int y = 64 - (y1/4 + 14);
  
  l(2+x,0+y, 0+x,2+y);
  l(0+x,2+y, 0+x,5+y);
  l(0+x,5+y, 2+x,7+y);
  l(2+x,7+y, 6+x,7+y);
  l(6+x,7+y, 8+x,9+y);
  l(8+x,9+y, 8+x,12+y);
  l(8+x,12+y, 6+x,14+y);
  l(6+x,14+y, 2+x,14+y);
  l(2+x,14+y, 0+x,12+y);
  l(0+x,12+y, 0+x,9+y);
  l(0+x,9+y, 2+x,7+y);
  l(6+x,7+y, 8+x,5+y);
  l(8+x,5+y, 8+x,2+y);
  l(8+x,2+y, 6+x,0+y);
  l(6+x,0+y, 2+x,0+y);
}

void draw9(byte x1, byte y1)
{
  int x = x1/4 - 4;
  int y = 64 - (y1/4 + 14);
  
  l(0+x,12+y, 2+x,14+y);
  l(2+x,14+y, 6+x,14+y);
  l(6+x,14+y, 8+x,12+y);
  l(8+x,12+y, 8+x,2+y);
  l(8+x,2+y, 6+x,0+y);
  l(6+x,0+y, 2+x,0+y);
  l(2+x,0+y, 0+x,2+y);
  l(0+x,2+y, 0+x,7+y);
  l(0+x,7+y, 2+x,9+y);
  l(2+x,9+y, 6+x,9+y);
  l(6+x,9+y, 8+x,8+y);
}

void drawLove()
{
  // Ring
  L(39,87, 52,87);
  L(52,87, 67,93);
  L(67,93, 78,104);
  L(78,104, 84,119);
  L(84,119, 84,132);
  L(84,132, 78,147);
  L(78,147, 67,158);
  L(67,158, 52,164);
  L(52,164, 39,164);
  L(39,164, 24,158);
  L(24,158, 13,147);
  L(13,147, 7,132);
  L(7,132, 7,119);
  L(7,119, 13,104);
  L(13,104, 24,93);
  L(24,93, 39,87);

  // Omega
  L(21,116, 23,104);
  L(23,104, 41,104);
  L(41,104, 41,106);
  L(41,106, 33,119);
  L(33,119, 32,127);
  L(31,128, 31,130);
  L(32,131, 33,140);
  L(33,140, 35,144);
  L(35,144, 41,148);
  L(42,149, 49,149);
  L(50,148, 56,144);
  L(56,144, 58,140);
  L(58,140, 59,131);
  L(60,130, 60,128);
  L(59,127, 58,119);
  L(58,119, 50,106);
  L(50,106, 50,104);
  L(50,104, 68,104);
  L(68,104, 70,116);
  L(69,116, 66,109);
  L(66,109, 55,109);
  L(55,109, 55,111);
  L(55,111, 60,116);
  L(60,116, 63,121);
  L(63,121, 66,126);
  L(66,126, 66,130); 
  L(67,131, 67,135);
  L(66,136, 65,141);
  L(65,141, 62,145);
  L(63,145, 58,148);
  L(58,148, 51,151); 
  L(51,151, 40,151);
  L(40,151, 33,148);
  L(33,148, 29,145);
  L(29,145, 26,141);
  L(26,141, 25,136);
  L(24,135, 24,131);
  L(25,130, 25,126);
  L(25,126, 28,121);
  L(28,121, 31,116);
  L(31,116, 36,111);
  L(36,111, 36,109);
  L(36,109, 25,109);
  L(25,109, 22,116);


  // Heart
  L(90,123, 127,160);
  L(128,160, 165,123);
  L(165,123, 167,117);
  L(167,117, 167,111);
  L(167,111, 166,106);
  L(166,106, 163,101);
  L(163,101, 159,97);
  L(159,97, 156,95);
  L(156,95, 150,93);
  L(150,93, 143,93);
  L(143,93, 139,94);
  L(139,94, 133,97);
  L(133,97, 130,100);
  L(130,100, 128,104);
  L(127,104, 125,100);
  L(125,100, 122,97);
  L(122,97, 116,94);
  L(116,94, 112,93);
  L(112,93, 105,93);
  L(105,93, 99,95);
  L(99,95, 96,97);
  L(96,97, 92,101);
  L(92,101, 89,106);
  L(89,106, 88,111);
  L(88,111, 88,117);
  L(88,117, 90,123);  
  
  // y "o"
  L(173,108, 175,104);
  L(175,104, 177,102);
  L(177,102, 181,100);
  L(181,100, 185,100);
  L(185,100, 189,102);
  L(189,102, 191,104);
  L(191,104, 193,108);
  L(193,108, 193,112);
  L(193,112, 191,116);
  L(191,116, 189,118);
  L(189,118, 185,120);
  L(185,120, 181,120);
  L(181,120, 177,118);
  L(177,118, 175,116);
  L(175,116, 173,112);
  L(173,112, 173,108);
  
  // y "/"
  L(209, 100, 226, 100);
  L(226, 100, 190, 153);
  L(190, 153, 173, 153);
  L(173, 153, 209, 100);

  // 6 "("
  L(232, 100, 249, 100);
  L(249, 100, 223, 138);
  L(223, 138, 222, 141);
  L(222, 141, 222, 147);
  L(222, 147, 223, 150);
  L(223, 150, 226, 153);
  L(226, 153, 210, 153);
  L(210, 153, 208, 151);
  L(208, 151, 206, 147);
  L(206, 147, 206, 141);
  L(206, 141, 207, 137);
  L(207, 137, 231, 100);

  // 6 "o"
  L(173+57,108+33, 175+57,104+33);
  L(175+57,104+33, 177+57,102+33);
  L(177+57,102+33, 181+57,100+33);
  L(181+57,100+33, 185+57,100+33);
  L(185+57,100+33, 189+57,102+33);
  L(189+57,102+33, 191+57,104+33);
  L(191+57,104+33, 193+57,108+33);
  L(193+57,108+33, 193+57,112+33);
  L(193+57,112+33, 191+57,116+33);
  L(191+57,116+33, 189+57,118+33);
  L(189+57,118+33, 185+57,120+33);
  L(185+57,120+33, 181+57,120+33);
  L(181+57,120+33, 177+57,118+33);
  L(177+57,118+33, 175+57,116+33);
  L(175+57,116+33, 173+57,112+33);
  L(173+57,112+33, 173+57,108+33);
}

