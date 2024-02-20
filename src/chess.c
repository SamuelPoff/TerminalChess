#include "chess.h"
#include "./data_structures/chess_coord_pool.h"

#include <stdlib.h>

void GetPawnMoves(enum CHESS_SIDE side, BoardState* boardState, ChessCoordPool* coordPool, int column, int row);
void GetKnightMoves(enum CHESS_SIDE side, BoardState* boardState, ChessCoordPool*, int column, int row);
void GetRookMoves(enum CHESS_SIDE side, BoardState* boardState, ChessCoordPool* coordPool, int column, int row);
void GetBishopMoves(enum CHESS_SIDE side, BoardState* boardState, ChessCoordPool* coordPool, int column, int row);
void GetQueenMoves(enum CHESS_SIDE side, BoardState* boardState, ChessCoordPool* coordPool, int column, int row);
void GetKingMoves(enum CHESS_SIDE side, BoardState* boardState, ChessCoordPool* coordPool, int column, int row);

void ChessPiece_Init(ChessPiece* piece, enum CHESS_SIDE side, enum CHESS_PIECE_TYPE type){
    piece->side = side;
    piece->type = type;
}

void ChessCoord_Init(ChessCoord* coord, int column, int row){
    coord->column = column;
    coord->row = row;
}

void ChessPiece_GetAvailableMoves(ChessPiece* piece, BoardState* boardState, ChessCoordPool* coordPool, int column, int row){

    ChessCoordPool_Reset(coordPool);

    switch(piece->type){

        case PAWN:
            GetPawnMoves(piece->side, boardState, coordPool, column, row);
            break;
        case KNIGHT:
            GetKnightMoves(piece->side, boardState, coordPool, column, row);
            break;
        case ROOK:
            GetRookMoves(piece->side, boardState, coordPool, column, row);
            break;
        case BISHOP:
            GetBishopMoves(piece->side, boardState, coordPool, column, row);
            break;
        case QUEEN:
            GetQueenMoves(piece->side, boardState, coordPool, column, row);
            break;
        case KING:
            GetKingMoves(piece->side, boardState, coordPool, column, row);
            break;

    }

}

