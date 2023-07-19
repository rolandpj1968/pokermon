#ifndef HAND_EVAL_HPP
#define HAND_EVAL_HPP

#include <tuple>
#include <utility>

#include "types.hpp"

namespace Poker {
  namespace HandEval {
    // Fast hand eval - to come...
    extern HandValueT mkHandValue(const HandT hand);

    typedef std::pair<HandRankingT, std::tuple<RankT, RankT, RankT, RankT, RankT>> HandEvalT;
    
    // Slow hand eval.
    extern HandEvalT eval_hand_7_card_slow(const CardT c0, const CardT c1, const CardT c2, const CardT c3, const CardT c4, const CardT c5, const CardT c6);
    extern HandEvalT eval_hand_holdem_slow(const std::pair<CardT, CardT> hole, const std::tuple<CardT, CardT, CardT> flop, const CardT turn, const CardT river);

    // Faster hand eval using ranks bit fiddling.
    extern HandEvalT eval_hand_5_to_9_card_fast1(HandT hand);
    extern HandEvalT eval_hand_7_card_fast1(const CardT c0, const CardT c1, const CardT c2, const CardT c3, const CardT c4, const CardT c5, const CardT c6);
    extern HandEvalT eval_hand_holdem_fast1(const std::pair<CardT, CardT> hole, const std::tuple<CardT, CardT, CardT> flop, const CardT turn, const CardT river);
    
    // Slow hand eval for Omaha...
    extern HandEvalT eval_hand_omaha(const std::tuple<CardT, CardT, CardT, CardT> hole, const std::tuple<CardT, CardT, CardT> flop, const CardT turn, const CardT river);
  }
}

#endif //ndef HAND_EVAL_HPP
