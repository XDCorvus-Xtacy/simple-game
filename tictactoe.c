#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define SIZE 3

// 보드 출력 함수
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

// 승자 판별 함수
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

    // 아직 승자 없음
    return ' ';
}

// 보드가 꽉 찼는지 확인
int isFull(char board[SIZE][SIZE]) {
    for (int i = 0; i < SIZE; i++) {
        for (int j = 0; j < SIZE; j++) {
            if (board[i][j] == ' ')
                return 0;
        }
    }
    return 1;
}

//컴퓨터의 랜덤 위치 선택 함수
void ComputerMove(char board[SIZE][SIZE])
{
    int row, col;
    while (1)
    {
        row = rand() % SIZE;
        col = rand() % SIZE;

        if (board[row][col] = " ")
        {
            board[row][col] = 'O';
            printf("🤖 컴퓨터가 (%d, %d)에 둡니다.\n", row + 1, col + 1);
            break
        }
    }

}

// 메인 함수
int main(void) {
    char board[SIZE][SIZE];
    for (int i = 0; i < SIZE; i++)
        for (int j = 0; j < SIZE; j++)
            board[i][j] = ' ';

    char currentPlayer = 'X';
    int row, col;
    char winner = ' ';

    printf("🎮 틱택토 게임 시작!\n");
    printBoard(board);

    while (1) {
        printf("플레이어 %c 차례입니다. (행 열 입력): ", currentPlayer);
        scanf("%d %d", &row, &col);

        if (row < 1 || row > 3 || col < 1 || col > 3) {
            printf("❌ 잘못된 입력입니다. (1~3 범위)\n");
            continue;
        }

        if (board[row - 1][col - 1] != ' ') {
            printf("⚠️ 이미 둔 자리입니다!\n");
            continue;
        }

        board[row - 1][col - 1] = currentPlayer;
        printBoard(board);

        winner = checkWin(board);
        if (winner != ' ') {
            printf("🎉 플레이어 %c 승리!\n", winner);
            break;
        } else if (isFull(board)) {
            printf("🤝 무승부입니다!\n");
            break;
        }

        // 턴 전환
        currentPlayer = (currentPlayer == 'X') ? 'O' : 'X';
    }

    return 0;
}
