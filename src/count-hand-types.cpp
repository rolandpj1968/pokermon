#include <cassert>
#include <cstdio>
#include <string>

#include "dealer.hpp"
#include "hand-eval.hpp"

using namespace Poker;

enum eval_algo_t { slow_eval_algo_t, fast_eval_algo_t, fastest_eval_algo_t, none_eval_algo_t, n_eval_algo_t };

const char* EVAL_ALGO_NAME[n_eval_algo_t] = { "slow", "fast", "fastest", "none" };
		  

int main(int argc, char* argv[]) {

  int n_deals = 1000000;
  if (argc > 1) {
    n_deals = std::atoi(argv[1]);
  }
  
  eval_algo_t algo = slow_eval_algo_t;
  if (argc > 2) {
    std::string algo_string = std::string(argv[2]);
    bool algo_found = false;
    for (int i = 0; i < n_eval_algo_t; i++) {
      if (algo_string == EVAL_ALGO_NAME[i]) {
	algo_found = true;
	algo = (eval_algo_t)i;
	break;
      }
    }
    assert(algo_found && "unrecognised hand eval algorithm");
  }
  
  int seed5 = 13;
  if (argc > 3) {
    seed5 = std::atoi(argv[3]);
  }
  
  int n_players = 2;
  if (argc > 4) {
    n_players = std::atoi(argv[4]);
  }

  assert(0 < n_players && n_players <= 24);

  std::seed_seq seed{2, 3, 5, 7, seed5};
  Dealer::DealerT dealer(seed);
  
  int player_hand_counts[n_players][NHandRankings] = {};
  
  for(int deal_no = 0; deal_no < n_deals; deal_no++) {
    int n_cards = 2*n_players+3+1+1;
    U8CardT cards[n_cards];
    dealer.deal(cards, n_cards);

    HandEval::HandEvalT player_hand_evals[n_players] = {};

    if (algo == none_eval_algo_t) {
      
      // nada - just measure dealing
      
    } else if (algo == fastest_eval_algo_t) {
      HandT non_hole_cards_hand = HandT(cards[2*n_players]).add(cards[2*n_players + 1]).add(cards[2*n_players + 2]).add(cards[2*n_players + 3]).add(cards[2*n_players + 4]);

      HandT player_hands[n_players] = {};

      for (int i = 0; i < n_players; i++) {
	player_hands[i] = non_hole_cards_hand;
	player_hands[i].add(cards[2*i+0]).add(cards[2*i+1]);
	
	player_hand_evals[i] = HandEval::eval_hand_5_to_9_card_fast1(player_hands[i]);
      }
      
    } else {
      std::pair<CardT, CardT> player_holes[n_players];

      for (int i = 0; i < n_players; i++ ) {
	player_holes[i] = std::make_pair(CardT(cards[2*i+0]), CardT(cards[2*i+1]));
      }
      
      auto flop = std::make_tuple(CardT(cards[2*n_players]), CardT(cards[2*n_players + 1]), CardT(cards[2*n_players + 2]));
      auto turn = CardT(cards[2*n_players + 3]);
      auto river = CardT(cards[2*n_players + 4]);

      for (int i = 0; i < n_players; i++) {
	player_hand_evals[i] = (algo == slow_eval_algo_t) ? HandEval::eval_hand_holdem(player_holes[i], flop, turn, river) : HandEval::eval_hand_holdem_fast1(player_holes[i], flop, turn, river);
      }
    }

    for (int i = 0; i < n_players; i++) {
      player_hand_counts[i][player_hand_evals[i].first]++;
    }
  }

  printf("%d deals using %s algo with seed %d for %d players\n\n", n_deals, EVAL_ALGO_NAME[algo], seed5, n_players);

  for (int i = 0; i < n_players; i++ ) {
    printf("Player %d:\n\n", i);
    for (int r = 0; r < NHandRankings; r++) {
      printf("%14s %10d - %8.5lf%%\n", HAND_EVALS[r], player_hand_counts[i][r], (double)player_hand_counts[i][r]/(double)n_deals * 100.0);
    }
    printf("\n");
  }
}
