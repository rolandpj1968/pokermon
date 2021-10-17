#include "dealer.hpp"

#include <cstdio>

int main() {
  const int N_deals = 10000000;
  const int n_cards = 9;

  std::seed_seq seed{1, 2, 3, 4, 5};
  Poker::Dealer::DealerT dealer(seed);

  int counts[52] = {0};

  for(int deal_no = 0; deal_no < N_deals; deal_no++) {
    auto cards = dealer.deal(n_cards);

    if(false) {
      printf("%4d:", deal_no);
    }
    for(int card_no = 0; card_no < n_cards; card_no++) {
      Poker::U8CardT card = cards[card_no];

      if(false) {
	printf(" %2d", card.u8_card);
      }
      counts[card.u8_card]++;
    }
    if(false) {
      printf("\n");
    }
  }

  printf("\ncounts:");
  for(int i = 0; i < 52; i++) {
    printf(" %6d", counts[i]);
  }
  printf("\n");
  
  return 0;
}

