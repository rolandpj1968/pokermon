#ifndef HAND_EVAL_HPP
#define HAND_EVAL_HPP

#include <tuple>
#include <utility>

#include "types.hpp"

namespace Poker {
  namespace HandEval {
    // Fast hand eval - to come...
    extern HandValueT mkHandValue(const HandT hand);

    // Representation as 6 x 4-bit encoding; HandRanking:RankT:RankT:RankT:RankT:RankT
    // Directly comparible with u32 ordering
    typedef u32 HandEvalCompactT;

    inline HandEvalCompactT make_hand_eval_compact(HandRankingT ranking, RankT r0, RankT r1, RankT r2, RankT r3, RankT r4) {
      return ((u32)ranking << 20) | ((u32)r0 << 16) | ((u32)r1 << 12) | ((u32)r2 << 8) | ((u32)r3 << 4) | (u32)r4;
    }

    typedef std::pair<HandRankingT, std::tuple<RankT, RankT, RankT, RankT, RankT>> HandEvalT;

    inline HandEvalT to_hand_eval(const HandEvalCompactT hand_eval_compact) {
      HandRankingT ranking = (HandRankingT)(hand_eval_compact >> 20);
      RankT r0 = (RankT)((hand_eval_compact >> 16) & 0xf);
      RankT r1 = (RankT)((hand_eval_compact >> 12) & 0xf);
      RankT r2 = (RankT)((hand_eval_compact >>  8) & 0xf);
      RankT r3 = (RankT)((hand_eval_compact >>  4) & 0xf);
      RankT r4 = (RankT)((hand_eval_compact >>  0) & 0xf);

      auto ranks = std::make_tuple(r0, r1, r2, r3, r4);

      return std::make_pair(ranking, ranks);
    }
    
    // Slow hand eval.
    extern HandEvalT eval_hand_7_card_slow(const CardT c0, const CardT c1, const CardT c2, const CardT c3, const CardT c4, const CardT c5, const CardT c6);
    extern HandEvalT eval_hand_holdem_slow(const std::pair<CardT, CardT> hole, const std::tuple<CardT, CardT, CardT> flop, const CardT turn, const CardT river);

    // Faster hand eval using ranks bit fiddling.
    extern HandEvalT eval_hand_5_to_9_card_fast1(HandT hand);
    extern HandEvalT eval_hand_7_card_fast1(const CardT c0, const CardT c1, const CardT c2, const CardT c3, const CardT c4, const CardT c5, const CardT c6);
    extern HandEvalT eval_hand_holdem_fast1(const std::pair<CardT, CardT> hole, const std::tuple<CardT, CardT, CardT> flop, const CardT turn, const CardT river);

    // Preferred hand eval algo
    inline HandEvalT eval_hand_5_to_9_card(HandT hand) {
      return eval_hand_5_to_9_card_fast1(hand);
    }
    inline HandEvalT eval_hand_7_card(const CardT c0, const CardT c1, const CardT c2, const CardT c3, const CardT c4, const CardT c5, const CardT c6) {
      return eval_hand_7_card_fast1(c0, c1, c2, c3, c4, c5, c6);
    }
    inline HandEvalT eval_hand_holdem(const std::pair<CardT, CardT> hole, const std::tuple<CardT, CardT, CardT> flop, const CardT turn, const CardT river){
      return eval_hand_holdem(hole, flop, turn, river);
    }
    
    // Slow hand eval for Omaha...
    extern HandEvalT eval_hand_omaha(const std::tuple<CardT, CardT, CardT, CardT> hole, const std::tuple<CardT, CardT, CardT> flop, const CardT turn, const CardT river);
  }
}

#endif //ndef HAND_EVAL_HPP
