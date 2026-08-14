#pragma once
#include <string>
#include <vector>
using namespace std;

struct Move{
    int from_row, from_col;
    int to_row, to_col;
    char promotion;
    bool is_castling;
    bool is_en_passant;


    Move() : from_row(0), from_col(0), to_row(0), to_col(0), 
    promotion('\0'), is_castling(false), is_en_passant(false) {}
    Move(int fr, int fc, int tr, int tc) : from_row(fr), from_col(fc), 
    to_row(tr), to_col(tc), promotion('\0'), is_castling(false), is_en_passant(false) {}
    
};

struct GameState {
    vector<string>board;
    bool white_to_move;
    bool white_kingside_castle;
    bool black_kingside_castle;
    bool white_queenside_castle;
    bool black_queenside_castle;

    int enpassant_row;
    int enpassant_col;


    GameState() {
        board = {
            "rnbqkbnr",
            "pppppppp",
            "........",
            "........",
            "........",
            "........",
            "PPPPPPPP",
            "RNBQKBNR"
        };
        white_to_move = true;
        white_kingside_castle = true;
        black_kingside_castle = true;
        white_queenside_castle = true;
        black_queenside_castle = true;

        enpassant_row = -1; // -1 means not available
        enpassant_col = -1;
    }
};