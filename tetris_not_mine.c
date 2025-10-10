// tetris_termux.c
// Termux / code-server 터미널용 완성형 테트리스 (C)
// 컴파일: gcc tetris_termux.c -o tetris -O2
// 실행: ./tetris

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <termios.h>
#include <sys/select.h>
#include <time.h>
#include <signal.h>

#define WIDTH 10
#define HEIGHT 20

// ANSI 색상 (배경)
#define BG_RESET   "\033[0m"
#define BG_I       "\033[48;5;39m"   // cyan-ish
#define BG_O       "\033[48;5;226m"  // yellow
#define BG_T       "\033[48;5;201m"  // magenta
#define BG_S       "\033[48;5;46m"   // green
#define BG_Z       "\033[48;5;196m"  // red
#define BG_J       "\033[48;5;21m"   // blue
#define BG_L       "\033[48;5;208m"  // orange
#define BG_WALL    "\033[48;5;240m"  // gray for border
#define FG_TEXT    "\033[38;5;15m"

// 블록 타입 인덱스
enum { I_T=0, O_T, T_T, S_T, Z_T, J_T, L_T, TYPE_COUNT };

// 게임 필드: 0 = 빈칸, 1..7 = 블록타입+1
int field[HEIGHT][WIDTH];

// 현재 조각 정보
typedef struct {
    int type;       // 0..6
    int rot;        // 0..3
    int x, y;       // 좌표: (x,y) 기준은 블록의 4x4 좌표 상단 왼쪽
} Piece;

Piece curPiece, nextPiece;
int level = 1;
int score = 0;
int lines_cleared = 0;
int game_over = 0;
int paused = 0;

// 터미널 원상복구
static struct termios orig_termios;
void restore_terminal(void) {
    tcsetattr(STDIN_FILENO, TCSANOW, &orig_termios);
    printf(BG_RESET);
    printf("\033[?25h"); // cursor show
    fflush(stdout);
}
void on_exit_cleanup(void) { restore_terminal(); }

// 터미널 raw 모드로 (비차단 입력에 사용)
void enable_raw_mode(void) {
    tcgetattr(STDIN_FILENO, &orig_termios);
    atexit(on_exit_cleanup);
    struct termios raw = orig_termios;
    raw.c_lflag &= ~(ECHO | ICANON); // no echo, non-canonical
    raw.c_iflag &= ~(IXON | ICRNL);
    raw.c_cc[VMIN] = 0;
    raw.c_cc[VTIME] = 0;
    tcsetattr(STDIN_FILENO, TCSANOW, &raw);
    printf("\033[?25l"); // cursor hide
}

// 키 입력 대기 없이 읽기 (select 사용)
int read_key_nonblock() {
    fd_set set;
    struct timeval tv = {0, 0};
    FD_ZERO(&set);
    FD_SET(STDIN_FILENO, &set);
    if (select(STDIN_FILENO+1, &set, NULL, NULL, &tv) > 0) {
        unsigned char c;
        if (read(STDIN_FILENO, &c, 1) == 1) return c;
    }
    return -1;
}

// 화면 제어
void cls() { printf("\033[H\033[J"); }
void gotoxy(int x, int y) { printf("\033[%d;%dH", y, x); }

// 기본 4x4 형태 (rotation은 함수로 처리)
int shape4[7][4][4] = {
    // I (기본 세로형)
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
    },
    // S
    {
        {0,0,0,0},
        {0,1,1,0},
        {1,1,0,0},
        {0,0,0,0}
    },
    // Z
    {
        {0,0,0,0},
        {1,1,0,0},
        {0,1,1,0},
        {0,0,0,0}
    },
    // J
    {
        {0,0,0,0},
        {1,0,0,0},
        {1,1,1,0},
        {0,0,0,0}
    },
    // L
    {
        {0,0,0,0},
        {0,0,1,0},
        {1,1,1,0},
        {0,0,0,0}
    }
};

// rotation: 0..3, return 1 if block present at (rx,ry) in rotated 4x4
int block_at(int type, int rot, int rx, int ry) {
    // rotation about 4x4 square clockwise rot times
    // mapping: for rot=1 (90deg): new[x][y] = old[3-y][x]
    int x = rx, y = ry;
    int tx, ty;
    int val = 0;
    if (rot == 0) val = shape4[type][ry][rx];
    else if (rot == 1) { tx = 3 - ry; ty = rx; val = shape4[type][ty][tx]; }
    else if (rot == 2) { tx = 3 - rx; ty = 3 - ry; val = shape4[type][ty][tx]; }
    else { tx = ry; ty = 3 - rx; val = shape4[type][ty][tx]; }
    return val;
}

