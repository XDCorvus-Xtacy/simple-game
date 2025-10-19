#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define SIZE 3

// 📘 보드 출력 함수
void printBoard(char board[SIZE][SIZE]) {
    printf("\n");
    for (int i = 0; i < SIZE; i++) {
        for (int j = 0; j < SIZE; j++) {
            printf(" %c ", board[i][j]);
            if (j < SIZE - 1)
                printf("|");
        }
        printf("\n");
        if (i < SIZE - 1)
            printf("---+---+---\n");
    }
    printf("\n");
}

// 📘 승자 판별 함수
char checkWin(char board[SIZE][SIZE]) {
    // 행 검사
    for (int i = 0; i < SIZE; i++) {
        if (board[i][0] != ' ' &&
            board[i][0] == board[i][1] &&
            board[i][1] == board[i][2])
            return board[i][0];
    }
    // 열 검사
    for (int i = 0; i < SIZE; i++) {
        if (board[0][i] != ' ' &&
            board[0][i] == board[1][i] &&
            board[1][i] == board[2][i])
            return board[0][i];
    }
    // 대각선 검사
    if (board[0][0] != ' ' &&
        board[0][0] == board[1][1] &&
        board[1][1] == board[2][2])
        return board[0][0];
    if (board[0][2] != ' ' &&
        board[0][2] == board[1][1] &&
        board[1][1] == board[2][0])
        return board[0][2];

    return ' '; // 승자 없음
}

// 📘 보드가 가득 찼는지 확인
int isFull(char board[SIZE][SIZE]) {
    for (int i = 0; i < SIZE; i++)
        for (int j = 0; j < SIZE; j++)
            if (board[i][j] == ' ')
                return 0;
    return 1;
}

// 📘 휴리스틱 평가 함수
int evaluateHeuristic(char board[SIZE][SIZE], int depth) {
    char winner = checkWin(board);
    if (winner == 'O') return 100 - depth;
    if (winner == 'X') return depth - 100;

    int score = 0;
    // 각 행, 열, 대각선에 대해 평가
    for (int i = 0; i < SIZE; i++) {
        int rowX = 0, rowO = 0;
        int colX = 0, colO = 0;

        for (int j = 0; j < SIZE; j++) {
            if (board[i][j] == 'X') rowX++;
            if (board[i][j] == 'O') rowO++;
            if (board[j][i] == 'X') colX++;
            if (board[j][i] == 'O') colO++;
        }

        // O는 AI, X는 플레이어
        if (rowO == 2 && rowX == 0) score += 5;
        if (colO == 2 && colX == 0) score += 5;
        if (rowX == 2 && rowO == 0) score -= 5;
        if (colX == 2 && colO == 0) score -= 5;
    }

    // 대각선 평가
    int diag1O = 0, diag1X = 0;
    int diag2O = 0, diag2X = 0;
    for (int i = 0; i < SIZE; i++) {
        if (board[i][i] == 'O') diag1O++;
        if (board[i][i] == 'X') diag1X++;
        if (board[i][SIZE - i - 1] == 'O') diag2O++;
        if (board[i][SIZE - i - 1] == 'X') diag2X++;
    }
    if (diag1O == 2 && diag1X == 0) score += 5;
    if (diag2O == 2 && diag2X == 0) score += 5;
    if (diag1X == 2 && diag1O == 0) score -= 5;
    if (diag2X == 2 && diag2O == 0) score -= 5;

    return score;
}

/*
// 📘 알파베타 가지치기 기반 minimax
int minimaxAlphaBeta(char board[SIZE][SIZE], int depth, int isMaximizing, int alpha, int beta) {
    int score = evaluateHeuristic(board, depth);
    char winner = checkWin(board);

    if (winner == 'O' || winner == 'X' || isFull(board))
        return score;

    if (isMaximizing) {
        int best = -1000;
        for (int i = 0; i < SIZE; i++) {
            for (int j = 0; j < SIZE; j++) {
                if (board[i][j] == ' ') {
                    board[i][j] = 'O';
                    int val = minimaxAlphaBeta(board, depth + 1, 0, alpha, beta);
                    board[i][j] = ' ';
                    if (val > best) best = val;
                    if (best > alpha) alpha = best;
                    if (beta <= alpha) return best; // ✂️ 가지치기
                }
            }
        }
        return best;
    } else {
        int best = 1000;
        for (int i = 0; i < SIZE; i++) {
            for (int j = 0; j < SIZE; j++) {
                if (board[i][j] == ' ') {
                    board[i][j] = 'X';
                    int val = minimaxAlphaBeta(board, depth + 1, 1, alpha, beta);
                    board[i][j] = ' ';
                    if (val < best) best = val;
                    if (best < beta) beta = best;
                    if (beta <= alpha) return best; // ✂️ 가지치기
                }
            }
        }
        return best;
    }
}
*/

