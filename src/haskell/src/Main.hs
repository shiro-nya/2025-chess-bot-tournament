module Main (main) where

import Chess

-- use zobrist hash modulo length of legal moves to get a pseudorandom move index
getMove board moves = moves !! (zobristKey board `mod` length moves)

main = do
  board <- getBoard
  submitMove $ getMove board $ legalMoves board
  main
