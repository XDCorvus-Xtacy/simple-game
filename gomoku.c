#include <stdio.h>


//보드 출력 함수
void printBoard(char board[SIZE][SIZE], int SIZE)
{
    printf("\n");
    printf("   ");  // 왼쪽 여백
    for (int j = 0; j < SIZE; j++)
        printf("%2d  ", j + 1); // 1~19 출력
    printf("\n");

    for (int i=0; i<SIZE; i++)
    {
        printf("%2d ", i + 1);  // 행 번호 출력
        for (int j=0; j<SIZE; j++)
        {
            printf(" %c ", board[i][j]);
            if (j < SIZE-1)    printf("|");
        }
        printf("\n");

        if (i < SIZE-1)
        {
            printf("   ");
            for (int j=0; j<SIZE; j++)
            {
                printf("---");
                if (j<SIZE-1)   printf("+");
            }
            printf("\n");
        }
    }
    printf("\n");
}

//승패 및 무승부 확인 함수
int checkWin(char board[SIZE][SIZE], int x, int y);

//수 두기 함수
int placeStone(char board[SIZE][SIZE], int x, int y, char current);

//플레이어 턴 전환 함수
char switchPlayer(char current);


//메인 함수
int main(void)
{
    int SIZE = 0;
    char buffer[100];
    while (1)
    {
        printf("보드 크기 입력 (3~19): ");
        fgets(buffer, sizeof(buffer), stdin);
        sscanf(buffer, "%d", &SIZE);
        if (SIZE >=3 && SIZE <= 19) break;
        else printf("범위에 맞는 수를 입력하세요 (3~19)\n");
    }

    char board[SIZE][SIZE];
    for (int i=0; i<SIZE; i++)
        for (int j=0; j<SIZE; j++)
            board[i][j] = ' ';

    printBoard(board, SIZE);

    return 0;
}