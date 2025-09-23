# Getting started (Python language version)

## Setup
- Ensure you have a working Python version installed
  - The API was tested with Python 3.12.11, so this is the recommended version.
- Copy `pychessapi.py`, `example_bot.py` into a directory
- Copy the bitboard and chessapi `.c` files from the C section of this Git repo into the same directory
- Build the Chess API into a library: `libchess.so` (`libchess.dll` on Windows)
  - The command to do so might look something like this: `cc -o libchess.so -shared bitboard.c chessapi.c -fPIC`
  - It *must* be named `libchess.so`/`libchess.dll`
- Modify `example_bot.py` with your own code
- Run your code!
  - Ensure `libchess.so`/`libchess.dll` is in the same directory as your bot when run
  - Tip: Easiest to test by adding as a bot to a compatible chess program

Be sure to refer to the [docs](https://github.com/shiro-nya/2025-chess-bot-tournament/wiki/Chess-API-(Python)) to see how to use the API!
