# Example Python bot using the Chess API

from pychessapi import API
from random import randrange

for i in range(500):
    # get the current board
    board = API.get_board()
    # get all legal moves for the position
    moves = board.get_legal_moves()
    # play one at random
    API.push(moves[randrange(0, len(moves))])
    # end our turn
    API.done()