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

int tetromino[7][4][4][4] = {
    // I
    {
        {
            {0,0,0,0},
            {1,1,1,1},
            {0,0,0,0},
            {0,0,0,0}
        },
        {
            {0,0,1,0},
            {0,0,1,0},
            {0,0,1,0},
            {0,0,1,0}
        }
    },
    // O
    {
        {
            {0,0,0,0},
            {0,1,1,0},
            {0,1,1,0},
            {0,0,0,0}
        }
    },
    // T
    {
        {
            {0,0,0,0},
            {1,1,1,0},
            {0,1,0,0},
            {0,0,0,0}
        },
        {
            {0,1,0,0},
            {1,1,0,0},
            {0,1,0,0},
            {0,0,0,0}
        },
        {
            {0,1,0,0},
            {1,1,1,0},
            {0,0,0,0},
            {0,0,0,0}
        },
        {
            {0,1,0,0},
            {0,1,1,0},
            {0,1,0,0},
            {0,0,0,0}
        }
    }
};

// 🎯 커서 이동 함수 (ANSI escape)
void gotoxy(int x, int y) {
    printf("\033[%d;%dH", y, x);
    fflush(stdout);
}

// 🎯 kbhit() 대체
int kbhit(void) {
    struct termios oldt, newt;
    int ch;
    int oldf;
    int bytesWaiting;

    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);

    struct timeval tv = {0L, 0L};
    fd_set readfds;
    FD_ZERO(&readfds);
    FD_SET(STDIN_FILENO, &readfds);
    select(STDIN_FILENO + 1, &readfds, NULL, NULL, &tv);

    int hit = FD_ISSET(STDIN_FILENO, &readfds);
    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    return hit;
}

// 🎯 getch() 대체
int getch(void) {
    struct termios oldt, newt;
    int ch;
    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);
    ch = getchar();
    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    return ch;
}

// 🎯 필드 출력
void drawField() {
    gotoxy(0, 0);
    for (int y = 0; y < HEIGHT; y++) {
        for (int x = 0; x < WIDTH; x++) {
            if (field[y][x])
                printf("■");
            else
                printf(" .");
        }
        printf("\n");
    }
    printf("\n점수: %d\n", score);
}

// 🎯 블록 충돌 검사
int checkCollision(int shape[4][4], int posX, int posY) {
    for (int y = 0; y < 4; y++)
        for (int x = 0; x < 4; x++)
            if (shape[y][x]) {
                int fx = posX + x;
                int fy = posY + y;
                if (fx < 0 || fx >= WIDTH || fy >= HEIGHT)
                    return 1;
                if (fy >= 0 && field[fy][fx])
                    return 1;
            }
    return 0;
}

// 🎯 블록을 필드에 고정
void mergeBlock(int shape[4][4], int posX, int posY) {
    for (int y = 0; y < 4; y++)
        for (int x = 0; x < 4; x++)
            if (shape[y][x] && posY + y >= 0)
                field[posY + y][posX + x] = 1;
}

// 🎯 줄 삭제
void clearLines() {
    for (int y = 0; y < HEIGHT; y++) {
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
        }
    }
}

int main() {
    srand(time(NULL));

    int shape[4][4];
    int curX = WIDTH / 2 - 2, curY = 0;
    int type = rand() % 3; // 단순히 3종류만 사용 (I, O, T)
    int rotation = 0;

    // 현재 블록 복사
    for (int y = 0; y < 4; y++)
        for (int x = 0; x < 4; x++)
            shape[y][x] = tetromino[type][rotation][y][x];

    system("clear");

    while (!gameOver) {
        drawField();

        // 입력 처리
        if (kbhit()) {
            char key = getch();
            if (key == 'a' && !checkCollision(shape, curX - 1, curY))
                curX--;
            else if (key == 'd' && !checkCollision(shape, curX + 1, curY))
                curX++;
            else if (key == 's' && !checkCollision(shape, curX, curY + 1))
                curY++;
            else if (key == 'q') break;
        }

        // 블록 아래로 이동
        if (!checkCollision(shape, curX, curY + 1))
            curY++;
        else {
            mergeBlock(shape, curX, curY);
            clearLines();

            curX = WIDTH / 2 - 2;
            curY = 0;
            type = rand() % 3;
            rotation = 0;

            for (int y = 0; y < 4; y++)
                for (int x = 0; x < 4; x++)
                    shape[y][x] = tetromino[type][rotation][y][x];

            if (checkCollision(shape, curX, curY))
                gameOver = 1;
        }

        usleep(300000); // 0.3초
    }

    printf("\n\n💀 게임 오버! 최종 점수: %d 💀\n", score);
    return 0;
}
