#include <map>
#include <utility>

#include <cstdio>

#include "dealer.hpp"
#include "normal.hpp"

static void dump_hand_counts(const std::map<std::pair<Poker::CardT, Poker::CardT>, size_t>& hand_counts, size_t N) {
  for(auto it = hand_counts.begin(); it != hand_counts.end(); it++) {
    auto hand = it->first;
    auto count = it->second;

    auto card0 = hand.first;
    auto card1 = hand.second;

    printf("  %c%c %c%c %6lu %6.4lf%%\n", Poker::RANK_CHARS[card0.rank], Poker::SUIT_CHARS[card0.suit], Poker::RANK_CHARS[card1.rank], Poker::SUIT_CHARS[card1.suit], count, (double)count/(double)N * 100.0);
  }
}

int main() {
  const int N_DEALS = 100000000;
  const int N_CARDS = 9;

  std::seed_seq seed{1, 2, 3, 4, 5};
  Poker::Dealer::DealerT dealer(seed);

  int counts[52] = {};
  std::map<std::pair<Poker::CardT, Poker::CardT>, size_t> p0_hand_counts;
  std::map<std::pair<Poker::CardT, Poker::CardT>, size_t> p1_hand_counts;

  std::map<std::pair<Poker::CardT, Poker::CardT>, size_t> p0_norm_hand_counts;
  std::map<std::pair<Poker::CardT, Poker::CardT>, size_t> p1_norm_hand_counts;
  
  for(int deal_no = 0; deal_no < N_DEALS; deal_no++) {
    auto cards = dealer.deal(N_CARDS);

    if(false) {
      printf("%4d:", deal_no);
    }
    for(int card_no = 0; card_no < N_CARDS; card_no++) {
      Poker::U8CardT card = cards[card_no];

      if(false) {
	printf(" %2d", card.u8_card);
      }
      counts[card.u8_card]++;
    }
    if(false) {
      printf("\n");
    }

    Poker::CardT p0_card0(cards[0]);
    Poker::CardT p0_card1(cards[1]);

    Poker::CardT p1_card0(cards[2]);
    Poker::CardT p1_card1(cards[3]);

    p0_hand_counts[std::make_pair(p0_card0, p0_card1)]++;
    p1_hand_counts[std::make_pair(p1_card0, p1_card1)]++;

    auto norm_p0_cards = Poker::holdem_normal(p0_card0, p0_card1);
    auto norm_p1_cards = Poker::holdem_normal(p1_card0, p1_card1);

    p0_norm_hand_counts[norm_p0_cards]++;
    p1_norm_hand_counts[norm_p1_cards]++;
  }

  printf("\ncounts:");
  for(int i = 0; i < 52; i++) {
    printf(" %6d", counts[i]);
  }
  printf("\n");

  printf("Player 0 normalised hand counts:\n");
  dump_hand_counts(p0_norm_hand_counts, N_DEALS);
  printf("\n");
  
  printf("Player 1 normalised hand counts:\n");
  dump_hand_counts(p1_norm_hand_counts, N_DEALS);
  printf("\n");
  
  return 0;
}

