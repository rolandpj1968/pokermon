#include <cstdio>
#include <utility>

#include "dealer.hpp"
#include "gto-eval.hpp"
#include "gto-strategy.hpp"
#include "hand-eval.hpp"
#include "normal.hpp"
#include "types.hpp"

using namespace Poker;
using namespace Poker::Gto;

typedef typename LimitRootHandStrategy<2>::type_t LimitRootTwoHandStrategy;
typedef PerHoleHandContainer<LimitRootTwoHandStrategy> LimitRootTwoHandHoleHandStrategies;
typedef typename LimitRootHandEval<2>::type_t LimitRootTwoHandEval;
typedef PerHoleHandContainer<LimitRootTwoHandEval> LimitRootTwoHandHoleHandEvals;

typedef GtoStrategy</*CAN_RAISE*/true> FoldCallRaiseStrategy;
typedef GtoStrategy</*CAN_RAISE*/false> FoldCallStrategy;

static void dump_fold_call_raise_strategy(const FoldCallRaiseStrategy& strategy) {
  printf("fold  %.4f call  %.4f raise %.4f", strategy.fold_p, strategy.call_p, strategy.raise_p);
}

static void dump_fold_call_strategy(const FoldCallStrategy& strategy) {
  printf("fold  %.4f call  %.4f", strategy.fold_p, strategy.call_p);
}

static void dump_p0_hand_strategy(int rank1, int rank2, bool suited, const LimitRootTwoHandStrategy& hand_strategy) {
  printf("%c%c%c\n", RANK_CHARS[rank1], RANK_CHARS[rank2], (suited ? 's' : 'o'));
  printf("  open:                   "); dump_fold_call_raise_strategy(hand_strategy.strategy); printf("\n");
  printf("  call-raise:             "); dump_fold_call_raise_strategy(hand_strategy.call.raise.strategy); printf("\n");
  printf("  call-raise-raise-raise: "); dump_fold_call_strategy(hand_strategy.call.raise.raise.raise.strategy); printf("\n");
  printf("  raise-raise:            "); dump_fold_call_raise_strategy(hand_strategy.raise.raise.strategy); printf("\n");
}

static void dump_p1_hand_strategy(int rank1, int rank2, bool suited, const LimitRootTwoHandStrategy& hand_strategy) {
  printf("%c%c%c\n", RANK_CHARS[rank1], RANK_CHARS[rank2], (suited ? 's' : 'o'));
  printf("  call:                   "); dump_fold_call_raise_strategy(hand_strategy.call.strategy); printf("\n");
  printf("  call-raise-raise:       "); dump_fold_call_raise_strategy(hand_strategy.call.raise.raise.strategy); printf("\n");
  printf("  raise:                  "); dump_fold_call_raise_strategy(hand_strategy.raise.strategy); printf("\n");
  printf("  raise-raise-raise:      "); dump_fold_call_strategy(hand_strategy.raise.raise.raise.strategy); printf("\n");
}

static void dump_p0_strategy(LimitRootTwoHandHoleHandStrategies& player_strategies) {

  printf("Player 0 - Small Blind - Strategy:\n\n");
  
  // Pocket pairs
  bool suited = false;
  for(RankT rank = Ace; rank > AceLow; rank = (RankT)(rank-1)) {
    dump_p0_hand_strategy(rank, rank, suited, player_strategies.get_pocket_pair_value(rank));
  }
  
  printf("\n\n");
  
  // Suited
  suited = true;
  for(RankT rank_hi = Ace; rank_hi > AceLow; rank_hi = (RankT)(rank_hi-1)) {
    for(RankT rank_lo = (RankT)(rank_hi-1); rank_lo > AceLow; rank_lo = (RankT)(rank_lo-1)) {
      dump_p0_hand_strategy(rank_hi, rank_lo, suited, player_strategies.get_suited_value(rank_hi, rank_lo));
    }
    printf("\n");
  }

  printf("\n\n");
  
  // Off-suit
  suited = false;
  for(RankT rank_hi = Ace; rank_hi > AceLow; rank_hi = (RankT)(rank_hi-1)) {
    for(RankT rank_lo = (RankT)(rank_hi-1); rank_lo > AceLow; rank_lo = (RankT)(rank_lo-1)) {
      dump_p0_hand_strategy(rank_hi, rank_lo, suited, player_strategies.get_offsuit_value(rank_hi, rank_lo));
    }

    printf("\n");
  }
}

