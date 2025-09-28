from __future__ import annotations

from ctypes import cdll, CDLL, c_uint8, c_uint64, c_int, c_bool, Structure, cast, byref, POINTER
from enum import Enum
from typing import Optional
import os
import platform


# import library
if platform.system() == "Windows":
    local_dir = os.path.dirname(os.path.realpath(__file__))
    cdll.LoadLibrary(local_dir + "/libchess.dll")
    lib = CDLL(local_dir + "/libchess.dll")
else:
    local_dir = os.path.dirname(os.path.realpath(__file__))
    cdll.LoadLibrary(local_dir + "/libchess.so")
    lib = CDLL(local_dir + "/libchess.so")


BitBoard = int
_BitBoard = c_uint64
_C_enum = c_int


class PlayerColor(Enum):
    WHITE = 0
    BLACK = 1


class PieceType(Enum):
    PAWN = 1
    BISHOP = 2
    KNIGHT = 3
    ROOK = 4
    QUEEN = 5
    KING = 6


class GameState(Enum):
    GAME_CHECKMATE = -1
    GAME_NORMAL = 0
    GAME_STALEMATE = 1
    

class Move(Structure):

    _fields_ = [("_from", _BitBoard),
                ("_to", _BitBoard),
                ("_promotion", c_uint8),
                ("_capture", c_bool),
                ("_castle", c_bool)]

    def __init__(self, copy_from: Optional[Move]):
        if copy_from is not None:
            # awkward copy ctor
            self._from = _BitBoard(BitBoard(copy_from._from))
            self._to = _BitBoard(BitBoard(copy_from._to))
            self._promotion = c_uint8(int(copy_from._promotion))
            self._capture = c_bool(bool(copy_from._capture))
            self._castle = c_bool(bool(copy_from._castle))

    @property
    def origin(self) -> BitBoard:
        return BitBoard(self._from)

    @property
    def target(self) -> BitBoard:
        return BitBoard(self._to)

    @property
    def promotion(self) -> Optional[PieceType]:
        return PieceType(self._promotion) if int(self._promotion) > 0 else None

    @property
    def castle(self) -> bool:
        return bool(self._castle)

    @property
    def capture(self) -> bool:
        return bool(self._capture)


class Board(Structure):

    _fields_ = []

    def __init__(self):
        pass

    def __del__(self):
        lib.chess_free_board(byref(self))

    def get_legal_moves(self) -> list[Move]:
        moves_len_C = c_int(0)
        moves_C = cast(lib.chess_get_legal_moves(byref(self), byref(moves_len_C)), POINTER(Move))
        moves_len = moves_len_C.value
        moves = [Move(moves_C[i]) for i in range(moves_len)]
        lib.chess_free_moves_array(moves_C)
        return moves

    def is_white_turn(self) -> bool:
        return lib.chess_is_white_turn(byref(self))

    def is_black_turn(self) -> bool:
        return lib.chess_is_black_turn(byref(self))

    def skip_turn(self) -> None:
        lib.chess_skip_turn(byref(self))

    def in_check(self) -> bool:
        return lib.chess_in_check(byref(self))

    def in_checkmate(self) -> bool:
        return lib.chess_in_checkmate(byref(self))

    def in_draw(self) -> bool:
        return lib.chess_in_draw(byref(self))

    def can_castle_kingside(self, color: PlayerColor) -> bool:
        return lib.chess_can_kingside_castle(byref(self), c_int(color.value))

    def can_castle_queenside(self, color: PlayerColor) -> bool:
        return lib.chess_can_queenside_castle(byref(self), c_int(color.value))

    def get_game_state(self) -> GameState:
        return GameState(lib.chess_get_game_state(byref(self)))

    def zobrist_key(self) -> int:
        return lib.chess_zobrist_key(byref(self))

    def make_move(self, move: Move) -> None:
        lib.chess_make_move(byref(self), move)

    def undo_move(self) -> None:
        lib.chess_undo_move(byref(self))

    def get_bitboard(self, color: PlayerColor, piece_type: PieceType) -> BitBoard:
        return BitBoard(lib.chess_get_bitboard(byref(self), c_int(color.value), c_int(piece_type.value)))
    
    def get_piece_from_index(self, index: int) -> Optional[PieceType]:
        piece_type: int = int(lib.chess_get_piece_from_index(byref(self), index))
        return PieceType(piece_type) if piece_type > 0 else None

    def get_piece_from_bitboard(self, bitboard: BitBoard) -> Optional[PieceType]:
        piece_type: int = int(lib.chess_get_piece_from_bitboard(byref(self), _BitBoard(bitboard)))
        return PieceType(piece_type) if piece_type > 0 else None

    def get_color_from_index(self, index: int) -> Optional[PlayerColor]:
        piece_color: int = int(lib.chess_get_color_from_index(byref(self), index))
        return PlayerColor(piece_color) if piece_color >= 0 else None

    def get_color_from_bitboard(self, bitboard: BitBoard) -> Optional[PlayerColor]:
        piece_color: int = int(lib.chess_get_color_from_bitboard(byref(self), _BitBoard(bitboard)))
        return PlayerColor(piece_color) if piece_color >= 0 else None
    
    def clone(self) -> Board:
        return lib.chess_clone_board(byref(self)).contents
    
    def get_full_moves(self) -> int:
        return lib.chess_get_full_moves(byref(self))
    
    def get_half_moves(self) -> int:
        return lib.chess_get_half_moves(byref(self))


