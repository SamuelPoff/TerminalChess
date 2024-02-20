# Terminal Chess

A small project to explore terminals and light networking code. 
Utilizes winsock2 for sockets. Can set up a **basic** online multiplayer chess game.

### Compiling
I compiled the (admittedly small amount of) code using the MSVC compiler. The number of files is small so I simply type the command out to compile and output to /bin.

`cl [options] src/chess.c src/main.c src/data_structures/chess_coord_pool.c`

Make sure to run this command in a **Developer Command Prompt** (you will have it if you have a version of Visual Studio)

### Running
Not intended to be multiplatform, so it only works on Windows.
Only works in a windows terminal (CMD and POWERSHELL are not true valid terminals, Windows is a strange beast). Windows has released Windows Terminal to emulate a true terminal experience, and was what I primarily used for testing. Although the terminal in VSCode has all the features required for a terminal, and therefore also runs the program correctly!
