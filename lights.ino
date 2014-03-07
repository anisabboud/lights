
// this project remixs a lot of the makey makey source code
// notably, the functions and data structures required for noise cancelling "low tech" inputs
// the Makey Makey firmware can be found at:
// https://github.com/sparkfun/MaKeyMaKey/blob/master/firmware/Arduino/makey_makey/

#define BUFFER_LENGTH    3     // 3 bytes gives us 24 samples
#define NUM_INPUTS       9    // 6 on the front + 12 on the back
//#define TARGET_LOOP_TIME 694   // (1/60 seconds) / 24 samples = 694 microseconds per sample 
//#define TARGET_LOOP_TIME 758  // (1/55 seconds) / 24 samples = 758 microseconds per sample 
#define TARGET_LOOP_TIME 744  // (1/56 seconds) / 24 samples = 744 microseconds per sample 
///////////////////////////
// NOISE CANCELLATION /////
///////////////////////////
#define SWITCH_THRESHOLD_OFFSET_PERC  5    // number between 1 and 49
                                           // larger value protects better against noise oscillations, but makes it harder to press and release
                                           // recommended values are between 2 and 20
                                           // default value is 5

#define SWITCH_THRESHOLD_CENTER_BIAS 55   // number between 1 and 99
                                          // larger value makes it easier to "release" keys, but harder to "press"
                                          // smaller value makes it easier to "press" keys, but harder to "release"
                                          // recommended values are between 30 and 70
                                          // 50 is "middle" 2.5 volt center
                                          // default value is 55
                                          // 100 = 5V (never use this high)
                                          // 0 = 0 V (never use this low

typedef struct {
  byte pinNumber;
  // int keyCode;   							//only used to bind for keyboard
  byte measurementBuffer[BUFFER_LENGTH]; 
  boolean oldestMeasurement;
  byte bufferSum;
  boolean pressed;
  boolean prevPressed;
  // boolean isMouseMotion;						//only for mouse functionality
  // boolean isMouseButton;						//only for mouse functionality
  // boolean isKey;								//only to use Keyboard.press()
} 
WaterKeyInput;

WaterKeyInput inputs[NUM_INPUTS];

///////////////////////////////////
// VARIABLES //////////////////////
///////////////////////////////////
int bufferIndex = 0;
byte byteCounter = 0;
byte bitCounter = 0;

int pressThreshold;
int releaseThreshold;
boolean inputChanged;

int pinNumbers[NUM_INPUTS] = {
  0, 1, 2, 3, 4, 5,	6, 7, 8							//basically first 9 pins for the 9 lights
};

// timing
int loopTime = 0;
int prevTime = 0;
int loopCounter = 0;


int button = 10;

const int N = 3;

bool level[N][N];
bool state[N][N];
bool solution[N][N];

bool inBoard(int row, int col) {
	return row >= 0 && row < N && col >= 0 && col < N;
}

void flip(int row, int col) {
	if (inBoard(row, col)) {
		state[row][col] = !state[row][col];
	}
}

