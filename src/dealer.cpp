#include <map>
#include <utility>

#include <cstdio>

#include "dealer.hpp"
#include "hand-eval.hpp"
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

const bool DUMP_DEALS = true;
const bool DUMP_HANDS = true;

int main() {
  const int N_DEALS = 100000;
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

    if(DUMP_DEALS) {
      printf("%4d:", deal_no);
    }
    for(int card_no = 0; card_no < N_CARDS; card_no++) {
      Poker::U8CardT card = cards[card_no];

      if(DUMP_DEALS) {
	printf(" %2d", card.u8_card);
      }
      counts[card.u8_card]++;
    }
    if(DUMP_DEALS) {
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

    auto p0_cards = std::make_pair(p0_card0, p0_card1);
    auto p1_cards = std::make_pair(p1_card0, p1_card1);

    Poker::CardT flop0(cards[4]);
    Poker::CardT flop1(cards[5]);
    Poker::CardT flop2(cards[6]);

    auto flop = std::make_tuple(flop0, flop1, flop2);

    Poker::CardT turn(cards[7]);
    Poker::CardT river(cards[8]);

    if(DUMP_HANDS) {
      printf("  player 0: %c%c/%c%c\n", Poker::RANK_CHARS[p0_card0.rank], Poker::SUIT_CHARS[p0_card0.suit], Poker::RANK_CHARS[p0_card1.rank], Poker::SUIT_CHARS[p0_card1.suit]);
      printf("  player 1: %c%c/%c%c\n", Poker::RANK_CHARS[p1_card0.rank], Poker::SUIT_CHARS[p1_card0.suit], Poker::RANK_CHARS[p1_card1.rank], Poker::SUIT_CHARS[p1_card1.suit]);
      printf("  flop: %c%c/%c%c/%c%c turn: %c%c river: %c%c\n\n", Poker::RANK_CHARS[flop0.rank], Poker::SUIT_CHARS[flop0.suit], Poker::RANK_CHARS[flop1.rank], Poker::SUIT_CHARS[flop1.suit], Poker::RANK_CHARS[flop2.rank], Poker::SUIT_CHARS[flop2.suit], Poker::RANK_CHARS[turn.rank], Poker::SUIT_CHARS[turn.suit], Poker::RANK_CHARS[river.rank], Poker::SUIT_CHARS[river.suit]);

      auto p0_eval = Poker::HandEval::eval_hand(p0_cards, flop, turn, river);
      auto p1_eval = Poker::HandEval::eval_hand(p1_cards, flop, turn, river);
      printf("    player 0: %c/%c/%c/%c/%c %s\n", Poker::RANK_CHARS[std::get<0>(p0_eval.second)], Poker::RANK_CHARS[std::get<1>(p0_eval.second)], Poker::RANK_CHARS[std::get<2>(p0_eval.second)], Poker::RANK_CHARS[std::get<3>(p0_eval.second)], Poker::RANK_CHARS[std::get<4>(p0_eval.second)], Poker::HAND_EVALS[p0_eval.first]);
      printf("    player 1: %c/%c/%c/%c/%c %s\n", Poker::RANK_CHARS[std::get<0>(p1_eval.second)], Poker::RANK_CHARS[std::get<1>(p1_eval.second)], Poker::RANK_CHARS[std::get<2>(p1_eval.second)], Poker::RANK_CHARS[std::get<3>(p1_eval.second)], Poker::RANK_CHARS[std::get<4>(p1_eval.second)], Poker::HAND_EVALS[p1_eval.first]);
      printf("\n");
    }
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

