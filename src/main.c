#define WIN32_LEAN_AND_MEAN

#include <stdio.h>
#include <stdlib.h>

#include <signal.h>

#include "chess.h"
#include "data_structures/chess_coord_pool.h"
#include "terminal_control.h"
#include "ansi_colors.h"

#include <windows.h>
#include <WinUser.h>
#include <winsock2.h>
#include <ws2tcpip.h>

#pragma comment (lib, "Ws2_32.lib")
#define DEFAULT_PORT "27015"
#define DEFAULT_BUFFER_LEN 256

#define CHECKER_WIDTH 6
#define CHECKER_HEIGHT 3

#define CHECKER_STRING "      \x1B[1B\x1B[6D      \x1B[1B\x1B[6D      "
#define PAWN_STRING "  ()  \x1B[1B\x1B[6D  ||  \x1B[1B\x1B[6D [  ] "
#define KNIGHT_STRING " /``) \x1B[1B\x1B[6D/_- | \x1B[1B\x1B[6D[    ]"
#define ROOK_STRING "|-||-|\x1B[1B\x1B[6D |  | \x1B[1B\x1B[6D[    ]"
#define BISHOP_STRING " _()_ \x1B[1B\x1B[6D  ||  \x1B[1B\x1B[6D_[||]_"
#define QUEEN_STRING " ~**~ \x1B[1B\x1B[6D  ||  \x1B[1B\x1B[6D[    ]"
#define KING_STRING " -ll- \x1B[1B\x1B[6D  ||  \x1B[1B\x1B[6D[    ]"

DWORD baseStdoutMode;
DWORD baseStdinMode;

int running = 1;
int networkGame = FALSE;
int connectionClosedFlag = FALSE;

enum CHESS_SIDE side = WHITE;
enum CHESS_SIDE activeSide = WHITE;

const int AVAILABLE_MOVE_COLOR = ANSI_COLOR_ID_FADED_MAG;
const int DEFAULT_BLACK = ANSI_COLOR_ID_LIGHT_BLK;
const int DEFAULT_WHITE = ANSI_COLOR_ID_DARK_WHT;

HANDLE wHnd; //Console write handle
HANDLE rHnd; //Console read handle

BoardState boardState;
ChessCoordPool availableMovePool;

int pieceSelected = 0;
ChessPiece* selectedPiece = NULL;
int selectedPieceColumn = 0;
int selectedPieceRow = 0;

int selectedColumn = 0;
int selectedRow = 0;

int terminalColumns = 0;
int terminalRows = 0;

SOCKET peerSocket = INVALID_SOCKET;

void RedrawScreen(int terminalColumns, int terminalRows);
void PrintBoard();
void PrintInfoBar(int row);
void PrintChecker(int column, int row, int bgColor, int textColor);
void PrintAvailableMoveSpaces();
void HandleInput(KEY_EVENT_RECORD keyEvent);

int CalculateBoardStartingColumn(int terminalColumns);
int CalculateBoardStartingRow(int terminalRows);
int GetCheckerNormalBGColor(int column, int row);
int GetCheckerNormalTextColor(int column, int row);
int GetCheckerPosX(int column);
int GetCheckerPosY(int row);

void SetupGame();
void SetupBoardPieces();
void LocalGame();
void HostGame();
void JoinGame();

void SetupWinsock();
void ResetConsole();

int main(int argc, char** argv){

    wHnd = GetStdHandle(STD_OUTPUT_HANDLE);
    rHnd = GetStdHandle(STD_INPUT_HANDLE);

    //Get the console mode for re-establishing it when the program closes
    if(!GetConsoleMode(wHnd, &baseStdoutMode)){
        printf("Could not save console state\n");
    }
    if(GetConsoleMode(rHnd, &baseStdinMode)){
        printf("Could not save console state\n");
    }

    printf("CHESS\n");
    printf("1. Local Game\n");
    printf("2. Host Game\n");
    printf("3. Join Game\n");

    char c = ' ';
    do{
        c = getchar();
    }
    while(c < 49 || c > 51); //less than 1 and greater than 3 in ascii codes

    switch(c){
        case '1':
            LocalGame();
            break;
        case '2':
            HostGame();
            break;
        case '3':
            JoinGame();
            break;
    }

    tc_cursor_to_home();
    tc_clear_screen();
    tc_reset_style();
    ResetConsole();
    tc_cursor_to_home();

    if(connectionClosedFlag){
        printf("\nOpponent disconnected :(\n");
    }

    return 0;

}

