#include <Adafruit_NeoPixel.h>
#include <Stepper.h>

// Macros
#define PIN                   6 // Pin on the Arduino connected to NeoPixel
#define NUMPIXELS            24 // Number of NeoPixels attached to Arduino
#define BUTTON_PIN            2 // Pin number of button
#define DELAYVAL            200 // Default delay value
#define BEGIN_INDEX           0 // Starting point of moving indices
#define DEFAULT_START_INDEX   0 // Default start index of valid zone in LED game
#define DEFAULT_END_INDEX    20 // Default end index of valid zone in LED game
#define STEP                  2 // Step size for index increment/decrement
#define SHIFT                20 // Number of pixels to shift the LEDs

// Motor
const int stepsPerRevolution = 2048;
int wStepsPerCandy = stepsPerRevolution / 6;
Stepper wStepper(stepsPerRevolution, 8, 10, 9, 11);
int wIncomingByte;


Adafruit_NeoPixel pixels(NUMPIXELS, PIN, NEO_GRB + NEO_KHZ800);

// Functions
void updateIdle();
void initGame();
void setupLevel(boolean new_game);
void updateGame();
void checkGameOver();
void exitGame();
void setAll(uint32_t color);
void blinkAll(int num, uint32_t color);
void blinkOne(int num, uint32_t color, int pos);
void setColor(int pos, uint32_t color);
void initIndices();
boolean isOver();
boolean isWithinZone(int index);
int getVirtualIndex(int index);
int wrapIndex(int index);

// Constants & Macros
const uint32_t OFF = pixels.Color(0, 0, 0);
const uint32_t WHITE = pixels.Color(50, 50, 50);
const uint32_t GREEN = pixels.Color(0, 150, 0);
const uint32_t RED = pixels.Color(150, 0, 0);
const uint32_t BLUE = pixels.Color(0, 0, 250);
const int LONG_DELAY = 1000;
const int MEDIUM_DELAY = 300;
const int SHORT_DELAY = 50;

// Variables
int idle_index;  // Index for leading LEDs that moves when idle
int game_index;  // Index for single LED that moves during the game
int start_index; // Index for beginning of valid zone
int end_index;   // Index for end of valid zone
int num_points;
int buttonState = 0;
boolean game_started = false;


void setup() {
  initIndices();
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  Serial.begin(9600);
  pixels.begin();
  
  // Motor
  wStepper.setSpeed(6);
}

void loop() {
  int buttonState = digitalRead(BUTTON_PIN);
  
  if (game_started) {
    if (buttonState == LOW) {
      checkGameOver();
    } else {
      updateGame();
    }
  } else {
    if (buttonState == LOW) {
      initGame();
    } else {
      updateIdle();
    }
  }
  // delay to avoid bouncing
  delay(SHORT_DELAY);
}


/* Game functions */

void updateIdle() {
//  Serial.println("Idling");

  int actual_index = idle_index % NUMPIXELS;
  if (idle_index < NUMPIXELS) { // Turn on LEDs
    setColor(actual_index, WHITE);
  } else {  // Turn off LEDs
    setColor(actual_index, OFF);
  }
  idle_index = (idle_index + 1) % (NUMPIXELS * 2);
  pixels.show();
  return;
}

void initGame() {
  //Serial.println("Init Game");

  // First turn off all LEDs and blink twice to signal game starting
  setAll(OFF);
  blinkAll(2, WHITE);
  delay(LONG_DELAY);
  
  // Setup the level
  setupLevel(false);
  game_started = true;
  return;
}

void setupLevel(boolean next_level) {
  game_index = wrapIndex(BEGIN_INDEX + SHIFT);
  if (isOver()) {
    //Serial.println("You won!!");
    exitGame();
    blinkAll(2, WHITE);
    return;
  }
  if (next_level) {
    //Serial.println("Setting next level game indices");
    start_index = wrapIndex(start_index + STEP);
    end_index = wrapIndex(end_index - STEP);
  }
  for(int i = 0; i < NUMPIXELS; i++) {
    if (isWithinZone(i)) {
      setColor(i, GREEN);
    } else {
      setColor(i, OFF);
    }
  }
  setColor(game_index, RED);
  pixels.show();
  return;
}

void updateGame() {
  //Serial.println("Updating");

  // Set current pixel to appropriate color and move red pixel forward
  if (isWithinZone(game_index)) {
    setColor(game_index, GREEN);
  } else {
    setColor(game_index, OFF); 
  }
  game_index = (game_index + 1) % NUMPIXELS;
  setColor(game_index, RED);
  pixels.show();
  return;
}

void checkGameOver() {
  //Serial.println("Checking!");

  setColor(game_index, OFF);
  boolean is_success = false;
  if (isWithinZone(game_index)) {
    blinkOne(3, BLUE, game_index);  // blinkAll(3, BLUE);
    num_points = num_points + 1;
    is_success = true;
  } else {
    exitGame();
    blinkOne(3, RED, game_index);   // blinkAll(3, RED);
    return;
  }
  setupLevel(is_success);
  return;
}

void exitGame() {
  //Serial.println(num_points);

  for (int m = 0; m < num_points; m++){
    delay(500);
    wStepper.step(wStepsPerCandy);
  }
  
  game_started = false;
  initIndices(); // Reset indices when game is over
}

/* Helper functions */

void blinkAll(int num, uint32_t color) {
  for (int i = 0; i < num; i++) {
    setAll(color);
    delay(MEDIUM_DELAY);
    setAll(OFF);
    delay(MEDIUM_DELAY);
  }
  return;
}

void blinkOne(int num, uint32_t color, int pos) {
  for (int i = 0; i < num; i++) {
    setColor(pos, color);
    pixels.show();
    delay(MEDIUM_DELAY);
    setColor(pos, OFF);
    pixels.show();
    delay(MEDIUM_DELAY);
  }
  return;
}

void setAll(uint32_t color) {
  for(int i = 0; i < NUMPIXELS; i++) {
    setColor(i, color);
  }
  pixels.show();
  return;
}

void setColor(int pos, uint32_t color) {
  pixels.setPixelColor(pos, color);
  return;
}

void initIndices() {
  game_index = wrapIndex(BEGIN_INDEX + SHIFT);
  idle_index = wrapIndex(BEGIN_INDEX + SHIFT);
  start_index = wrapIndex(DEFAULT_START_INDEX + SHIFT);
  end_index = wrapIndex(DEFAULT_END_INDEX + SHIFT);
  num_points = 0;
}

boolean isOver() {
  return getVirtualIndex(start_index) >= getVirtualIndex(end_index);
}

boolean isWithinZone(int index) {
  return (getVirtualIndex(index) >= getVirtualIndex(start_index) && getVirtualIndex(index) <= getVirtualIndex(end_index));
}

int getVirtualIndex(int index) {
  return wrapIndex(index - SHIFT);
}

int wrapIndex(int index) {
  return ((index % NUMPIXELS) + NUMPIXELS) % NUMPIXELS;
}