static void dump_p1_strategy(LimitRootTwoHandHoleHandStrategies& player_strategies) {

  printf("Player 1 - Big Blind - Strategy:\n\n");
  
  // Pocket pairs
  bool suited = false;
  for(RankT rank = Ace; rank > AceLow; rank = (RankT)(rank-1)) {
    dump_p1_hand_strategy(rank, rank, suited, player_strategies.get_pocket_pair_value(rank));
  }
  printf("\n\n");
  
  // Suited
  suited = true;
  for(RankT rank_hi = Ace; rank_hi > AceLow; rank_hi = (RankT)(rank_hi-1)) {
    for(RankT rank_lo = (RankT)(rank_hi-1); rank_lo > AceLow; rank_lo = (RankT)(rank_lo-1)) {
      dump_p1_hand_strategy(rank_hi, rank_lo, suited, player_strategies.get_suited_value(rank_hi, rank_lo));
    }
    printf("\n");
  }

  printf("\n\n");
  
  // Off-suit
  suited = false;
  for(RankT rank_hi = Ace; rank_hi > AceLow; rank_hi = (RankT)(rank_hi-1)) {
    for(RankT rank_lo = (RankT)(rank_hi-1); rank_lo > AceLow; rank_lo = (RankT)(rank_lo-1)) {
      dump_p1_hand_strategy(rank_hi, rank_lo, suited, player_strategies.get_offsuit_value(rank_hi, rank_lo));
    }

    printf("\n");
  }
}

static void dump_player_strategy(bool is_p0, const LimitRootTwoHandHoleHandStrategies& strategies) {

  printf("Player %c - Small Blind - Strategy:\n\n", (is_p0 ? '0' : '1'));
  
  // Pocket pairs
  bool suited = false;
  for(RankT rank = Ace; rank > AceLow; rank = (RankT)(rank-1)) {
    int rank1 = rank == Ace ? AceLow : rank;

    dump_p0_hand_strategy(rank1, rank1, suited, strategies.pocket_pairs[rank1]);
  }
  
  printf("\n\n");
  
  // Suited
  suited = true;
  for(RankT rank_hi = Ace; rank_hi > AceLow; rank_hi = (RankT)(rank_hi-1)) {
    int rank1 = rank_hi == Ace ? 0 : rank_hi;
    
    for(RankT rank_lo = (RankT)(rank_hi-1); rank_lo > AceLow; rank_lo = (RankT)(rank_lo-1)) {
      int rank2 = rank_lo == Ace ? 0 : rank_lo;

      size_t r0 = rank1 == Ace ? rank2 : rank1;
      size_t r1 = rank1 == Ace ? AceLow : rank2;
      dump_p0_hand_strategy(rank1, rank2, suited, strategies.suited[LimitRootTwoHandHoleHandStrategies::get_non_pair_index(r0, r1)]);
    }

    printf("\n");
  }

  printf("\n\n");
  
  // Off-suit
  suited = false;
  for(RankT rank_hi = Ace; rank_hi > AceLow; rank_hi = (RankT)(rank_hi-1)) {
    int rank1 = rank_hi == Ace ? 0 : rank_hi;
    
    for(RankT rank_lo = (RankT)(rank_hi-1); rank_lo > AceLow; rank_lo = (RankT)(rank_lo-1)) {
      int rank2 = rank_lo == Ace ? 0 : rank_lo;
  
      size_t r0 = rank1 == Ace ? rank2 : rank1;
      size_t r1 = rank1 == Ace ? AceLow : rank2;
      dump_p0_hand_strategy(rank1, rank2, suited, strategies.offsuit[LimitRootTwoHandHoleHandStrategies::get_non_pair_index(r0, r1)]);
    }

    printf("\n");
  }
}

