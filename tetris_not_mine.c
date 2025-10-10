#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <termios.h>
#include <sys/select.h>
#include <time.h>

#define WIDTH 10
#define HEIGHT 20

int field[HEIGHT][WIDTH] = {0};
int score = 0;
int gameOver = 0;

int tetromino[3][4][4] = {
    // I
    {
        {0,1,0,0},
        {0,1,0,0},
        {0,1,0,0},
        {0,1,0,0}
    },
    // O
    {
        {0,0,0,0},
        {0,1,1,0},
        {0,1,1,0},
        {0,0,0,0}
    },
    // T
    {
        {0,0,0,0},
        {1,1,1,0},
        {0,1,0,0},
        {0,0,0,0}
    }
};

// --- 입력 비차단 모드 제어 ---
void setBufferedInput(int enable) {
    static struct termios oldt, newt;
    if (!enable) {
        tcgetattr(STDIN_FILENO, &oldt);
        newt = oldt;
        newt.c_lflag &= ~(ICANON | ECHO);
        tcsetattr(STDIN_FILENO, TCSANOW, &newt);
    } else {
        tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    }
}

// --- 키 입력 체크 ---
int kbhit(void) {
    struct timeval tv = {0L, 0L};
    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(STDIN_FILENO, &fds);
    return select(STDIN_FILENO+1, &fds, NULL, NULL, &tv);
}

// --- 커서 이동 ---
void gotoxy(int x, int y) {
    printf("\033[%d;%dH", y, x);
}

// --- 화면 클리어 ---
void clearScreen() {
    printf("\033[2J");
    fflush(stdout);
}

// --- 필드 그리기 ---
void drawField() {
    gotoxy(0, 0);
    for (int y = 0; y < HEIGHT; y++) {
        for (int x = 0; x < WIDTH; x++) {
            printf(field[y][x] ? "[]" : " .");
        }
        printf("\n");
    }
    printf("\n점수: %d\n", score);
    fflush(stdout);
}

// --- 충돌 검사 ---
int checkCollision(int shape[4][4], int posX, int posY) {
    for (int y = 0; y < 4; y++) {
        for (int x = 0; x < 4; x++) {
            if (shape[y][x]) {
                int fx = posX + x;
                int fy = posY + y;
                if (fx < 0 || fx >= WIDTH || fy >= HEIGHT)
                    return 1;
                if (fy >= 0 && field[fy][fx])
                    return 1;
            }
        }
    }
    return 0;
}

// --- 블록 고정 ---
void mergeBlock(int shape[4][4], int posX, int posY) {
    for (int y = 0; y < 4; y++)
        for (int x = 0; x < 4; x++)
            if (shape[y][x] && posY + y >= 0)
                field[posY + y][posX + x] = 1;
}

// --- 줄 삭제 ---
void clearLines() {
    for (int y = HEIGHT - 1; y >= 0; y--) {
        int full = 1;
        for (int x = 0; x < WIDTH; x++) {
            if (!field[y][x]) {
                full = 0;
                break;
            }
        }
        if (full) {
            score += 100;
            for (int yy = y; yy > 0; yy--)
                for (int x = 0; x < WIDTH; x++)
                    field[yy][x] = field[yy - 1][x];
            for (int x = 0; x < WIDTH; x++)
                field[0][x] = 0;
            y++; // 같은 줄 다시 검사
        }
    }
}

int main() {
    srand(time(NULL));
    clearScreen();
    setBufferedInput(0); // 입력 버퍼 끄기

    int shape[4][4];
    int curX = WIDTH / 2 - 2;
    int curY = 0;
    int type = rand() % 3;

    for (int y = 0; y < 4; y++)
        for (int x = 0; x < 4; x++)
            shape[y][x] = tetromino[type][y][x];

    while (!gameOver) {
        drawField();

        // 키 입력
        if (kbhit()) {
            char key = getchar();
            if (key == 'a' && !checkCollision(shape, curX - 1, curY))
                curX--;
            else if (key == 'd' && !checkCollision(shape, curX + 1, curY))
                curX++;
            else if (key == 's' && !checkCollision(shape, curX, curY + 1))
                curY++;
            else if (key == 'q') break;
        }

        // 아래로 이동 or 고정
        if (!checkCollision(shape, curX, curY + 1)) {
            curY++;
        } else {
            mergeBlock(shape, curX, curY);
            clearLines();

            curX = WIDTH / 2 - 2;
            curY = 0;
            type = rand() % 3;
            for (int y = 0; y < 4; y++)
                for (int x = 0; x < 4; x++)
                    shape[y][x] = tetromino[type][y][x];

            if (checkCollision(shape, curX, curY))
                gameOver = 1;
        }

        usleep(300000);
    }

    setBufferedInput(1);
    gotoxy(0, HEIGHT + 3);
    printf("\n💀 게임 오버! 점수: %d 💀\n", score);
    return 0;
}