void LocalGame(){

    SetupGame();

    running = 1;
    INPUT_RECORD inputRecord;

    while(running){
        
        long unsigned numberOfEvents;
        long unsigned eventsRead;
        numberOfEvents = 0;
        GetNumberOfConsoleInputEvents(rHnd, &numberOfEvents);

        if(numberOfEvents > 0){
            for(int i = 0; i < numberOfEvents;i++){
                ReadConsoleInput(rHnd, &inputRecord, 1, &eventsRead);
                
                switch(inputRecord.EventType){
                    case WINDOW_BUFFER_SIZE_EVENT:
                        terminalColumns = inputRecord.Event.WindowBufferSizeEvent.dwSize.X;
                        terminalRows = inputRecord.Event.WindowBufferSizeEvent.dwSize.Y;
                        boardState.x = CalculateBoardStartingColumn(terminalColumns);
                        boardState.y = CalculateBoardStartingRow(terminalRows);
                        RedrawScreen(inputRecord.Event.WindowBufferSizeEvent.dwSize.X, inputRecord.Event.WindowBufferSizeEvent.dwSize.Y);
                        break;
                    case KEY_EVENT:
                        HandleInput(inputRecord.Event.KeyEvent);
                        break;
                }
                
            }
        }
    }

}
void HostGame(){

    SetupWinsock();

    int result;

    struct addrinfo *addrResult=NULL, hints;
    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_flags = AI_PASSIVE;
    result = getaddrinfo(NULL, DEFAULT_PORT, &hints, &addrResult);
    if(result != 0){
        printf("Error at getaddrinfo() ERROR CODE: %d\n", result);
        WSACleanup();
        ResetConsole();
        exit(1);
    }

    //Setup listener socket
    SOCKET listenerSocket = INVALID_SOCKET;
    listenerSocket = socket(addrResult->ai_family, addrResult->ai_socktype, addrResult->ai_protocol);
    if(listenerSocket == INVALID_SOCKET){
        printf("Error while creating listenerSocket: %ld\n", WSAGetLastError());
        WSACleanup();
        freeaddrinfo(addrResult);
        ResetConsole();
        exit(1);
    }

    //Bind listenerSocket
    result = bind(listenerSocket, addrResult->ai_addr, (int)addrResult->ai_addrlen);
    if(result == SOCKET_ERROR){
        printf("Error while binding listenerSocket: %d\n", WSAGetLastError());
        WSACleanup();
        freeaddrinfo(addrResult);
        ResetConsole();
        exit(1);
    }
    freeaddrinfo(addrResult);

    
    printf("Listening on PORT: %s...\n", DEFAULT_PORT);
    if(listen(listenerSocket, 4) == SOCKET_ERROR){
        printf("listen() failed with error: %d\n", WSAGetLastError());
        closesocket(listenerSocket);
        WSACleanup();
        ResetConsole();
        exit(1);
    }

    //Create client socket
    SOCKET clientSocket = INVALID_SOCKET;
    clientSocket = accept(listenerSocket, NULL, NULL);
    if(clientSocket == INVALID_SOCKET){
        printf("Error occured accepting the client socket: %d\n", WSAGetLastError());
        closesocket(listenerSocket);
        WSACleanup();
        ResetConsole();
        exit(1);
    }

    printf("Connected to client successfully!\n");
    SetupGame();

    peerSocket = clientSocket;
    networkGame = TRUE;
    running = TRUE;
    INPUT_RECORD inputRecord;

    int sendResult, recvResult, selectResult;
    char sendBuffer[DEFAULT_BUFFER_LEN];
    ZeroMemory(sendBuffer, DEFAULT_BUFFER_LEN);
    char recvBuffer[DEFAULT_BUFFER_LEN];
    ZeroMemory(recvBuffer, DEFAULT_BUFFER_LEN);

    fd_set fdRead;
    fd_set fdWrite;

    struct timeval timeInterval;
    timeInterval.tv_sec = 0;
    timeInterval.tv_usec = 60;

    while(running){
        
        //Console event handling
        long unsigned numberOfEvents;
        long unsigned eventsRead;
        numberOfEvents = 0;
        GetNumberOfConsoleInputEvents(rHnd, &numberOfEvents);
        if(numberOfEvents > 0){
            for(int i = 0; i < numberOfEvents;i++){
                ReadConsoleInput(rHnd, &inputRecord, 1, &eventsRead);
                
                switch(inputRecord.EventType){
                    case WINDOW_BUFFER_SIZE_EVENT:
                        terminalColumns = inputRecord.Event.WindowBufferSizeEvent.dwSize.X;
                        terminalRows = inputRecord.Event.WindowBufferSizeEvent.dwSize.Y;
                        boardState.x = CalculateBoardStartingColumn(terminalColumns);
                        boardState.y = CalculateBoardStartingRow(terminalRows);
                        RedrawScreen(inputRecord.Event.WindowBufferSizeEvent.dwSize.X, inputRecord.Event.WindowBufferSizeEvent.dwSize.Y);
                        break;
                    case KEY_EVENT:
                        HandleInput(inputRecord.Event.KeyEvent);
                        break;
                }
                
            }
        }
        //Console event handling

        //Network handling
        FD_ZERO(&fdRead);
        FD_ZERO(&fdWrite);
        FD_SET(clientSocket, &fdRead);
        FD_SET(clientSocket, &fdWrite);

        if((selectResult = select(0, &fdRead, &fdWrite, NULL, &timeInterval)) == SOCKET_ERROR){
            printf("Error occured on select(): %d\n", WSAGetLastError());
            closesocket(clientSocket);
            WSACleanup();
            exit(1);
        }
        if(selectResult > 0){
            if(FD_ISSET(clientSocket, &fdRead)){
                //Something to recv
                ZeroMemory(recvBuffer, DEFAULT_BUFFER_LEN);
                recvResult = recv(clientSocket, recvBuffer, DEFAULT_BUFFER_LEN, 0);
                if(recvResult > 0){
                    //Chess move
                    
                    ChessMove *move = (ChessMove*)recvBuffer;
                    SetBoardPieceType(&boardState, move->fromCol, move->fromRow, NONE);
                    SetBoardPieceType(&boardState, move->toCol, move->toRow, move->toType);
                    SetBoardPieceSide(&boardState, move->toCol, move->toRow, move->toSide);
                    activeSide = OppositeChessSide(activeSide);
                    PrintBoard();
                    PrintInfoBar(terminalRows);
                }
                else if(recvResult == 0){
                    connectionClosedFlag = TRUE;
                    running = FALSE;
                }
                else{
                    printf("Error occured at recv(): %d\n", WSAGetLastError());
                    closesocket(clientSocket);
                    WSACleanup();
                    tc_clear_screen();
                    tc_reset_style();
                    ResetConsole();
                    exit(1);
                }
            }
            if(FD_ISSET(clientSocket, &fdWrite)){
                //client is available to send to
            }
        }
        //Network handling
    }

    WSACleanup();
    closesocket(listenerSocket);
    closesocket(clientSocket);

}
void JoinGame(){

    SetupWinsock();
    side = BLACK;
    
    //Flush stdin
    int c;
    while((c=getchar()) != '\n' && c != EOF);
    
    char ipBuffer[128];
    const int ipBufferLen = 128;
    ZeroMemory(ipBuffer, ipBufferLen);

    printf("Enter IP to connect to: ");
    fgets(ipBuffer, ipBufferLen, stdin);

    char* replaceAddr = strchr(ipBuffer, '\n');
    *replaceAddr = '\0';

    printf("\nEntered ip address: %s\n", ipBuffer);

    int result;

    struct addrinfo *addrResult=NULL, *ptr, hints;
    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    result = getaddrinfo(ipBuffer, DEFAULT_PORT, &hints, &addrResult);
    if(result != 0){
        printf("Error at getaddrinfo() ERROR CODE: %d\n", result);
        WSACleanup();
        ResetConsole();
        exit(1);
    }

    //Attempt to connect to first address returned by getaddrinfo call
    SOCKET connectSocket = INVALID_SOCKET;
    ptr = addrResult;
    connectSocket = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);
    if(connectSocket == INVALID_SOCKET){
        printf("Error while creating connection socket: %ld\n", WSAGetLastError());
        WSACleanup();
        freeaddrinfo(addrResult);
        ResetConsole();
        exit(1);
    }

    result = connect(connectSocket, ptr->ai_addr, (int)ptr->ai_addrlen);
    if(result == SOCKET_ERROR){
        
        closesocket(connectSocket);
        connectSocket = INVALID_SOCKET;

    }
    //NOTE!!!!!!!!!!
    //You should try the next address here but for now it will simply free resources and return if this fails!!!
    freeaddrinfo(addrResult);

    if(connectSocket == INVALID_SOCKET){
        printf("Could not connect to server...\n");
        WSACleanup();
        ResetConsole();
        exit(1);
    }

    SetupGame();

    peerSocket = connectSocket;
    networkGame = TRUE;
    running = TRUE;
    INPUT_RECORD inputRecord;

    int sendResult, recvResult, selectResult;
    char sendBuffer[DEFAULT_BUFFER_LEN];
    ZeroMemory(sendBuffer, DEFAULT_BUFFER_LEN);
    char recvBuffer[DEFAULT_BUFFER_LEN];
    ZeroMemory(recvBuffer, DEFAULT_BUFFER_LEN);

    fd_set fdRead;
    fd_set fdWrite;

    struct timeval timeInterval;
    timeInterval.tv_sec = 0;
    timeInterval.tv_usec = 60;

    while(running){
        
        //Console event handling
        long unsigned numberOfEvents;
        long unsigned eventsRead;
        numberOfEvents = 0;
        GetNumberOfConsoleInputEvents(rHnd, &numberOfEvents);
        if(numberOfEvents > 0){
            for(int i = 0; i < numberOfEvents;i++){
                ReadConsoleInput(rHnd, &inputRecord, 1, &eventsRead);
                
                switch(inputRecord.EventType){
                    case WINDOW_BUFFER_SIZE_EVENT:
                        terminalColumns = inputRecord.Event.WindowBufferSizeEvent.dwSize.X;
                        terminalRows = inputRecord.Event.WindowBufferSizeEvent.dwSize.Y;
                        boardState.x = CalculateBoardStartingColumn(terminalColumns);
                        boardState.y = CalculateBoardStartingRow(terminalRows);
                        RedrawScreen(inputRecord.Event.WindowBufferSizeEvent.dwSize.X, inputRecord.Event.WindowBufferSizeEvent.dwSize.Y);
                        break;
                    case KEY_EVENT:
                        HandleInput(inputRecord.Event.KeyEvent);
                        break;
                }
                
            }
        }
        //Console event handling

        //Network handling
        FD_ZERO(&fdRead);
        FD_ZERO(&fdWrite);
        FD_SET(connectSocket, &fdRead);
        FD_SET(connectSocket, &fdWrite);

        if((selectResult = select(0, &fdRead, &fdWrite, NULL, &timeInterval)) == SOCKET_ERROR){
            printf("Error occured on select(): %d\n", WSAGetLastError());
            closesocket(connectSocket);
            WSACleanup();
            ResetConsole();
            exit(1);
        }
        if(selectResult > 0){
            if(FD_ISSET(connectSocket, &fdRead)){
                //Something to recv
                ZeroMemory(recvBuffer, DEFAULT_BUFFER_LEN);
                recvResult = recv(connectSocket, recvBuffer, DEFAULT_BUFFER_LEN, 0);
                if(recvResult > 0){
                    //Chess move
                    
                    ChessMove *move = (ChessMove*)recvBuffer;
                    SetBoardPieceType(&boardState, move->fromCol, move->fromRow, NONE);
                    SetBoardPieceType(&boardState, move->toCol, move->toRow, move->toType);
                    SetBoardPieceSide(&boardState, move->toCol, move->toRow, move->toSide);
                    activeSide = OppositeChessSide(activeSide);
                    PrintBoard();
                    PrintInfoBar(terminalRows);
                    
                }
                else if(recvResult == 0){
                    connectionClosedFlag = TRUE;
                    running = FALSE;
                }
                else{
                    printf("Error occured at recv(): %d\n", WSAGetLastError());
                    closesocket(connectSocket);
                    WSACleanup();
                    tc_cursor_to_home();
                    tc_clear_screen();
                    tc_reset_style();
                    tc_cursor_to_home();
                    ResetConsole();
                    exit(1);
                }
            }
            if(FD_ISSET(connectSocket, &fdWrite)){
                //client is available to send to
            }
        }
        //Network handling
    }
    
    closesocket(connectSocket);
    WSACleanup();

}