static void dump_hand_eval(bool is_p0, int rank1, int rank2, bool suited, const LimitRootTwoHandEval& hand_eval, double& total_activity, double& total_p0_profit, double& total_p1_profit) {
  printf("P%c %c%c%c", (is_p0 ? '0' : '1'), RANK_CHARS[rank1], RANK_CHARS[rank2], (suited ? 's' : 'o'));
  printf(" activity: %11.4lf p0 %11.4lf p1 %11.4lf rel-p0 %6.4lf rel-p1 %6.4lf\n", hand_eval.eval.activity, hand_eval.eval.player_profits.profits[0], hand_eval.eval.player_profits.profits[1], hand_eval.eval.player_profits.profits[0]/hand_eval.eval.activity, hand_eval.eval.player_profits.profits[1]/hand_eval.eval.activity);

  total_activity += hand_eval.eval.activity;
  total_p0_profit += hand_eval.eval.player_profits.profits[0];
  total_p1_profit += hand_eval.eval.player_profits.profits[1];
}

static void dump_player_eval(bool is_p0, /*const*/ LimitRootTwoHandHoleHandEvals& player_evals) {
  double total_activity = 0.0, total_p0_profit = 0.0, total_p1_profit = 0.0;
  
  // Pocket pairs
  bool suited = false;
  for(RankT rank = Ace; rank > AceLow; rank = (RankT)(rank-1)) {
    dump_hand_eval(is_p0, rank, rank, suited, player_evals.get_pocket_pair_value(rank), total_activity, total_p0_profit, total_p1_profit);
  }
  
  printf("\n\n");
  
  // Suited
  suited = true;
  for(RankT rank_hi = Ace; rank_hi > AceLow; rank_hi = (RankT)(rank_hi-1)) {
    for(RankT rank_lo = (RankT)(rank_hi-1); rank_lo > AceLow; rank_lo = (RankT)(rank_lo-1)) {
      dump_hand_eval(is_p0, rank_hi, rank_lo, suited, player_evals.get_suited_value(rank_hi, rank_lo), total_activity, total_p0_profit, total_p1_profit);
    }
    printf("\n");
  }

  printf("\n\n");
  
  // Off-suit
  suited = false;
  for(RankT rank_hi = Ace; rank_hi > AceLow; rank_hi = (RankT)(rank_hi-1)) {
    for(RankT rank_lo = (RankT)(rank_hi-1); rank_lo > AceLow; rank_lo = (RankT)(rank_lo-1)) {
      dump_hand_eval(is_p0, rank_hi, rank_lo, suited, player_evals.get_offsuit_value(rank_hi, rank_lo), total_activity, total_p0_profit, total_p1_profit);
    }
    printf("\n");
  }
  printf("\nOverall outcome: %11.4lf p0 %11.4lf p1 %11.4lf p0-EV %6.4lf p1-EV %6.4lf\n", total_activity, total_p0_profit, total_p1_profit, total_p0_profit/total_activity, total_p1_profit/total_activity);
}

static void adjust_strategies(LimitRootTwoHandStrategy& strategy, /*const*/ LimitRootTwoHandEval& p0_hand_eval, /*const*/ LimitRootTwoHandEval& p1_hand_eval, const StrategyAdjustPolicyT& policy) {
  PlayerEvals<2, LimitRootTwoHandEval> player_evals = {};
  player_evals.evals[0] = &p0_hand_eval;
  player_evals.evals[1] = &p1_hand_eval;

  strategy.adjust(player_evals, policy);
}
			       
static void adjust_strategies(LimitRootTwoHandHoleHandStrategies& player_strategies, /*const*/ LimitRootTwoHandHoleHandEvals& p0_eval, /*const*/ LimitRootTwoHandHoleHandEvals& p1_eval, const StrategyAdjustPolicyT& policy) {

  // Pocket pairs
  for(RankT rank = Ace; rank > AceLow; rank = (RankT)(rank-1)) {
    adjust_strategies(player_strategies.get_pocket_pair_value(rank), p0_eval.get_pocket_pair_value(rank), p1_eval.get_pocket_pair_value(rank), policy);
  }
  
  // Suited
  for(RankT rank_hi = Ace; rank_hi > AceLow; rank_hi = (RankT)(rank_hi-1)) {
    for(RankT rank_lo = (RankT)(rank_hi-1); rank_lo > AceLow; rank_lo = (RankT)(rank_lo-1)) {
      adjust_strategies(player_strategies.get_offsuit_value(rank_hi, rank_lo), p0_eval.get_offsuit_value(rank_hi, rank_lo), p1_eval.get_offsuit_value(rank_hi, rank_lo), policy);
    }
  }

  // Off-suit
  for(RankT rank_hi = Ace; rank_hi > AceLow; rank_hi = (RankT)(rank_hi-1)) {
    for(RankT rank_lo = (RankT)(rank_hi-1); rank_lo > AceLow; rank_lo = (RankT)(rank_lo-1)) {
      adjust_strategies(player_strategies.get_suited_value(rank_hi, rank_lo), p0_eval.get_suited_value(rank_hi, rank_lo), p1_eval.get_suited_value(rank_hi, rank_lo), policy);
    }
  }
}

