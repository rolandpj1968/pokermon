#ifndef NORMAL_HPP
#define NORMAL_HPP

#include <algorithm>
#include <cassert>
#include <utility>

#include "types.hpp"
#include "util.hpp"

namespace Poker {
  namespace Normal {
    
    // Normalised Texas Hold'em hand.
    // Two cards - the higher card is suit 0, other is suit 0/1
    inline std::pair<CardT, CardT> holdem_hole_normal(CardT card0, CardT card1) {
      RankT rank0 = to_ace_hi(card0.rank);
      RankT rank1 = to_ace_hi(card1.rank);

      Util::sort_desc(rank0, rank1);

      auto norm_card0 = CardT(Spades, rank0);
      auto norm_card1 = CardT((card0.suit == card1.suit ? Spades : Hearts), rank1);

      return std::make_pair(norm_card0, norm_card1);
    }

    inline HandT flop_to_hand(std::tuple<CardT, CardT, CardT> flop) {
      const CardT cards[3] = { std::get<0>(flop), std::get<1>(flop), std::get<2>(flop) };
      return mkHand(cards, 3);
    }

    // Accumulate cards out of RankBitsT into a card array
    inline void add_cards(SuitT suit, RankBitsT ranks, std::size_t& cards_index, CardT cards[], std::size_t cards_len) {

      if(ranks == 0) {
	return;
      }

      for(RankT rank = Ace; rank > AceLow; rank = (RankT)((int)rank-1)) {
	if(ranks & RankBits[rank]) {
	  assert(cards_index < cards_len);
	  cards[cards_index++] = CardT(suit, rank);
	}
      }
    }

#ifdef INCOMPLETE_TODO

    // $#$%#$% should be flop_normal
    
    // Normalised Texas Hold'em flop, for suited hole cards
    inline std::tuple<CardT, CardT, CardT> holdem_hole_normal_suited(SuitT suit, std::tuple<CardT, CardT, CardT> flop) {
      HandT flop_hand = flop_to_hand(flop);

      // Cards with same suit as hole cards
      RankBitsT hole_suit_ranks = flop_hand.suits[suit];

      // Remove hole suit cards from the flop hand
      flop_hand.suits[suit] = 0;

      // Sort the flop suits by rank bits in descending order
      std::sort(flop_hand.suits, flop_hand.suits + NSuits, std::greater<>());

      CardT flop_norm[3];
      std::size_t flop_norm_index = 0;

      add_cards(Spades, hole_suit_ranks, flop_norm_index, flop_norm, 3);
      add_cards(Hearts, flop_hand.suits[0], flop_norm_index, flop_norm, 3);
      add_cards(Diamonds, flop_hand.suits[1], flop_norm_index, flop_norm, 3);
      add_cards(Clubs, flop_hand.suits[2], flop_norm_index, flop_norm, 3);

      assert(flop_norm_index == 3);

      return std::make_tuple(flop_norm[0], flop_norm[1], flop_norm[2]);
    }
  
    // Normalised Texas Hold'em flop, for pocket-pair hole cards
    inline std::tuple<CardT, CardT, CardT> holdem_hole_normal_pocket_pair(SuitT suit0, SuitT suit1, std::tuple<CardT, CardT, CardT> flop) {
      HandT flop_hand = flop_to_hand(flop);
      // #$%#$%
    }
  
    // Normalised Texas Hold'em flop, for off-suit non-pocket-pair hole cards
    inline std::tuple<CardT, CardT, CardT> holdem_hole_normal_offsuit(CardT card0, CardT card1, std::tuple<CardT, CardT, CardT> flop) {
      HandT flop_hand = flop_to_hand(flop);
      // #$%#$%
    }
  
    // Normalised Texas Hold'em flop, relative to the hole cards
    inline std::tuple<CardT, CardT, CardT> holdem_hole_normal(CardT card0, CardT card1, std::tuple<CardT, CardT, CardT> flop) {
      RankT rank0 = to_ace_hi(card0.rank);
      RankT rank1 = to_ace_hi(card1.rank);

      if(card0.suit == card1.suit) {
	return holdem_hole_normal_suited(card0.suit, flop);
      } else if(rank0 == rank1) {
	return holdem_hole_normal_pocket_pair(card0.suit, card1.suit, flop);
      } else {
	return holdem_hole_normal_offsuit(card0, card1, flop);
      }
    }
#endif // INCOMPLETE_TODO

    // Normalised Omaha hand.
    // Four cards - we sort the suits by bitmap (aces-high) and then map back to CardT's
    inline std::tuple<CardT, CardT, CardT, CardT> omaha_hole_normal(CardT card0, CardT card1, CardT card2, CardT card3) {
      card0 = to_ace_hi(card0);
      card1 = to_ace_hi(card1);
      card2 = to_ace_hi(card2);
      card3 = to_ace_hi(card3);

      const CardT cards[4] = { card0, card1, card2, card3 };
      HandT hand = mkHand(cards, 4);

      std::sort(hand.suits, hand.suits + 4, std::greater<RankBitsT>());
    
      CardT norm_cards[4] = {};
      std::size_t norm_index = 0;

      add_cards(Spades, hand.suits[0], norm_index, norm_cards, 4);
      add_cards(Hearts, hand.suits[1], norm_index, norm_cards, 4);
      add_cards(Diamonds, hand.suits[2], norm_index, norm_cards, 4);
      add_cards(Clubs, hand.suits[3], norm_index, norm_cards, 4);

      assert(norm_index == 4);

      return std::make_tuple(norm_cards[0], norm_cards[1], norm_cards[2], norm_cards[3]);
    }

    // Determined empirically
    const std::size_t N_OMAHA_HOLE_NORMALS = 16432;
    // Index for invalid hold card sets (due to duplicates)
    const int INVALID_OMAHA_HOLE_NORMAL_INDEX = -1;

    // Call before using omaha_hole_normal_index() or omaha_hole_normal_from_index()
    extern void init_omaha_hold_normal_index();
    // @return index in [0, N_OMAHA_HOLD_NORMALS) of normalised Omaha hole cards
    extern int omaha_hole_normal_index(CardT card0, CardT card1, CardT card2, CardT card3);
    // @return normalised Omaha hole cards for given index in [0, N_OMAHA_HOLD_NORMALS)
    extern std::tuple<CardT, CardT, CardT, CardT> omaha_hole_normal_from_index(int index);

  } // namespace Normal
} // namespace Poker

#endif //ndef NORMAL_HPP