void SetupWinsock(){
    WSADATA wsaData;
    int result = 0;
    if( (result = WSAStartup(MAKEWORD(2,2), &wsaData)) != 0){
        printf("Error at WSAStartup() ERROR CODE: %d\n", result);
        ResetConsole();
        exit(1);
    }
}

void HandleInput(KEY_EVENT_RECORD keyEvent){

    if(keyEvent.bKeyDown){

        switch(keyEvent.wVirtualKeyCode){
            case VK_ESCAPE:
                tc_clear_screen();
                tc_cursor_to_home();
                running = 0;
                break;
            case VK_UP:
                if(selectedRow - 1 >= 0){
                    PrintChecker(selectedColumn, selectedRow, GetCheckerNormalBGColor(selectedColumn, selectedRow), GetCheckerNormalTextColor(selectedColumn, selectedRow));
                    if(pieceSelected) PrintAvailableMoveSpaces();
                    selectedRow--;
                    PrintChecker(selectedColumn, selectedRow, ANSI_COLOR_ID_FADED_CYN, ANSI_COLOR_ID_BLK);
                    PrintInfoBar(terminalRows);
                }
                break;
            case VK_DOWN:
                if((selectedRow + 1) < 8){
                    PrintChecker(selectedColumn, selectedRow, GetCheckerNormalBGColor(selectedColumn, selectedRow), GetCheckerNormalTextColor(selectedColumn, selectedRow));
                    if(pieceSelected) PrintAvailableMoveSpaces();
                    selectedRow++;
                    PrintChecker(selectedColumn, selectedRow, ANSI_COLOR_ID_FADED_CYN, ANSI_COLOR_ID_BLK);
                    PrintInfoBar(terminalRows);
                }
                break;
            case VK_RIGHT:
                if((selectedColumn + 1) < 8){
                    PrintChecker(selectedColumn, selectedRow, GetCheckerNormalBGColor(selectedColumn, selectedRow), GetCheckerNormalTextColor(selectedColumn, selectedRow));
                    if(pieceSelected) PrintAvailableMoveSpaces();
                    selectedColumn++;
                    PrintChecker(selectedColumn, selectedRow, ANSI_COLOR_ID_FADED_CYN, ANSI_COLOR_ID_BLK);
                    PrintInfoBar(terminalRows);
                }
                break;
            case VK_LEFT:
                if(selectedColumn - 1 >= 0){
                    PrintChecker(selectedColumn, selectedRow, GetCheckerNormalBGColor(selectedColumn, selectedRow), GetCheckerNormalTextColor(selectedColumn, selectedRow));
                    if(pieceSelected) PrintAvailableMoveSpaces();
                    selectedColumn--;
                    PrintChecker(selectedColumn, selectedRow, ANSI_COLOR_ID_FADED_CYN, ANSI_COLOR_ID_BLK);
                    PrintInfoBar(terminalRows);
                }
                break;
            case VK_SPACE:
                if(networkGame){
                    if(activeSide != side){
                        break;
                    }
                }
                if(!pieceSelected){
                    int currentIndex = GetBoardIndexFromColumnRow(selectedColumn, selectedRow);
                    if(boardState.board[currentIndex].type != NONE){
                        
                        ChessPiece_GetAvailableMoves(&boardState.board[currentIndex], &boardState, &availableMovePool, selectedColumn, selectedRow);
                        PrintAvailableMoveSpaces();

                        pieceSelected = TRUE;
                        selectedPiece = &boardState.board[currentIndex];
                        selectedPieceColumn = selectedColumn;
                        selectedPieceRow = selectedRow;

                        PrintInfoBar(terminalRows);
                    }
                }
                else{

                    if(selectedColumn == selectedPieceColumn && selectedRow == selectedPieceRow){
                        pieceSelected = FALSE;
                        selectedPiece = NULL;
                        selectedPieceColumn = 0;
                        selectedPieceRow = 0;
                        PrintBoard();
                        PrintInfoBar(terminalRows);
                    }

                    int selectedPieceIndex = GetBoardIndexFromColumnRow(selectedPieceColumn, selectedPieceRow);
                    if(boardState.board[selectedPieceIndex].side != activeSide){
                        break;
                    }

                    int currentIndex = GetBoardIndexFromColumnRow(selectedColumn, selectedRow);

                    int validMove = FALSE;
                    for(int i = 0;i < availableMovePool.length; i++){
                        if(availableMovePool.chessCoords[i].column == selectedColumn && availableMovePool.chessCoords[i].row == selectedRow){
                            validMove = TRUE;
                            break;
                        }
                    }

                    if(validMove){
                        if(networkGame){
                            
                            ChessMove move;
                            move.fromCol = selectedPieceColumn;
                            move.fromRow = selectedPieceRow;
                            move.toCol = selectedColumn;
                            move.toRow = selectedRow;
                            move.toSide = selectedPiece->side;
                            move.toType = selectedPiece->type;

                            char* moveData = (char*)&move;

                            int sendResult = send(peerSocket, moveData, sizeof(move), 0);
                            if(sendResult == SOCKET_ERROR){
                                printf("Error occured at send(): %d", WSAGetLastError());
                                ResetConsole();
                                exit(1);
                            }
                        }
                        SetBoardPieceType(&boardState, selectedColumn, selectedRow, selectedPiece->type);
                        SetBoardPieceSide(&boardState, selectedColumn, selectedRow, selectedPiece->side);
                        SetBoardPieceType(&boardState, selectedPieceColumn, selectedPieceRow, NONE);
                        selectedPiece = NULL;
                        selectedPieceColumn = 0;
                        selectedPieceRow = 0;
                        pieceSelected = FALSE;
                        activeSide = OppositeChessSide(activeSide);
                        PrintBoard();
                        PrintInfoBar(terminalRows);
                    }
                }
                break;
            
        }

    }

}

