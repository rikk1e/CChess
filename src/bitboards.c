#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <stdbool.h>
#include <string.h>
#include "raylib.h"
#include "raymath.h"
#include <unistd.h>

#define X_WIDTH 8
#define Y_WIDTH 8
#define WINDOW_HEIGHT 960
#define WINDOW_WIDTH 960
#define BOARD_PADDING 8

#define SQUARE(file, rank) (1ULL << (rank * X_WIDTH + file))
#define SQUARE_BIT(file, rank) (rank * X_WIDTH + file)
#define RANK(rank) (0xFFULL << (8 * (rank)))
#define FILE(file) (0x0101010101010101ULL << (file))
#define SHIFT_UP(bb) ((bb) << 8)
#define SHIFT_DOWN(bb) ((bb) >> 8)
#define SHIFT_RIGHT(bb) (((bb) << 1) & 0xFEFEFEFEFEFEFEFEULL)
#define SHIFT_LEFT(bb) (((bb) >> 1) & 0x7F7F7F7F7F7F7F7FULL)

// ****************
// type definitions
// ****************

typedef short square;

typedef struct
{
    square original;
    square next;
} move;

typedef struct node node_t;

typedef struct node
{
    move move;
    struct node *next;
} node;

typedef struct
{
    node *head;
    node *foot;
    int length;
} list_t;

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

typedef struct
{
    board board;
    uint8_t metadata;
    uint16_t en_passants;
    list_t moves;
} game;

// *******************
// function prototypes
// *******************
void addMove(list_t *moveList, move move);

// *************************
// bitboard/board operations
// *************************

void printMove(move move)
{
    printf("%c%d", (move.original % 8) + 97, (move.original / 8) + 1);
    printf("%c%d \n", (move.next % 8) + 97, (move.next / 8) + 1);
}

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
    empty.king = SQUARE(3, 0) | SQUARE(3, 7);
    empty.queen = SQUARE(4, 0) | SQUARE(4, 7);
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
        empty.white |= SQUARE(i, 0);
        empty.white |= SQUARE(i, 1);
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
    return (game){generateStartingBoard(), 1 << 7, 0, 0, 0};
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