// 충돌 검사: piece를 (px,py,rot)로 놓을 수 있는가?
int collide_piece(int type, int rot, int px, int py) {
    for (int ry = 0; ry < 4; ++ry) {
        for (int rx = 0; rx < 4; ++rx) {
            if (!block_at(type, rot, rx, ry)) continue;
            int fx = px + rx;
            int fy = py + ry;
            if (fx < 0 || fx >= WIDTH) return 1;
            if (fy >= HEIGHT) return 1;
            if (fy >= 0 && field[fy][fx]) return 1;
        }
    }
    return 0;
}

// 현재 조각을 필드에 병합 (고정)
void merge_piece(Piece *p) {
    for (int ry=0; ry<4; ++ry) for (int rx=0; rx<4; ++rx) {
        if (!block_at(p->type, p->rot, rx, ry)) continue;
        int fx = p->x + rx;
        int fy = p->y + ry;
        if (fy >= 0 && fy < HEIGHT && fx >= 0 && fx < WIDTH) {
            field[fy][fx] = p->type + 1; // 저장할 때 1..7
        }
    }
}

// 한 줄 지우기 검사 및 처리
void clear_lines_and_score() {
    int cleared = 0;
    for (int y = HEIGHT-1; y >= 0; --y) {
        int full = 1;
        for (int x = 0; x < WIDTH; ++x) if (!field[y][x]) { full = 0; break; }
        if (full) {
            cleared++;
            // 위로 한 칸씩 내리기
            for (int yy = y; yy > 0; --yy) for (int x=0;x<WIDTH;++x) field[yy][x] = field[yy-1][x];
            for (int x=0;x<WIDTH;++x) field[0][x] = 0;
            ++y; // 같은 행 다시 검사 (since rows moved down)
        }
    }
    if (cleared) {
        lines_cleared += cleared;
        // 일반 테트리스식 점수: 1줄=100, 2줄=300, 3줄=500, 4줄=800 (간단 가중치)
        static int scoreTable[5] = {0,100,300,500,800};
        score += scoreTable[cleared] * level;
        // 레벨업: 예시로 10라인마다 레벨업
        if (lines_cleared >= level * 10) { level++; }
    }
}

// 랜덤 조각 생성
Piece make_random_piece() {
    Piece p;
    p.type = rand() % TYPE_COUNT;
    p.rot = 0;
    p.x = (WIDTH / 2) - 2; // 중앙에 배치
    p.y = -1; // spawn slightly above board so O/I can appear well
    return p;
}

// 색상 매핑
const char* color_for_type(int t) {
    switch(t) {
        case I_T: return BG_I;
        case O_T: return BG_O;
        case T_T: return BG_T;
        case S_T: return BG_S;
        case Z_T: return BG_Z;
        case J_T: return BG_J;
        case L_T: return BG_L;
        default: return BG_RESET;
    }
}

// 필드와 HUD 그리기
void draw_all(Piece *p, Piece *nextP) {
    // move cursor to top-left
    gotoxy(1,1);
    // 좌측: 보드 (테두리 포함)
    printf(FG_TEXT);
    for (int y = -1; y <= HEIGHT; ++y) {
        for (int x = -1; x <= WIDTH; ++x) {
            if (y == -1 || y == HEIGHT || x == -1 || x == WIDTH) {
                // border
                printf(BG_WALL "  " BG_RESET);
            } else {
                // check current falling piece occupies this cell?
                int occupied = 0;
                if (p) {
                    for (int ry=0; ry<4 && !occupied; ++ry) for (int rx=0; rx<4; ++rx) {
                        if (!block_at(p->type, p->rot, rx, ry)) continue;
                        int fx = p->x + rx;
                        int fy = p->y + ry;
                        if (fx == x && fy == y) { occupied = 1; break; }
                    }
                }
                if (occupied && p->y <= y) {
                    printf("%s  " BG_RESET, color_for_type(p->type));
                } else {
                    int v = field[y][x];
                    if (v) {
                        printf("%s  " BG_RESET, color_for_type(v-1));
                    } else {
                        printf("  ");
                    }
                }
            }
        }
        // 오른쪽에 HUD (한 줄마다)
        if (y == 0) printf("   %sTETRIS (Termux)%s\n", FG_TEXT, BG_RESET);
        else if (y == 1) printf("   SCORE: %d\n", score);
        else if (y == 2) printf("   LEVEL: %d\n", level);
        else if (y == 3) printf("   LINES: %d\n", lines_cleared);
        else if (y == 5) printf("   NEXT:\n");
        else if (y >= 6 && y <= 9) {
            // show next piece in 4x4 block
            int ry = y - 6;
            printf("   ");
            for (int rx = 0; rx < 4; ++rx) {
                if (block_at(nextP->type, nextP->rot, rx, ry)) {
                    printf("%s  " BG_RESET, color_for_type(nextP->type));
                } else printf("  ");
            }
            printf("\n");
        }
        else if (y == 11) printf("   Controls:\n");
        else if (y == 12) printf("   a:left  d:right  s:down  w:rotate\n");
        else if (y == 13) printf("   space:hard drop  p:pause  q:quit\n");
        else printf("\n");
    }
    fflush(stdout);
}

