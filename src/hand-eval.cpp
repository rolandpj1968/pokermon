#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <map>
#include <set>
#include <tuple>
#include <utility>

#include "hand-eval.hpp"
#include "types.hpp"
#include "util.hpp"

using namespace Poker;

HandValueT mkHandValue(const HandT hand) {
  return HandValueT();
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Slow hand evaluation
//
/////////////////////////////////////////////////////////////////////////////////////////////////////////

// @return (true, high-card-rank) is straight else (false, _)
static std::pair<bool, RankT> eval_straight_slow(const std::set<RankT>& ranks_ace_hi_only) {
  // Ranks only has ace high - augment with ace low if necessary
  std::set<RankT> ranks = ranks_ace_hi_only;
  if (ranks.find(Ace) != ranks.end()) {
    ranks.insert(AceLow);
  }

  bool found_straight = false;
  RankT highest_straight_rank = AceLow;

  int seq_len = 0;
  for(RankT rank = AceLow; rank < NRanks; rank = (RankT)(rank+1)) {
    if(ranks.find(rank) != ranks.end()) {
      seq_len++;
      if(seq_len >= 5) {
	found_straight = true;
	highest_straight_rank = rank;
      }	
    } else {
      seq_len = 0;
    }
  }

  return std::make_pair(found_straight, highest_straight_rank);
}

// @return ranks of the given cards filtered by the given suit
// Expand Ace/AceLow to _both_ Ace and AceLow
std::set<RankT> filter_by_suit_slow(const std::set<CardT>& cards, SuitT suit) {
  std::set<RankT> suited_ranks;
  for(auto it = cards.begin(); it != cards.end(); it++) {
    CardT card = *it;
    if(card.suit == suit) {
      if(card.rank == AceLow || card.rank == Ace) {
	suited_ranks.insert(AceLow);
	suited_ranks.insert(Ace);
      } else {
	suited_ranks.insert(card.rank);
      }
    }
  }
  return suited_ranks;
}

// @return (true, high-card-rank) if straight flush else (false, _)
static std::pair<bool, RankT> eval_straight_flush_slow(const std::set<CardT>& cards, SuitT suit) {
  // Filter the cards by the given suit
  auto suited_ranks = filter_by_suit_slow(cards, suit);

  return eval_straight_slow(suited_ranks);
}

// @return map: rank->count
static std::map<RankT, int> get_rank_counts_slow(const std::set<CardT>& cards) {
  // Count each rank - but Aces are always high.
  std::map<RankT, int> rank_counts;
  for(auto it = cards.begin(); it != cards.end(); it++) {
    RankT rank = it->rank;
    if(rank == AceLow || rank == Ace) {
      rank_counts[Ace]++;
    } else {
      rank_counts[rank]++;
    }
  }
  return rank_counts;
}

// Only valid if there is no higher eval
// @return (true, (quads-rank, kicker_rank)) if four of a kind else (false, _)
static std::pair<bool, std::pair<RankT, RankT>> eval_four_of_a_kind_slow(const std::set<CardT>& cards) {
  // Count each rank - but Aces are always high.
  auto rank_counts = get_rank_counts_slow(cards);

  // Find the highest quads and highest (non-quads) kicker.
  RankT highest_kicker = AceLow;
  RankT quads_rank = AceLow;
  for(RankT rank = Two; rank < NRanks; rank = (RankT)(rank+1)) {
    int rank_count = rank_counts[rank];
    if(rank_count > 0) {
      if(rank_count == 4) {
	quads_rank = rank;
      } else {
	highest_kicker = rank;
      }
    }
  }

  return std::make_pair(quads_rank != AceLow, std::make_pair(quads_rank, highest_kicker));
}

// Only valid if there is no higher eval
// @return (true, (trips-rank, pair_rank)) if full house else (false, _)
static std::pair<bool, std::pair<RankT, RankT>> eval_full_house_slow(const std::set<CardT>& cards) {
  // Count each rank - but Aces are always high.
  std::map<RankT, int> rank_counts;
  for(auto it = cards.begin(); it != cards.end(); it++) {
    RankT rank = it->rank;
    if(rank == AceLow || rank == Ace) {
      rank_counts[Ace]++;
    } else {
      rank_counts[rank]++;
    }
  }

  // Find the highest quads and highest (non-quads) kicker.
  RankT trips_rank = AceLow;
  RankT pair_rank = AceLow;
  for(RankT rank = Two; rank < NRanks; rank = (RankT)(rank+1)) {
    int rank_count = rank_counts[rank];
    if(rank_count > 0) {
      if(rank_count == 3) {
	pair_rank = std::max(trips_rank, pair_rank);
	trips_rank = rank;
      } else if(rank_count == 2) {
	pair_rank = rank;
      }
    }
  }

  return std::make_pair(trips_rank != AceLow && pair_rank != AceLow, std::make_pair(trips_rank, pair_rank));
}

// Only valid if there is no higher eval
// @return (true, (hi-card, 2nd-hi-card, ..., 5th-hi-card)) if flush else (false, _)
static std::pair<bool, std::tuple<RankT, RankT, RankT, RankT, RankT>> eval_flush_slow(const std::set<CardT>& cards, SuitT suit) {
  // Filter the cards by the given suit
  auto suited_ranks = filter_by_suit_slow(cards, suit);
  // Remove AceLow otherwise we count two aces
  suited_ranks.erase(AceLow);

  if(suited_ranks.size() >= 5) {
    auto it = suited_ranks.rbegin();
    RankT r0 = *it++;
    RankT r1 = *it++;
    RankT r2 = *it++;
    RankT r3 = *it++;
    RankT r4 = *it++;
    return std::make_pair(true, std::make_tuple(r0, r1, r2, r3, r4));
  } else {
    return std::make_pair(false, std::tuple<RankT, RankT, RankT, RankT, RankT>());
  }
}

// Only valid if there is no higher eval
// @return (true, high-card-rank) if straight else (false, _)
static std::pair<bool, RankT> eval_straight_slow(const std::set<CardT>& cards) {
  // Translate cards to ranks
  std::set<RankT> ranks;
  for(auto it = cards.begin(); it != cards.end(); it++) {
    ranks.insert(it->rank);
  }

  return eval_straight_slow(ranks);
}

// Only valid if there is no higher eval
// @return (true, (trips-rank, kicker, kicker2) if trips else (false, _)
static std::pair<bool, std::tuple<RankT, RankT, RankT>> eval_trips_slow(const std::set<CardT>& cards) {
  // Count each rank - but Aces are always high.
  auto rank_counts = get_rank_counts_slow(cards);

  // Find the highest set and highest two (non-set) kickers.
  RankT trips_rank = AceLow;
  RankT kicker = AceLow;
  RankT kicker2 = AceLow;
  for(RankT rank = Two; rank < NRanks; rank = (RankT)(rank+1)) {
    int rank_count = rank_counts[rank];
    if(rank_count > 0) {
      if(rank_count == 3) {
	trips_rank = rank;
      } else {
	kicker2 = kicker;
	kicker = rank;
      }
    }
  }

  return std::make_pair(trips_rank != AceLow, std::make_tuple(trips_rank, kicker, kicker2));
}

// Only valid if there is no higher eval
// @return (true, (hi-pair-rank, lo-pair-rank, kicker) if two-pairs else (false, _)
static std::pair<bool, std::tuple<RankT, RankT, RankT>> eval_two_pair_slow(const std::set<CardT>& cards) {
  // Count each rank - but Aces are always high.
  auto rank_counts = get_rank_counts_slow(cards);

  // Find the highest two pairs and highest kicker.
  RankT hi_pair_rank = AceLow;
  RankT lo_pair_rank = AceLow;
  RankT kicker = AceLow;
  for(RankT rank = Two; rank < NRanks; rank = (RankT)(rank+1)) {
    int rank_count = rank_counts[rank];
    if(rank_count > 0) {
      if(rank_count == 2) {
	kicker = std::max(lo_pair_rank, kicker);
	lo_pair_rank = hi_pair_rank;
	hi_pair_rank = rank;
      } else {
	kicker = rank;
      }
    }
  }

  return std::make_pair((hi_pair_rank != AceLow && lo_pair_rank != AceLow), std::make_tuple(hi_pair_rank, lo_pair_rank, kicker));
}

// Only valid if there is no higher eval
// @return (true, (pair-rank, kicker, kicker2, kicker3) if pair else (false, _)
static std::pair<bool, std::tuple<RankT, RankT, RankT, RankT>> eval_pair_slow(const std::set<CardT>& cards) {
  // Count each rank - but Aces are always high.
  auto rank_counts = get_rank_counts_slow(cards);

  // Find the highest pair and highest three kickers.
  RankT pair_rank = AceLow;
  RankT kicker = AceLow;
  RankT kicker2 = AceLow;
  RankT kicker3 = AceLow;
  for(RankT rank = Two; rank < NRanks; rank = (RankT)(rank+1)) {
    int rank_count = rank_counts[rank];
    if(rank_count > 0) {
      if(rank_count == 2) {
	pair_rank = rank;
      } else {
	kicker3 = kicker2;
	kicker2 = kicker;
	kicker = rank;
      }
    }
  }

  return std::make_pair(pair_rank != AceLow, std::make_tuple(pair_rank, kicker, kicker2, kicker3));
}

// Slow hand eval... 7 card hand like Holdem
// @return pair(ranking, 5-characteristic-ranks)
Poker::HandEval::HandEvalT Poker::HandEval::eval_hand_7_card_slow(const CardT c0, const CardT c1, const CardT c2, const CardT c3, const CardT c4, const CardT c5, const CardT c6) {
  // Make sure all cards are unique
  // Also standardize on ace high.
  std::set<CardT> unique_cards;
  unique_cards.insert(to_ace_hi(c0));
  unique_cards.insert(to_ace_hi(c1));
  unique_cards.insert(to_ace_hi(c2));
  unique_cards.insert(to_ace_hi(c3));
  unique_cards.insert(to_ace_hi(c4));
  unique_cards.insert(to_ace_hi(c5));
  unique_cards.insert(to_ace_hi(c6));

  assert(unique_cards.size() == 7 && "duplicate cards");

  // Highest ranked hand is Straight Flush - Royal Flush is just a special case of this.
  // Check for straight flushes - there can only be one suit with a straight flush.
  {
    for (auto suit = Spades; suit < NSuits; suit = (SuitT)(suit+1)) {
      auto straight_flush_eval = eval_straight_flush_slow(unique_cards, suit);
      bool is_straight_flush = straight_flush_eval.first;

      if (is_straight_flush) {
	RankT high_card_rank = straight_flush_eval.second;
	auto hand_ranks = std::make_tuple((RankT)high_card_rank, (RankT)(high_card_rank-1), (RankT)(high_card_rank-2), (RankT)(high_card_rank-3), (RankT)(high_card_rank-4));

	return std::make_pair(StraightFlush, hand_ranks);
      }
    }
  }

  // Next highest ranked hand is Four of a Kind
  {
    auto four_of_a_kind_eval = eval_four_of_a_kind_slow(unique_cards);
    bool is_four_of_a_kind = four_of_a_kind_eval.first;

    if(is_four_of_a_kind) {
      auto quads_and_kicker_ranks = four_of_a_kind_eval.second;
      RankT quads_rank = quads_and_kicker_ranks.first;
      RankT kicker_rank = quads_and_kicker_ranks.second;

      auto hand_ranks = std::make_tuple(quads_rank, quads_rank, quads_rank, quads_rank, kicker_rank);

      return std::make_pair(FourOfAKind, hand_ranks);
    }
  }

  // Next highest ranked hand is a Full House
  {
    auto full_house_eval = eval_full_house_slow(unique_cards);
    bool is_full_house = full_house_eval.first;

    if(is_full_house) {
      auto trips_and_pair_ranks = full_house_eval.second;
      RankT trips_rank = trips_and_pair_ranks.first;
      RankT pair_rank = trips_and_pair_ranks.second;

      auto hand_ranks = std::make_tuple(trips_rank, trips_rank, trips_rank, pair_rank, pair_rank);

      return std::make_pair(FullHouse, hand_ranks);
    }
  }

  // Next highest ranked hand is a Flush
  // Check for flush in each suit - there can only be one suit with a straight flush.
  {
    for(auto suit = Spades; suit < NSuits; suit = (SuitT)(suit+1)) {
      auto flush_eval = eval_flush_slow(unique_cards, suit);
      bool is_flush = flush_eval.first;

      if(is_flush) {
	return std::make_pair(Flush, flush_eval.second);
      }
    }
  }

  // Next highest ranked hand is a Straight
  {
    auto straight_eval = eval_straight_slow(unique_cards);
    bool is_straight = straight_eval.first;

    if(is_straight) {
      RankT high_card_rank = straight_eval.second;
      auto hand_ranks = std::make_tuple((RankT)high_card_rank, (RankT)(high_card_rank-1), (RankT)(high_card_rank-2), (RankT)(high_card_rank-3), (RankT)(high_card_rank-4));

      return std::make_pair(Straight, hand_ranks);
    }
  }

  // Next highest ranked hand is Three of a Kind aka a Set aka Trips
  {
    auto trips_eval = eval_trips_slow(unique_cards);
    bool is_trips = trips_eval.first;

    if(is_trips) {
      RankT trips_rank = std::get<0>(trips_eval.second);
      RankT kicker_rank = std::get<1>(trips_eval.second);
      RankT kicker2_rank = std::get<2>(trips_eval.second);

      return std::make_pair(Set, std::make_tuple(trips_rank, trips_rank, trips_rank, kicker_rank, kicker2_rank));
    }
  }

  // Next highest ranked hand is Two Pairs
  {
    auto two_pair_eval = eval_two_pair_slow(unique_cards);
    bool is_two_pair = two_pair_eval.first;

    if(is_two_pair) {
      RankT hi_pair_rank = std::get<0>(two_pair_eval.second);
      RankT lo_pair_rank = std::get<1>(two_pair_eval.second);
      RankT kicker_rank = std::get<2>(two_pair_eval.second);

      return std::make_pair(TwoPair, std::make_tuple(hi_pair_rank, hi_pair_rank, lo_pair_rank, lo_pair_rank, kicker_rank));
    }
  }

  // Next highest ranked hand is a Pair
  {
    auto pair_eval = eval_pair_slow(unique_cards);
    bool is_pair = pair_eval.first;

    if(is_pair) {
      RankT pair_rank = std::get<0>(pair_eval.second);
      RankT kicker_rank = std::get<1>(pair_eval.second);
      RankT kicker2_rank = std::get<2>(pair_eval.second);
      RankT kicker3_rank = std::get<3>(pair_eval.second);

      return std::make_pair(Pair, std::make_tuple(pair_rank, pair_rank, kicker_rank, kicker2_rank, kicker3_rank));
    }
  }

  // We got nothing
  {
    auto ranks_counts = get_rank_counts_slow(unique_cards);
    auto it = ranks_counts.rbegin();
    RankT r0 = (it++)->first;
    RankT r1 = (it++)->first;
    RankT r2 = (it++)->first;
    RankT r3 = (it++)->first;
    RankT r4 = (it++)->first;
    return std::make_pair(HighCard, std::make_tuple(r0, r1, r2, r3, r4));
  }
}


// Slow hand eval...
Poker::HandEval::HandEvalT Poker::HandEval::eval_hand_holdem_slow(const std::pair<CardT, CardT> hole, const std::tuple<CardT, CardT, CardT> flop, const CardT turn, const CardT river) {
  return eval_hand_7_card_slow(hole.first, hole.second, std::get<0>(flop), std::get<1>(flop), std::get<2>(flop), turn, river);
}

// Find straights by bit-wise and (&) of rank bits at 5 adjacent shifts
static RankBitsT get_straight_hicard_ranks(RankBitsT ranks) {
  RankBitsT straight_bits_01 = ranks & (ranks << 1);
  RankBitsT straight_bits_0123 = straight_bits_01 & (straight_bits_01 << 2);
  // Now has bits set for top-ranks of straights
  RankBitsT straight_hicard_ranks = straight_bits_0123 & (ranks << 4);

  return straight_hicard_ranks;
}

// Extract the highest rank from a ranks bitset
static RankT get_hi_rank(RankBitsT ranks) {
  return (RankT) Util::hibit((u64)ranks);
}

// Remove a rank from a ranks bitset
static RankBitsT remove_rank(RankBitsT ranks, RankT rank) {
  return (RankBitsT) Util::removebit((u64)ranks, (int)rank);
}

// Extract and remove the highest rank from a ranks bitset
static RankT get_and_remove_hi_rank(RankBitsT& ranks) {
  RankT hi_rank = get_hi_rank(ranks);
  ranks = remove_rank(ranks, hi_rank);
  return hi_rank;
}

// Make a compact eval from the hand ranking and five top ranks
static Poker::HandEval::HandEvalCompactT make_hand_eval_compact_from_five_high_ranks(HandRankingT ranking, RankBitsT ranks) {
  RankBitsT ranks_left = ranks;
  RankT r0 = get_and_remove_hi_rank(ranks_left);
  RankT r1 = get_and_remove_hi_rank(ranks_left);
  RankT r2 = get_and_remove_hi_rank(ranks_left);
  RankT r3 = get_and_remove_hi_rank(ranks_left);
  RankT r4 = get_hi_rank(ranks_left);

  return Poker::HandEval::make_hand_eval_compact(ranking, r0, r1, r2, r3, r4);
}
/////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Faster hand evaluation
//
// Valid (only) for 5-9 card hands
//
/////////////////////////////////////////////////////////////////////////////////////////////////////////

// @return HandEvalCompactT
Poker::HandEval::HandEvalCompactT Poker::HandEval::eval_hand_5_to_9_card_compact_fast1(HandT hand) {
  // Aces hi and lo
  RankBitsT ranks_0 = hand.suits[0];
  RankBitsT ranks_1 = hand.suits[1];
  RankBitsT ranks_2 = hand.suits[2];
  RankBitsT ranks_3 = hand.suits[3];

  // Aces hi only
  RankBitsT no_ace_lo_mask = ~ ((RankBitsT)1 << AceLow);
  RankBitsT ranks_no_ace_lo_0 = ranks_0 & no_ace_lo_mask;
  RankBitsT ranks_no_ace_lo_1 = ranks_1 & no_ace_lo_mask;
  RankBitsT ranks_no_ace_lo_2 = ranks_2 & no_ace_lo_mask;
  RankBitsT ranks_no_ace_lo_3 = ranks_3 & no_ace_lo_mask;

  int ranks_no_ace_lo_count_0 = Util::bitcount(ranks_no_ace_lo_0);
  int ranks_no_ace_lo_count_1 = Util::bitcount(ranks_no_ace_lo_1);
  int ranks_no_ace_lo_count_2 = Util::bitcount(ranks_no_ace_lo_2);
  int ranks_no_ace_lo_count_3 = Util::bitcount(ranks_no_ace_lo_3);

  int card_count = ranks_no_ace_lo_count_0 + ranks_no_ace_lo_count_1 + ranks_no_ace_lo_count_2 + ranks_no_ace_lo_count_3;
  assert(5 <= card_count && card_count <= 9);

  // Combine the rank counts (without ace lo) and 14-bit ranks (ace hi and low) for each suit, to
  //   find the suit with the maximum card count, and then the 14-bit ranks of that
  //   suit for later straight flush evaluation.
  u32 count_and_ranks_0 = ((u32)ranks_no_ace_lo_count_0 << 16) | ranks_0;
  u32 count_and_ranks_1 = ((u32)ranks_no_ace_lo_count_1 << 16) | ranks_1;
  u32 count_and_ranks_2 = ((u32)ranks_no_ace_lo_count_2 << 16) | ranks_2;
  u32 count_and_ranks_3 = ((u32)ranks_no_ace_lo_count_3 << 16) | ranks_3;

  u32 max_count_and_ranks_01 = std::max(count_and_ranks_0, count_and_ranks_1);
  u32 max_count_and_ranks_23 = std::max(count_and_ranks_2, count_and_ranks_3);

  u32 max_count_and_ranks = std::max(max_count_and_ranks_01, max_count_and_ranks_23);

  int max_suit_count = (int)(max_count_and_ranks >> 16);
  RankBitsT flush_ranks = (RankBitsT)(max_count_and_ranks & 0xffff);

  // If any suit has at least 5 cards, then we have a flush.
  // For total card count of nine or less (e.g. Hold'em with seven card), there can only be one such suit.
  bool is_flush = max_suit_count >= 5;

  // Identify all ranks present, ignoring suits, by or'ing (|) all rank bits of all suits
  RankBitsT ranks_01 = ranks_0 | ranks_1;
  RankBitsT ranks_23 = ranks_2 | ranks_3;
  RankBitsT ranks = ranks_01 | ranks_23;

  // Find straights by bit-wise and (&) of rank bits at 5 adjacent shifts
  RankBitsT straight_hicard_ranks = get_straight_hicard_ranks(ranks);

  bool is_straight = straight_hicard_ranks != 0;

  // Check for straight flush (unlikely)
  if (is_flush && is_straight) {
    // We have a flush and a straight but not necessarily a straight flush.
    // Check if the flush suit bits make a straight themselves.

    RankBitsT straight_flush_hicard_ranks = get_straight_hicard_ranks(flush_ranks);

    bool is_straight_flush = straight_flush_hicard_ranks != 0;

    if (is_straight_flush) {
      RankT high_card_rank = get_hi_rank(straight_flush_hicard_ranks);

      return Poker::HandEval::make_hand_eval_compact(StraightFlush, high_card_rank, (RankT)(high_card_rank-1), (RankT)(high_card_rank-2), (RankT)(high_card_rank-3), (RankT)(high_card_rank-4));
    }
  }

  RankBitsT ranks_no_ace_lo = ranks & no_ace_lo_mask;
  int ranks_count = Util::bitcount(ranks_no_ace_lo);

  bool no_flush_or_straight = !(is_flush || is_straight);

  // Fast path for hi card only.
  // Unlikely - ~17.4% in Holdem, but faster path for hi card even with branch misprediction
  if (no_flush_or_straight && card_count == ranks_count) {
    return make_hand_eval_compact_from_five_high_ranks(HighCard, ranks);
  }

  // Ranks with an even card count (0, 2, 4) can be identified as 0 bits in xor (^) of rank bits of all suits.
  // After eliminating quads, even card counts can only be pairs or 0-count, and we can eliminate 0-count ranks
  //   using the combined bit-set of ranks of all suits.
  RankBitsT odd_ranks_01 = ranks_0 ^ ranks_1;
  RankBitsT odd_ranks_23 = ranks_2 ^ ranks_3;
  RankBitsT odd_ranks = odd_ranks_01 ^ odd_ranks_23;

  RankBitsT even_ranks = ~odd_ranks;

  RankBitsT zero_count_ranks = ~ranks;

  RankBitsT non_zero_even_ranks = even_ranks & ~zero_count_ranks;

  RankBitsT non_zero_even_ranks_no_ace_lo = non_zero_even_ranks & no_ace_lo_mask;
  int non_zero_evens_count = Util::bitcount(non_zero_even_ranks_no_ace_lo);

  // Fast path for pairs only (there may be more than two pairs in fact)
  // Likely: ~44% pair and ~23.5% two-pairs for Holdem
  if (no_flush_or_straight && card_count == ranks_count + non_zero_evens_count) {
    // Pair or Two-Pair hand
    RankBitsT pair_ranks = non_zero_even_ranks;

    RankT pair_rank = get_hi_rank(pair_ranks);
    RankBitsT ranks_left = remove_rank(ranks, pair_rank);

    // ~2/3 likely pair:two-pairs in Holdem
    if (non_zero_evens_count == 1) {
      // Pair
      // Three kickers - remaining three highest ranks
      RankT kicker_rank = get_and_remove_hi_rank(ranks_left);
      RankT kicker2_rank = get_and_remove_hi_rank(ranks_left);
      RankT kicker3_rank = get_hi_rank(ranks_left);

      return Poker::HandEval::make_hand_eval_compact(Pair, pair_rank, pair_rank, kicker_rank, kicker2_rank, kicker3_rank);

    } else {
      // Two Pair
      RankBitsT pair_ranks_left = remove_rank(pair_ranks, pair_rank);
      RankT second_pair_rank = get_hi_rank(pair_ranks_left);
      ranks_left = remove_rank(ranks_left, second_pair_rank);

      // One kicker - remaining high card
      RankT kicker_rank = get_hi_rank(ranks_left);

      return Poker::HandEval::make_hand_eval_compact(TwoPair, pair_rank, pair_rank, second_pair_rank, second_pair_rank, kicker_rank);
    }
  }

  // Remaining hands are less common
  // Quads - ~0.2% quads, ~2.6% full-house, ~3% flush, ~4.6% straight, ~4.8% trips in Holdem

  // Identify quads by bitwise and (&) of ranks of all suits
  RankBitsT quad_ranks_01 = ranks_0 & ranks_1;
  RankBitsT quad_ranks_23 = ranks_2 & ranks_3;
  RankBitsT quad_ranks = quad_ranks_01 & quad_ranks_23;

  bool is_quads = (quad_ranks != 0);

  if (is_quads) {
    RankT quads_rank = get_hi_rank(quad_ranks);

    // Kicker is the highest remaining card
    RankBitsT non_quads_ranks = remove_rank(ranks, quads_rank);
    RankT kicker_rank = get_hi_rank(non_quads_ranks);

    return Poker::HandEval::make_hand_eval_compact(FourOfAKind, quads_rank, quads_rank, quads_rank, quads_rank, kicker_rank);
  }

  // After eliminating quads, non-zero even card counts can only be pairs
  RankBitsT pair_ranks = non_zero_even_ranks;

  // Trips are identified by a brute force evaluation - would be nice if there were a better way.
  RankBitsT trips_ranks_012 = ranks_0 & ranks_1 & ranks_2 & ~ranks_3;
  RankBitsT trips_ranks_013 = ranks_0 & ranks_1 & ~ranks_2 & ranks_3;
  RankBitsT trips_ranks_023 = ranks_0 & ~ranks_1 & ranks_2 & ranks_3;
  RankBitsT trips_ranks_123 = ~ranks_0 & ranks_1 & ranks_2 & ranks_3;

  RankBitsT trips_ranks = trips_ranks_012 | trips_ranks_013 | trips_ranks_023 | trips_ranks_123;
  RankBitsT trips_ranks_no_ace_lo = trips_ranks & no_ace_lo_mask;
  int trips_count = Util::bitcount(trips_ranks_no_ace_lo);

  bool has_trips = trips_ranks != 0;
  bool has_pair = pair_ranks != 0;

  // If we have multiple trips, this is actually (also) a full house, using (only) two of the second trips rank.
  bool is_full_house = has_trips && (trips_count > 1 || has_pair);

  if (is_full_house) {
    RankT trips_rank = get_hi_rank(trips_ranks);
    RankT pair_rank;
    if (trips_count > 1) {
      // Second trips rank is used for the pair in the full house
      RankBitsT trips_ranks_left = remove_rank(trips_ranks, trips_rank);
      pair_rank = get_hi_rank(trips_ranks_left);
    } else {
      // Highest pair completes the full house
      pair_rank = get_hi_rank(pair_ranks);
    }

    return Poker::HandEval::make_hand_eval_compact(FullHouse, trips_rank, trips_rank, trips_rank, pair_rank, pair_rank);
  }

  // With quads and full house out of the way, back to flush and straight

  if (is_flush) {
    // Flush is characterized by the five (high) cards involved.
    return make_hand_eval_compact_from_five_high_ranks(Flush, flush_ranks);
  }

  if (is_straight) {
    RankT high_card_rank = get_hi_rank(straight_hicard_ranks);

    return Poker::HandEval::make_hand_eval_compact(Straight, (RankT)high_card_rank, (RankT)(high_card_rank-1), (RankT)(high_card_rank-2), (RankT)(high_card_rank-3), (RankT)(high_card_rank-4));
  }

  // Hi card, pair and two-pairs handled above - we must have trips
  assert(has_trips);

  RankT trips_rank = get_hi_rank(trips_ranks);

  // Two kicker ranks are highest cards excluding the trips rank.
  // At this stage there are no (other) trips or pairs.
  RankBitsT ranks_left = remove_rank(ranks, trips_rank);
  RankT kicker_rank = get_and_remove_hi_rank(ranks_left);
  RankT kicker2_rank = get_hi_rank(ranks_left);

  return Poker::HandEval::make_hand_eval_compact(Set, trips_rank, trips_rank, trips_rank, kicker_rank, kicker2_rank);
}

// Faster hand eval... 7 hand card like Holdem
// @return pair(ranking, 5-characteristic-ranks)
Poker::HandEval::HandEvalT Poker::HandEval::eval_hand_7_card_fast1(const CardT c0, const CardT c1, const CardT c2, const CardT c3, const CardT c4, const CardT c5, const CardT c6) {
  HandT hand = HandT(c0).add(c1).add(c2).add(c3).add(c4).add(c5).add(c6);

  return eval_hand_5_to_9_card_fast1(hand);
}

// Faster Holdem hand eval...
Poker::HandEval::HandEvalT Poker::HandEval::eval_hand_holdem_fast1(const std::pair<CardT, CardT> hole, const std::tuple<CardT, CardT, CardT> flop, const CardT turn, const CardT river) {
  return Poker::HandEval::eval_hand_7_card_fast1(hole.first, hole.second, std::get<0>(flop), std::get<1>(flop), std::get<2>(flop), turn, river);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Slow Omaha hand evaluation
//
// Omaha is special in that the best hand MUST comprise exactly two hole cards and three table cards.
//
/////////////////////////////////////////////////////////////////////////////////////////////////////////


// Slow hand eval for Omaha...
// Omaha hands _have_ to include exactly two hole cards and three table cards.
// Here we iterate over all 60 possible combo's - select 2 from 4 (hole cards) times select 3 from 5 (table cards)
Poker::HandEval::HandEvalT Poker::HandEval::eval_hand_omaha_slow(const std::tuple<CardT, CardT, CardT, CardT> hole, const std::tuple<CardT, CardT, CardT> flop, const CardT turn, const CardT river) {

  CardT hole0 = std::get<0>(hole);
  CardT hole1 = std::get<1>(hole);
  CardT hole2 = std::get<2>(hole);
  CardT hole3 = std::get<3>(hole);

  std::pair<CardT, CardT> hole_card_pairs[6] = {
    std::make_pair(hole0, hole1),
    std::make_pair(hole0, hole2),
    std::make_pair(hole0, hole3),
    std::make_pair(hole1, hole2),
    std::make_pair(hole1, hole3),
    std::make_pair(hole2, hole3),
  };


  CardT table0 = std::get<0>(flop);
  CardT table1 = std::get<1>(flop);
  CardT table2 = std::get<2>(flop);
  CardT table3 = turn;
  CardT table4 = river;

  std::tuple<CardT, CardT, CardT> table_card_triples[10] = {
    std::make_tuple(table0, table1, table2),
    std::make_tuple(table0, table1, table3),
    std::make_tuple(table0, table1, table4),
    std::make_tuple(table0, table2, table3),
    std::make_tuple(table0, table2, table4),
    std::make_tuple(table0, table3, table4),
    std::make_tuple(table1, table2, table3),
    std::make_tuple(table1, table2, table4),
    std::make_tuple(table1, table3, table4),
    std::make_tuple(table2, table3, table4),
  };

  bool have_hand_eval = false;
  Poker::HandEval::HandEvalT best_hand_eval;

  for (int table_card_triple_no = 0; table_card_triple_no < 10; table_card_triple_no++) {

    std::tuple<CardT, CardT, CardT> table_card_triple = table_card_triples[table_card_triple_no];

    HandT table_cards_hand = HandT(std::get<0>(table_card_triple)).add(std::get<1>(table_card_triple)).add(std::get<2>(table_card_triple));

    for (int hole_card_pair_no = 0; hole_card_pair_no < 6; hole_card_pair_no++) {

      std::pair<CardT, CardT> hole_card_pair = hole_card_pairs[hole_card_pair_no];

      HandT hand = table_cards_hand;
      hand.add(hole_card_pair.first);
      hand.add(hole_card_pair.second);

      auto hand_eval = eval_hand_5_to_9_card_fast1(hand);

      if (!have_hand_eval || best_hand_eval < hand_eval) {
	best_hand_eval = hand_eval;
	have_hand_eval = true;
      }
    }
  }

  return best_hand_eval;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Faster Omaha hand evaluation
//
// Omaha is special in that the best hand MUST comprise exactly two hole cards and three table cards.
//
/////////////////////////////////////////////////////////////////////////////////////////////////////////

// @#$%@#$% TODO redo with refactor...

// @return HandEvalCompactT
Poker::HandEval::HandEvalCompactT Poker::HandEval::eval_hand_omaha_compact_fast1(HandT hole_hand, HandT table_hand) {

  // Aces hi only
  u64 no_ace_lo_mask = ~ ((u64)1 << AceLow);
  
  // Omaha hands MUST be exactly four hole cards and five table cards
  u64 ranks14_hole_no_ace_lo = hole_hand.hand & no_ace_lo_mask;
  u64 ranks14_table_no_ace_lo = table_hand.hand & no_ace_lo_mask;
  assert(Util::bitcount(ranks14_hole_no_ace_lo) == 4 && Util::bitcount(ranks14_table_no_ace_lo) == 5);

  // Combine hole and table cards for "optimistic" evaluation of flush and straight potential.
  HandT hand(hole_hand, table_hand);
  
  // Aces hi and lo
  u64 ranks14_0 = hand.suits[0];
  u64 ranks14_1 = hand.suits[1];
  u64 ranks14_2 = hand.suits[2];
  u64 ranks14_3 = hand.suits[3];

  u64 ranks14_no_ace_lo_0 = ranks14_0 & no_ace_lo_mask;
  u64 ranks14_no_ace_lo_1 = ranks14_1 & no_ace_lo_mask;
  u64 ranks14_no_ace_lo_2 = ranks14_2 & no_ace_lo_mask;
  u64 ranks14_no_ace_lo_3 = ranks14_3 & no_ace_lo_mask;

  int ranks14_no_ace_lo_count_0 = Util::bitcount(ranks14_no_ace_lo_0);
  int ranks14_no_ace_lo_count_1 = Util::bitcount(ranks14_no_ace_lo_1);
  int ranks14_no_ace_lo_count_2 = Util::bitcount(ranks14_no_ace_lo_2);
  int ranks14_no_ace_lo_count_3 = Util::bitcount(ranks14_no_ace_lo_3);

  int card_count = ranks14_no_ace_lo_count_0 + ranks14_no_ace_lo_count_1 + ranks14_no_ace_lo_count_2 + ranks14_no_ace_lo_count_3;
  // Hole and table cards must be distinct.
  assert(card_count == 9);

  // Combine the rank counts (without ace lo) and suit to find the suit with the maximum card count.
  //   There can be only one candidate flush suit since there are nine cards in total.
  //   We then need to check that a flush can be made with exactly two hole cards and three table cards.
  u32 count_and_suit_0 = ((u32)ranks14_no_ace_lo_count_0 << 8) | 0;
  u32 count_and_suit_1 = ((u32)ranks14_no_ace_lo_count_1 << 8) | 1;
  u32 count_and_suit_2 = ((u32)ranks14_no_ace_lo_count_2 << 8) | 2;
  u32 count_and_suit_3 = ((u32)ranks14_no_ace_lo_count_3 << 8) | 3;

  u32 max_count_and_suit_01 = std::max(count_and_suit_0, count_and_suit_1);
  u32 max_count_and_suit_23 = std::max(count_and_suit_2, count_and_suit_3);

  u32 max_count_and_suit = std::max(max_count_and_suit_01, max_count_and_suit_23);

  int max_suit_count = max_count_and_suit >> 8;
  u64 flush_suit = max_count_and_suit & 0xff;

  // If any suit has at least 5 cards, then we have a candidate flush.
  // For total card count of nine cards, there can only be one such suit.
  // For Omaha we still need to check that we can make a flush with exactly two hole cards and three table cards.
  bool is_candidate_flush = max_suit_count >= 5;
  bool is_flush = false;

  // Check that the candidate flush obeys Omaha constraints - two hole cards and three table cards.
  // Unlikely branch.
  if (is_candidate_flush) {
    u64 flush_hole_ranks14 = hole_hand.suits[flush_suit];
    u64 flush_hole_ranks14_no_ace_lo = flush_hole_ranks14 & no_ace_lo_mask;

    int flush_hole_ranks14_no_ace_lo_count = Util::bitcount(flush_hole_ranks14_no_ace_lo);

    u64 flush_table_ranks14 = table_hand.suits[flush_suit];
    u64 flush_table_ranks14_no_ace_lo = flush_table_ranks14 & no_ace_lo_mask;

    int flush_table_ranks14_no_ace_lo_count = Util::bitcount(flush_table_ranks14_no_ace_lo);
    
    // We need at least 2 hole cards and at least three table cards in the candidate flush suit 
    is_flush = (flush_hole_ranks14_no_ace_lo_count >= 2) && (flush_table_ranks14_no_ace_lo_count >= 3);

    // We will identify the best flush cards later...
  }

  // TODO @#$%@#$@%
  u64 flush_ranks14 = 0; // TODO TODO - just to compile for now

  // Identify all ranks present, ignoring suits, by or'ing (|) all rank bits of all suits
  u64 ranks14_01 = ranks14_0 | ranks14_1;
  u64 ranks14_23 = ranks14_2 | ranks14_3;
  u64 ranks14 = ranks14_01 | ranks14_23;

  // Find straights by bit-wise and (&) of rank bits at 5 adjacent shifts
  u64 straight_hicard_ranks14 = get_straight_hicard_ranks(ranks14);

  bool is_candidate_straight = straight_hicard_ranks14 != 0;
  bool is_straight = false;

  // Filter the candidate straights according to Omaha constraints - two hole cards and three table cards.
  if (is_candidate_straight) { /*TODO*/ }

  // @#$%@#$%@#$% TODO...

  // Check for straight flush (unlikely)
  if (is_flush && is_straight) {
    // We have a flush and a straight but not necessarily a straight flush.
    // Check if the flush suit bits make a straight themselves.

    u64 straight_flush_hicard_ranks14 = get_straight_hicard_ranks(flush_ranks14);

    bool is_straight_flush = straight_flush_hicard_ranks14 != 0;

    if (is_straight_flush) {
      RankT high_card_rank = get_hi_rank(straight_flush_hicard_ranks14);

      return Poker::HandEval::make_hand_eval_compact(StraightFlush, (RankT)high_card_rank, (RankT)(high_card_rank-1), (RankT)(high_card_rank-2), (RankT)(high_card_rank-3), (RankT)(high_card_rank-4));
    }
  }

  u64 ranks14_no_ace_lo = ranks14 & no_ace_lo_mask;
  int ranks_count = Util::bitcount(ranks14_no_ace_lo);

  bool no_flush_or_straight = !(is_flush || is_straight);

  // Fast path for hi card only.
  // Unlikely - ~17.4% in Holdem, but faster path for hi card even with branch misprediction
  if (no_flush_or_straight && card_count == ranks_count) {
    return make_hand_eval_compact_from_five_high_ranks(HighCard, ranks14);
  }

  // Ranks with an even card count (0, 2, 4) can be identified as 0 bits in xor (^) of rank bits of all suits.
  // After eliminating quads, even card counts can only be pairs or 0-count, and we can eliminate 0-count ranks
  //   using the combined bit-set of ranks of all suits.
  u64 odd_ranks14_01 = ranks14_0 ^ ranks14_1;
  u64 odd_ranks14_23 = ranks14_2 ^ ranks14_3;
  u64 odd_ranks14 = odd_ranks14_01 ^ odd_ranks14_23;

  u64 even_ranks14 = ~odd_ranks14;

  u64 zero_count_ranks14 = ~ranks14;

  u64 non_zero_even_ranks14 = even_ranks14 & ~zero_count_ranks14;

  u64 non_zero_even_ranks14_no_ace_lo = non_zero_even_ranks14 & no_ace_lo_mask;
  int non_zero_evens_count = Util::bitcount(non_zero_even_ranks14_no_ace_lo);

  // Fast path for pairs only (there may be more than two pairs in fact)
  // Likely: ~44% pair and ~23.5% two-pairs for Holdem
  if (no_flush_or_straight && card_count == ranks_count + non_zero_evens_count) {
    // Pair or Two-Pair hand
    u64 pair_ranks14 = non_zero_even_ranks14;

    RankT pair_rank = get_hi_rank(pair_ranks14);
    u64 ranks14_left = remove_rank(ranks14, pair_rank);

    // ~2/3 likely pair:two-pairs in Holdem
    if (non_zero_evens_count == 1) {
      // Pair
      // Three kickers - remaining three highest ranks
      RankT kicker_rank = get_hi_rank(ranks14_left);
      ranks14_left = remove_rank(ranks14_left, kicker_rank);
      RankT kicker2_rank = get_hi_rank(ranks14_left);
      ranks14_left = remove_rank(ranks14_left, kicker2_rank);
      RankT kicker3_rank = get_hi_rank(ranks14_left);

      return Poker::HandEval::make_hand_eval_compact(Pair, pair_rank, pair_rank, kicker_rank, kicker2_rank, kicker3_rank);

    } else {
      // Two Pair
      u64 pair_ranks14_left = remove_rank(pair_ranks14, pair_rank);
      RankT second_pair_rank = get_hi_rank(pair_ranks14_left);
      ranks14_left = remove_rank(ranks14_left, second_pair_rank);

      // One kicker - remaining high card
      RankT kicker_rank = get_hi_rank(ranks14_left);

      return Poker::HandEval::make_hand_eval_compact(TwoPair, pair_rank, pair_rank, second_pair_rank, second_pair_rank, kicker_rank);
    }
  }

  // Remaining hands are less common
  // Quads - ~0.2% quads, ~2.6% full-house, ~3% flush, ~4.6% straight, ~4.8% trips in Holdem

  // Identify quads by bitwise and (&) of ranks of all suits
  u64 quad_ranks14_01 = ranks14_0 & ranks14_1;
  u64 quad_ranks14_23 = ranks14_2 & ranks14_3;
  u64 quad_ranks14 = quad_ranks14_01 & quad_ranks14_23;

  bool is_quads = (quad_ranks14 != 0);

  if (is_quads) {
    RankT quads_rank = get_hi_rank(quad_ranks14);

    // Kicker is the highest remaining card
    u64 non_quads_ranks14 = remove_rank(ranks14, quads_rank);
    RankT kicker_rank = get_hi_rank(non_quads_ranks14);

    return Poker::HandEval::make_hand_eval_compact(FourOfAKind, quads_rank, quads_rank, quads_rank, quads_rank, kicker_rank);
  }

  // After eliminating quads, non-zero even card counts can only be pairs
  u64 pair_ranks14 = non_zero_even_ranks14;

  // Trips are identified by a brute force evaluation - would be nice if there were a better way.
  u64 trips_ranks14_012 = ranks14_0 & ranks14_1 & ranks14_2 & ~ranks14_3;
  u64 trips_ranks14_013 = ranks14_0 & ranks14_1 & ~ranks14_2 & ranks14_3;
  u64 trips_ranks14_023 = ranks14_0 & ~ranks14_1 & ranks14_2 & ranks14_3;
  u64 trips_ranks14_123 = ~ranks14_0 & ranks14_1 & ranks14_2 & ranks14_3;

  u64 trips_ranks14 = trips_ranks14_012 | trips_ranks14_013 | trips_ranks14_023 | trips_ranks14_123;
  u64 trips_ranks14_no_ace_lo = trips_ranks14 & no_ace_lo_mask;
  int trips_count = Util::bitcount(trips_ranks14_no_ace_lo);

  bool has_trips = trips_ranks14 != 0;
  bool has_pair = pair_ranks14 != 0;

  // If we have multiple trips, this is actually (also) a full house, using (only) two of the second trips rank.
  bool is_full_house = has_trips && (trips_count > 1 || has_pair);

  if (is_full_house) {
    RankT trips_rank = get_hi_rank(trips_ranks14);
    RankT pair_rank;
    if (trips_count > 1) {
      // Second trips rank is used for the pair in the full house
      u64 trips_ranks14_left = remove_rank(trips_ranks14, trips_rank);
      pair_rank = get_hi_rank(trips_ranks14_left);
    } else {
      // Highest pair completes the full house
      pair_rank = get_hi_rank(pair_ranks14);
    }

    return Poker::HandEval::make_hand_eval_compact(FullHouse, trips_rank, trips_rank, trips_rank, pair_rank, pair_rank);
  }

  // With quads and full house out of the way, back to flush and straight

  if (is_flush) {
    // Flush is characterized by the five (high) cards involved.
    return make_hand_eval_compact_from_five_high_ranks(Flush, flush_ranks14);
  }

  if (is_straight) {
    RankT high_card_rank = get_hi_rank(straight_hicard_ranks14);

    return Poker::HandEval::make_hand_eval_compact(Straight, (RankT)high_card_rank, (RankT)(high_card_rank-1), (RankT)(high_card_rank-2), (RankT)(high_card_rank-3), (RankT)(high_card_rank-4));
  }

  // Hi card, pair and two-pairs handled above - we must have trips
  assert(has_trips);

  RankT trips_rank = get_hi_rank(trips_ranks14);

  // Two kicker ranks are highest cards excluding the trips rank.
  // At this stage there are no (other) trips or pairs.
  u64 ranks14_left = remove_rank(ranks14, trips_rank);
  RankT kicker_rank = get_hi_rank(ranks14_left);
  ranks14_left = remove_rank(ranks14_left, kicker_rank);
  RankT kicker2_rank = get_hi_rank(ranks14_left);

  return Poker::HandEval::make_hand_eval_compact(Set, trips_rank, trips_rank, trips_rank, kicker_rank, kicker2_rank);
}

