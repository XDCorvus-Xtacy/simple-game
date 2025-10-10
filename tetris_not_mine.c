// tetris_termux.c
// Termux / Linux terminal optimized full Tetris (15x25)
// Controls: Left/Right arrows, Down arrow = soft drop, 's' = rotate, Space = hard drop, p = pause, q = quit
// Compile: gcc -O2 -o tetris_termux tetris_termux.c
// Run: ./tetris_termux

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <termios.h>
#include <sys/select.h>
#include <time.h>
#include <signal.h>
#include <sys/ioctl.h>

#define BOARD_WIDTH 15
#define BOARD_HEIGHT 25
#define TYPE_COUNT 7

// ANSI color backgrounds (each cell prints two spaces with bg color)
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

// Field: 0 = empty, 1..7 = block type index +1
static int field[BOARD_HEIGHT][BOARD_WIDTH];

// piece structure
typedef struct { int type, rot, x, y; } Piece;

static Piece curPiece, nextPiece;
static int score = 0;
static int lines_cleared = 0;
static int level = 1;
static int game_over = 0;
static int paused = 0;

// terminal original state
static struct termios orig_termios;

// restore terminal on exit
void restore_terminal(void) {
    tcsetattr(STDIN_FILENO, TCSANOW, &orig_termios);
    printf(BG_RESET);
    printf("\033[?25h"); // show cursor
    fflush(stdout);
}

void on_exit_cleanup(void) {
    restore_terminal();
}

// enable raw input (non-canonical, no-echo)
void enable_raw_mode(void) {
    tcgetattr(STDIN_FILENO, &orig_termios);
    atexit(on_exit_cleanup);

    struct termios raw = orig_termios;
    raw.c_lflag &= ~(ECHO | ICANON); // no echo, non-canonical
    raw.c_iflag &= ~(IXON | ICRNL);
    raw.c_oflag &= ~(OPOST);
    raw.c_cc[VMIN] = 0;
    raw.c_cc[VTIME] = 0;
    tcsetattr(STDIN_FILENO, TCSANOW, &raw);
    printf("\033[?25l"); // hide cursor
}

// read key if available (non-blocking). returns -1 if none.
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

// get terminal size (columns)
int get_terminal_cols() {
    struct winsize ws;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1) return 80;
    return ws.ws_col;
}

// clear screen and move to home
void cls() { printf("\033[H\033[J"); }

// move cursor to top-left (1,1)
void gotoxy1() { printf("\033[H"); }

