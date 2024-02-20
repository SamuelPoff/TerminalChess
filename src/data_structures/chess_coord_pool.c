#include "chess_coord_pool.h"

void ChessCoordPool_Init(ChessCoordPool* chessCoordPool){

    chessCoordPool->length = 0;
    for(int i = 0; i < MAX_POSSIBLE_MOVES; ++i){
        chessCoordPool->chessCoords[i].column = 0;
        chessCoordPool->chessCoords[i].row = 0;
    }

}

void ChessCoordPool_Reset(ChessCoordPool* chessCoordPool){
    chessCoordPool->length = 0;
}

void ChessCoordPool_Add(ChessCoordPool* chessCoordPool, int column, int row){

    chessCoordPool->chessCoords[chessCoordPool->length].column = column;
    chessCoordPool->chessCoords[chessCoordPool->length].row = row;
    chessCoordPool->length++;

}