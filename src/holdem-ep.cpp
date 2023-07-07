#include <algorithm>
#include <cstdio>
#include <utility>

#include "dealer.hpp"
#include "hand-eval.hpp"
#include "normal.hpp"
#include "types.hpp"

using namespace Poker;

static void dump_action_and_value(std::pair<int, double> action_and_value[13][13]) {
  printf("                                                    suited\n\n");
  printf("       A       K       Q       J       X       9       8       7       6       5       4       3       2\n");
  printf("    -------------------------------------------------------------------------------------------------------\n");
  
  for (int rank0 = Ace; rank0 > AceLow; rank0--) {
    printf(" %c |", RANK_CHARS[rank0]);
    
    for (int rank1 = Ace; rank1 > AceLow; rank1--) {
      std::pair<int, double>& entry = action_and_value[rank0-1][rank1-1];
      printf(" %+4.2lf |", entry.second/entry.first);
    }

    printf("\n");
    printf("    -------------------------------------------------------------------------------------------------------\n");
  }
  printf("\n                                                    offsuit\n");
}

static void add_action_and_value(std::pair<int, double> action_and_value[13][13], std::pair<CardT, CardT> hole_normal, double value) {
  CardT card0 = hole_normal.first;
  CardT card1 = hole_normal.second;
  
  // hole_normal is always ace-high
  int i0 = (int)card0.rank - 1;
  int i1 = (int)card1.rank - 1;

  if (card0.suit != card1.suit) {
    std::swap(i0, i1);
  }

  std::pair<int, double>& entry = action_and_value[i0][i1];

  entry.first++;
  entry.second += value;
}

static void add_action_and_value_by_p1_hole(std::pair<int, double> (*p0_action_and_value_by_p1_hole)[13][13][13], std::pair<CardT, CardT> p0_hole_normal, std::pair<CardT, CardT> p1_hole_normal, double value) {
  CardT p0_card0 = p0_hole_normal.first;
  CardT p0_card1 = p0_hole_normal.second;
  
  // hole_normal is always ace-high
  int p0_i0 = (int)p0_card0.rank - 1;
  int p0_i1 = (int)p0_card1.rank - 1;

  if (p0_card0.suit != p0_card1.suit) {
    std::swap(p0_i0, p0_i1);
  }

  std::pair<int, double> (*p1_hole_action_and_value)[13] = p0_action_and_value_by_p1_hole[p0_i0][p0_i1];

  add_action_and_value(p1_hole_action_and_value, p1_hole_normal, value);
}

int main() {
  std::seed_seq seed{2, 3, 5, 7, 11};
  Dealer::DealerT dealer(seed);

  const int N_DEALS = 100000000;

  double p0_total_value = 0.0;
  double p1_total_value = 0.0;

  std::pair<int, double> p0_action_and_value[13][13] = {};
  std::pair<int, double> p1_action_and_value[13][13] = {};

  std::pair<int, double> (*p0_action_and_value_by_p1_hole)[13][13][13] = new std::pair<int, double>[13][13][13][13];

  for(int deal_no = 0; deal_no < N_DEALS; deal_no++) {
    auto cards = dealer.deal(2+2+3+1+1);

    auto p0_hole = std::make_pair(CardT(cards[0+0]), CardT(cards[0+1]));
    auto p1_hole = std::make_pair(CardT(cards[2+0]), CardT(cards[2+1]));

    auto p0_hole_normal = holdem_hole_normal(p0_hole.first, p0_hole.second);
    auto p1_hole_normal = holdem_hole_normal(p1_hole.first, p1_hole.second);

    auto flop = std::make_tuple(CardT(cards[2*2]), CardT(cards[2*2 + 1]), CardT(cards[2*2 + 2]));
    auto turn = CardT(cards[2*2 + 3]);
    auto river = CardT(cards[2*2 + 4]);

    auto p0_hand_eval = HandEval::eval_hand(p0_hole, flop, turn, river);
    auto p1_hand_eval = HandEval::eval_hand(p1_hole, flop, turn, river);

    double p0_hand_value = 0.0;
    double p1_hand_value = 0.0;
    
    if(p0_hand_eval > p1_hand_eval) {
      p0_hand_value = 1.0;
      p1_hand_value = -1.0;
    } else if(p1_hand_eval > p0_hand_eval) {
      p0_hand_value = -1.0;
      p1_hand_value = 1.0;
    }

    p0_total_value += p0_hand_value;
    p1_total_value += p1_hand_value;

    add_action_and_value(p0_action_and_value, p0_hole_normal, p0_hand_value);
    add_action_and_value(p1_action_and_value, p1_hole_normal, p1_hand_value);

    add_action_and_value_by_p1_hole(p0_action_and_value_by_p1_hole, p0_hole_normal, p1_hole_normal, p0_hand_value);
  }

  printf("%d deals / p0 EV %+4.2lf / p1 EV %+4.2lf\n", N_DEALS, p0_total_value/N_DEALS, p1_total_value/N_DEALS);

  printf("\nPlayer 0:\n\n");
  dump_action_and_value(p0_action_and_value);
  printf("\n\nPlayer 1:\n\n");
  dump_action_and_value(p1_action_and_value);

  printf("\n\n");

  for (int rank0 = Ace; rank0 > AceLow; rank0--) {
    for (int rank1 = Ace; rank1 > AceLow; rank1--) {
      printf("Player 0 %c%c%c vs Player 1:\n\n", RANK_CHARS[rank0], RANK_CHARS[rank1], (rank0 < rank1 ? 's' : 'o'));

      dump_action_and_value(p0_action_and_value_by_p1_hole[rank0-1][rank1-1]);

      printf("\n");
    }
  }
  
}
