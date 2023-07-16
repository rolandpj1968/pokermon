#ifndef TYPES_HPP
#define TYPES_HPP

#include <cstdint>

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

namespace Poker {

  enum SuitT: u8 {
    Spades,
    Hearts,
    Diamonds,
    Clubs,
    NSuits
  };

  [[maybe_unused]]
  static const char* SUIT_CHARS = "shdc"; // traditionally presented as smalls

  enum RankT: u8 {
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

  static constexpr inline RankT to_ace_low(const RankT rank) {
    return rank == Ace ? AceLow : rank;
  }

  static constexpr inline RankT to_ace_hi(const RankT rank) {
    return rank == AceLow ? Ace : rank;
  }

  [[maybe_unused]]
  static const char* RANK_CHARS = "a23456789XJQKA";  // traditionally presented as capitals
  
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

  // Card represented as a single u8 in [0, 52).
  // For ease of manipulation bottom 2 bits are the suit, and hi bits are the rank.
  // Aces are low
  struct U8CardT {
    u8 u8_card;

    U8CardT(u8 u8_card):
      u8_card(u8_card) {}

    inline SuitT suit() const {
      return (SuitT) (u8_card & 0x3);
    }

    // @return AceLow for Aces
    inline RankT rank() const {
      return (RankT) (u8_card >> 2);
    }
  };

  struct CardT {
    SuitT suit;
    RankT rank;

    CardT():
      suit(Spades), rank(Ace) {}
    
    CardT(const SuitT suit, const RankT rank):
      suit(suit), rank(rank) {}

    CardT(const U8CardT u8_card):
      suit(u8_card.suit()), rank(u8_card.rank()) {}

    // TODO can this be auto-defined somehow?
    bool operator<(const CardT& other) const {
      return suit < other.suit || (suit == other.suit && rank < other.rank);
    }

    bool operator==(const CardT& other) const {
      return suit == other.suit && rank == other.rank;
    }
  };

  static inline CardT to_ace_low(const CardT card) {
    return CardT(card.suit, to_ace_low(card.rank));
  }

  static inline CardT to_ace_hi(const CardT card) {
    return CardT(card.suit, to_ace_hi(card.rank));
  }

  static inline U8CardT to_u8card(const CardT card) {
    return U8CardT((u8)((to_ace_low(card.rank) << 2) + card.suit));
  }

  struct HandT {
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

  enum HandRankingT: u8 {
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

  [[maybe_unused]]
  static const char* HAND_EVALS[] = {
    "High-Card",
    "Pair",
    "Two-Pairs",
    "Set",
    "Straight",
    "Flush",
    "Full-House",
    "Four-Of-A-Kind",
    "Straight-Flush",
  };

  // Represents the detailed extra value of a hand, beyond the hand ranking, such as kickers:
  //
  //  - for a Pair, the hi u16 is the rank of the pair, and the low u16 is the rank-bits of the remaining (three) cards
  //  - for TwoPair the hi u16 is the rank of the high pair (hi u8) and the rank of the low pair (low u8) and the low u16 is the rank-bits of the kicker
  //  - for a Set,  the hi u16 is the rank of the set, and the low u16 is rank-bits of the remaining (two) cards
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

