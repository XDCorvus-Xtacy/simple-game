#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <termios.h>
#include <sys/ioctl.h>
#include <time.h>

// 색상 코드
#define RESET "\033[0m"
#define CYAN "\033[36m"
#define GREEN "\033[32m"
#define RED "\033[31m"
#define BLUE "\033[34m"
#define YELLOW "\033[33m"
#define MAGENTA "\033[35m"

// 보드 크기
#define BOARD_WIDTH 15
#define BOARD_HEIGHT 25

// 블록 형태 (간단화)
int block[4][4] = {
    {0, 1, 0, 0},
    {0, 1, 0, 0},
    {0, 1, 1, 0},
    {0, 0, 0, 0}
};

char board[BOARD_HEIGHT][BOARD_WIDTH];

// 터미널 입력 설정
struct termios orig_term;

void disableRawMode() {
    tcsetattr(STDIN_FILENO, TCSANOW, &orig_term);
}

void enableRawMode() {
    tcgetattr(STDIN_FILENO, &orig_term);
    atexit(disableRawMode);
    struct termios raw = orig_term;
    raw.c_lflag &= ~(ECHO | ICANON);
    tcsetattr(STDIN_FILENO, TCSANOW, &raw);
}

// 터미널 크기 구하기
void getTerminalSize(int *rows, int *cols) {
    struct winsize w;
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
    *rows = w.ws_row > 0 ? w.ws_row : 24;
    *cols = w.ws_col > 0 ? w.ws_col : 80;
}

// 화면 초기화
void clearScreen() {
    printf("\033[H\033[J");
}

// 보드 출력
void drawBoard(int bx, int by) {
    int termRows, termCols;
    getTerminalSize(&termRows, &termCols);

    int leftMargin = (termCols - (BOARD_WIDTH * 2)) / 2;
    int topMargin = (termRows - BOARD_HEIGHT) / 2;

    clearScreen();

    for (int i = 0; i < topMargin; i++) printf("\n");

    for (int y = 0; y < BOARD_HEIGHT; y++) {
        for (int i = 0; i < leftMargin; i++) printf(" ");

        for (int x = 0; x < BOARD_WIDTH; x++) {
            if (y >= by && y < by + 4 && x >= bx && x < bx + 4 &&
                block[y - by][x - bx]) {
                printf(CYAN "■ " RESET);
            } else if (board[y][x]) {
                printf(GREEN "■ " RESET);
            } else {
                printf("  ");
            }
        }
        printf("\n");
    }
}

// 블록 충돌 검사
int checkCollision(int bx, int by) {
    for (int y = 0; y < 4; y++)
        for (int x = 0; x < 4; x++)
            if (block[y][x]) {
                int ny = by + y, nx = bx + x;
                if (ny >= BOARD_HEIGHT || nx < 0 || nx >= BOARD_WIDTH || board[ny][nx])
                    return 1;
            }
    return 0;
}

// 블록 고정
void lockBlock(int bx, int by) {
    for (int y = 0; y < 4; y++)
        for (int x = 0; x < 4; x++)
            if (block[y][x])
                board[by + y][bx + x] = 1;
}

// 블록 회전
void rotateBlock() {
    int temp[4][4];
    for (int y = 0; y < 4; y++)
        for (int x = 0; x < 4; x++)
            temp[x][3 - y] = block[y][x];
    for (int y = 0; y < 4; y++)
        for (int x = 0; x < 4; x++)
            block[y][x] = temp[y][x];
}

// 라인 클리어
void clearLines() {
    for (int y = BOARD_HEIGHT - 1; y >= 0; y--) {
        int full = 1;
        for (int x = 0; x < BOARD_WIDTH; x++)
            if (!board[y][x]) full = 0;
        if (full) {
            for (int ty = y; ty > 0; ty--)
                for (int tx = 0; tx < BOARD_WIDTH; tx++)
                    board[ty][tx] = board[ty - 1][tx];
            for (int tx = 0; tx < BOARD_WIDTH; tx++)
                board[0][tx] = 0;
            y++;
        }
    }
}

// 키 입력 읽기
int readKey() {
    char seq[3];
    if (read(STDIN_FILENO, &seq[0], 1) == 0) return 0;
    if (seq[0] == '\033') {
        read(STDIN_FILENO, &seq[1], 1);
        read(STDIN_FILENO, &seq[2], 1);
        if (seq[1] == '[') {
            switch (seq[2]) {
                case 'A': return 'U'; // ↑
                case 'B': return 'D'; // ↓
                case 'C': return 'R'; // →
                case 'D': return 'L'; // ←
            }
        }
        return 0;
    } else {
        return seq[0];
    }
}

int main() {
    enableRawMode();
    srand(time(NULL));

    int bx = BOARD_WIDTH / 2 - 2, by = 0;
    int dropDelay = 300000; // 자동 낙하 속도 (마이크로초)
    clock_t lastDrop = clock();

    while (1) {
        drawBoard(bx, by);

        if (((clock() - lastDrop) * 1000000 / CLOCKS_PER_SEC) > dropDelay) {
            if (!checkCollision(bx, by + 1))
                by++;
            else {
                lockBlock(bx, by);
                clearLines();
                bx = BOARD_WIDTH / 2 - 2;
                by = 0;
                if (checkCollision(bx, by)) break;
            }
            lastDrop = clock();
        }

        usleep(10000);
        if (read(STDIN_FILENO, NULL, 0) == -1) continue;

        int key = readKey();
        if (key == 'q' || key == 'Q') break;
        else if (key == 'L' && !checkCollision(bx - 1, by)) bx--;
        else if (key == 'R' && !checkCollision(bx + 1, by)) bx++;
        else if (key == 'D' && !checkCollision(bx, by + 1)) by++; // 소프트 드롭
        else if (key == ' ') { // 하드 드롭
            while (!checkCollision(bx, by + 1)) by++;
        }
        else if (key == 's' || key == 'S') { // 회전
            rotateBlock();
            if (checkCollision(bx, by)) rotateBlock(), rotateBlock(), rotateBlock();
        }
    }

    clearScreen();
    printf(RED "\n\n   GAME OVER!\n\n" RESET);
    disableRawMode();
    return 0;
}
