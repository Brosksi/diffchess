#include "game.h"
#include "movegen.h"

void makeMove(GameState & state, const Move& move) {
    char piece = state.board[move.from_row][move.from_col];
    state.enpassant_row = -1;
    state.enpassant_col = -1;

    char upperPiece = isWhite(piece) ? piece : piece - 'a' + 'A';

    if (upperPiece == 'P' && abs(move.to_row - move.from_row) ==2) {
        state.enpassant_row = (move.from_row + move.to_row)/2;
        state.enpassant_col = move.from_col;
    }

    if (move.is_en_passant) {
        int capturedPawnRow = move.from_row;
        state.board[capturedPawnRow][move.to_col] = '.';
    }

    state.board[move.to_row][move.to_col] = piece;
    state.board[move.from_row][move.from_col] = '.';

    if (move.promotion != '\0') {
        char promoChar = move.promotion;
        if (isWhite(piece)) promoChar = promoChar - 'a' + 'A';
        state.board[move.to_row][move.to_col] = promoChar;
    }

    if (move.is_castling){
        if (move.to_col == 6) {
            state.board[move.to_row][5] = state.board[move.to_row][7];
            state.board[move.to_row][7] = '.';
        } else if( move.to_col == 2){
            state.board[move.to_row][3] = state.board[move.to_row][0];
            state.board[move.to_row][0] = '.';
        }
    }

    if (piece == 'K') {
        state.white_kingside_castle = false;
        state.white_queenside_castle = false;
    }
    if (piece == 'k'){
        state.black_kingside_castle = false;
        state.black_queenside_castle = false;
    }

    if (piece == 'R') {
        if (move.from_col == 0 && move.from_row ==7) state.white_queenside_castle = false;
        if (move.from_row == 7 && move.from_col == 7) state.white_kingside_castle = false;
    }

    if (piece == 'r') {
        if (move.from_row == 0 && move.from_col == 0 ) state.black_queenside_castle = false;
        if (move.from_row == 0 && move.from_col == 7) state.black_kingside_castle = false;
    }

    if (move.to_row == 7 && move.to_col == 0) state.white_queenside_castle = false;
    if (move.to_row == 7 && move.to_col == 7) state.white_kingside_castle = false;
    if (move.to_row == 0 && move.to_col == 0) state.black_queenside_castle = false;;
    if (move.to_row == 0 && move.to_col == 7) state.black_kingside_castle = false;


    state.white_to_move = !state.white_to_move;
}

bool isCheckmate(const GameState& state) {
    if (!isinCheck(state, state.white_to_move)) return false;
    vector<Move> legalMoves = generateLegalMoves(state);
    return legalMoves.empty();
}

bool isStalemate(const GameState& state) {
    if (isinCheck(state, state.white_to_move)) return false;
    vector<Move> legalMoves = generateLegalMoves(state);
    return legalMoves.empty();
}

bool isGameOver(const GameState& state) {
    return (isCheckmate(state)) || (isStalemate(state));
}

