
int button = 0;

int N = 3;

bool level[N][N];
bool state[N][N];

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
			level[row][col] = (randNumber % 2 == 1);
			randNumber >>= 1;
		}
	}	
}

void setup() {
	for (int row = 0; row < N; row++) {
		for (int col = 0; col < N; col++) {
			level[row][col] = false;
			state[row][col] = false;
		}
	}

	randomSeed(analogRead(5));
}

void loop() {

}