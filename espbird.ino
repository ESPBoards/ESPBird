#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// Bird sprite dimensions
#define BIRD_SIZE 16

// GPIO pins
#define I2C_SDA 13
#define I2C_SCL 12
#define BUTTON_PIN 11

// OLED display settings
#define OLED_WIDTH 128 // OLED width,  in pixels
#define OLED_HEIGHT 64 // OLED height, in pixels
Adafruit_SSD1306 oled(OLED_WIDTH, OLED_HEIGHT, &Wire, -1);

// Bitmaps for bird animation
static const unsigned char PROGMEM bird_frame1[] = {
    B00000000, B00000000, 
    B00000000, B00000000, 
    B00000000, B00000000, 
    B00000011, B11100000, 
    B01000100, B00010001, 
    B00101001, B01001010, 
    B00011000, B00001100, 
    B00001010, B00101000, 
    B00001001, B11001000, 
    B00000100, B00010000, 
    B00000011, B11100000, 
    B00000000, B00000000, 
    B00000000, B00000000, 
    B00000000, B00000000, 
    B00000000, B00000000, 
    B00000000, B00000000
};

static const unsigned char PROGMEM bird_frame2[] = {
    B00000000, B00000000, 
    B00000000, B00000000, 
    B00000000, B00000000, 
    B00000011, B11100000, 
    B00000100, B00010000, 
    B00001001, B01001000, 
    B00011000, B00001100, 
    B00101010, B00101010, 
    B01001001, B11001001, 
    B00000100, B00010000, 
    B00000011, B11100000, 
    B00000000, B00000000, 
    B00000000, B00000000, 
    B00000000, B00000000, 
    B00000000, B00000000, 
    B00000000, B00000000
};


// Game settings
#define FRAME_DELAY 50

// Game states
#define STATE_START 0
#define STATE_PLAYING 1
#define STATE_GAMEOVER 2

int current_state = STATE_START;
int birdX = OLED_WIDTH / 4;
int birdY, gravity;
int score, bestScore;

int wallX[2];
int gapY[2];
#define GAP_SIZE 30
#define WALL_THICKNESS 10

void setup() {
  Serial.begin(9600);

  // Setup I2C on custom pins
  Wire.begin(I2C_SDA, I2C_SCL);

  if (!oled.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("OLED Initialization Failed"));
    for (;;);
  }

  oled.clearDisplay();
  oled.setTextColor(WHITE);

  pinMode(BUTTON_PIN, INPUT_PULLUP);

  randomSeed(analogRead(10));
}

void loop() {
  if (current_state == STATE_PLAYING) {
    updateGame();
  } else if (current_state == STATE_GAMEOVER) {
    showGameOver();
  } else {
    showStartScreen();
  }
  delay(FRAME_DELAY);
}

void updateGame() {
  oled.clearDisplay();

  // Flap mechanism
  if (digitalRead(BUTTON_PIN) == LOW) gravity = -4;

  gravity++;
  birdY += gravity;

  birdY = constrain(birdY, 0, OLED_HEIGHT - BIRD_SIZE);
  if (birdY == OLED_HEIGHT - BIRD_SIZE) gravity = -2;

  // Bird animation
  const unsigned char *sprite = (gravity < 0 && random(2)) ? bird_frame1 : bird_frame2;
  oled.drawBitmap(birdX, birdY, sprite, BIRD_SIZE, BIRD_SIZE, WHITE);

  // Walls logic
  for (int i = 0; i < 2; i++) {
    oled.fillRect(wallX[i], 0, WALL_THICKNESS, gapY[i], WHITE);
    oled.fillRect(wallX[i], gapY[i] + GAP_SIZE, WALL_THICKNESS, OLED_HEIGHT - gapY[i] + GAP_SIZE, WHITE);

    wallX[i] -= 4;

    if (wallX[i] < -WALL_THICKNESS) {
      wallX[i] = OLED_WIDTH;
      gapY[i] = random(10, OLED_HEIGHT - GAP_SIZE - 10);
    }

    if (wallX[i] == birdX) {
      score++;
      bestScore = max(bestScore, score);
    }

    if ((birdX + BIRD_SIZE > wallX[i] && birdX < wallX[i] + WALL_THICKNESS)
        && (birdY < gapY[i] || birdY + BIRD_SIZE > gapY[i] + GAP_SIZE)) {
      current_state = STATE_GAMEOVER;
    }
  }

  oled.setCursor(OLED_WIDTH / 2 - 6, 0);
  oled.print(score);
  oled.display();
}

void showGameOver() {
  wipeScreen();
  oled.setCursor(OLED_WIDTH / 2 - 30, OLED_HEIGHT / 2 - 20);
  oled.print(F("GAME OVER"));

  oled.setCursor(OLED_WIDTH / 2 - 3, OLED_HEIGHT / 2 - 10);
  oled.print(score);

  oled.setCursor(OLED_WIDTH / 2 - 30, OLED_HEIGHT - 26);
  oled.print(F("BEST SCORE"));

  oled.setCursor(OLED_WIDTH / 2 - 3, OLED_HEIGHT - 18);
  oled.print(bestScore);

  oled.display();

  while (digitalRead(BUTTON_PIN) == LOW);

  birdY = OLED_HEIGHT / 2;
  gravity = -4;
  score = 0;
  wallX[0] = OLED_WIDTH;
  gapY[0] = OLED_HEIGHT / 2 - GAP_SIZE / 2;
  wallX[1] = OLED_WIDTH + OLED_WIDTH / 2;
  gapY[1] = random(10, OLED_HEIGHT - GAP_SIZE - 10);

  while (digitalRead(BUTTON_PIN) == HIGH);

  wipeScreen();
  current_state = STATE_PLAYING;
}

void showStartScreen() {
  oled.clearDisplay();
  oled.setCursor(OLED_WIDTH / 2 - 30, OLED_HEIGHT / 2 - 8);
  oled.print(F("ESP BIRD"));
  oled.setCursor(OLED_WIDTH / 2 - 42, OLED_HEIGHT / 2 + 8);
  oled.print(F("Press to Start"));
  oled.display();

  if (digitalRead(BUTTON_PIN) == LOW) {
    wipeScreen();
    birdY = OLED_HEIGHT / 2;
    gravity = -4;
    score = 0;
    wallX[0] = OLED_WIDTH;
    gapY[0] = OLED_HEIGHT / 2 - GAP_SIZE / 2;
    wallX[1] = OLED_WIDTH + OLED_WIDTH / 2;
    gapY[1] = random(10, OLED_HEIGHT - GAP_SIZE - 10);
    current_state = STATE_PLAYING;
  }
}

void wipeScreen() {
  oled.clearDisplay();
  oled.display();
}
