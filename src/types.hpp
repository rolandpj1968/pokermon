#ifndef TYPES_HPP
#define TYPES_HPP

#include <cstdint>

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

namespace Poker {

  enum SuitT {
    Spades,
    Hearts,
    Diamonds,
    Clubs,
    NSuits
  };

  enum RankT {
    AceLow,
    Two,
    Three,
    Four,
    Five,
    Six,
    Seven,
    Eight,
    Nine,
    Ten,
    Jack,
    Queen,
    King,
    Ace,
    NRanks
  };

  // Note that Ace appears in bit 0 and bit 13 typically
  typedef u16 RankBitsT;

  const RankBitsT RankBits[NRanks] = {
    (1 << AceLow) | (1 << Ace),
    (1 << Two),
    (1 << Three),
    (1 << Four),
    (1 << Five),
    (1 << Six),
    (1 << Seven),
    (1 << Eight),
    (1 << Nine),
    (1 << Ten),
    (1 << Jack),
    (1 << Queen),
    (1 << King),
    (1 << AceLow) | (1 << Ace),
  };

  struct CardT {
    const SuitT suit;
    const RankT rank;

    CardT(const SuitT suit, const RankT rank):
      suit(suit), rank(rank) {}
  };

  struct HandT {
    // Mmm - want these to be const but can't work out how to construct from card
    union {
      RankBitsT suits[NSuits];
      u64 hand;
    };
    
    // Empty hand
    HandT(): hand(0) {}
    
    // Construct a hand from a card
    HandT(const CardT card): hand(0) {
      suits[card.suit] = RankBits[card.rank];
    }
    
    // Combine two hands
    HandT(const HandT hand1, const HandT hand2):
      hand(hand1.hand | hand2.hand) {}
    
  };
  
  // Build a hand from a bunch of cards
  inline HandT mkHand(const CardT cards[], const int nCards) {
    HandT hand;
    
    for(int i = 0; i < nCards; i++) {
      hand = HandT(hand, HandT(cards[i]));
    }
    
    return hand;
  }

  enum HandRankingT {
    HighCard,
    Pair,
    TwoPair,
    Set,
    Straight,
    Flush,
    FullHouse,
    FourOfAKind,
    StraightFlush,
    NHandRankings
  };

  // Represents the detailed extra value of a hand, beyond the hand ranking, such as kickers:
  //
  //  - for a Pair, the hi u16 is the rank of the pair, and the low u16 is the rank-bits of the remaining (three) cards
  //  - for TwoPair the hi u16 is the rank of the high pair (hi u8) and the rank of the low pair (low u8) and the low u16 is the rank-bits of the kicker
  //  - for a Set, the hi u16 is the rank of the set, and the low u16 is rank-bits of the remaining (two) cards
  //  - for a FullHouse the hi u16 is the rank of the set/trip and the low u16 is the rank of the pair
  //  - for FourOfAKind, the hi u16 is the rank of the pair, and the low u16 is the rank-bits of the kicker
  //  - for all other hands the high u16 is 0 and the low u16 is the rank-bits of all cards in the hand
  //
  // Note that for the same hand rank, this always allows us to compare hands with arithmetic comparison ==, <=, => etc.
  //
  // Not a struct/union cos I want to enforce layout to allow explicit (correct) arithmentic comparison
  typedef u32 HandValueExtrasT;

  inline HandValueExtrasT mkRankBitsExtras(const RankBitsT rankBits) { return (HandValueExtrasT)rankBits; }

  inline HandValueExtrasT mkPairExtras(const RankT pairRank, const RankBitsT kickersRankBits) {
    return ((u32)pairRank << 16) | (u32)kickersRankBits;
  }

  inline HandValueExtrasT mkTwoPairExtras(const RankT hiPairRank, const RankT loPairRank, const RankBitsT kickerRankBits) {
    return ((u32)hiPairRank << 24) | ((u32)loPairRank << 16) | (u32)kickerRankBits;
  }

  inline HandValueExtrasT mkSetExtras(const RankT setRank, const RankBitsT kickersRankBits) {
    return ((u32)setRank << 16) | (u32)kickersRankBits;
  }
    
  inline HandValueExtrasT mkFullHouseExtras(const RankT setRank, const RankT pairRank) {
    return ((u32)setRank << 16) | (u32)pairRank << 16;
  }

  inline HandValueExtrasT mkFourOfAKindExtras(const RankT quadsRank, const RankBitsT kickerRankBits) {
    return ((u32)quadsRank << 16) | (u32)kickerRankBits;
  }

  // The hand ranking and detailed extras.
  // High u32 is the hand ranking. Low u32 is the value extras.
  // This always allows us to compare hands with arithmetic comparison ==, <=, => etc.
  // Not a struct/union cos I want to enforce layout to allow explicit (correct) arithmentic comparison
  typedef u64 HandValueT;

  inline HandValueT mkHandValue(const HandRankingT ranking, HandValueExtrasT extras) {
    return ((u64)ranking << 32) | (u64)extras;
  }

} // namespace Poker

#endif //ndef TYPES_HPP

