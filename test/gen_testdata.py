#!/usr/bin/env -S uv --quiet run --script
# /// script
# requires-python = ">=3.13"
# dependencies = [
#     "chess",
# ]
# ///

import chess
import random

for i in range(1000):
    board = chess.Board()

    j = 0
    while not board.is_checkmate() and not board.is_stalemate():
        board.push(random.choice(list(board.legal_moves)))
        print(board.fen())
        print(' '.join((m.uci() for m in board.legal_moves)))

        j += 1
        if j > 150:
            break
