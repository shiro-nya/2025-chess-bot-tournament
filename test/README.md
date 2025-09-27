# API test program

This is a test data generator script and a program to test if the C chess API is able to find all valid moves for a board.

Generate test data with:

```
./gen_testdata.py >testdata
```
You need to have `uv` installed for this. Alternatively install the `chess` module with pip and run the script with `python3`.

Build the tester binary with:

```
g++ -O3 --std=c++20 -I../src/c chess_api_tester.cpp ../src/c/chessapi.c ../src/c/bitboard.c -o chess_api_tester
```

Run the tester with:

```
cat testdata | ./chess_api_tester
```

The tester will print out test cases where the found move set doesn't match the expected ones. The output can be captured and used as input data for debugging.
