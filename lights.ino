
#include "MovingAverageFilter.h"

#define N 3
#define NUM_INPUTS       9    // 6 on the front + 12 on the back
#define MIN_THRESHOLD   333
#define MAX_THRESHOLD   666
#define COIN_THRESHOLD  600

#define HINT_TIMEOUT 4000
#define BLINK_TIMEOUT 500
#define BLINK_TIMES 3

float inputs[NUM_INPUTS];
bool pressed[NUM_INPUTS];
bool newLevelPressed = false;  // We don't need this because we have a delay.

MovingAverageFilter movingAverageFilters[NUM_INPUTS];

// A0 – D18
// A1 – D19
// A2 – D20
// A3 – D21
// A4 – D22
// A5 – D23
// A6 – D4
// A7 – D6
// A8 – D8
// A9 – D9
// A10 – D10
// A11 – D12
const int inPinNumbers[NUM_INPUTS] = {
  A0, A3, A6, A1, A4, A7, A2, A5, A8              // basically first 9 pins for the 9 lights
};
const int outPinNumbers[NUM_INPUTS] = {
  // 0, 1, 2, 3, 5, 7, 9, 10, 11             // basically first unused 9 pins for the 9 lights
  0, 3, 9, 1, 5, 10, 2, 7, 11
};

const int newLevelPin = 13;  // Digital.
const int coinPin = A11;  // Analog 11 is actually Digital 12.

bool level[N][N];
bool state[N][N];
bool solution[N][N];

bool inBoard(int row, int col) {
  return row >= 0 && row < N && col >= 0 && col < N;
}

bool complete() {
  for (int row = 0; row < N; row++) {
    for (int col = 0; col < N; col++) {
      if (!state[row][col]) {
        return false;
      }
    }
  }
  return true;
}

void turnOffAllLights() {
  for (int i = 0; i < NUM_INPUTS; i++) {
    digitalWrite(outPinNumbers[i], LOW);
  }
}

void turnLights(bool matrix[N][N]) {
  for (int row = 0; row < N; row++) {
    for (int col = 0; col < N; col++) {
      int lightIndex = row * N + col;
      digitalWrite(outPinNumbers[lightIndex], (matrix[row][col] ? HIGH : LOW));
    }
  }
}

void flip(int row, int col) {
  if (inBoard(row, col)) {
    state[row][col] = !state[row][col];
  }
}

void logMatrix(bool matrix[N][N], String header) {
  Serial.println(header);
  for (int row = 0; row < N; row++) {
    for (int col = 0; col < N; col++) {
      Serial.print(matrix[row][col] ? "1" : "0");
    }
    Serial.println();
  }
  Serial.println();
}

void click(int row, int col) {
  Serial.println("You clicked " + String(row) + ", " + String(col));
  //logMatrix(level, "Level Before Click:");
  //logMatrix(state, "State Before Click:");

  flip(row, col);
  flip(row - 1, col);
  flip(row + 1, col);
  flip(row, col - 1);
  flip(row, col + 1);

  turnLights(state);

  if (complete()) {  // Blink all lights.
    for (int i = 0; i < BLINK_TIMES; i++) {
      delay(BLINK_TIMEOUT);
      turnOffAllLights();
      delay(BLINK_TIMEOUT);
      turnLights(state);
    }
  }
  
  //logMatrix(state, "State After Click:");
}

void newLevel() {  
  int randNumber = random(1 << (N * N));

  for (int row = 0; row < N; row++) {
    for (int col = 0; col < N; col++) {
      state[row][col] = level[row][col] = (randNumber % 2 == 1);
      randNumber >>= 1;
    }
  }

  turnLights(state);
  logMatrix(level, "New Level!");
}

void solveCorner(int row, int col) {
  solution[row][col] = (!state[row][col] + !state[row][2 - col] + !state[2 - row][col] + !state[1][2 - col] + !state[2 - row][1]) % 2 == 1;
}

// Magic. Don't touch.
// Assumes N==3!
void solve() {
  solveCorner(0, 0);
  solveCorner(0, 2);
  solveCorner(2, 2);
  solveCorner(2, 2);

  solution[0][1] = (!state[1][1] + !state[2][0] + !state[2][1] + !state[2][2]) % 2 == 1;
  solution[1][0] = (!state[1][1] + !state[0][2] + !state[2][2] + !state[2][2]) % 2 == 1;
  solution[1][2] = (!state[1][1] + !state[0][0] + !state[1][0] + !state[2][0]) % 2 == 1;
  solution[2][1] = (!state[1][1] + !state[0][0] + !state[0][1] + !state[0][2]) % 2 == 1;

  solution[1][1] = (!state[0][1] + !state[1][0] + !state[1][1] + !state[1][2] + !state[2][1]) % 2 == 1;
}

void showHint() {
  solve();  // Update solution[][] matrix.
  turnLights(solution);  // Show the solution.
  delay(HINT_TIMEOUT);  // Wait a few seconds.
  turnLights(state);  // Hide the solution and show the state back.
}

void intializeHardware() {
  /* Set up input pins 
   DEactivate the internal pull-ups, since we're using external resistors */
  for (int i = 0; i < NUM_INPUTS; i++)
  {
    pinMode(inPinNumbers[i], INPUT);
    pinMode(outPinNumbers[i], OUTPUT);
    pressed[i] = false;
  }
  // pinMode(testLed, OUTPUT);
  // digitalWrite(testLed, LOW);
  pinMode(newLevelPin, INPUT);
  pinMode(coinPin, INPUT);
}

void initializeGame() {
  for (int row = 0; row < N; row++) {
    for (int col = 0; col < N; col++) {
      level[row][col] = false;
      state[row][col] = false;
    }
  }

  randomSeed(analogRead(coinPin));  
}

void setup() {
  intializeHardware();
  initializeGame();
  turnOffAllLights();
}

int count = 0;
void loop() {
  count++;
//  if (count % 1000 == 0) {
//    Serial.print(String(analogRead(A6)) + " ");
//  }
//  if (count % 10000 == 0) {
//    Serial.println();
//  }
//  return;
  
  
  // Hint.
//  if (analogRead(coinPin) > COIN_THRESHOLD) {
//    showHint();
//    return;
//  }

  // New Level.
  if (!newLevelPressed && digitalRead(newLevelPin)) {
    newLevelPressed = true;
    newLevel();
    return;
  }
  newLevelPressed = (digitalRead(newLevelPin) == HIGH);

  // Gameplay.
  for (int i = 0; i < NUM_INPUTS; i++) {
  	// Filter input for noise reduction.
    inputs[i] = movingAverageFilters[i].process(analogRead(inPinNumbers[i]));
    if (count % 50 == 0) {
      Serial.print(inputs[i]);
      Serial.print("  ");
    }
      
    if (inputs[i] < MIN_THRESHOLD) {  // Pressed a button. Call click().
      if (!pressed[i]) {
        pressed[i] = true;
        // advance game state
        click(i / N, i % N);
      }
    } else if (inputs[i] > MAX_THRESHOLD) {  // Released the button. We currently don't do anything in this case.
      if (pressed[i]) {
        pressed[i] = false;
      }
    }
  }
  if(count %50 == 0) {
    Serial.println();
  }
}

