fn main() {
    let mut hash = 0;

    loop {
        let board = chess_api::Board::get();
        hash ^= board.zobrist_key();
        let moves = board.get_legal_moves();
        chess_api::push(moves[hash as usize % moves.len()]);
        chess_api::done();
    }
}
