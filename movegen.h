#pragma once 
#include "types.h"
using namespace std;


bool onBoard(int row, int col);
bool isWhite (char piece);
bool isBlack (char piece);
bool isOwnPiece(char piece, bool whiteMove);
bool isEnemyPiece(char piece, bool whiteMove);
bool isSquareAttacked(const GameState& state, int row, int col, bool byWhite);
bool isinCheck(const GameState& state, bool whiteKing);
vector<Move> generatePsueduoLegalMoves(const GameState& state);
vector<Move> generateLegalMoves(const GameState& state);