void RedrawScreen(int terminalColumns, int terminalRows){

    tc_clear_screen();
    PrintBoard();
    PrintInfoBar(terminalRows);
    tc_hide_cursor();

}

int CalculateBoardStartingColumn(int terminalColumns){
    return terminalColumns/2 - (CHECKER_WIDTH*8)/2;
}

int CalculateBoardStartingRow(int terminalRows){
    return terminalRows/2 - (CHECKER_HEIGHT*8)/2;
}

void PrintBoard(){

    int boardColumn = 0, boardRow = 0;
    for(int i = 0; i < 64; i++){

        boardColumn = i % 8;
        boardRow = i / 8;

        int textColor = GetCheckerNormalTextColor(boardColumn, boardRow);
        int bgColor = GetCheckerNormalBGColor(boardColumn, boardRow);

        if(boardRow == selectedRow && boardColumn == selectedColumn){
            bgColor = ANSI_COLOR_ID_FADED_CYN;
        }

        PrintChecker(boardColumn, boardRow, bgColor, textColor);
        
    }

    //Print board locations on the side
    tc_set_text_color(DEFAULT_WHITE);
    for(int i = 0; i < 8;i++){
        int posX = GetCheckerPosX(-1);
        int posY = GetCheckerPosY(i);
        tc_set_cursor_position(posX+CHECKER_WIDTH/2, posY+CHECKER_HEIGHT/2);
        printf("%d", 8-i);
    }
    for(int i = 0; i < 8;i++){
        int posX = GetCheckerPosX(i);
        int posY = GetCheckerPosY(8);
        tc_set_cursor_position(posX+CHECKER_WIDTH/2, posY+CHECKER_HEIGHT/2);
        printf("%c", i+65);
    }

    tc_reset_style();
    tc_hide_cursor();

}

