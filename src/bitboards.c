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
#define SHIFT_UP(bb) ((bb) << 8)
#define SHIFT_DOWN(bb) ((bb) >> 8)
#define SHIFT_RIGHT(bb) (((bb) << 1) & 0xFEFEFEFEFEFEFEFEULL)
#define SHIFT_LEFT(bb) (((bb) >> 1) & 0x7F7F7F7F7F7F7F7FULL)

// ****************
// type definitions
// ****************

typedef uint64_t bitboard;

typedef struct
{
    bitboard king;
    bitboard queen;
    bitboard rook;
    bitboard bishop;
    bitboard knight;
    bitboard pawn;

    bitboard white;
    // bitboard black;
} board;

typedef short square;

typedef struct
{
    square original;
    square next;
} move;

typedef struct
{
    board board;
    uint8_t metadata;
} game;

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

// *************************
// bitboard/board operations
// *************************

void printBB(bitboard bb)
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

board generateStartingBoard()
{
    board empty;
    empty.king = SQUARE(4, 0) | SQUARE(4, 7);
    empty.queen = SQUARE(3, 0) | SQUARE(3, 7);
    empty.rook = SQUARE(0, 0) | SQUARE(7, 0) | SQUARE(0, 7) | SQUARE(7, 7);
    empty.bishop = SQUARE(2, 0) | SQUARE(5, 0) | SQUARE(2, 7) | SQUARE(5, 7);
    empty.knight = SQUARE(1, 0) | SQUARE(6, 0) | SQUARE(1, 7) | SQUARE(6, 7);

    empty.pawn = 0;
    for (int i = 0; i < X_WIDTH; i++)
    {
        empty.pawn |= SQUARE(i, 1);
        empty.pawn |= SQUARE(i, 6);
    }

    empty.white = 0;
    // empty.black = 0;
    for (int i = 0; i < X_WIDTH; i++)
    {
        empty.white |= SQUARE(i, 6);
        empty.white |= SQUARE(i, 7);
        // empty.black |= SQUARE(i, 0);
        // empty.black |= SQUARE(i, 1);
    }

    return empty;
}

void printBoard(board board)
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

// implements Brian Kernighan's algorithm
int numSignificantBits(bitboard bitboard)
{
    int count = 0;
    while (bitboard > 0)
    {
        bitboard &= (bitboard - 1);
        count++;
    }
    return count;
}

int trailingZeros(bitboard bitboard)
{
    int count = 0;
    while ((bitboard & 1) == 0)
    {
        bitboard >>= 1;
        count++;
    }
    return count;
}

bitboard getPieces(board board)
{
    return (board.king | board.queen | board.rook | board.bishop | board.knight | board.pawn);
}

short *getPieceSquares(bitboard bitboard)
{
    int size = numSignificantBits(bitboard);
    short *array = (short *)malloc(size * sizeof(u_int8_t));
    if (array == NULL)
    {
        return NULL;
    }
    int count = 0;
    while (size > 0)
    {
        count++;
        if (bitboard >> count & 1)
        {
            array[size - 1] = count;
            size--;
        }
    }
    return array;
}

// returns the number of the square that a given piece is on
short getNthSBit(bitboard bitboard, int n)
{
    int count = 0;
    while (bitboard)
    {
        if ((bitboard & 1) && (n-- == 0))
            return count;
        bitboard = bitboard >> 1;
        count++;
    }
    return 0;
}
// ***************************
// graphics related operations
// ***************************

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

// ***********************
// game related operations
// ***********************

game newGame()
{
    return (game){generateStartingBoard(), 1 << 7};
}

bitboard bishopMovement(bitboard bishop, bitboard blockers, short direction)
{
    bitboard attacks = 0;
    bitboard ray = bishop;
    for (int i = 0; i < 7; i++)
    {
        if (direction == 0) // top left
        {
            ray = SHIFT_UP(SHIFT_LEFT(ray));
        }
        else if (direction == 1) // down
        {
            ray = SHIFT_UP(SHIFT_RIGHT(ray));
        }
        else if (direction == 2) // left
        {
            ray = SHIFT_DOWN(SHIFT_RIGHT(ray));
        }
        else if (direction == 3) // right
        {
            ray = SHIFT_DOWN(SHIFT_LEFT(ray));
        }

        attacks |= ray;

        if (ray & blockers)
        {
            break;
        }
    }
    return attacks;
}

