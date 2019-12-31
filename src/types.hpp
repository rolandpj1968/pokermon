#ifndef TYPES_HPP
#define TYPES_HPP

#include <cstdint>

typedef uint8_t u8;
typedef uint16_t u16;
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

    // Construct a hand from a card
    HandT(const CardT card): hand(0) {
      suits[card.suit] = RankBits[card.rank];
    }

    // Combine two hands
    HandT(const HandT hand1, const HandT hand2):
      hand(hand1.hand | hand2.hand) {}

  };

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

  // Represents the detailed value of a hand, beyond the hand ranking:
  //
  //  - for a Pair, the hi u16 is the rank of the pair, and the low u16 is the rank-bits of the remaining (three) cards
  //  - for TwoPair the hi u16 is the rank of the high pair (hi u8) and the rank of the low pair (low u8) and the low u16 is the rank-bits of the kicker
  //  - for a Set, the hi u16 is the rank of the set, and the low u16 is rank-bits of the remaining (two) cards
  //  - for a FullHouse the hi u16 is the rank of the set/trip and the low u16 is the rank of the pair
  //  - for FourOfAKind, the hi u16 is the rank of the pair, and the low u16 is the rank-bits of the kicker
  //  - for all other hands the high u16 is 0 and the low u16 is the rank-bits of all cards in the hand
  //
  // Note that for the same hand rank, this always allows us to compare hands with arithmetic comparison ==, <=, => etc.
  typedef u32 HandValueExtrasT;

  

} // namespace Poker

#endif //ndef TYPES_HPP