//Prints the checker at the column and row of the checker board
void PrintChecker(int column, int row, int bgColor, int textColor){

    tc_set_text_color(textColor);
    tc_set_bg_color(bgColor);

    tc_set_cursor_position(boardState.x + (column*CHECKER_WIDTH+1), boardState.y + (row*CHECKER_HEIGHT+1));

    int boardIndex = GetBoardIndexFromColumnRow(column, row);

    switch(boardState.board[boardIndex].type){
        case NONE:
            printf(CHECKER_STRING);
            break;
        case PAWN:
            printf(PAWN_STRING);
            break;
        case KNIGHT:
            printf(KNIGHT_STRING);
            break;
        case ROOK:
            printf(ROOK_STRING);
            break;
        case BISHOP:
            printf(BISHOP_STRING);
            break;
        case QUEEN:
            printf(QUEEN_STRING);
            break;
        case KING:
            printf(KING_STRING);
            break;
        default:
            printf(CHECKER_STRING);
    }

    tc_reset_style();
    tc_hide_cursor();

}

int GetCheckerPosX(int column){
    return boardState.x + (column*CHECKER_WIDTH+1);
}

int GetCheckerPosY(int row){
    return boardState.y + (row*CHECKER_HEIGHT+1);
}

void PrintAvailableMoveSpaces(){

    for(int i = 0; i < availableMovePool.length; i++){
        ChessCoord coord = availableMovePool.chessCoords[i];
        PrintChecker(coord.column, coord.row, AVAILABLE_MOVE_COLOR, ANSI_COLOR_ID_BLK);
    }

}

