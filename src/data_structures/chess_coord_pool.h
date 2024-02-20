#ifndef H_CHESSMOVESCOLLECTION
#define H_CHESSMOVESCOLLECTION

#include "../chess_coord.h"

#define MAX_POSSIBLE_MOVES 27 //A queen on an empty board could move to at most 27 other positions

typedef struct ChessCoordPool{

    int length;
    ChessCoord chessCoords[MAX_POSSIBLE_MOVES];

} ChessCoordPool;

void ChessCoordPool_Init(ChessCoordPool* chessMovePool);
void ChessCoordPool_Reset(ChessCoordPool* chessCoordPool);
void ChessCoordPool_Add(ChessCoordPool* chessMovePool, int column, int row);

#endif