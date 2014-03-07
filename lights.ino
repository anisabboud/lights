
int button = 0;

int N = 3;

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