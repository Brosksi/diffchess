#Trying something new with Chess bots : August 14 2026

for a long time chess bots have remained at their core to be just search alogrithms, this is a reserch project i have aimed to changed that. For now i will be focusing on Diffusion models
and GNNs and experimenting with them in chess. Currently im reading other works that have been done here. My goal is to build an engine that can rival other medium to high grade chess engines.
Currently i have built the engine section of the project, i have 3 files.

 **movegen.cpp** : this file is responsible for the ground rules. I first defined some simple functions such as is this player white, is this our player, is this on the board, etc. Then i setup a isSqaureAttacked function to tackle the king
bassically u first generate all moves not considering the king and whether or not the king is at harm (this is done in PseudoLegalMoves) and then we create the LegalMoves function which reteurns the real list of moves

**game.cpp** : this file is solely responsible for making the actual moves themselves, before you make a move, the LegalMove engine would already have a list of moves, each of these moves has different parameters such as castling, enpassant, etc. The actual moving of these normal and special scenarios is handled in this part of the code

**main.cpp** : this is the interface between the player and the bot, i didnt add any minmax or alpha beta pruning or any other search algorithm becuase thats not the goal. Here we bassically create the input and output interface and the moving of the board interface.

This project as of now is in its very early stages, we have just created a random bot that makes random moves. I will contiue working on this project. Hoping to get something meaningful out of this. 

- Divay agarwal



