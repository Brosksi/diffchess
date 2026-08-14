#pragma once 

#include "types.h"

void makeMove(GameState & state, const Move& move);
bool isCheckmate(const GameState& state);
bool isStalemate(const GameState& state);
bool isGameOver(const GameState& state);