struct ConvergeOneRoundConfig {
  Dealer::DealerT& dealer;
  int n_deals;
  bool do_dump;
  double leeway;
  double min_strategy;
  StrategyClampT strategy_clamp;
};

static void converge_heads_up_preflop_strategies_one_round(LimitRootTwoHandHoleHandStrategies& player_strategies, const ConvergeOneRoundConfig& config) {
  if(false && config.do_dump) {
    printf("Evaluating preflop strategies\n\n");
    dump_p0_strategy(player_strategies);
    printf("\n\n");
    dump_p1_strategy(player_strategies);
  }

  // Allocate on the heap cos these are large.
  LimitRootTwoHandHoleHandEvals* ptr_p0_eval = new LimitRootTwoHandHoleHandEvals();
  LimitRootTwoHandHoleHandEvals* ptr_p1_eval = new LimitRootTwoHandHoleHandEvals();
  LimitRootTwoHandHoleHandEvals& p0_eval = *ptr_p0_eval;
  LimitRootTwoHandHoleHandEvals& p1_eval = *ptr_p1_eval;

  if(false && config.do_dump) {
    // What is the initial state
    printf("Player 0 - Small Blind - initial state - should be all 0.0\n\n");
    dump_player_eval(true, p0_eval);
    printf("\n\n");
    printf("Player 1 - Big Blind - initial state - should be all 0.0\n\n");
    dump_player_eval(false, p1_eval);
    printf("\n\n");
  }

  int n_p0_aa = 0, n_p0_norm_aa = 0;
  int n_p0_kk = 0, n_p0_norm_kk = 0;
  int n_hands = 0;

  for(int deal_no = 0; deal_no < config.n_deals; deal_no++) {
    auto cards = config.dealer.deal(2+2+3+1+1);

    auto p0_hole = std::make_pair(CardT(cards[0+0]), CardT(cards[0+1]));
    auto p1_hole = std::make_pair(CardT(cards[2+0]), CardT(cards[2+1]));

    auto p0_hole_norm = holdem_normal(p0_hole.first, p0_hole.second);
    auto p1_hole_norm = holdem_normal(p1_hole.first, p1_hole.second);

    if((p0_hole.first.rank == Ace || p0_hole.first.rank == AceLow) && (p0_hole.second.rank == Ace || p0_hole.second.rank == AceLow)) {
      n_p0_aa++;
    }
    if((p0_hole_norm.first.rank == Ace || p0_hole_norm.first.rank == AceLow) && (p0_hole_norm.second.rank == Ace || p0_hole_norm.second.rank == AceLow)) {
      n_p0_norm_aa++;
    }
    if(p0_hole.first.rank == King && p0_hole.second.rank == King) {
      n_p0_kk++;
    }
    if(p0_hole_norm.first.rank == King && p0_hole_norm.second.rank == King) {
      n_p0_norm_kk++;
    }
    n_hands++;

    if(false && config.do_dump) {
      printf("Deal: p0 %c%c+%c%c p0-norm %c%c+%c%c p1 %c%c+%c%c p1-norm %c%c+%c%c\n",
	     RANK_CHARS[p0_hole.first.rank], SUIT_CHARS[p0_hole.first.suit], RANK_CHARS[p0_hole.second.rank], SUIT_CHARS[p0_hole.second.suit], 
	     RANK_CHARS[p0_hole_norm.first.rank], SUIT_CHARS[p0_hole_norm.first.suit], RANK_CHARS[p0_hole_norm.second.rank], SUIT_CHARS[p0_hole_norm.second.suit], 
	     RANK_CHARS[p1_hole.first.rank], SUIT_CHARS[p1_hole.first.suit], RANK_CHARS[p1_hole.second.rank], SUIT_CHARS[p1_hole.second.suit], 
	     RANK_CHARS[p1_hole_norm.first.rank], SUIT_CHARS[p1_hole_norm.first.suit], RANK_CHARS[p1_hole_norm.second.rank], SUIT_CHARS[p1_hole_norm.second.suit]);
    }
    
    const char* winner;
    PlayerHandEvals<2> player_hand_evals = {};
    {
      auto flop = std::make_tuple(CardT(cards[2*2]), CardT(cards[2*2 + 1]), CardT(cards[2*2 + 2]));
      auto turn = CardT(cards[2*2 + 3]);
      auto river = CardT(cards[2*2 + 4]);

      auto p0_hand_eval = HandEval::eval_hand(p0_hole, flop, turn, river);
      player_hand_evals.evals[0] = p0_hand_eval;
      auto p1_hand_eval = HandEval::eval_hand(p1_hole, flop, turn, river);
      player_hand_evals.evals[1] = p1_hand_eval;
      
      if(p0_hand_eval > p1_hand_eval) {
	winner = "P0Wins";
      } else if(p1_hand_eval > p0_hand_eval) {
	winner = "P1Wins";
      } else {
	winner = "P0P1Push";
      }

      if(false && config.do_dump) {
	printf("           flop %c%c+%c%c+%c%c turn %c%c river %c%c\n",
	       RANK_CHARS[std::get<0>(flop).rank], SUIT_CHARS[std::get<0>(flop).suit], RANK_CHARS[std::get<1>(flop).rank], SUIT_CHARS[std::get<1>(flop).suit], RANK_CHARS[std::get<2>(flop).rank], SUIT_CHARS[std::get<2>(flop).suit],
	       RANK_CHARS[turn.rank], SUIT_CHARS[turn.suit],
	       RANK_CHARS[river.rank], SUIT_CHARS[river.suit]
	       );
	printf("                                                              p0 hand %s p1 hand %s winner %s\n", HAND_EVALS[p0_hand_eval.first], HAND_EVALS[p1_hand_eval.first], winner);
      }
    }

    PlayerStrategies<2, LimitRootTwoHandStrategy> player_hand_strategies = {};
    PlayerEvals<2, LimitRootTwoHandEval> player_evals = {};

    player_hand_strategies.strategies[0] = &player_strategies.get_value(p0_hole_norm.first, p0_hole_norm.second);
    player_evals.evals[0] = &p0_eval.get_value(p0_hole_norm.first, p0_hole_norm.second);
    
    player_hand_strategies.strategies[1] = &player_strategies.get_value(p1_hole_norm.first, p1_hole_norm.second);
    player_evals.evals[1] = &p1_eval.get_value(p1_hole_norm.first, p1_hole_norm.second);

    LimitRootTwoHandEval::evaluate_hand(1.0, player_evals, player_hand_strategies, player_hand_evals);
  }

  if(true && config.do_dump) {
    printf("P0 AA %d norm AA %d\n\n", n_p0_aa, n_p0_norm_aa);
    printf("P0 KK %d norm KK %d\n\n", n_p0_kk, n_p0_norm_kk);
    printf("   n_hands %d expecting %d - AA is %.4lf%% KK is %.4lf%%\n", n_hands, config.n_deals, (double)n_p0_aa/(double)n_hands * 100.0, (double)n_p0_kk/(double)n_hands * 100.0);
    // What is the outcome
    printf("Player 0 - Small Blind - outcomes\n\n");
    dump_player_eval(true, p0_eval);
    printf("\n\n");
    printf("Player 1 - Big Blind - outcomes\n\n");
    dump_player_eval(false, p1_eval);
    printf("\n\n");
  }

  printf("Adjusting strategies...\n\n");
  const StrategyAdjustPolicyT adjust_policy = { config.leeway, config.min_strategy, config.strategy_clamp };
  adjust_strategies(player_strategies, p0_eval, p1_eval, adjust_policy);

  delete ptr_p0_eval;
  delete ptr_p1_eval;
}