//Return normal bg color for a given square on the board(wether it should be black or white)
int GetCheckerNormalBGColor(int column, int row){

    int boardIndex = GetBoardIndexFromColumnRow(column, row);
    ChessPiece piece = boardState.board[boardIndex];

    int bgColor = DEFAULT_BLACK;

    if(row%2 ==0){
        if(column%2 == 0){
            bgColor = DEFAULT_WHITE;
        }
    }else{
        if(column%2!=0){
            bgColor = DEFAULT_WHITE;
        }
    }

    if(piece.type == NONE) return bgColor;

    if(piece.side == WHITE && bgColor == DEFAULT_BLACK){
        bgColor = ANSI_COLOR_ID_LIGHTER_BLK;
    }
    else if(piece.side == WHITE && bgColor == DEFAULT_WHITE){
        bgColor = DEFAULT_WHITE;
    }
    else if(piece.side == BLACK && bgColor == DEFAULT_BLACK){
        bgColor = ANSI_COLOR_ID_DARK_BLK;
    }
    else{
        bgColor = ANSI_COLOR_ID_DARKER_WHT;
    }

    return bgColor;

}

int GetCheckerNormalTextColor(int column, int row){

    int boardIndex = GetBoardIndexFromColumnRow(column, row);
    if(boardState.board[boardIndex].side == WHITE) return 231;
    else if(GetCheckerNormalBGColor(column, row) == ANSI_COLOR_ID_DARKER_WHT) return ANSI_COLOR_ID_BLK;
    else return ANSI_COLOR_ID_FADED_BLK;

}

