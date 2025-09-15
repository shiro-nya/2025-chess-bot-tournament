from __future__ import annotations

from _chessapi_cffi import ffi, lib
from enum import Enum
from typing import Any


BitBoard = int


class PlayerColor(Enum):
    WHITE = 0,
    BLACK = 1


class PieceType(Enum):
    PAWN = 1,
    BISHOP = 2,
    KNIGHT = 3,
    ROOK = 4,
    QUEEN = 5,
    KING = 6


class GameState(Enum):
    GAME_CHECKMATE = -1,
    GAME_NORMAL = 0,
    GAME_STALEMATE = 1


class Move:

    origin: BitBoard
    target: BitBoard
    promotion: PieceType
    castle: bool
    capture: bool
    _share_ptr: Any

    def __init__(self, make_ptr: bool = True):
        if make_ptr:
            self._share_ptr = ffi.new("Move *")

    @classmethod
    def load(cls, ptr) -> Move:
        move = Move()
        move.origin = getattr(ptr, "from")
        move.target = getattr(ptr, "to")
        move.promotion = getattr(ptr, "promotion")
        move.castle = getattr(ptr, "castle")
        move.capture = getattr(ptr, "capture")
        return move

    def dump(self):
        setattr(self._share_ptr, "from", self.origin)
        setattr(self._share_ptr, "to", self.target)
        setattr(self._share_ptr, "promotion", self.promotion)
        setattr(self._share_ptr, "castle", self.castle)
        setattr(self._share_ptr, "capture", self.capture)
        return self._share_ptr[0]


class Board:

    ptr: int

    def __init__(self, ptr: int):
        self.ptr = ptr

    def __del__(self):
        lib.chess_free_board(self.ptr)

    def get_legal_moves(self) -> list[Move]:
        moves_len_C = ffi.new("int *")
        moves_C = lib.chess_get_legal_moves(self.ptr, moves_len_C)
        moves_len = moves_len_C[0]
        moves = ffi.unpack(moves_C, moves_len)
        for i in range(len(moves)):
            moves[i] = Move.load(moves[i])
        lib.chess_free_moves_array(moves_C)
        return moves

    def is_white_turn(self) -> bool:
        return lib.chess_is_white_turn(self.ptr)

    def skip_turn(self) -> None:
        lib.chess_skip_turn(self.ptr)

    def in_check(self) -> bool:
        return lib.chess_in_check(self.ptr)

    def in_checkmate(self) -> bool:
        return lib.chess_in_checkmate(self.ptr)

    def in_draw(self) -> bool:
        return lib.chess_in_draw(self.ptr)

    def can_castle_kingside(self, color: PlayerColor) -> bool:
        return lib.chess_can_castle_kingside(self.ptr, color)

    def can_castle_queenside(self, color: PlayerColor) -> bool:
        return lib.chess_can_castle_queenside(self.ptr, color)

    def is_game_ended(self) -> GameState:
        return lib.chess_is_game_ended(self.ptr)

    def zobrist_key(self) -> int:
        return lib.chess_zobrist_key(self.ptr)

    def make_move(self, move: Move) -> None:
        lib.chess_make_move(self.ptr, move.dump())

    def undo_move(self) -> None:
        lib.chess_undo_move(self.ptr)

    def get_bitboard(self, color: PlayerColor, piece_type: PieceType) -> BitBoard:
        return lib.chess_get_bitboard(self.ptr, color, piece_type)


class API:

    @classmethod
    def __init__(cls):
        pass

    @classmethod
    def get_board(cls) -> Board:
        return Board(lib.chess_get_board())
    
    @classmethod
    def push(cls, move: Move) -> None:
        lib.chess_push(move.dump())

    @classmethod
    def done(cls) -> None:
        lib.chess_done()

    @classmethod
    def get_time_millis(cls) -> int:
        return lib.chess_get_time_millis()

    @classmethod
    def get_opponent_time_millis(cls) -> int:
        return lib.chess_get_opponent_time_millis()

    @classmethod
    def get_elapsed_time_millis(cls) -> int:
        return lib.chess_get_elapsed_time_millis()