struct ConvergeConfig {
  Dealer::DealerT& dealer;
  int n_rounds;
  int n_deals;
  int n_deals_inc;
  double leeway;
  double leeway_inc;
  double min_strategy;
  StrategyClampT clamp_policy;
  int clamp_to_min_n_rounds = 64; // Only if clamp_policy is ClampToZero
  int dump_n_rounds; // Dump output only every dump_n_rounds rounds
};

//template <int N_PLAYERS, typename HandStrategyT>
static void converge_heads_up_preflop_strategies(LimitRootTwoHandHoleHandStrategies& hole_hand_strategies, const ConvergeConfig& config) {

  int n_deals = config.n_deals;
  double leeway = config.leeway;
  
  for(int round = 0; round < config.n_rounds; round++) {
    printf("\n\n");
    printf("==========================================================================================\n");
    printf("==============                                                             ===============\n");
    printf("==============                     Round %3d                               ===============\n", round);
    printf("==============                                                             ===============\n");
    printf("==========================================================================================\n\n");
    printf("deals %d - leeway %.2lf\n\n", n_deals, leeway);

    bool do_dump = round % config.dump_n_rounds == 0;
    
    if(do_dump) {
      dump_p0_strategy(hole_hand_strategies);
      printf("\n\n");
      dump_p1_strategy(hole_hand_strategies);
    }
    
    printf("\n\nEvaluating and adjusting...\n\n");

    StrategyClampT clamp_policy = config.clamp_policy;
    if(clamp_policy == ClampToZero && config.clamp_to_min_n_rounds != 0 && round % config.clamp_to_min_n_rounds == 0) {
      clamp_policy = ClampToMin;
    }
    const ConvergeOneRoundConfig one_round_config = { config.dealer, n_deals, do_dump, leeway, config.min_strategy, clamp_policy };
    converge_heads_up_preflop_strategies_one_round(hole_hand_strategies, one_round_config);
    
    printf("\n\n... finished evaluation and adjustment\n\n");
    
    n_deals += config.n_deals_inc;
    leeway += config.leeway_inc;
  }
}