void PrintInfoBar(int row){

    tc_set_cursor_position(0, row);
    printf("%d,%d ", selectedColumn, selectedRow);
    
    int currentPieceIndex = GetBoardIndexFromColumnRow(selectedColumn, selectedRow);
    ChessPiece *piece = &boardState.board[currentPieceIndex];

    if(piece->type != NONE){
        if(piece->side == WHITE){
            tc_set_bg_color(ANSI_COLOR_ID_BRIGHT_WHT);
            tc_set_text_color(ANSI_COLOR_ID_DARK_WHT);
        }else{
            tc_set_bg_color(ANSI_COLOR_ID_LIGHT_BLK);
            tc_set_text_color(ANSI_COLOR_ID_LIGHTER_BLK);
        }
        switch(piece->type){
            case PAWN:
                printf("[PAWN]");
                break;
            case KNIGHT:
                printf("[KNIGHT]");
                break;
            case BISHOP:
                printf("[BISHOP]");
                break;
            case ROOK:
                printf("[ROOK]");
                break;
            case QUEEN:
                printf("[QUEEN]");
                break;
            case KING:
                printf("[KING]");
                break;
        }

        tc_reset_style();
    }

    if(pieceSelected){
        printf(" PIECE");
        if(selectedPiece->side == WHITE){
            tc_set_bg_color(ANSI_COLOR_ID_BRIGHT_WHT);
            tc_set_text_color(ANSI_COLOR_ID_DARK_WHT);
        }else{
            tc_set_bg_color(ANSI_COLOR_ID_LIGHT_BLK);
            tc_set_text_color(ANSI_COLOR_ID_LIGHTER_BLK);
        }
        switch(selectedPiece->type){
        case PAWN:
            printf("[PAWN]");
            break;
        case KNIGHT:
            printf("[KNIGHT]");
            break;
        case BISHOP:
            printf("[BISHOP]");
            break;
        case ROOK:
            printf("[ROOK]");
            break;
        case QUEEN:
            printf("[QUEEN]");
            break;
        case KING:
            printf("[KING]");
            break;
        }
    }

    tc_reset_style();

    if(networkGame){
        printf(" SIDE");
        if(side == WHITE){
            tc_set_bg_color(ANSI_COLOR_ID_BRIGHT_WHT);
            printf("[]");
        }else{
            tc_set_bg_color(ANSI_COLOR_ID_LIGHT_BLK);
            tc_set_text_color(ANSI_COLOR_ID_LIGHTER_BLK);
            printf("[]");
        }
    }else{
        printf(" TURN");
        if(activeSide == WHITE){
            tc_set_bg_color(ANSI_COLOR_ID_BRIGHT_WHT);
            printf("[]");
        }else{
            tc_set_bg_color(ANSI_COLOR_ID_LIGHT_BLK);
            tc_set_text_color(ANSI_COLOR_ID_LIGHTER_BLK);
            printf("[]");
        }
    }

    tc_reset_style();

    if(networkGame){
        if(activeSide != side){
            printf(" OPPONENTS TURN");
        }else{
            printf(" YOUR TURN");
        }
    }

    CONSOLE_SCREEN_BUFFER_INFO csbi;
    GetConsoleScreenBufferInfo(wHnd, &csbi);

    for(int i = csbi.dwCursorPosition.X; i < csbi.dwSize.X; i++){
        printf(" ");
    }

    tc_hide_cursor();

}

