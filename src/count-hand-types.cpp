#include <cassert>
#include <cstdio>
#include <string>

#include "dealer.hpp"
#include "hand-eval.hpp"

using namespace Poker;

enum eval_algo_t { slow_eval_algo_t, fast_eval_algo_t };
		  

int main(int argc, char* argv[]) {
  
  int p0_hand_counts[NHandRankings] = {};
  int p1_hand_counts[NHandRankings] = {};
  
  int n_deals = 1000000;
  if (argc > 1) {
    n_deals = std::atoi(argv[1]);
  }

  eval_algo_t algo = slow_eval_algo_t;
  if (argc > 2) {
    std::string algo = std::string(argv[2]);
    if (algo == "slow") {
      algo = slow_eval_algo_t;
    } else if (algo == "fast") {
      algo = fast_eval_algo_t;
    } else {
      assert(false && "unrecognised hand eval algorithm - expexting fast or slow");
    }
  }
  
  int seed5 = 13;
  if (argc > 3) {
    seed5 = std::atoi(argv[3]);
  }
  
  std::seed_seq seed{2, 3, 5, 7, seed5};
  Dealer::DealerT dealer(seed);
  
  for(int deal_no = 0; deal_no < n_deals; deal_no++) {
    constexpr int N_CARDS = 2+2+3+1+1;
    auto cards = dealer.deal(cards, n_cards);
    
    auto p0_hole = std::make_pair(CardT(cards[0+0]), CardT(cards[0+1]));
    auto p1_hole = std::make_pair(CardT(cards[2+0]), CardT(cards[2+1]));
    
    auto flop = std::make_tuple(CardT(cards[2*2]), CardT(cards[2*2 + 1]), CardT(cards[2*2 + 2]));
    auto turn = CardT(cards[2*2 + 3]);
    auto river = CardT(cards[2*2 + 4]);
    
    auto p0_hand_eval = (algo = slow_eval_algo_t ? HandEval::eval_hand_holdem(p0_hole, flop, turn, river) : HandEval::eval_hand_holdem_fast1(p0_hole, flop, turn, river);
    auto p1_hand_eval = (algo = slow_eval_algo_t ? HandEval::eval_hand_holdem(p1_hole, flop, turn, river) : HandEval::eval_hand_holdem_fast1(p1_hole, flop, turn, river);

    p0_hand_counts[p0_hand_eval.first]++;
    p1_hand_counts[p1_hand_eval.first]++;
  }

  printf("%d deals using %s algo with seed %d\n\n", n_deals, (use_fast ? "fast" : "slow"), seed5);

  printf("Player 0:\n\n");
  for (int i = 0; i < NHandRankings; i++) {
    printf("%14s %10d - %8.5lf%%\n", HAND_EVALS[i], p0_hand_counts[i], (double)p0_hand_counts[i]/(double)n_deals * 100.0);
  }
  printf("\n");
  printf("Player 1:\n\n");
  for (int i = 0; i < NHandRankings; i++) {
    printf("%14s %10d - %8.5lf%%\n", HAND_EVALS[i], p1_hand_counts[i], (double)p1_hand_counts[i]/(double)n_deals * 100.0);
  }
  printf("\n");
}