void click(int row, int col) {
	flip(row, col);
	flip(row - 1, col);
	flip(row + 1, col);
	flip(row, col - 1);
	flip(row, col + 1);
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

void newLevel() {
	int randNumber = random(1 << (N * N));

	for (int row = 0; row < N; row++) {
		for (int col = 0; col < N; col++) {
			state[row][col] = level[row][col] = (randNumber % 2 == 1);
			randNumber >>= 1;
		}
	}	
}

// Magic. Don't touch.
void solveCorner(int row, int col) {
	solution[row][col] = (!state[row][col] + !state[row][2 - col] + !state[2 - row][col] + !state[1][2 - col] + !state[2 - row][1]) % 2 == 1;
}
void solveEdge(int row, int col) {
	solution[row][col] = (!state[1][1] + !state[2 - row][2 - col] + !state[2 - row][col] + !state[1][2 - col]) % 2 == 1;
}
void solveCenter() {
	solution[1][1] = (!state[0][1] + !state[1][0] + !state[1][1] + !state[1][2] + !state[2][1]) % 2 == 1;
}

// Assumes N==3!
void solve() {
	solveCorner(0, 0);
	solveCorner(0, 2);
	solveCorner(2, 2);
	solveCorner(2, 2);

	solveEdge(0, 1);
	solveEdge(1, 0);
	solveEdge(1, 2);
	solveEdge(2, 1);

	solveCenter();
}

void initializeGame();
void initializeInputs();
void intializeHardware();
void updateMeasurementBuffers();
void updateBufferSums();
void updateBufferIndex();
void updateInputStates();
void addDelay();

void setup() {
	intializeHardware();
	initializeInputs();
	initializeGame();
}

void loop() {
	updateMeasurementBuffers();
	updateBufferSums();
	updateBufferIndex();
	updateInputStates();
	addDelay();
}


void initializeGame(){
	for (int row = 0; row < N; row++) {
		for (int col = 0; col < N; col++) {
			level[row][col] = false;
			state[row][col] = false;
		}
	}

	randomSeed(analogRead(5));	
}

void intializeHardware(){
   /* Set up input pins 
   DEactivate the internal pull-ups, since we're using external resistors */
  for (int i=0; i<NUM_INPUTS; i++)
  {
    pinMode(pinNumbers[i], INPUT);
    digitalWrite(pinNumbers[i], LOW);
  }

}

void initializeInputs() {
	//removed keyboard/mouse code from original makey makey
  float thresholdPerc = SWITCH_THRESHOLD_OFFSET_PERC;
  float thresholdCenterBias = SWITCH_THRESHOLD_CENTER_BIAS/50.0;
  float pressThresholdAmount = (BUFFER_LENGTH * 8) * (thresholdPerc / 100.0);
  float thresholdCenter = ( (BUFFER_LENGTH * 8) / 2.0 ) * (thresholdCenterBias);
  pressThreshold = int(thresholdCenter + pressThresholdAmount);
  releaseThreshold = int(thresholdCenter - pressThresholdAmount);

#ifdef DEBUG
  Serial.println(pressThreshold);
  Serial.println(releaseThreshold);
#endif

  for (int i=0; i<NUM_INPUTS; i++) {
    inputs[i].pinNumber = pinNumbers[i];
    // inputs[i].keyCode = keyCodes[i];

    for (int j=0; j<BUFFER_LENGTH; j++) {
      inputs[i].measurementBuffer[j] = 0;
    }
    inputs[i].oldestMeasurement = 0;
    inputs[i].bufferSum = 0;

    inputs[i].pressed = false;
    inputs[i].prevPressed = false;

#ifdef DEBUG
    Serial.println(i);
#endif
  }
}

//////////////////////////////
// UPDATE MEASUREMENT BUFFERS
//////////////////////////////
void updateMeasurementBuffers() {

  for (int i=0; i<NUM_INPUTS; i++) {

    // store the oldest measurement, which is the one at the current index,
    // before we update it to the new one 
    // we use oldest measurement in updateBufferSums
    byte currentByte = inputs[i].measurementBuffer[byteCounter];
    inputs[i].oldestMeasurement = (currentByte >> bitCounter) & 0x01; 

    // make the new measurement
    boolean newMeasurement = digitalRead(inputs[i].pinNumber);

    // invert so that true means the switch is closed
    newMeasurement = !newMeasurement; 

    // store it    
    if (newMeasurement) {
      currentByte |= (1<<bitCounter);
    } 
    else {
      currentByte &= ~(1<<bitCounter);
    }
    inputs[i].measurementBuffer[byteCounter] = currentByte;
  }
}

///////////////////////////
// UPDATE BUFFER SUMS
///////////////////////////
void updateBufferSums() {

  // the bufferSum is a running tally of the entire measurementBuffer
  // add the new measurement and subtract the old one

  for (int i=0; i<NUM_INPUTS; i++) {
    byte currentByte = inputs[i].measurementBuffer[byteCounter];
    boolean currentMeasurement = (currentByte >> bitCounter) & 0x01; 
    if (currentMeasurement) {
      inputs[i].bufferSum++;
    }
    if (inputs[i].oldestMeasurement) {
      inputs[i].bufferSum--;
    }
  }  
}

///////////////////////////
// UPDATE BUFFER INDEX
///////////////////////////
void updateBufferIndex() {
  bitCounter++;
  if (bitCounter == 8) {
    bitCounter = 0;
    byteCounter++;
    if (byteCounter == BUFFER_LENGTH) {
      byteCounter = 0;
    }
  }
}

///////////////////////////
// UPDATE INPUT STATES
///////////////////////////
//again, modified to remove mouse/keyboard code from original makey makey firmware
void updateInputStates() {
  inputChanged = false;
  for (int i=0; i<NUM_INPUTS; i++) {
    inputs[i].prevPressed = inputs[i].pressed; // store previous pressed state (only used for mouse buttons)
    if (inputs[i].pressed) {
      if (inputs[i].bufferSum < releaseThreshold) {  
        inputChanged = true;
        inputs[i].pressed = false;
      }
    } 
    else if (!inputs[i].pressed) {
      if (inputs[i].bufferSum > pressThreshold) {  // input becomes pressed
        inputChanged = true;
        inputs[i].pressed = true; 
        /////////////////////////
        // WE SHOULD CALL OUR GAME'S STATE MACHINE HERE
        /////////////////////////
      }
    }
  }
#ifdef DEBUG3
  if (inputChanged) {
    Serial.println("change");
  }
#endif
}

///////////////////////////
// ADD DELAY
///////////////////////////
void addDelay() {

  loopTime = micros() - prevTime;
  if (loopTime < TARGET_LOOP_TIME) {
    int wait = TARGET_LOOP_TIME - loopTime;
    delayMicroseconds(wait);
  }

  prevTime = micros();

#ifdef DEBUG_TIMING
  if (loopCounter == 0) {
    int t = micros()-prevTime;
    Serial.println(t);
  }
  loopCounter++;
  loopCounter %= 999;
#endif

}