void SetupGame(){

    SetConsoleMode(rHnd, ENABLE_WINDOW_INPUT);
    SetConsoleMode(wHnd, 0x0004 | 0x0001);
    
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    int columns, rows;

    GetConsoleScreenBufferInfo( wHnd, &csbi );
        
    columns = csbi.dwSize.X;
    rows = csbi.dwSize.Y;

    terminalColumns = columns;
    terminalRows = rows;

    boardState.x  = columns/2 - (CHECKER_WIDTH*8)/2;
    boardState.y = rows/2 - (CHECKER_HEIGHT*8)/2;
    boardState.width = CHECKER_WIDTH * 8;
    boardState.height = CHECKER_HEIGHT * 8;

    ChessCoordPool_Init(&availableMovePool);
    for(int i = 0; i < 64; i++){
        enum CHESS_SIDE side = BLACK;
        enum CHESS_PIECE_TYPE type = NONE;

        if(i >= 32){
            side = WHITE;
        }

        ChessPiece_Init(&boardState.board[i], side, type);
    }

    SetupBoardPieces();

    tc_cursor_to_home();
    tc_clear_screen();
    
    tc_set_text_color(ANSI_COLOR_ID_BLK);
    tc_set_bg_color(ANSI_COLOR_ID_WHT);

    PrintBoard(columns, rows);
    PrintInfoBar(terminalRows);

}
void SetupBoardPieces(){
    SetBoardPieceType(&boardState, 0,0, ROOK);
    SetBoardPieceType(&boardState, 1,0, KNIGHT);
    SetBoardPieceType(&boardState, 2,0, BISHOP);
    SetBoardPieceType(&boardState, 3,0, QUEEN);
    SetBoardPieceType(&boardState, 4,0, KING);
    SetBoardPieceType(&boardState, 5,0, ROOK);
    SetBoardPieceType(&boardState, 6,0, KNIGHT);
    SetBoardPieceType(&boardState, 7,0, BISHOP);

    for(int i=0;i < 8;i++){
        SetBoardPieceType(&boardState, i,1, PAWN);
    }

    for(int i=0;i < 8;i++){
        SetBoardPieceType(&boardState, i,6, PAWN);
    }

    SetBoardPieceType(&boardState, 0, 7, ROOK);
    SetBoardPieceType(&boardState, 1, 7, KNIGHT);
    SetBoardPieceType(&boardState, 2, 7, BISHOP);
    SetBoardPieceType(&boardState, 3, 7, QUEEN);
    SetBoardPieceType(&boardState, 4, 7, KING);
    SetBoardPieceType(&boardState, 5, 7, BISHOP);
    SetBoardPieceType(&boardState, 6, 7, KNIGHT);
    SetBoardPieceType(&boardState, 7, 7, ROOK);
}

void ResetConsole(){

    SetConsoleMode(wHnd, baseStdoutMode);
    SetConsoleMode(rHnd, baseStdinMode);

}