# Getting started (C language version)

## Setup
- Ensure you have a working C compiler installed (e.g. gcc)
- Copy all of the `.c`, `.h` files into a directory
- Modify `example_bot.c` with your own code
- Compile and run!
  - Tip: Easiest to test by adding as a bot to a compatible chess program

Be sure to refer to the [docs](https://github.com/shiro-nya/2025-chess-bot-tournament/wiki/Chess-API-(C)) to see how to use the API!

## Specifics
- C submissions will be compiled using `gcc -std=c23 -O3 -o bot_exec bot_source.c -L. -lchess -lm`
- C++ submissions will be compiled using `g++ -std=c++23 -O3 -o bot_exec bot_source.c -L. -lchess -lm`
