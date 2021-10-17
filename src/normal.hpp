#ifndef NORMAL_HPP
#define NORMAL_HPP

#include <utility>

#include "types.hpp"

namespace Poker {
  // Normalised Texas Hold'em hand.
  // Two cards - the higher card is suit 0, other is suit 0/1

  inline std::pair<CardT, CardT> holdem_normal(CardT card0, CardT card1) {
    RankT rank0 = card0.rank, rank1 = card1.rank;
    
    if(card0.rank < card1.rank) {
      std::swap(rank0, rank1);
    }

    auto norm_card0 = CardT(Spades, rank0);
    auto norm_card1 = CardT((card0.suit == card1.suit ? Spades : Hearts), rank1);

    return std::make_pair(norm_card0, norm_card1);
  }
}// namespace Poker

#endif //ndef NORMAL_HPP
