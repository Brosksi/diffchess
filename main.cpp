#include <iostream>
#include <string>
#include <vector>
#include <cstdlib>
#include <ctime>
#include "types.h"
#include "movegen.h"
#include "game.h"

using namespace std;

string from, to;

string pieceSymbol(char piece){
    switch(piece){
        case 'r': return "♖";
        case 'n': return "♘";
        case 'b': return "♗";
        case 'q': return "♕";
        case 'k': return "♔";
        case 'p': return "♙";

        case 'R': return "♜";
        case 'N': return "♞";
        case 'B': return "♝";
        case 'Q': return "♛";
        case 'K': return "♚";
        case 'P': return "♟";

        default: return ".";
    };
};
void print(const vector<string>& board){

    cout <<"   -----------------"<< endl;
    for (int row=0;row<8; row++){
        cout << 8-row << " " << "|" << " ";
        for (int col=0; col<8; col++){
            cout << pieceSymbol(board[row][col]) << " ";
        }
        cout << "|" <<endl;
    }
    cout <<"   -----------------"<< endl;
    cout <<"    a b c d e f g h"<< endl;
};

void parseSqaure(const string& sq, int& row, int& col){
    col = sq[0] - 'a';
    int rank = sq[1] - '0';
    row = 8 - rank;
}

string squareName(int row, int col) {
    string s;
    s += ('a'+col);
    s+= ('0' + (8-row));
    return s;
}

bool matchesMove(const Move& m, int fromRow, int fromCol, int toRow, int toCol) {
    return m.from_row == fromRow && m.from_col == fromCol &&
    m.to_row == toRow && m.to_col == toCol;
}

int main(){
    srand(time(0));
    GameState state;
    bool gameOver = isGameOver(state);
    cout << " You play as white and the bot plays as black" << endl;
    print(state.board);

    if (isCheckmate(state)){
        cout << (state.white_to_move ? "White loses because of checkmate" : "Black loses becuase of checkmate") << endl;
    }
    
    if(isStalemate(state)){
            cout << "Stalemate" << endl;
    }

    while (!gameOver) {
        vector<Move> whiteMoves = generateLegalMoves(state);
        string fromStr, toStr;
        char promo;
        bool validInput = false;
        Move playerMove;

        while (!validInput) {
            cout << "your move (white): ";
            cin >> fromStr;
            if (fromStr == "quit"){
                cout << "Game Over" << endl;
                return 0;
            }
            cin >> toStr;
            int fromRow, fromCol, toRow, toCol;
            parseSqaure(fromStr, fromRow, fromCol);
            parseSqaure(toStr, toRow, toCol);

            for (const Move& m : whiteMoves){
                if (matchesMove(m, fromRow, fromCol, toRow, toCol)){
                    playerMove = m;
                    if (m.promotion != '\0'){
                        cout << "what do u want to promote to: ";
                        cin >> promo;
                        cout<<endl;
                        playerMove.promotion = promo;
                    }
                    validInput = true;
                    break;
                }
            }

            if (!validInput) {
                cout<< "ILLEGAL MOVE! try again" << endl;
            }

            }

        makeMove(state, playerMove);
        print(state.board);

        vector<Move> blackMoves = generateLegalMoves(state);
        if (isinCheck(state, false)){
            cout << "Black is in check" << endl;
        }

        int randomIndex = rand() % blackMoves.size();
        Move botMove = blackMoves[randomIndex];

        makeMove(state, botMove);
        cout << "Bot plays: " << squareName(botMove.from_row, botMove.from_col) << " -> " << squareName(botMove.to_row, botMove.to_col) << endl;
        print(state.board);

        if (isinCheck(state, true)) {
            cout << " You are in Check" << endl;
        }
    }
}