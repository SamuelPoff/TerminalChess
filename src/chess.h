#ifndef H_CHESS_PIECES
#define H_CHESS_PIECES

#include "chess_coord.h"
#include "./data_structures/chess_coord_pool.h"

enum CHESS_SIDE{
    WHITE,
    BLACK
};

enum CHESS_PIECE_TYPE{
    NONE,
    PAWN,
    KNIGHT,
    ROOK,
    BISHOP,
    QUEEN,
    KING
};

typedef struct ChessPiece{
    enum CHESS_SIDE side;
    enum CHESS_PIECE_TYPE type;
} ChessPiece;

typedef struct BoardState{

    int x;
    int y;
    int width;
    int height;

    ChessPiece board[64];

} BoardState;

typedef struct ChessMove{
    int fromCol;
    int fromRow;
    int toCol;
    int toRow;
    enum CHESS_SIDE toSide;
    enum CHESS_PIECE_TYPE toType;

} ChessMove;

int GetBoardIndexFromColumnRow(int column, int row);
void SetBoardPieceType(BoardState* boardState, int column, int row, enum CHESS_PIECE_TYPE type);
void SetBoardPieceSide(BoardState* boardState, int column, int row, enum CHESS_SIDE side);

void ChessPiece_Init(ChessPiece* piece, enum CHESS_SIDE side, enum CHESS_PIECE_TYPE type);
void ChessPiece_GetAvailableMoves(ChessPiece* piece, BoardState* boardState, ChessCoordPool* coordPool, int column, int row);

void ChessCoord_Init(ChessCoord* coord, int column, int row);

enum CHESS_SIDE OppositeChessSide(enum CHESS_SIDE side);

#endif