// basic 4x4 templates for each piece in its spawn orientation
int shape4[7][4][4] = {
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

// returns 1 if block exists at rotated (rx,ry) for type, rot
int block_at(int type, int rot, int rx, int ry) {
    int val = 0;
    if (rot == 0) val = shape4[type][ry][rx];
    else if (rot == 1) { int tx = 3 - ry, ty = rx; val = shape4[type][ty][tx]; }
    else if (rot == 2) { int tx = 3 - rx, ty = 3 - ry; val = shape4[type][ty][tx]; }
    else { int tx = ry, ty = 3 - rx; val = shape4[type][ty][tx]; }
    return val;
}

// collision check: returns 1 if collides / invalid
int collide_piece(int type, int rot, int px, int py) {
    for (int ry=0; ry<4; ry++) for (int rx=0; rx<4; rx++) {
        if (!block_at(type, rot, rx, ry)) continue;
        int fx = px + rx, fy = py + ry;
        if (fx < 0 || fx >= BOARD_WIDTH) return 1;
        if (fy >= BOARD_HEIGHT) return 1;
        if (fy >= 0 && field[fy][fx]) return 1;
    }
    return 0;
}

// merge piece into field (fix)
void merge_piece(Piece *p) {
    for (int ry=0; ry<4; ry++) for (int rx=0; rx<4; rx++) {
        if (!block_at(p->type, p->rot, rx, ry)) continue;
        int fx = p->x + rx, fy = p->y + ry;
        if (fy >= 0 && fy < BOARD_HEIGHT && fx >= 0 && fx < BOARD_WIDTH) {
            field[fy][fx] = p->type + 1; // store 1..7
        }
    }
}

// clear full lines, update score & lines_cleared & level
void clear_lines_and_score() {
    int cleared = 0;
    for (int y = BOARD_HEIGHT-1; y >= 0; --y) {
        int full = 1;
        for (int x=0; x<BOARD_WIDTH; ++x) if (!field[y][x]) { full = 0; break; }
        if (full) {
            cleared++;
            for (int yy=y; yy>0; --yy) for (int x=0; x<BOARD_WIDTH; ++x) field[yy][x] = field[yy-1][x];
            for (int x=0; x<BOARD_WIDTH; ++x) field[0][x] = 0;
            ++y; // re-check this row after shift
        }
    }
    if (cleared) {
        lines_cleared += cleared;
        static int scoreTable[5] = {0,100,300,500,800};
        score += scoreTable[cleared] * level;
        // level up each 10 lines
        if (lines_cleared >= level * 10) level++;
    }
}

// create random piece
Piece make_random_piece() {
    Piece p;
    p.type = rand() % TYPE_COUNT;
    p.rot = 0;
    p.x = (BOARD_WIDTH / 2) - 2;
    p.y = -1; // spawn slightly above
    return p;
}

// color mapping
const char* color_for_type(int t) {
    switch(t) {
        case 0: return BG_I;
        case 1: return BG_O;
        case 2: return BG_T;
        case 3: return BG_S;
        case 4: return BG_Z;
        case 5: return BG_J;
        case 6: return BG_L;
        default: return BG_RESET;
    }
}

// draw board + HUD centered (computes terminal width)
void draw_all(Piece *p, Piece *nextP) {
    gotoxy1();
    int cols = get_terminal_cols();
    int needed = (BOARD_WIDTH + 2) * 2; // border + cells (each cell=2chars)
    int left_margin = (cols - needed) / 2;
    if (left_margin < 1) left_margin = 1;

    int hud_start = left_margin + needed + 2;

    for (int y = -1; y <= BOARD_HEIGHT; ++y) {
        // left padding
        for (int i=0;i<left_margin;i++) putchar(' ');

        // board row
        for (int x = -1; x <= BOARD_WIDTH; ++x) {
            if (y == -1 || y == BOARD_HEIGHT || x == -1 || x == BOARD_WIDTH) {
                // border
                printf("%s  %s", BG_WALL, BG_RESET);
            } else {
                // check if current piece occupies
                int occ = 0;
                if (p) {
                    for (int ry=0; ry<4 && !occ; ++ry) for (int rx=0; rx<4; ++rx) {
                        if (!block_at(p->type, p->rot, rx, ry)) continue;
                        int fx = p->x + rx, fy = p->y + ry;
                        if (fx == x && fy == y) { occ = 1; break; }
                    }
                }
                if (occ && p->y <= y) {
                    printf("%s  %s", color_for_type(p->type), BG_RESET);
                } else {
                    int v = field[y][x];
                    if (v) printf("%s  %s", color_for_type(v-1), BG_RESET);
                    else printf("  ");
                }
            }
        }

        // HUD (a few lines to the right)
        int afterBoardSpaces = (hud_start - (left_margin + needed));
        for (int s=0;s<afterBoardSpaces;s++) putchar(' ');
        // print HUD based on row index
        if (y == -1) printf("   %sTETRIS (Termux)%s", FG_TEXT, BG_RESET);
        else if (y == 0)  printf("   SCORE: %d", score);
        else if (y == 1)  printf("   LEVEL: %d", level);
        else if (y == 2)  printf("   LINES: %d", lines_cleared);
        else if (y == 4)  printf("   NEXT:");
        else if (y >= 5 && y <= 8) {
            int ry = y - 5;
            printf("   ");
            for (int rx=0; rx<4; ++rx) {
                if (block_at(nextP->type, nextP->rot, rx, ry)) printf("%s  %s", color_for_type(nextP->type), BG_RESET);
                else printf("  ");
            }
        }
        else if (y == 10) printf("   Controls:");
        else if (y == 11) printf("   ←/→: move  ↓: soft drop  s: rotate");
        else if (y == 12) printf("   space: hard drop  p:pause  q:quit");
        else if (y == BOARD_HEIGHT) printf("   (Resize terminal if output looks broken)");
        // newline
        putchar('\n');
    }
    fflush(stdout);
}

// hard drop (immediate)
void hard_drop(Piece *p) {
    while (!collide_piece(p->type, p->rot, p->x, p->y + 1)) p->y++;
    merge_piece(p);
    // hard drop bonus
    score += level * 2;
    clear_lines_and_score();
}

// simple wall-kick: try left/right if rotate into collision
int try_rotate_with_kick(Piece *p, int newrot) {
    if (!collide_piece(p->type, newrot, p->x, p->y)) { p->rot = newrot; return 1; }
    if (!collide_piece(p->type, newrot, p->x-1, p->y)) { p->x -= 1; p->rot = newrot; return 1; }
    if (!collide_piece(p->type, newrot, p->x+1, p->y)) { p->x += 1; p->rot = newrot; return 1; }
    return 0;
}

// SIGINT handler cleanup
void sigint_handler(int s) {
    restore_terminal();
    printf("\nInterrupted. Exiting.\n");
    exit(0);
}

int main(void) {
    srand((unsigned)time(NULL));
    signal(SIGINT, sigint_handler);

    memset(field, 0, sizeof(field));
    enable_raw_mode();
    cls();

    nextPiece = make_random_piece();
    curPiece = make_random_piece();

    // if spawn collides -> game over (board too small or blocked)
    if (collide_piece(curPiece.type, curPiece.rot, curPiece.x, curPiece.y)) {
        restore_terminal();
        fprintf(stderr, "Cannot spawn piece. Increase terminal size or clear board.\n");
        return 1;
    }

    // timing
    int base_delay_ms = 500; // gravity base
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    unsigned long last_tick = ts.tv_sec*1000 + ts.tv_nsec/1000000;

    while (!game_over) {
        draw_all(&curPiece, &nextPiece);

        int k = read_key_nonblock();
        if (k != -1) {
            if (k == 27) {
                // likely an arrow sequence: ESC [ A/B/C/D
                int a = read_key_nonblock();
                int b = read_key_nonblock();
                if (a == '[' && b != -1) {
                    if (b == 'D') { // left arrow
                        if (!collide_piece(curPiece.type, curPiece.rot, curPiece.x - 1, curPiece.y)) curPiece.x--;
                    } else if (b == 'C') { // right arrow
                        if (!collide_piece(curPiece.type, curPiece.rot, curPiece.x + 1, curPiece.y)) curPiece.x++;
                    } else if (b == 'B') { // down arrow (soft drop)
                        if (!collide_piece(curPiece.type, curPiece.rot, curPiece.x, curPiece.y + 1)) {
                            curPiece.y++;
                            score += 1; // small soft-drop reward
                        }
                    } else if (b == 'A') { // up arrow -> rotate (support, but primary rotate is 's')
                        int nr = (curPiece.rot + 1) % 4;
                        try_rotate_with_kick(&curPiece, nr);
                    }
                }
            } else if (k == 'a') {
                if (!collide_piece(curPiece.type, curPiece.rot, curPiece.x - 1, curPiece.y)) curPiece.x--;
            } else if (k == 'd') {
                if (!collide_piece(curPiece.type, curPiece.rot, curPiece.x + 1, curPiece.y)) curPiece.x++;
            } else if (k == 's') { // rotate (user requested s)
                int nr = (curPiece.rot + 1) % 4;
                try_rotate_with_kick(&curPiece, nr);
            } else if (k == ' ') { // hard drop
                hard_drop(&curPiece);
                // spawn next
                curPiece = nextPiece;
                nextPiece = make_random_piece();
                if (collide_piece(curPiece.type, curPiece.rot, curPiece.x, curPiece.y)) game_over = 1;
            } else if (k == 'p') {
                paused = !paused;
                if (paused) {
                    gotoxy1();
                    printf("\n\n==== PAUSED: press 'p' to resume ====\n");
                    fflush(stdout);
                } else {
                    cls();
                }
            } else if (k == 'q') {
                game_over = 1;
            }
        }

        // gravity tick
        clock_gettime(CLOCK_MONOTONIC, &ts);
        unsigned long now_ms = ts.tv_sec*1000 + ts.tv_nsec/1000000;
        unsigned long delay_ms = base_delay_ms;
        if (level > 1) {
            delay_ms = base_delay_ms * (100 - (level-1)*7) / 100;
            if (delay_ms < 50) delay_ms = 50;
        }

        if (!paused && (now_ms - last_tick >= delay_ms)) {
            last_tick = now_ms;
            if (!collide_piece(curPiece.type, curPiece.rot, curPiece.x, curPiece.y + 1)) {
                curPiece.y++;
            } else {
                // lock piece
                merge_piece(&curPiece);
                clear_lines_and_score();
                // spawn
                curPiece = nextPiece;
                nextPiece = make_random_piece();
                if (collide_piece(curPiece.type, curPiece.rot, curPiece.x, curPiece.y)) game_over = 1;
            }
        }

        // tiny sleep to avoid 100% CPU
        usleep(8000);
    }

    // final cleanup and stats
    cls();
    restore_terminal();
    printf("===== GAME OVER =====\n");
    printf("Score: %d\n", score);
    printf("Lines: %d\n", lines_cleared);
    printf("Level: %d\n", level);
    printf("Thanks for playing!\n");
    return 0;
}