int main() {
  int N_ROUNDS = 10000;
  //int N_ROUNDS = 1000;
  //int N_DEALS = 10608/*52*51*4*/;
  int N_DEALS = 2*3*5*7*11;
  //int N_DEALS = 16*10608/*52*51*4*/;
  // int N_ROUNDS = 1;
  // int N_DEALS = 1;
  // int N_DEALS_INC = 10608/*52*51*4*/ / 4;
  // double leeway = 0.1;
  //int N_DEALS_INC = 0; // 10608/*52*51*4*/ / 4;
  //int N_DEALS_INC = 128; ///*52*51*4*/ / 8;
  int N_DEALS_INC = 13;
  double leeway = 0.1;
  double leeway_inc = 0.0001;
  double min_strategy = 0.000000001;
  int dump_n_rounds = 16;
  StrategyClampT clamp_policy = ClampToZero;
  int clamp_to_min_n_rounds = 64; // only if clamp_policy is ClampToZero
  
  std::seed_seq seed{1, 2, 3, 4, 6};
  Dealer::DealerT dealer(seed);

  // Allocate on heap, not stack cos this is a fairly large structure
  LimitRootTwoHandHoleHandStrategies* hole_hand_strategies = new LimitRootTwoHandHoleHandStrategies();

  const ConvergeConfig config = { dealer, N_ROUNDS, N_DEALS, N_DEALS_INC, leeway, leeway_inc, min_strategy, clamp_policy, clamp_to_min_n_rounds, dump_n_rounds };

  converge_heads_up_preflop_strategies(*hole_hand_strategies, config);

  //
  // Do two last rounds to get decent data:
  //   1. Round one with ClampToMin
  //   2. Round two with ClampToZero
  // From round 2 we get decent eval EV's even for 0.0 probability choices
  //
  // TODO
  //

  printf("\n\n");
  printf("==========================================================================================\n");
  printf("==============                                                             ===============\n");
  printf("==============                     Final Strategies                        ===============\n");
  printf("==============                                                             ===============\n");
  printf("==========================================================================================\n\n\n");;

  dump_p0_strategy(*hole_hand_strategies);
  printf("\n\n");
  dump_p1_strategy(*hole_hand_strategies);

  delete hole_hand_strategies;

  return 0;
}