// 하드 드롭 (즉시 내려서 고정)
void hard_drop(Piece *p) {
    while (!collide_piece(p->type, p->rot, p->x, p->y + 1)) p->y++;
    merge_piece(p);
    clear_lines_and_score();
}

// 시그널 (예: Ctrl+C) 처리: 터미널 복구 후 종료
void sigint_handler(int signo) {
    restore_terminal();
    printf("\nInterrupted. Exiting.\n");
    exit(0);
}

int main() {
    srand(time(NULL));
    signal(SIGINT, sigint_handler);

    // 초기화
    memset(field, 0, sizeof(field));
    enable_raw_mode();
    cls();

    nextPiece = make_random_piece();
    curPiece = make_random_piece();
    // if spawn collides immediately -> game over
    if (collide_piece(curPiece.type, curPiece.rot, curPiece.x, curPiece.y)) {
        restore_terminal();
        printf("Cannot spawn. Terminal too small or board blocked.\n");
        return 0;
    }

    // 타이머 설정: 기본 delay(마이크로초), level이 올라갈수록 빨라짐
    int base_delay_ms = 500; // 기본 500ms
    unsigned long last_tick = 0;
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    last_tick = ts.tv_sec * 1000 + ts.tv_nsec / 1000000;

    while (!game_over) {
        // draw
        draw_all(&curPiece, &nextPiece);

        // input 처리 (비차단)
        int k = read_key_nonblock();
        if (k != -1) {
            if (k == 'a') {
                if (!collide_piece(curPiece.type, curPiece.rot, curPiece.x - 1, curPiece.y)) curPiece.x--;
            } else if (k == 'd') {
                if (!collide_piece(curPiece.type, curPiece.rot, curPiece.x + 1, curPiece.y)) curPiece.x++;
            } else if (k == 's') {
                if (!collide_piece(curPiece.type, curPiece.rot, curPiece.x, curPiece.y + 1)) curPiece.y++;
            } else if (k == 'w') {
                int nr = (curPiece.rot + 1) % 4;
                if (!collide_piece(curPiece.type, nr, curPiece.x, curPiece.y)) curPiece.rot = nr;
                else {
                    // simple wall-kick attempt: try shift left/right
                    if (!collide_piece(curPiece.type, nr, curPiece.x - 1, curPiece.y)) { curPiece.x--; curPiece.rot = nr; }
                    else if (!collide_piece(curPiece.type, nr, curPiece.x + 1, curPiece.y)) { curPiece.x++; curPiece.rot = nr; }
                }
            } else if (k == ' ') {
                hard_drop(&curPiece);
                // spawn
                curPiece = nextPiece;
                nextPiece = make_random_piece();
                if (collide_piece(curPiece.type, curPiece.rot, curPiece.x, curPiece.y)) game_over = 1;
            } else if (k == 'p') {
                paused = !paused;
                if (paused) {
                    gotoxy(1, HEIGHT/2);
                    printf("==== PAUSED: Press 'p' to resume ====\n");
                } else {
                    // redraw immediately
                    cls();
                }
            } else if (k == 'q') {
                game_over = 1;
            }
        }

        // tick: gravity
        clock_gettime(CLOCK_MONOTONIC, &ts);
        unsigned long now_ms = ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
        unsigned long delay_ms = base_delay_ms;
        // speed up by level (simple): each level reduces delay
        if (level > 1) {
            delay_ms = base_delay_ms * (100 - (level-1)*7) / 100; // decrease 7% per level approx
            if (delay_ms < 50) delay_ms = 50;
        }

        if (!paused && now_ms - last_tick >= delay_ms) {
            last_tick = now_ms;
            // try move down
            if (!collide_piece(curPiece.type, curPiece.rot, curPiece.x, curPiece.y + 1)) {
                curPiece.y++;
            } else {
                // lock piece
                merge_piece(&curPiece);
                clear_lines_and_score();
                // spawn next
                curPiece = nextPiece;
                nextPiece = make_random_piece();
                if (collide_piece(curPiece.type, curPiece.rot, curPiece.x, curPiece.y)) {
                    game_over = 1;
                }
            }
        }

        // 소소한 대기 (너무 높은 CPU 사용을 막기 위해)
        usleep(8000); // 8ms
    }

    // 종료 루틴
    cls();
    restore_terminal();
    printf("===== GAME OVER =====\n");
    printf("Score: %d\n", score);
    printf("Lines: %d\n", lines_cleared);
    printf("Level: %d\n", level);
    printf("Thanks for playing!\n");
    return 0;
}
