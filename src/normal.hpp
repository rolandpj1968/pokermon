#ifndef NORMAL_HPP
#define NORMAL_HPP

#include <cassert>
#include <utility>

#include "types.hpp"
#include "util.hpp"

namespace Poker {
  // Normalised Texas Hold'em hand.
  // Two cards - the higher card is suit 0, other is suit 0/1
  inline std::pair<CardT, CardT> holdem_hole_normal(CardT card0, CardT card1) {
    RankT rank0 = to_ace_hi(card0.rank);
    RankT rank1 = to_ace_hi(card1.rank);

    //printf("holdem_normal rank0 %c -> %c. rank1 %c -> %c\n", RANK_CHARS[card0.rank], RANK_CHARS[rank0], RANK_CHARS[card1.rank], RANK_CHARS[rank1]);

    Util::sort_desc(rank0, rank1);

    //printf("      ordered - rank0 %c rank1 %c\n", RANK_CHARS[rank0], RANK_CHARS[rank1]);

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

    return std::make_tuple(flop_norm[0], flop_norm[1], flop_norm[2]);
  }
  
  // Normalised Texas Hold'em flop, for pocket-pair hole cards
  inline std::tuple<CardT, CardT, CardT> holdem_hole_normal_pocket_pair(SuitT suit0, SuitT suit1, std::tuple<CardT, CardT, CardT> flop) {
    HandT flop_hand = flop_to_hand(flop);
    // #$%#$%
  }
  
  // Normalised Texas Hold'em flop, for off-suit non-pocket-pair hole cards
  inline std::tuple<CardT, CardT, CardT> holdem_hole_normal_pocket_pair(CardT card0, CardT card1, std::tuple<CardT, CardT, CardT> flop) {
    HandT flop_hand = flop_to_hand(flop);
    // #$%#$%
  }
  
  // Normalised Texas Hold'em flop, relative to the hole cards
  inline std::tuple<CardT, CardT, CardT> holdem_hole_normal_offsuit(CardT card0, CardT card1, std::tuple<CardT, CardT, CardT> flop) {
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
  
}// namespace Poker

#endif //ndef NORMAL_HPP
