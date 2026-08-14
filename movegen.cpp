#include "movegen.h"
#include <cstdlib>

bool onBoard(int row, int col) {
    return row >=0 && row <8 && col >=0 && col <8;
}

bool isWhite (char piece) {
    return piece >= 'A' && piece <= 'Z';
}

bool isBlack (char piece) {
    return piece >= 'a' && piece <= 'z';
}

bool isOwnPiece(char piece, bool whiteMove) {
    if (piece == '.') return false;
    return whiteMove ? isWhite(piece) : isBlack(piece);
}

bool isEnemyPiece(char piece, bool whiteMove) {
    if (piece =='.') return false;
    return whiteMove ? isBlack(piece) : isWhite(piece);
}

bool isSquareAttacked(const GameState& state, int row, int col, bool byWhite) {
    const vector<string>& board = state.board;
    
    char pawn = byWhite ? 'P' : 'p';
    int pawnDir = byWhite ? 1: -1;
    if (onBoard(row + pawnDir, col-1) && board[row+pawnDir][col-1] == pawn) return true;
    if(onBoard(row + pawnDir, col+1) && board[row+pawnDir][col+1] == pawn) return true;

     char knight = byWhite ? 'N' : 'n';
     static const int knightMoves[8][2] = {{-2, -1}, {-2, 1}, {-1, -2}, {-1, 2}, 
                                {1,-2}, {1,2}, {2, -1}, {2,1}};
    for (auto& move : knightMoves) {
        int r = row + move[0];
        int c = col + move[1];
        if (onBoard(r,c) && board[r][c] == knight) return true;
    }
    char king = byWhite ? 'K' : 'k';
    static const int kingMoves[8][2] = {{1,0},{1,-1},{0,1},{1,1},{0,-1},{-1,1},{-1,0},{-1,-1}};
    for (auto& move : kingMoves) {
        int r = row + move[0];
        int c = col + move[1];
        if (onBoard(r,c) && board[r][c] == king) return true;
    }
    char rook = byWhite ? 'R' : 'r';
    char queen = byWhite ? 'Q' : 'q';
    char bishop = byWhite ? 'B' : 'b';
    static const int straight[4][2] = {{1,0},{-1,0},{0,1},{0,-1}};
    for (auto& dir : straight) {
        int r = row + dir[0];
        int c = col + dir[1];
        while (onBoard(r,c)) {
            char piece = board[r][c];
            if (piece != '.') {
                if (piece == rook || piece == queen) return true;
                break;
            }
            r += dir[0];
            c += dir[1];
        }
    }
    static const int diagonal[4][2] = {{1,1},{1,-1},{-1,1},{-1,-1}};
    for (auto& dir : diagonal) {
        int r = row + dir[0], c = col + dir[1];
        while (onBoard(r,c)) {
            if (board[r][c] !='.'){
                if (board[r][c] == bishop || board[r][c] == queen) return true;
                break;
            }
            r += dir[0];
            c += dir[1];
        }
    }
    return false;
}
bool isinCheck(const GameState& state, bool whiteKing) {
    char kingChar = whiteKing ? 'K' : 'k';
    for (int row = 0; row <8; row++) {
        for (int col = 0; col<8; col++){
            if (state.board[row][col] == kingChar){
                return isSquareAttacked(state, row, col, !whiteKing);
            }
        }
    }
    return false; 
}

