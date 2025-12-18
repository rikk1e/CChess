#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <stdbool.h>
#include <string.h>
#include "raylib.h"
#include "raymath.h"

#define X_WIDTH 8
#define Y_WIDTH 8
#define WINDOW_HEIGHT 960
#define WINDOW_WIDTH 960
#define BOARD_PADDING 8

#define SQUARE(rank, file) (1ULL << (rank * X_WIDTH + file))
#define SQUARE_BIT(rank, file) (rank * X_WIDTH + file)
#define RANK(rank) (0xFFULL << (8 * (rank)))
#define FILE(file) (0x0101010101010101ULL << (file))
#define posY(file, rank) ((WINDOW_HEIGHT / Y_WIDTH) * rank)
#define posX(file, rank) ((WINDOW_WIDTH / X_WIDTH) * (file - 64))

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

Vector2 getCoordinate(char file, int rank)
{
    int sqWidth = (WINDOW_WIDTH - 2 * BOARD_PADDING) / X_WIDTH;
    int sqHeight = (WINDOW_HEIGHT - 2 * BOARD_PADDING) / Y_WIDTH;

    return (Vector2){BOARD_PADDING + sqWidth * file, BOARD_PADDING + sqHeight * rank};
}

void drawPiece(Texture2D texture, int file, int rank)
{

    int sqWidth = (WINDOW_WIDTH - 2 * BOARD_PADDING) / X_WIDTH;
    int sqHeight = (WINDOW_HEIGHT - 2 * BOARD_PADDING) / Y_WIDTH;
    DrawTextureEx(texture, getCoordinate(file, rank), 0, (sqWidth + sqHeight) / 256.0, WHITE);
    return;
}

void renderBoard(board board, Texture2D textures[13])
{
    for (int i = 0; i < X_WIDTH; i++)
    {
        for (int j = 0; j < Y_WIDTH; j++)
        {
            if (board.pawn >> SQUARE_BIT(j, i) & 1)
            {
                if (board.white >> SQUARE_BIT(j, i) & 1)
                {
                    drawPiece(textures[5], j, i);
                }
                else
                {
                    drawPiece(textures[11], j, i);
                }
            }
            else if (board.king >> SQUARE_BIT(j, i) & 1)
            {
                if (board.white >> SQUARE_BIT(j, i) & 1)
                {
                    drawPiece(textures[0], j, i);
                }
                else
                {
                    drawPiece(textures[6], j, i);
                }
            }
            else if (board.queen >> SQUARE_BIT(j, i) & 1)
            {
                if (board.white >> SQUARE_BIT(j, i) & 1)
                {
                    drawPiece(textures[1], j, i);
                }
                else
                {
                    drawPiece(textures[7], j, i);
                }
            }
            else if (board.rook >> SQUARE_BIT(j, i) & 1)
            {
                if (board.white >> SQUARE_BIT(j, i) & 1)
                {
                    drawPiece(textures[2], j, i);
                }
                else
                {
                    drawPiece(textures[8], j, i);
                }
            }
            else if (board.bishop >> SQUARE_BIT(j, i) & 1)
            {
                if (board.white >> SQUARE_BIT(j, i) & 1)
                {
                    drawPiece(textures[3], j, i);
                }
                else
                {
                    drawPiece(textures[9], j, i);
                }
            }
            else if (board.knight >> SQUARE_BIT(j, i) & 1)
            {
                if (board.white >> SQUARE_BIT(j, i) & 1)
                {
                    drawPiece(textures[4], j, i);
                }
                else
                {
                    drawPiece(textures[10], j, i);
                }
            }
            else
            {
                continue;
            }
        }
    }
}

int main(void)
{
    InitWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Meowl Chess");

    // textures loaded with colour then piece letter in notation

    Texture2D wk = LoadTexture("res/pieces-basic-png/white-king.png");
    Texture2D wq = LoadTexture("res/pieces-basic-png/white-queen.png");
    Texture2D wr = LoadTexture("res/pieces-basic-png/white-rook.png");
    Texture2D wb = LoadTexture("res/pieces-basic-png/white-bishop.png");
    Texture2D wn = LoadTexture("res/pieces-basic-png/white-knight.png");
    Texture2D wp = LoadTexture("res/pieces-basic-png/white-pawn.png");

    Texture2D bk = LoadTexture("res/pieces-basic-png/black-king.png");
    Texture2D bq = LoadTexture("res/pieces-basic-png/black-queen.png");
    Texture2D br = LoadTexture("res/pieces-basic-png/black-rook.png");
    Texture2D bb = LoadTexture("res/pieces-basic-png/black-bishop.png");
    Texture2D bn = LoadTexture("res/pieces-basic-png/black-knight.png");
    Texture2D bp = LoadTexture("res/pieces-basic-png/black-pawn.png");

    Texture2D boardtexture = LoadTexture("res/pieces-basic-png/rect-8x8.png");

    Texture2D textures[13] = {wk, wq, wr, wb, wn, wp, bk, bq, br, bb, bn, bp};

    board empty_board = starting_board();

    while (!WindowShouldClose())
    {
        BeginDrawing();
        ClearBackground(WHITE);
        DrawTextureEx(boardtexture, (Vector2){0.0, 0.0}, 0, (float)(WINDOW_HEIGHT + WINDOW_WIDTH) / 1568, WHITE);
        renderBoard(empty_board, textures);
        EndDrawing();
    }

    for (int i = 0; i < sizeof(textures) / sizeof(Texture2D); i++)
    {
        UnloadTexture(textures[i]);
    }

    CloseWindow();

    return 0;
}