class API:

    @classmethod
    def __init__(cls):
        pass

    @classmethod
    def get_board(cls) -> Board:
        return lib.chess_get_board().contents
    
    @classmethod
    def push(cls, move: Move) -> None:
        lib.chess_push(move)

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
    
    @classmethod
    def get_index_from_bitboard(cls, bitboard: BitBoard) -> int:
        return lib.chess_get_index_from_bitboard(_BitBoard(bitboard))
    
    @classmethod
    def get_bitboard_from_index(cls, index: int) -> BitBoard:
        return BitBoard(lib.chess_get_bitboard_from_index(index))
    
    @classmethod
    def get_opponent_move(cls) -> Move:
        return lib.chess_get_opponent_move()


# set type restrictions
lib.chess_get_board.argtypes = ()
lib.chess_get_board.restype = POINTER(Board)
lib.chess_get_legal_moves.argtypes = (POINTER(Board), POINTER(c_int))
lib.chess_get_legal_moves.restype = POINTER(Move)
lib.chess_is_white_turn.argtypes = (POINTER(Board),)
lib.chess_is_white_turn.restype = c_bool
lib.chess_is_black_turn.argtypes = (POINTER(Board),)
lib.chess_is_black_turn.restype = c_bool
lib.chess_skip_turn.argtypes = (POINTER(Board),)
lib.chess_skip_turn.restype = None
lib.chess_in_check.argtypes = (POINTER(Board),)
lib.chess_in_check.restype = c_bool
lib.chess_in_checkmate.argtypes = (POINTER(Board),)
lib.chess_in_checkmate.restype = c_bool
lib.chess_in_draw.argtypes = (POINTER(Board),)
lib.chess_in_draw.restype = c_bool
lib.chess_can_kingside_castle.argtypes = (POINTER(Board), _C_enum)
lib.chess_can_kingside_castle.restype = c_bool
lib.chess_can_queenside_castle.argtypes = (POINTER(Board), _C_enum)
lib.chess_can_queenside_castle.restype = c_bool
lib.chess_get_game_state.argtypes = (POINTER(Board),)
lib.chess_get_game_state.restype = _C_enum
lib.chess_zobrist_key.argtypes = (POINTER(Board),)
lib.chess_zobrist_key.restype = c_uint64
lib.chess_make_move.argtypes = (POINTER(Board), Move)
lib.chess_make_move.restype = None
lib.chess_undo_move.argtypes = (POINTER(Board),)
lib.chess_undo_move.restype = None
lib.chess_free_board.argtypes = (POINTER(Board),)
lib.chess_free_board.restype = None
lib.chess_get_bitboard.argtypes = (POINTER(Board), _C_enum, _C_enum)
lib.chess_get_bitboard.restype = _BitBoard
lib.chess_push.argtypes = (Move,)
lib.chess_push.restype = None
lib.chess_done.argtypes = ()
lib.chess_done.restype = None
lib.chess_get_time_millis.argtypes = ()
lib.chess_get_time_millis.restype = c_uint64
lib.chess_get_opponent_time_millis.argtypes = ()
lib.chess_get_opponent_time_millis.restype = c_uint64
lib.chess_get_elapsed_time_millis.argtypes = ()
lib.chess_get_elapsed_time_millis.restype = c_uint64
lib.chess_free_moves_array.argtypes = (POINTER(Move),)
lib.chess_free_moves_array.restype = None
lib.chess_get_piece_from_index.argtypes = (POINTER(Board), c_int)
lib.chess_get_piece_from_index.restype = _C_enum
lib.chess_get_piece_from_bitboard.argtypes = (POINTER(Board), _BitBoard)
lib.chess_get_piece_from_bitboard.restype = _C_enum
lib.chess_get_color_from_index.argtypes = (POINTER(Board), c_int)
lib.chess_get_color_from_index.restype = _C_enum
lib.chess_get_color_from_bitboard.argtypes = (POINTER(Board), _BitBoard)
lib.chess_get_color_from_bitboard.restype = _C_enum
lib.chess_get_index_from_bitboard.argtypes = (_BitBoard,)
lib.chess_get_index_from_bitboard.restype = c_int
lib.chess_get_bitboard_from_index.argtypes = (c_int,)
lib.chess_get_bitboard_from_index.restype = _BitBoard
lib.chess_get_opponent_move.argtypes = ()
lib.chess_get_opponent_move.restype = Move
lib.chess_clone_board.argtypes = (POINTER(Board),)
lib.chess_clone_board.restype = POINTER(Board)
lib.chess_get_full_moves.argtypes = (POINTER(Board),)
lib.chess_get_full_moves.restype = c_int
lib.chess_get_half_moves.argtypes = (POINTER(Board),)
lib.chess_get_half_moves.restype = c_int