move *getBishopMoves(game game, bitboard colour)
{
    bitboard bishops = (game.board.bishop & colour);
    bitboard allPieces = getPieces(game.board);
    bitboard enemyPieces = allPieces & ~colour;

    int maxMoves = numSignificantBits(bishops) * (X_WIDTH - 1 + Y_WIDTH - 1);
    move *moves = (move *)malloc((maxMoves + 1) * sizeof(move));
    int moveCount = 0;

    for (int bishopNum = 0; bishopNum < numSignificantBits(bishops); bishopNum++)
    {
        short activeBishop = getNthSBit(bishops, bishopNum);
        short bishopFile = activeBishop % 8;
        short bishopRank = activeBishop / 8;
        bitboard bishop = 1ULL << activeBishop;
        bitboard attacks = 0;
        for (int dir = 0; dir < 4; dir++)
        {
            attacks |= bishopMovement(bishop, allPieces, dir);
        }
        bitboard validMoves = attacks & ~colour;
        while (validMoves > 0)
        {
            int targetSq = trailingZeros(validMoves);
            moves[moveCount].original = (square){SQUARE_BIT(bishopFile, bishopRank)};
            moves[moveCount].next = (square){SQUARE_BIT(targetSq % 8, targetSq / 8)};
            moveCount++;
            validMoves &= validMoves - 1;
        }
    }

    moves[moveCount].original = (square){-1};
    return moves;
}

bitboard rookMovement(bitboard rook, bitboard blockers, int direction)
{
    bitboard attacks = 0;
    bitboard ray = rook;
    for (int i = 0; i < 7; i++)
    {
        if (direction == 0) // up
        {
            ray = SHIFT_UP(ray);
        }
        else if (direction == 1) // down
        {
            ray = SHIFT_DOWN(ray);
        }
        else if (direction == 2) // left
        {
            ray = SHIFT_LEFT(ray);
        }
        else if (direction == 3) // right
        {
            ray = SHIFT_RIGHT(ray);
        }

        attacks |= ray;

        if (ray & blockers)
        {
            break;
        }
    }
    return attacks;
}

move *getRookMoves(game game, bitboard colour)
{
    bitboard rooks = (game.board.rook & colour);
    bitboard allPieces = getPieces(game.board);
    bitboard enemyPieces = allPieces & ~colour;

    int maxMoves = numSignificantBits(rooks) * (X_WIDTH - 1 + Y_WIDTH - 1);
    move *moves = (move *)malloc((maxMoves + 1) * sizeof(move));
    int moveCount = 0;

    for (int rookNum = 0; rookNum < numSignificantBits(rooks); rookNum++)
    {
        short activeRook = getNthSBit(rooks, rookNum);
        short rookFile = activeRook % 8;
        short rookRank = activeRook / 8;
        bitboard rook = 1ULL << activeRook;
        bitboard attacks = 0;
        for (int dir = 0; dir < 4; dir++)
        {
            attacks |= rookMovement(rook, allPieces, dir);
        }
        bitboard validMoves = attacks & ~colour;
        while (validMoves > 0)
        {
            int targetSq = trailingZeros(validMoves);
            moves[moveCount].original = (square){SQUARE_BIT(rookFile, rookRank)};
            moves[moveCount].next = (square){SQUARE_BIT(targetSq % 8, targetSq / 8)};
            moveCount++;
            validMoves &= validMoves - 1;
        }
    }

    moves[moveCount].original = (square){-1};
    return moves;
}

int main(void)
{

    InitWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Meowl Chess");

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

    Texture2D textures[13] = {wk, wq, wr, wb, wn, wp, bk, bq, br, bb, bn, bp, boardtexture};
    game game = newGame();

    while (!WindowShouldClose())
    {
        BeginDrawing();
        ClearBackground(WHITE);
        DrawTextureEx(boardtexture, (Vector2){0.0, 0.0}, 0, (float)(WINDOW_HEIGHT + WINDOW_WIDTH) / 1568, WHITE);
        renderBoard(game.board, textures);
        EndDrawing();
    }

    for (int i = 0; i < sizeof(textures) / sizeof(Texture2D); i++)
    {
        UnloadTexture(textures[i]);
    }

    CloseWindow();

    return 0;
}