void GetPawnMoves(enum CHESS_SIDE side, BoardState* boardState, ChessCoordPool* coordPool, int column, int row){

    int direction = 1;
    if(side == WHITE) direction = -1;

    ChessCoordPool_Reset(coordPool);

    int rowBeingChecked = row+direction;
    //Pawn cant move past the end of the board duh
    if(rowBeingChecked >= 8 || rowBeingChecked < 0){
        coordPool->length = 0;
        return;
    }

    //Check space in front of pawn
    int nextSpaceIndex = GetBoardIndexFromColumnRow(column, rowBeingChecked);
    if(boardState->board[nextSpaceIndex].type == NONE){
        ChessCoordPool_Add(coordPool, column, rowBeingChecked);
    }

    //Check 2 spaces in front of pawn if the pawn is at its starting position
    if((side==WHITE && row == 6) || (side==BLACK && row == 1)){
        rowBeingChecked = row+direction*2;
        nextSpaceIndex = GetBoardIndexFromColumnRow(column, rowBeingChecked);
        if(boardState->board[nextSpaceIndex].type == NONE){
            ChessCoordPool_Add(coordPool, column, rowBeingChecked);
        }
    }

    //Check pawn attacking locations
    rowBeingChecked = row+direction;
    int sideToCheck = 1;
    if(column+sideToCheck < 8){
        nextSpaceIndex = GetBoardIndexFromColumnRow(column+sideToCheck, rowBeingChecked);
        if(boardState->board[nextSpaceIndex].side == OppositeChessSide(side) && boardState->board[nextSpaceIndex].type!=NONE){
            ChessCoordPool_Add(coordPool, column+sideToCheck, rowBeingChecked);
        }
    }

    sideToCheck = -1;
    if(column+sideToCheck >= 0){
        nextSpaceIndex = GetBoardIndexFromColumnRow(column+sideToCheck, rowBeingChecked);
        if(boardState->board[nextSpaceIndex].side == OppositeChessSide(side) && boardState->board[nextSpaceIndex].type!=NONE){
            ChessCoordPool_Add(coordPool, column+sideToCheck, rowBeingChecked);
        }
    }


}
void GetKnightMoves(enum CHESS_SIDE side, BoardState* boardState, ChessCoordPool* coordPool, int column, int row){

    int vertical = 2;
    int horizontal = 1;

    //Contains the offset of every space a knight needs to check
    ChessCoord spacesToCheck[8];
    ChessCoord_Init(&spacesToCheck[0], 2, 1);
    ChessCoord_Init(&spacesToCheck[1], 1, 2);
    ChessCoord_Init(&spacesToCheck[2], 2, -1);
    ChessCoord_Init(&spacesToCheck[3], 1, -2);
    ChessCoord_Init(&spacesToCheck[4], -2, -1);
    ChessCoord_Init(&spacesToCheck[5], -1, -2);
    ChessCoord_Init(&spacesToCheck[6], -2, 1);
    ChessCoord_Init(&spacesToCheck[7], -1, 2);

    enum CHESS_SIDE oppositeSide = OppositeChessSide(side);

    for(int i = 0; i < 8; i++){

        int columnToCheck = column + spacesToCheck[i].column;
        int rowToCheck = row + spacesToCheck[i].row;

        if(columnToCheck < 0 || columnToCheck >= 8 || rowToCheck < 0 || rowToCheck >= 8){
            continue;
        }

        int spaceToCheckIndex = GetBoardIndexFromColumnRow(columnToCheck, rowToCheck);
        if(boardState->board[spaceToCheckIndex].type == NONE){
            ChessCoordPool_Add(coordPool, columnToCheck, rowToCheck);
        }
        else if(boardState->board[spaceToCheckIndex].side == oppositeSide){
            ChessCoordPool_Add(coordPool, columnToCheck, rowToCheck);
        }

    }

}
void GetRookMoves(enum CHESS_SIDE side, BoardState* boardState, ChessCoordPool* coordPool, int column, int row){

    //Check everything down
    for(int i = row+1; i < 8; i++){
        int spaceToCheckIndex = GetBoardIndexFromColumnRow(column, i);
        if(boardState->board[spaceToCheckIndex].type == NONE){
            ChessCoordPool_Add(coordPool, column, i);
        }
        else if(boardState->board[spaceToCheckIndex].side == side){
            break;
        }
        else{
            ChessCoordPool_Add(coordPool, column, i);
            break;
        }
    }

    //Check everything up
    for(int i = row-1; i >= 0; i--){
        int spaceToCheckIndex = GetBoardIndexFromColumnRow(column, i);
        if(boardState->board[spaceToCheckIndex].type == NONE){
            ChessCoordPool_Add(coordPool, column, i);
        }
        else if(boardState->board[spaceToCheckIndex].side == side){
            break;
        }
        else{
            ChessCoordPool_Add(coordPool, column, i);
            break;
        }
    }

    //Check everything right
    for(int i = column+1; i < 8; i++){
        int spaceToCheckIndex = GetBoardIndexFromColumnRow(i, row);
        if(boardState->board[spaceToCheckIndex].type == NONE){
            ChessCoordPool_Add(coordPool, i, row);
        }
        else if(boardState->board[spaceToCheckIndex].side == side){
            break;
        }
        else{
            ChessCoordPool_Add(coordPool, i, row);
            break;
        }
    }

    //Check everything down
    for(int i = column-1; i >= 0; i--){
        int spaceToCheckIndex = GetBoardIndexFromColumnRow(i, row);
        if(boardState->board[spaceToCheckIndex].type == NONE){
            ChessCoordPool_Add(coordPool, i, row);
        }
        else if(boardState->board[spaceToCheckIndex].side == side){
            break;
        }
        else{
            ChessCoordPool_Add(coordPool, i, row);
            break;
        }
    }

}
void GetBishopMoves(enum CHESS_SIDE side, BoardState* boardState, ChessCoordPool* coordPool, int column, int row){

    int columnToCheck = column+1;
    int rowToCheck = row+1;
    while(columnToCheck >= 0 && columnToCheck < 8 && rowToCheck >= 0 && rowToCheck < 8){
        int boardIndex = GetBoardIndexFromColumnRow(columnToCheck, rowToCheck);
        if(boardState->board[boardIndex].type == NONE){
            ChessCoordPool_Add(coordPool, columnToCheck, rowToCheck);
        }
        else{
            if(boardState->board[boardIndex].side != side){
                ChessCoordPool_Add(coordPool, columnToCheck, rowToCheck);
            }
            break;
        }
        columnToCheck++;
        rowToCheck++;
    }
    columnToCheck = column-1;
    rowToCheck = row+1;
    while(columnToCheck >= 0 && columnToCheck < 8 && rowToCheck >= 0 && rowToCheck < 8){
        int boardIndex = GetBoardIndexFromColumnRow(columnToCheck, rowToCheck);
        if(boardState->board[boardIndex].type == NONE){
            ChessCoordPool_Add(coordPool, columnToCheck, rowToCheck);
        }
        else{
            if(boardState->board[boardIndex].side != side){
                ChessCoordPool_Add(coordPool, columnToCheck, rowToCheck);
            }
            break;
        }
        columnToCheck--;
        rowToCheck++;
    }
    columnToCheck = column+1;
    rowToCheck = row-1;
    while(columnToCheck >= 0 && columnToCheck < 8 && rowToCheck >= 0 && rowToCheck < 8){
        int boardIndex = GetBoardIndexFromColumnRow(columnToCheck, rowToCheck);
        if(boardState->board[boardIndex].type == NONE){
            ChessCoordPool_Add(coordPool, columnToCheck, rowToCheck);
        }
        else{
            if(boardState->board[boardIndex].side != side){
                ChessCoordPool_Add(coordPool, columnToCheck, rowToCheck);
            }
            break;
        }
        columnToCheck++;
        rowToCheck--;
    }
    columnToCheck = column-1;
    rowToCheck = row-1;
    while(columnToCheck >= 0 && columnToCheck < 8 && rowToCheck >= 0 && rowToCheck < 8){
        int boardIndex = GetBoardIndexFromColumnRow(columnToCheck, rowToCheck);
        if(boardState->board[boardIndex].type == NONE){
            ChessCoordPool_Add(coordPool, columnToCheck, rowToCheck);
        }
        else{
            if(boardState->board[boardIndex].side != side){
                ChessCoordPool_Add(coordPool, columnToCheck, rowToCheck);
            }
            break;
        }
        columnToCheck--;
        rowToCheck--;
    }

}
void GetQueenMoves(enum CHESS_SIDE side, BoardState* boardState, ChessCoordPool* coordPool, int column, int row){

    GetRookMoves(side, boardState, coordPool, column, row);
    GetBishopMoves(side, boardState, coordPool, column, row);

}
void GetKingMoves(enum CHESS_SIDE side, BoardState* boardState, ChessCoordPool* coordPool, int column, int row){
    
    ChessCoord coordOffsets[8];
    ChessCoord_Init(&coordOffsets[0], 1, 0);
    ChessCoord_Init(&coordOffsets[1], 1, 1);
    ChessCoord_Init(&coordOffsets[2], 0, 1);
    ChessCoord_Init(&coordOffsets[3], -1, 1);
    ChessCoord_Init(&coordOffsets[4], -1, 0);
    ChessCoord_Init(&coordOffsets[5], -1, -1);
    ChessCoord_Init(&coordOffsets[6], 0, -1);
    ChessCoord_Init(&coordOffsets[7], 1, -1);

    for(int i = 0; i < 8; i++){

        int columnToCheck = coordOffsets[i].column+column;
        int rowToCheck = coordOffsets[i].row+row;
        if(columnToCheck < 0 || columnToCheck >= 8 || rowToCheck < 0 || rowToCheck >= 8){
            continue;
        }

        int boardIndex = GetBoardIndexFromColumnRow(columnToCheck, rowToCheck);
        ChessPiece piece = boardState->board[boardIndex];
        if(piece.type != NONE && piece.side == side){
            continue;
        }

        ChessCoordPool_Add(coordPool, columnToCheck, rowToCheck);
        
    }

}

enum CHESS_SIDE OppositeChessSide(enum CHESS_SIDE side){

    return (enum CHESS_SIDE)!side;

}

int GetBoardIndexFromColumnRow(int column, int row){
    return column + (row*8);
}

void SetBoardPieceSide(BoardState* boardState, int column, int row, enum CHESS_SIDE side){

    int index = GetBoardIndexFromColumnRow(column, row);
    boardState->board[index].side = side;

}

void SetBoardPieceType(BoardState* boardState, int column, int row, enum CHESS_PIECE_TYPE type){

    int index = GetBoardIndexFromColumnRow(column, row);
    boardState->board[index].type = type;

}
