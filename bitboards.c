#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <stdbool.h>
#include <string.h>

#define X_WIDTH 8
#define Y_WIDTH 8

#define SQUARE(rank, file) (1ULL << (rank * X_WIDTH + file))
#define SQUARE_BIT(rank, file) (rank * X_WIDTH + file)
#define RANK(rank) (0xFFULL << (8 * (rank)))
#define FILE(file) (0x0101010101010101ULL << (file))

typedef uint64_t bitboard;

typedef struct
{
    uint8_t x_coord;
    uint8_t y_coord;
} square;

typedef struct
{
    bitboard king;
    bitboard queen;
    bitboard rook;
    bitboard bishop;
    bitboard knight;
    bitboard pawn;

    bitboard white;
    bitboard black;
} board;

typedef struct
{
    square original;
    square next;
} move;

typedef char *data_t;

typedef struct node node_t;

struct node
{
    data_t data;
    node_t *next;
    node_t *prev;
};

typedef struct
{
    node_t *head;
    node_t *foot;
} list_t;

void print_bb(bitboard bb)
{
    for (int i = 0; i < X_WIDTH; i++)
    {
        for (int j = 0; j < Y_WIDTH; j++)
        {
            printf("%s", (bb >> (j * X_WIDTH + i)) & 1 ? "[X]" : "[ ]");
        }
        printf("\n");
    }
}

board starting_board()
{
    board empty;
    empty.king = SQUARE(4, 0) + SQUARE(4, 7);
    empty.queen = SQUARE(3, 0) + SQUARE(3, 7);
    empty.rook = SQUARE(0, 0) + SQUARE(7, 0) + SQUARE(0, 7) + SQUARE(7, 7);
    empty.bishop = SQUARE(2, 0) + SQUARE(5, 0) + SQUARE(2, 7) + SQUARE(5, 7);
    empty.knight = SQUARE(1, 0) + SQUARE(6, 0) + SQUARE(1, 7) + SQUARE(6, 7);

    empty.pawn = 0;
    for (int i = 0; i < X_WIDTH; i++)
    {
        empty.pawn += SQUARE(i, 1);
        empty.pawn += SQUARE(i, 6);
    }

    empty.white = 0;
    for (int i = 0; i < X_WIDTH; i++)
    {
        empty.white += SQUARE(i, 0);
        empty.white += SQUARE(i, 1);
        empty.black += SQUARE(i, 7);
        empty.black += SQUARE(i, 6);
    }

    return empty;
}

void print_board(board board)
{
    for (int i = 0; i < X_WIDTH; i++)
    {
        for (int j = 0; j < Y_WIDTH; j++)
        {
            if (board.pawn >> SQUARE_BIT(j, i) & 1)
            {
                if (board.white >> SQUARE_BIT(j, i) & 1)
                {
                    printf("[P]");
                }
                else
                {
                    printf("[p]");
                }
            }
            else if (board.king >> SQUARE_BIT(j, i) & 1)
            {
                if (board.white >> SQUARE_BIT(j, i) & 1)
                {
                    printf("[K]");
                }
                else
                {
                    printf("[k]");
                }
            }
            else if (board.queen >> SQUARE_BIT(j, i) & 1)
            {
                if (board.white >> SQUARE_BIT(j, i) & 1)
                {
                    printf("[Q]");
                }
                else
                {
                    printf("[q]");
                }
            }
            else if (board.rook >> SQUARE_BIT(j, i) & 1)
            {
                if (board.white >> SQUARE_BIT(j, i) & 1)
                {
                    printf("[R]");
                }
                else
                {
                    printf("[r]");
                }
            }
            else if (board.bishop >> SQUARE_BIT(j, i) & 1)
            {
                if (board.white >> SQUARE_BIT(j, i) & 1)
                {
                    printf("[B]");
                }
                else
                {
                    printf("[b]");
                }
            }
            else if (board.knight >> SQUARE_BIT(j, i) & 1)
            {
                if (board.white >> SQUARE_BIT(j, i) & 1)
                {
                    printf("[N]");
                }
                else
                {
                    printf("[n]");
                }
            }
            else
            {
                printf("[X]");
            }
        }
        printf("\n");
    }
}

int main(int argc, char *argv[])
{
    board new = starting_board();
    print_bb(new.pawn);
    print_board(new);
    return 0;
}