//시각화용 minimaxAlphaBeta
int minimaxAlphaBeta(char board[SIZE][SIZE], int depth, int isMaximizing, int alpha, int beta) {
    int score = evaluateHeuristic(board, depth);
    char winner = checkWin(board);
    if (winner == 'O' || winner == 'X' || isFull(board))
        return score;

    // 🌱 깊이에 따른 들여쓰기 (탐색 단계 시각화)
    for (int i = 0; i < depth; i++) printf("    ");

    printf("[Depth %d | α=%d, β=%d] → %s 턴\n",
           depth, alpha, beta, isMaximizing ? "AI(O)" : "플레이어(X)");

    if (isMaximizing) {
        int best = -1000;
        for (int i = 0; i < SIZE; i++) {
            for (int j = 0; j < SIZE; j++) {
                if (board[i][j] == ' ') {
                    board[i][j] = 'O';
                    int val = minimaxAlphaBeta(board, depth + 1, 0, alpha, beta);
                    board[i][j] = ' ';
                    if (val > best) best = val;
                    if (best > alpha) alpha = best;

                    for (int k = 0; k < depth; k++) printf("    ");
                    printf("AI(O)이 (%d,%d)에 둠 → val=%d, α=%d, β=%d\n",
                           i + 1, j + 1, val, alpha, beta);

                    if (beta <= alpha) {
                        for (int k = 0; k < depth; k++) printf("    ");
                        printf("✂️ 가지치기 발생! (β <= α)\n");
                        return best;
                    }
                }
            }
        }
        return best;
    } 
    else {
        int best = 1000;
        for (int i = 0; i < SIZE; i++) {
            for (int j = 0; j < SIZE; j++) {
                if (board[i][j] == ' ') {
                    board[i][j] = 'X';
                    int val = minimaxAlphaBeta(board, depth + 1, 1, alpha, beta);
                    board[i][j] = ' ';
                    if (val < best) best = val;
                    if (best < beta) beta = best;

                    for (int k = 0; k < depth; k++) printf("    ");
                    printf("플레이어(X)가 (%d,%d)에 둠 → val=%d, α=%d, β=%d\n",
                           i + 1, j + 1, val, alpha, beta);

                    if (beta <= alpha) {
                        for (int k = 0; k < depth; k++) printf("    ");
                        printf("✂️ 가지치기 발생! (β <= α)\n");
                        return best;
                    }
                }
            }
        }
        return best;
    }
}

// 📘 최적의 수 찾기
void findBestMove(char board[SIZE][SIZE]) {
    int bestScore = -1000;
    int bestRow = -1, bestCol = -1;

    for (int i = 0; i < SIZE; i++) {
        for (int j = 0; j < SIZE; j++) {
            if (board[i][j] == ' ') {
                board[i][j] = 'O';
                int moveScore = minimaxAlphaBeta(board, 0, 0, -1000, 1000);
                board[i][j] = ' ';
                if (moveScore > bestScore) {
                    bestScore = moveScore;
                    bestRow = i;
                    bestCol = j;
                }
            }
        }
    }

    board[bestRow][bestCol] = 'O';
    printf("🤖 컴퓨터가 (%d, %d)에 둡니다.\n", bestRow + 1, bestCol + 1);
}

// 📘 메인 함수
int main(void) {
    char board[SIZE][SIZE];
    for (int i = 0; i < SIZE; i++)
        for (int j = 0; j < SIZE; j++)
            board[i][j] = ' ';

    srand((unsigned int)time(NULL));

    char winner = ' ';
    int row, col;

    printf("🎮 틱택토 (플레이어 vs 컴퓨터)\n");
    printf("당신은 X입니다. (1~3 사이의 행, 열을 입력하세요)\n");
    printBoard(board);

    while (1) {
        // 🧍 플레이어 차례
        printf("플레이어 차례입니다. (행 열 입력): ");
        scanf("%d %d", &row, &col);

        if (row < 1 || row > 3 || col < 1 || col > 3) {
            printf("❌ 잘못된 입력입니다. 1~3 사이의 숫자를 입력하세요.\n");
            continue;
        }
        if (board[row - 1][col - 1] != ' ') {
            printf("⚠️ 이미 둔 자리입니다!\n");
            continue;
        }

        board[row - 1][col - 1] = 'X';
        printBoard(board);

        winner = checkWin(board);
        if (winner == 'X') {
            printf("🎉 플레이어 승리!\n");
            break;
        }
        if (isFull(board)) {
            printf("🤝 무승부입니다!\n");
            break;
        }

        // 💻 컴퓨터 차례
        printf("컴퓨터가 두는 중...\n");
        findBestMove(board);
        printBoard(board);

        winner = checkWin(board);
        if (winner == 'O') {
            printf("💻 컴퓨터 승리!\n");
            break;
        }
        if (isFull(board)) {
            printf("🤝 무승부입니다!\n");
            break;
        }
    }

    printf("게임 종료!\n");
    return 0;
}