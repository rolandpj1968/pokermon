#include <cassert>
#include <cstdio>

#include "dealer.hpp"
#include "hand-eval.hpp"

using namespace Poker;

static void dump_deal(std::pair<CardT, CardT> p0_hole, std::pair<CardT, CardT> p1_hole, std::tuple<CardT, CardT, CardT> flop, CardT turn, CardT river) {
  printf("p0-hole: %c%c / %c%c\n", RANK_CHARS[p0_hole.first.rank], SUIT_CHARS[p0_hole.first.suit], RANK_CHARS[p0_hole.second.rank], SUIT_CHARS[p0_hole.second.suit]);
  printf("p1-hole: %c%c / %c%c\n", RANK_CHARS[p1_hole.first.rank], SUIT_CHARS[p1_hole.first.suit], RANK_CHARS[p1_hole.second.rank], SUIT_CHARS[p1_hole.second.suit]);
  printf("flop:    %c%c / %c%c / %c%c\n", RANK_CHARS[std::get<0>(flop).rank], SUIT_CHARS[std::get<0>(flop).suit], RANK_CHARS[std::get<1>(flop).rank], SUIT_CHARS[std::get<1>(flop).suit], RANK_CHARS[std::get<2>(flop).rank], SUIT_CHARS[std::get<2>(flop).suit]);
  printf("turn:    %c%c\n", RANK_CHARS[turn.rank], SUIT_CHARS[turn.suit]);
  printf("river:   %c%c\n", RANK_CHARS[river.rank], SUIT_CHARS[river.suit]);
}

static void dump_hand_eval(HandEval::HandEvalT hand_eval) {
  HandRankingT hand_ranking = hand_eval.first;
  auto ranks = hand_eval.second;

  printf("%13s - %c/%c/%c/%c/%c  ", HAND_EVALS[hand_ranking], RANK_CHARS[std::get<0>(ranks)], RANK_CHARS[std::get<1>(ranks)], RANK_CHARS[std::get<2>(ranks)], RANK_CHARS[std::get<3>(ranks)], RANK_CHARS[std::get<4>(ranks)]);
}

int main() {

  std::seed_seq seed{2, 3, 5, 7, 13};
  Dealer::DealerT dealer(seed);

  const int N_DEALS = 10000000;

  for(int deal_no = 0; deal_no < N_DEALS; deal_no++) {
    auto cards = dealer.deal(2+2+3+1+1);

    auto p0_hole = std::make_pair(CardT(cards[0+0]), CardT(cards[0+1]));
    auto p1_hole = std::make_pair(CardT(cards[2+0]), CardT(cards[2+1]));

    auto flop = std::make_tuple(CardT(cards[2*2]), CardT(cards[2*2 + 1]), CardT(cards[2*2 + 2]));
    auto turn = CardT(cards[2*2 + 3]);
    auto river = CardT(cards[2*2 + 4]);

    auto p0_hand_eval = HandEval::eval_hand_holdem(p0_hole, flop, turn, river);
    auto p1_hand_eval = HandEval::eval_hand_holdem(p1_hole, flop, turn, river);

    auto p0_hand_eval_fast1 = HandEval::eval_hand_holdem_fast1(p0_hole, flop, turn, river);
    auto p1_hand_eval_fast1 = HandEval::eval_hand_holdem_fast1(p1_hole, flop, turn, river);

    if (!(p0_hand_eval == p0_hand_eval_fast1)) {
      printf("Booo - failed p0 eval after %d deals\n\n", deal_no);
      dump_deal(p0_hole, p1_hole, flop, turn, river);
      printf("\n");
      printf("p0: "); dump_hand_eval(p0_hand_eval); printf(" | "); dump_hand_eval(p0_hand_eval_fast1);
      printf("\n");
      printf("\n");
    }
    if (!(p1_hand_eval == p1_hand_eval_fast1)) {
      printf("Booo - failed p1 eval after %d deals\n", deal_no);
    }

    assert(p0_hand_eval == p0_hand_eval_fast1);
    assert(p1_hand_eval == p1_hand_eval_fast1);
  }
}