vector<Move> generatePsueduoLegalMoves(const GameState& state) {
    vector<Move> moves;
    const vector<string>& board = state.board;
    bool white = state.white_to_move;
    for (int r =0; r<8; r++) {
        for (int c =0; c<8; c++){
            char piece = board[r][c];
            if (!isOwnPiece(piece, white)) continue;
            char upperPiece = isWhite(piece) ? piece : piece - 'a' + 'A';

            if (upperPiece == 'P'){
                int direction = white ? -1 : 1;
                int startRow = white ? 6 : 1;
                int promoRow = white ? 0 : 7;

                int newRow = r + direction; 
                if (onBoard(newRow, c) && board[newRow][c] == '.') {
                    if (newRow == promoRow){
                        for (char promo : {'q', 'r', 'b', 'n'}){
                            Move m(r, c, newRow, c); 
                            m.promotion = promo;
                            moves.push_back(m);
                        }
                    } else{
                        moves.push_back(Move(r, c, newRow, c)); 
                    }
                    if (r == startRow){
                        int twoRow = r + (2 * direction); 
                        if (board[twoRow][c] =='.'){ 
                            moves.push_back(Move(r, c, twoRow, c)); 
                        }
                    }
                }
                for ( int dc : {-1, 1}){
                    int nc = c + dc;
                    if (!onBoard(newRow, nc)) continue;

                    if (isEnemyPiece(board[newRow][nc], white)){
                        if (newRow == promoRow){
                            for (char promo: {'q', 'r', 'b', 'n'}){
                                Move m(r,c,newRow, nc);
                                m.promotion = promo;
                                moves.push_back(m);
                            }
                        }else {
                            moves.push_back(Move(r,c,newRow, nc));
                        }
                    }

                    if (newRow == state.enpassant_row && nc == state.enpassant_col) {
                        Move m(r,c, newRow, nc);
                        m.is_en_passant = true;
                        moves.push_back(m);
                    }
                }
            }

            else if (upperPiece == 'N') {
                static const int knightMoves[8][2] = {{-2,-1}, {-2,1}, {-1,-2}, {-1,2},
                    {1,-2},  {1,2},  {2,-1}, {2,1}};
                for (auto& m : knightMoves){
                    int nr = r + m[0], nc = c + m[1];
                    if (onBoard(nr,nc) && !isOwnPiece(board[nr][nc], white)) {
                        moves.push_back(Move(r,c,nr,nc));
                    }
                }
            }
            else if (upperPiece == 'B') {
                static const int dirs[4][2] = {{-1,-1}, {-1,1}, {1,-1}, {1,1}};
                for (auto& d: dirs) {
                    int nr = r + d[0], nc = c + d[1];
                    while (onBoard(nr,nc)){
                        if (isOwnPiece(board[nr][nc], white)) break;
                        moves.push_back(Move(r,c,nr,nc));
                        if (isEnemyPiece(board[nr][nc], white)) break;
                        nr += d[0];
                        nc += d[1];
                    }
                }                
            }
            else if(upperPiece == 'R'){
                static const int dirs[4][2] = {{-1,0}, {1,0}, {0,-1}, {0,1}};
                for (auto& d : dirs){
                    int nr = r + d[0], nc = c + d[1];
                    while (onBoard(nr, nc)){
                        if (isOwnPiece(board[nr][nc], white)) break;
                        moves.push_back(Move(r,c,nr,nc));
                        if(isEnemyPiece(board[nr][nc], white)) break;
                        nr += d[0];
                        nc += d[1];

                    }
                }
            }
            else if (upperPiece == 'Q'){
                static const int dirs[8][2] = {{-1,0}, {1,0}, {0,-1}, {0,1}, 
                {-1,-1}, {-1,1}, {1,-1}, {1,1}};
                for (auto& d: dirs){
                    int nr = r + d[0], nc = c + d[1];
                    while(onBoard(nr, nc)){
                        if (isOwnPiece(board[nr][nc], white)) break;
                        moves.push_back(Move(r,c,nr, nc));
                        if (isEnemyPiece(board[nr][nc], white)) break;
                        nr += d[0];
                        nc += d[1];
                    }
                }

            }
            else if (upperPiece == 'K'){
                static const int dirs[8][2] = {{1,0},{1,-1},{0,1},
                {1,1},{0,-1},{-1,1},{-1,0},{-1,-1}};
                for (auto& d: dirs){
                    int nr = r + d[0], nc = c + d[1];
                    if (onBoard(nr, nc) && !isOwnPiece(board[nr][nc], white)) {
                        moves.push_back(Move(r,c,nr, nc));
                    }
                }
            }

            if (white) {
                if (state.white_kingside_castle && board[7][5] == '.' && board[7][6] == '.' &&
                    board[7][7] == 'R' &&
                    !isSquareAttacked(state, 7,4,false) &&
                    !isSquareAttacked(state,7,5,false) &&
                    !isSquareAttacked(state, 7,6,false)){
                    Move m(7,4,7,6);
                    m.is_castling = true;
                    moves.push_back(m);
                }
               if (state.white_queenside_castle && board[7][3] == '.' && board[7][2] == '.' &&
                    board[7][0] =='R' && 
                    !isSquareAttacked(state, 7,4,false) &&
                    !isSquareAttacked(state, 7,3,false)&&
                    !isSquareAttacked(state, 7,2,false)){
                    Move m(7,4,7,2);
                    m.is_castling = true;
                    moves.push_back(m);
                }
            }else {
                if (state.black_kingside_castle && board[0][5] == '.' && board[0][6] =='.' &&
                    board[0][7] =='r' &&
                    !isSquareAttacked(state, 0,4,true) &&
                    !isSquareAttacked(state, 0,5, true) &&
                    !isSquareAttacked(state, 0,6,true)){
                        Move m(0,4,0,6);
                        m.is_castling = true;
                        moves.push_back(m);
                    }

                if (state.black_queenside_castle && board[0][3] == '.' && board[0][2] == '.' &&
                    board[0][1] == 'r' &&
                    !isSquareAttacked(state, 0,4,true) &&
                    !isSquareAttacked(state, 0,3,true) &&
                    !isSquareAttacked(state, 0,2, true)) {
                        Move m(0,4,0,2);
                        m.is_castling = true;
                        moves.push_back(m);
                    }
            } 
        } 

    }
    return moves;
}


vector<Move> generateLegalMoves(const GameState& state){
    vector<Move> psuedoMoves = generatePsueduoLegalMoves(state);
    vector<Move> legalMoves;

    for (Move& move : psuedoMoves) {
        GameState temp = state;
        char piece = temp.board[move.from_row][move.from_col];
        temp.board[move.to_row][move.to_col] = piece;
        temp.board[move.from_row][move.from_col] = '.';
        
        if  (move.is_en_passant) {
            int capturePawnRow = move.from_row;
            temp.board[capturePawnRow][move.to_col] = '.';
        }

        if (move.promotion != '\0'){
            char promoChar = move.promotion;
            if (isWhite(piece)) promoChar = promoChar - 'a' + 'A';
            temp.board[move.to_row][move.to_col] = promoChar;
        }

        if (move.is_castling) {
            if (move.to_col == 6) {
                temp.board[move.to_row][5] = temp.board[move.to_row][7];
                temp.board[move.to_row][7] = '.';
            }else if(move.to_col ==2) {
                temp.board[move.to_row][3] = temp.board[move.to_row][0];
                temp.board[move.to_row][0] ='.';
            }

        }

        temp.white_to_move = !temp.white_to_move;
        bool kingSafe = !isinCheck(temp, state.white_to_move);
        if (kingSafe){
            legalMoves.push_back(move);
        }
    }
    return legalMoves;             
}

