# Chess bot tournament Kotlin API
## UNDER CONSTRUCTION: Bindings do not exist yet! Soon!
Kotlin bindings for Shiro's Chess bot API for the Neurocord Chess bot tournament

## Usage
- Download a jar file from releases or build the chess-api artifact from the source
- Put the jar file anywhere you want, and make sure libchessapi.so compiled from Shiro's code is in `native/libchessapi.so`, compilation steps:
```bash
git pull https://github.com/shiro-nya/2025-chess-bot-tournament.git
cd 2025-chess-bot-tournament
gcc -fPIC -shared ./src/c/chessapi.c ./src/c/bitboard.c -Isrc/c -o libchessapi.so
```
- Include the jar file in your Gradle project with `implementation(files("path/to/chess-api.jar"))`

## Example
Simple bot that makes random moves, same as the one found in the original repository
```kt
fun main() {
    for (i in 0..<500) {
        val board = ChessApi.getBoard()
        val moves = ChessApi.getLegalMoves(board)
        ChessApi.chessPush(moves.random())
        ChessApi.chessDone()
    }
}
```
