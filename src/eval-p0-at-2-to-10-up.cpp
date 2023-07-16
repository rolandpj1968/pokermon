#include <map>
#include <utility>

#include <cstdio>

#include "dealer.hpp"
#include "hand-eval.hpp"
#include "normal.hpp"

static void eval_p0_up_to_n_up(int n_players) {

  if(n_players < 2 || 10 < n_players) {
    fprintf(stderr, "n_players must be 2-10 but got %d\n", n_players);
    exit(1);
  }
  
  const int N_DEALS = 1000000;
  
  const int N_CARDS = 2*n_players + 3/*flop*/ + 1/*turn*/ + 1/*turn*/;

  std::seed_seq seed{1, 2, 3, 4, 5};
  Poker::Dealer::DealerT dealer(seed);

  // Count of player 0 hands of each normalised type
  std::map<std::pair<Poker::CardT, Poker::CardT>, size_t> p0_hand_counts;

  // Count of wins/pushes (i.e. non-losses) for player 0 by hand vs N players
  std::vector<std::map<std::pair<Poker::CardT, Poker::CardT>, size_t>> p0_wins(n_players-1);
  
  // Total profit for each player 0 hand against N other players.
  // For each hand there are two cases:
  //   1. Player 0 has the best hand, possibly shared with some other players.
  //        Player 0's profit is its share of the losing players' bets.
  //   2. Player 0 does not have the best hand - profit is -1.0.
  std::vector<std::map<std::pair<Poker::CardT, Poker::CardT>, double>> p0_profits(n_players-1);
  
  int n_p0_win = 0;
  int n_p1_win = 0;
  int n_push = 0;
  
  for(int deal_no = 0; deal_no < N_DEALS; deal_no++) {
    auto cards = dealer.deal(N_CARDS);

    auto p0_hole = std::make_pair(Poker::CardT(cards[0]), Poker::CardT(cards[1]));

    auto p0_hole_norm = Poker::Normal::holdem_hole_normal(p0_hole.first, p0_hole.second);
    p0_hand_counts[p0_hole_norm]++;

    auto flop = std::make_tuple(Poker::CardT(cards[2*n_players]), Poker::CardT(cards[2*n_players + 1]), Poker::CardT(cards[2*n_players + 2]));
    auto turn = Poker::CardT(cards[2*n_players + 3]);
    auto river = Poker::CardT(cards[2*n_players + 4]);

    auto p0_hand_eval = Poker::HandEval::eval_hand_holdem(p0_hole, flop, turn, river);

    // Before we compare to other players, player 0 has the best hand!
    bool is_player0_best = true;
    
    // How many other players do we tie/push for the best hand?
    int n_best_hands = 1;
    
    for(int player_no = 1; player_no < n_players; player_no++) {
      // We don't have to always evaluate up to all players - as soon as one other player
      //   is better than player 0 then it loses when we add more players
      if(!is_player0_best) {
	p0_profits[player_no-1][p0_hole_norm] -= 1.0; // lose :(
	continue;
      }

      auto p_hole = std::make_pair(cards[2*player_no], cards[2*player_no + 1]);

      auto p_hand_eval = Poker::HandEval::eval_hand_holdem(p_hole, flop, turn, river);

      if(p0_hand_eval < p_hand_eval) {
	// Oops - player 0 is no longer the best
	is_player0_best = false;
	p0_profits[player_no-1][p0_hole_norm] -= 1.0; // lose :(
	continue;
      }

      // Still the best!
      p0_wins[player_no-1][p0_hole_norm]++;
      
      if(p0_hand_eval == p_hand_eval) {
	// Another tie/push for best hand
	n_best_hands += 1;
      }

      int n_losers = (player_no + 1) - n_best_hands;
      p0_profits[player_no-1][p0_hole_norm] += (double)n_losers/(double)n_best_hands;
    }
  }

  printf("PROFITS:\n\n");
  for(auto it = p0_hand_counts.begin(); it != p0_hand_counts.end(); it++) {
    auto hand = it->first;
    auto count = it->second;

    auto card0 = hand.first;
    auto card1 = hand.second;
    
    printf("  %c%c %c%c %6lu %6.4lf%% vs", Poker::RANK_CHARS[card0.rank], Poker::SUIT_CHARS[card0.suit], Poker::RANK_CHARS[card1.rank], Poker::SUIT_CHARS[card1.suit], count, (double)count/(double)N_DEALS * 100.0);
    for(int player_no = 1; player_no < n_players; player_no++) {
      double profit = p0_profits[player_no-1][hand];
      printf(" %d: EV %+6.4lf", player_no, profit/(double)count);
    }
    printf("\n");
  }

  printf("\nWIN %%:\n\n");
  for(auto it = p0_hand_counts.begin(); it != p0_hand_counts.end(); it++) {
    auto hand = it->first;
    auto count = it->second;

    auto card0 = hand.first;
    auto card1 = hand.second;
    
    printf("  %c%c %c%c %6lu %6.4lf%% vs", Poker::RANK_CHARS[card0.rank], Poker::SUIT_CHARS[card0.suit], Poker::RANK_CHARS[card1.rank], Poker::SUIT_CHARS[card1.suit], count, (double)count/(double)N_DEALS * 100.0);
    for(int player_no = 1; player_no < n_players; player_no++) {
      size_t wins = p0_wins[player_no-1][hand];
      printf(" %d: win%% %+6.4lf", player_no, (double)wins/(double)count);
    }
    printf("\n");
  }
}

int main() {
  eval_p0_up_to_n_up(10);
  return 0;
}
