#include <algorithm>
#include <cstdio>
#include <utility>
#include <set>

#include "dealer.hpp"
#include "hand-eval.hpp"
#include "normal.hpp"
#include "types.hpp"

using namespace Poker;

// Dump EV by normalised hole cards in descending EV order
static void dump_action_and_value(std::pair<int, double>* action_and_value, const char* indent) {
  // pair(EV, index)
  std::pair<double, int>* ev_by_index = new std::pair<double, int>[Normal::N_OMAHA_HOLE_NORMALS];
  for (int index = 0; index < (int)Normal::N_OMAHA_HOLE_NORMALS; index++) {
    ev_by_index[index] = std::make_pair(action_and_value[index].second/action_and_value[index].first, index);
  }

  std::sort(ev_by_index, ev_by_index + Normal::N_OMAHA_HOLE_NORMALS, std::greater<>());

  for (int i = 0; i < (int)Normal::N_OMAHA_HOLE_NORMALS; i++) {
    double ev = ev_by_index[i].first;
    int index = ev_by_index[i].second;

    auto hole_normal = Normal::omaha_hole_normal_from_index(index);
    printf("%s%c%c/%c%c/%c%c/%c%c - %+5.3lf\n",
	   indent,
	   RANK_CHARS[std::get<0>(hole_normal).rank], SUIT_CHARS[std::get<0>(hole_normal).suit], RANK_CHARS[std::get<1>(hole_normal).rank], SUIT_CHARS[std::get<1>(hole_normal).suit], RANK_CHARS[std::get<2>(hole_normal).rank], SUIT_CHARS[std::get<2>(hole_normal).suit], RANK_CHARS[std::get<3>(hole_normal).rank], SUIT_CHARS[std::get<3>(hole_normal).suit],
	   ev);
  }
}

static void add_action_and_value(std::pair<int, double>* action_and_value, int hole_normal_index, double value) {
  std::pair<int, double>& entry = action_and_value[hole_normal_index];

  entry.first++;
  entry.second += value;
}

int main() {
  std::seed_seq seed{2, 3, 5, 7, 13};
  Dealer::DealerT dealer(seed);

  Normal::init_omaha_hold_normal_index();

  const int N_DEALS = 1000000;

  double p0_total_value = 0.0;
  double p1_total_value = 0.0;

  std::pair<int, double>* p0_action_and_value = new std::pair<int, double>[Normal::N_OMAHA_HOLE_NORMALS]();
  std::pair<int, double>* p1_action_and_value = new std::pair<int, double>[Normal::N_OMAHA_HOLE_NORMALS]();

  for(int deal_no = 0; deal_no < N_DEALS; deal_no++) {
    auto cards = dealer.deal(4+4+3+1+1);

    auto p0_hole = std::make_tuple(CardT(cards[0+0]), CardT(cards[0+1]), CardT(cards[0+2]), CardT(cards[0+3]));
    auto p1_hole = std::make_tuple(CardT(cards[4+0]), CardT(cards[4+1]), CardT(cards[4+2]), CardT(cards[4+3]));

    int p0_hole_normal_index = Normal::omaha_hole_normal_index(std::get<0>(p0_hole), std::get<1>(p0_hole), std::get<2>(p0_hole), std::get<3>(p0_hole));
    assert(0 <= p0_hole_normal_index && (std::size_t)p0_hole_normal_index < Poker::Normal::N_OMAHA_HOLE_NORMALS);
    int p1_hole_normal_index = Normal::omaha_hole_normal_index(std::get<0>(p1_hole), std::get<1>(p1_hole), std::get<2>(p1_hole), std::get<3>(p1_hole));
    assert(0 <= p1_hole_normal_index && (std::size_t)p1_hole_normal_index < Poker::Normal::N_OMAHA_HOLE_NORMALS);

    auto flop = std::make_tuple(CardT(cards[4*2]), CardT(cards[4*2 + 1]), CardT(cards[4*2 + 2]));
    auto turn = CardT(cards[4*2 + 3]);
    auto river = CardT(cards[4*2 + 4]);

    auto p0_hand_eval = HandEval::eval_hand_omaha(p0_hole, flop, turn, river);
    auto p1_hand_eval = HandEval::eval_hand_omaha(p1_hole, flop, turn, river);

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

    add_action_and_value(p0_action_and_value, p0_hole_normal_index, p0_hand_value);
    add_action_and_value(p1_action_and_value, p1_hole_normal_index, p1_hand_value);
  }

  printf("%d deals / p0 EV %+4.2lf / p1 EV %+4.2lf\n", N_DEALS, p0_total_value/N_DEALS, p1_total_value/N_DEALS);

  printf("\nPlayer 0:\n\n");
  dump_action_and_value(p0_action_and_value, "  ");
  printf("\n\nPlayer 1:\n\n");
  dump_action_and_value(p1_action_and_value, "  ");

  printf("\n\n");
}

