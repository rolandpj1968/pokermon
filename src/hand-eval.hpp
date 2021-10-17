#ifndef HAND_EVAL_HPP
#define HAND_EVAL_HPP

#include <tuple>
#include <utility>

#include "types.hpp"

namespace Poker {
  namespace HandEval {
    // Fast hand eval - to come...
    extern HandValueT mkHandValue(const HandT hand);

    // Slow hand eval.
    extern std::pair<HandRankingT, std::tuple<RankT, RankT, RankT, RankT, RankT>> eval_hand(const std::pair<CardT, CardT> player, const std::tuple<CardT, CardT, CardT> flop, const CardT turn, const CardT river);
  }
}

#endif //ndef HAND_EVAL_HPP