move *getBishopMoves(game game)
{
    bitboard bishops = 0;
    bitboard enemyPieces = 0;
    bitboard friendlyPieces = 0;
    bitboard allPieces = getPieces(game.board);

    bool isWhite;
    if (game.metadata >> 7 & 1)
    {
        bishops = game.board.bishop & game.board.white;
        friendlyPieces = game.board.white;
        enemyPieces = getPieces(game.board) & ~game.board.white;
        isWhite = true;
    }
    else
    {
        bishops = game.board.bishop & ~game.board.white;
        friendlyPieces = getPieces(game.board) & ~game.board.white;
        enemyPieces = game.board.white;
        isWhite = false;
    }

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
        bitboard validMoves = attacks & ~friendlyPieces;
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

move *getRookMoves(game game)
{
    bitboard rooks = 0;
    bitboard enemyPieces = 0;
    bitboard friendlyPieces = 0;
    bitboard allPieces = getPieces(game.board);

    bool isWhite;
    if (game.metadata >> 7 & 1)
    {
        rooks = game.board.rook & game.board.white;
        friendlyPieces = game.board.white;
        enemyPieces = getPieces(game.board) & ~game.board.white;
        isWhite = true;
    }
    else
    {
        rooks = game.board.rook & ~game.board.white;
        friendlyPieces = getPieces(game.board) & ~game.board.white;
        enemyPieces = game.board.white;
        isWhite = false;
    }

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
        bitboard validMoves = attacks & ~friendlyPieces;
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

bitboard knightMovement(bitboard knight, bitboard blockers, short direction)
{
    bitboard attack = knight;
    if (direction == 0)
    {
        attack = SHIFT_UP(SHIFT_UP(SHIFT_RIGHT(attack)));
    }
    else if (direction == 1)
    {
        attack = SHIFT_UP(SHIFT_RIGHT(SHIFT_RIGHT(attack)));
    }
    else if (direction == 2)
    {
        attack = SHIFT_DOWN(SHIFT_RIGHT(SHIFT_RIGHT(attack)));
    }
    else if (direction == 3)
    {
        attack = SHIFT_DOWN(SHIFT_DOWN(SHIFT_RIGHT(attack)));
    }
    else if (direction == 4)
    {
        attack = SHIFT_DOWN(SHIFT_DOWN(SHIFT_LEFT(attack)));
    }
    else if (direction == 5)
    {
        attack = SHIFT_DOWN(SHIFT_LEFT(SHIFT_LEFT(attack)));
    }
    else if (direction == 6)
    {
        attack = SHIFT_UP(SHIFT_LEFT(SHIFT_LEFT(attack)));
    }
    else if (direction == 7)
    {
        attack = SHIFT_UP(SHIFT_UP(SHIFT_LEFT(attack)));
    }

    return attack;
}

move *getKnightMoves(game game)
{

    bitboard allPieces = getPieces(game.board);
    bitboard colour = 0;
    if (game.metadata >> 7 & 1)
    {
        colour = game.board.white;
    }
    else
    {
        colour = allPieces & ~game.board.white;
    }
    bitboard knights = (game.board.knight & colour);

    bitboard enemyPieces = allPieces & ~colour;

    int maxMoves = numSignificantBits(knights) * 8;
    move *moves = (move *)malloc((maxMoves + 1) * sizeof(move));
    int moveCount = 0;

    for (int knightNum = 0; knightNum < numSignificantBits(knights); knightNum++)
    {
        short activeknight = getNthSBit(knights, knightNum);
        short knightFile = activeknight % 8;
        short knightRank = activeknight / 8;
        bitboard knight = 1ULL << activeknight;
        bitboard attacks = 0;
        for (int dir = 0; dir < 8; dir++)
        {
            attacks |= knightMovement(knight, allPieces, dir);
        }
        bitboard validMoves = attacks & ~colour;
        while (validMoves > 0)
        {
            int targetSq = trailingZeros(validMoves);
            moves[moveCount].original = (square){SQUARE_BIT(knightFile, knightRank)};
            moves[moveCount].next = (square){SQUARE_BIT(targetSq % 8, targetSq / 8)};
            moveCount++;
            validMoves &= validMoves - 1;
        }
    }

    moves[moveCount].original = (square){-1};
    return moves;
}

bitboard queenMovement(bitboard queen, bitboard blockers, short direction)
{
    bitboard ray = queen;
    bitboard attacks = 0;

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
        if (direction == 4) // up
        {
            ray = SHIFT_UP(ray);
        }
        else if (direction == 5) // down
        {
            ray = SHIFT_DOWN(ray);
        }
        else if (direction == 6) // left
        {
            ray = SHIFT_LEFT(ray);
        }
        else if (direction == 7) // right
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

move *getQueenMoves(game game)
{
    bitboard queens = 0;
    bitboard enemyPieces = 0;
    bitboard friendlyPieces = 0;
    bitboard allPieces = getPieces(game.board);

    bool isWhite;
    if (game.metadata >> 7 & 1)
    {
        queens = game.board.queen & game.board.white;
        friendlyPieces = game.board.white;
        enemyPieces = getPieces(game.board) & ~game.board.white;
        isWhite = true;
    }
    else
    {
        queens = game.board.queen & ~game.board.white;
        friendlyPieces = getPieces(game.board) & ~game.board.white;
        enemyPieces = game.board.white;
        isWhite = false;
    }

    int maxMoves = numSignificantBits(queens) * 2 * (X_WIDTH - 1 + Y_WIDTH - 1);
    move *moves = (move *)malloc((maxMoves + 1) * sizeof(move));
    int moveCount = 0;

    for (int queenNum = 0; queenNum < numSignificantBits(queens); queenNum++)
    {
        short activequeen = getNthSBit(queens, queenNum);
        short queenFile = activequeen % 8;
        short queenRank = activequeen / 8;
        bitboard queen = 1ULL << activequeen;
        bitboard attacks = 0;
        for (int dir = 0; dir < 8; dir++)
        {
            attacks |= queenMovement(queen, allPieces, dir);
        }
        bitboard validMoves = attacks & ~friendlyPieces;
        while (validMoves > 0)
        {
            int targetSq = trailingZeros(validMoves);
            moves[moveCount].original = (square){SQUARE_BIT(queenFile, queenRank)};
            moves[moveCount].next = (square){SQUARE_BIT(targetSq % 8, targetSq / 8)};
            moveCount++;
            validMoves &= validMoves - 1;
        }
    }

    moves[moveCount].original = (square){-1};
    return moves;
}

bitboard kingMovement(bitboard king, bitboard blockers, short direction)
{
    bitboard ray = king;
    bitboard attacks = 0;
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
    if (direction == 4) // up
    {
        ray = SHIFT_UP(ray);
    }
    else if (direction == 5) // down
    {
        ray = SHIFT_DOWN(ray);
    }
    else if (direction == 6) // left
    {
        ray = SHIFT_LEFT(ray);
    }
    else if (direction == 7) // right
    {
        ray = SHIFT_RIGHT(ray);
    }

    attacks |= ray;
    if (ray & blockers)
    {
        return attacks;
    }
    return attacks;
}

move *getKingMoves(game game)
{
    bitboard kings = 0;
    bitboard enemyPieces = 0;
    bitboard friendlyPieces = 0;
    bitboard allPieces = getPieces(game.board);

    bool isWhite;
    if (game.metadata >> 7 & 1)
    {
        kings = game.board.king & game.board.white;
        friendlyPieces = game.board.white;
        enemyPieces = getPieces(game.board) & ~game.board.white;
        isWhite = true;
    }
    else
    {
        kings = game.board.king & ~game.board.white;
        friendlyPieces = getPieces(game.board) & ~game.board.white;
        enemyPieces = game.board.white;
        isWhite = false;
    }

    int maxMoves = 8;
    move *moves = (move *)malloc((maxMoves + 1) * sizeof(move));
    int moveCount = 0;

    for (int kingNum = 0; kingNum < numSignificantBits(kings); kingNum++)
    {
        short activeking = getNthSBit(kings, kingNum);
        short kingFile = activeking % 8;
        short kingRank = activeking / 8;
        bitboard king = 1ULL << activeking;
        bitboard attacks = 0;
        for (int dir = 0; dir < 8; dir++)
        {
            attacks |= kingMovement(king, allPieces, dir);
        }
        bitboard validMoves = attacks & ~friendlyPieces;
        while (validMoves > 0)
        {
            int targetSq = trailingZeros(validMoves);
            moves[moveCount].original = (square){SQUARE_BIT(kingFile, kingRank)};
            moves[moveCount].next = (square){SQUARE_BIT(targetSq % 8, targetSq / 8)};
            moveCount++;
            validMoves &= validMoves - 1;
        }
    }

    moves[moveCount].original = (square){-1};
    return moves;
}

bitboard pawnMovement(bitboard pawn, bitboard enemyPieces, bitboard friendlyPieces, bool isWhite, uint16_t *metadata)
{
    bitboard attacks = 0;

    if (isWhite == true)
    {
        if (SHIFT_UP(SHIFT_LEFT(pawn)) & enemyPieces)
        {
            attacks |= SHIFT_UP(SHIFT_LEFT(pawn));
        }
        if (SHIFT_UP(SHIFT_RIGHT(pawn)) & enemyPieces)
        {
            attacks |= SHIFT_UP(SHIFT_RIGHT(pawn));
        }

        if (!(SHIFT_UP(pawn) & (friendlyPieces | enemyPieces)))
        {
            attacks |= SHIFT_UP(pawn);
            if (!(SHIFT_UP(SHIFT_UP(pawn)) & (friendlyPieces | enemyPieces)) && pawn <= SQUARE(7, 1))
            {
                attacks |= SHIFT_UP(SHIFT_UP(pawn));
                *metadata |= (1 << FILE(pawn));
            }
        }
    }
    else
    {
        if (SHIFT_DOWN(SHIFT_LEFT(pawn)) & enemyPieces)
        {
            attacks |= SHIFT_DOWN(SHIFT_LEFT(pawn));
        }
        if (SHIFT_DOWN(SHIFT_RIGHT(pawn)) & enemyPieces)
        {
            attacks |= SHIFT_DOWN(SHIFT_RIGHT(pawn));
        }

        if (!(SHIFT_DOWN(pawn) & (friendlyPieces | enemyPieces)))
        {
            attacks |= SHIFT_DOWN(pawn);
            if (!(SHIFT_DOWN(SHIFT_DOWN(pawn)) & (friendlyPieces | enemyPieces)) && pawn >= SQUARE(0, 6))
            {
                attacks |= SHIFT_DOWN(SHIFT_DOWN(pawn));
                *metadata |= 1 << (FILE(pawn) + 7);
            }
        }
    }
    return attacks;
}

move *getPawnMoves(game game)
{
    bitboard pawns = 0;
    bitboard enemyPieces = 0;
    bitboard friendlyPieces = 0;
    bool isWhite;
    if (game.metadata >> 7 & 1)
    {
        pawns = game.board.pawn & game.board.white;
        friendlyPieces = game.board.white;
        enemyPieces = getPieces(game.board) & ~game.board.white;
        isWhite = true;
    }
    else
    {
        pawns = game.board.pawn & ~game.board.white;
        friendlyPieces = getPieces(game.board) & ~game.board.white;
        enemyPieces = game.board.white;
        isWhite = false;
    }

    int maxMoves = numSignificantBits(pawns) * 4; // each pawn: up to 2 pushes + 2 captures
    move *moves = (move *)malloc((maxMoves + 1) * sizeof(move));
    int moveCount = 0;

    for (int pawnNum = 0; pawnNum < numSignificantBits(pawns); pawnNum++)
    {
        short activePawn = getNthSBit(pawns, pawnNum);
        short pawnFile = activePawn % 8;
        short pawnRank = activePawn / 8;
        bitboard pawn = 1ULL << activePawn;
        bitboard attacks = 0;
        attacks |= pawnMovement(pawn, enemyPieces, friendlyPieces, isWhite, &game.en_passants);
        bitboard validMoves = attacks;
        while (validMoves > 0)
        {
            int targetSq = trailingZeros(validMoves);
            moves[moveCount].original = (square){SQUARE_BIT(pawnFile, pawnRank)};
            moves[moveCount].next = (square){SQUARE_BIT(targetSq % 8, targetSq / 8)};
            moveCount++;
            validMoves &= validMoves - 1;
        }
    }

    moves[moveCount].original = (square){-1};
    moves[moveCount].next = (square){-1};
    return moves;
}

void executeMove(game *game, move move)
{

    if (game->board.white & 1ULL << move.original)
    {
        game->board.white ^= 1ULL << move.original;
        game->board.white ^= 1ULL << move.next;
    }

    if (game->board.queen & 1ULL << move.original)
    {
        game->board.queen ^= 1ULL << move.original;
        game->board.queen |= 1ULL << move.next;
    }
    else if (game->board.king & 1ULL << move.original)
    {
        game->board.king ^= 1ULL << move.original;
        game->board.king |= 1ULL << move.next;
    }
    else if (game->board.pawn & 1ULL << move.original)
    {
        game->board.pawn ^= 1ULL << move.original;
        game->board.pawn |= 1ULL << move.next;
    }
    else if (game->board.rook & 1ULL << move.original)
    {
        game->board.rook ^= 1ULL << move.original;
        game->board.rook |= 1ULL << move.next;
    }
    else if (game->board.bishop & 1ULL << move.original)
    {
        game->board.bishop ^= 1ULL << move.original;
        game->board.bishop |= 1ULL << move.next;
    }
    else if (game->board.knight & 1ULL << move.original)
    {
        game->board.knight ^= 1ULL << move.original;
        game->board.knight |= 1ULL << move.next;
    }

    game->metadata ^= 1 << 7;
}

void addMove(list_t *moveList, move move)
{
    if (moveList->head == 0)
    {
        moveList->head = (node *)malloc(sizeof(node));
        moveList->head->move = move;
        moveList->head->next = 0;
    }
    else
    {
        node *currentNode = moveList->head;
        while (currentNode->next != 0)
        {
            currentNode = currentNode->next;
        }
        currentNode->next = (node *)malloc(sizeof(node));
        currentNode->next->move = move;
        currentNode->next->next = 0;
    }
}

int getNumValidMoves(game game)
{
    int rval = 0;
    int count = 0;
    while ((getPawnMoves(game))[count].original != -1)
    {
        count++;
        rval++;
    }
    count = 0;
    while ((getKingMoves(game))[count].original != -1)
    {
        count++;
        rval++;
    }
    count = 0;
    while ((getRookMoves(game))[count].original != -1)
    {
        count++;
        rval++;
    }
    count = 0;
    while ((getKnightMoves(game))[count].original != -1)
    {
        count++;
        rval++;
    }
    count = 0;
    while ((getQueenMoves(game))[count].original != -1)
    {
        count++;
        rval++;
    }
    count = 0;
    while ((getBishopMoves(game))[count].original != -1)
    {
        count++;
        rval++;
    }
    count = 0;
    return rval;
}

move *getValidMoves(game game)
{
    move *rval = 0;
}
// *************************
// engine related operations
// *************************

int main(void)
{
    ChangeDirectory("/Applications/Developer/meowl");

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

    Texture2D boardTexture = LoadTexture("res/pieces-basic-png/rect-8x8.png");

    Texture2D textures[13] = {wk, wq, wr, wb, wn, wp, bk, bq, br, bb, bn, bp, boardTexture};
    game game = newGame();

    game.moves.head = 0;
    game.moves.foot = 0;

    move *moves = getPawnMoves(game);
    executeMove(&game, moves[1]);
    move *moves2 = getPawnMoves(game);
    executeMove(&game, moves2[3]);

    int moveCount = 0;
    while (moves2[moveCount].original != -1)
    {
        printf("%d :", moveCount);
        printMove(moves2[moveCount]);
        moveCount++;
    }

    while (!WindowShouldClose())
    {
        BeginDrawing();
        ClearBackground(WHITE);
        DrawTextureEx(boardTexture, (Vector2){0.0, 0.0}, 0, (float)(WINDOW_HEIGHT + WINDOW_WIDTH) / 1568, WHITE);
        renderBoard(game.board, textures);
        executeMove(&game, (move){SQUARE_BIT(0, 1), SQUARE_BIT(0, 3)});
        EndDrawing();
    }

    for (int i = 0; i < sizeof(textures) / sizeof(Texture2D); i++)
    {
        UnloadTexture(textures[i]);
    }

    CloseWindow();

    return 0;
}
