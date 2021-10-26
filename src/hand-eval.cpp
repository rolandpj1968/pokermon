#include <cstdio>
#include <cstdlib>
#include <map>
#include <set>
#include <tuple>
#include <utility>

#include "hand-eval.hpp"
#include "types.hpp"

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
static std::pair<bool, RankT> eval_straight(const std::set<RankT>& ranks) {
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
std::set<RankT> filter_by_suit(const std::set<CardT>& cards, SuitT suit) {
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
static std::pair<bool, RankT> eval_straight_flush(const std::set<CardT>& cards, SuitT suit) {
  // Filter the cards by the given suit
  auto suited_ranks = filter_by_suit(cards, suit);

  return eval_straight(suited_ranks);
}

// @return map: rank->count
static std::map<RankT, int> get_rank_counts(const std::set<CardT>& cards) {
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
static std::pair<bool, std::pair<RankT, RankT>> eval_four_of_a_kind(const std::set<CardT>& cards) {
  // Count each rank - but Aces are always high.
  auto rank_counts = get_rank_counts(cards);

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
static std::pair<bool, std::pair<RankT, RankT>> eval_full_house(const std::set<CardT>& cards) {
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
static std::pair<bool, std::tuple<RankT, RankT, RankT, RankT, RankT>> eval_flush(const std::set<CardT>& cards, SuitT suit) {
  // Filter the cards by the given suit
  auto suited_ranks = filter_by_suit(cards, suit);
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
static std::pair<bool, RankT> eval_straight(const std::set<CardT>& cards) {
  // Translate cards to ranks
  std::set<RankT> ranks;
  for(auto it = cards.begin(); it != cards.end(); it++) {
    ranks.insert(it->rank);
  }

  return eval_straight(ranks);
}

// Only valid if there is no higher eval
// @return (true, (trips-rank, kicker, kicker2) if trips else (false, _)
static std::pair<bool, std::tuple<RankT, RankT, RankT>> eval_trips(const std::set<CardT>& cards) {
  // Count each rank - but Aces are always high.
  auto rank_counts = get_rank_counts(cards);

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
static std::pair<bool, std::tuple<RankT, RankT, RankT>> eval_two_pair(const std::set<CardT>& cards) {
  // Count each rank - but Aces are always high.
  auto rank_counts = get_rank_counts(cards);

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
static std::pair<bool, std::tuple<RankT, RankT, RankT, RankT>> eval_pair(const std::set<CardT>& cards) {
  // Count each rank - but Aces are always high.
  auto rank_counts = get_rank_counts(cards);

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

// Slow hand eval... 7 hand card like Holdem or each Omaha option.
// @return pair(ranking, 5-characteristic-ranks)
static std::pair<HandRankingT, std::tuple<RankT, RankT, RankT, RankT, RankT>> eval_hand_7card(const CardT c0, const CardT c1, const CardT c2, const CardT c3, const CardT c4, const CardT c5, const CardT c6) {
  // Make sure all cards are unique
  std::set<CardT> unique_cards;
  unique_cards.insert(c0);
  unique_cards.insert(c1);
  unique_cards.insert(c2);
  unique_cards.insert(c3);
  unique_cards.insert(c4);
  unique_cards.insert(c5);
  unique_cards.insert(c6);

  if(unique_cards.size() != 7) {
    // We have dups - error!
    fprintf(stderr, "Duplicate cards in hand_eval()\n");
    exit(1);
  }

  // Highest ranked hand is Straight Flush - Royal Flush is just a special case of this.
  // Check for straight flushes - there can only be one suit with a straight flush.
  {
    for(auto suit = Spades; suit < NSuits; suit = (SuitT)(suit+1)) {
      auto straight_flush_eval = eval_straight_flush(unique_cards, suit);
      bool is_straight_flush = straight_flush_eval.first;
      
      if(is_straight_flush) {
	RankT high_card_rank = straight_flush_eval.second;
	auto hand_ranks = std::make_tuple((RankT)high_card_rank, (RankT)(high_card_rank-1), (RankT)(high_card_rank-2), (RankT)(high_card_rank-3), (RankT)(high_card_rank-4));
	
	return std::make_pair(StraightFlush, hand_ranks);
      }
    }
  }

  // Next highest ranked hand is Four of a Kind
  {
    auto four_of_a_kind_eval = eval_four_of_a_kind(unique_cards);
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
    auto full_house_eval = eval_full_house(unique_cards);
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
      auto flush_eval = eval_flush(unique_cards, suit);
      bool is_flush = flush_eval.first;
      
      if(is_flush) {
	return std::make_pair(Flush, flush_eval.second);
      }
    }
  }
  
  // Next highest ranked hand is a Straight
  {
    auto straight_eval = eval_straight(unique_cards);
    bool is_straight = straight_eval.first;
    
    if(is_straight) {
      RankT high_card_rank = straight_eval.second;
      auto hand_ranks = std::make_tuple((RankT)high_card_rank, (RankT)(high_card_rank-1), (RankT)(high_card_rank-2), (RankT)(high_card_rank-3), (RankT)(high_card_rank-4));
      
      return std::make_pair(Straight, hand_ranks);
    }
  }

  // Next highest ranked hand is Three of a Kind aka a Set aka Trips
  {
    auto trips_eval = eval_trips(unique_cards);
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
    auto two_pair_eval = eval_two_pair(unique_cards);
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
    auto pair_eval = eval_pair(unique_cards);
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
    auto ranks_counts = get_rank_counts(unique_cards);
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
std::pair<HandRankingT, std::tuple<RankT, RankT, RankT, RankT, RankT>> Poker::HandEval::eval_hand(const std::pair<CardT, CardT> player, const std::tuple<CardT, CardT, CardT> flop, const CardT turn, const CardT river) {
  return eval_hand_7card(player.first, player.second, std::get<0>(flop), std::get<1>(flop), std::get<2>(flop), turn, river);
}

