#ifndef GTO_STRATEGY
#define GTO_STRATEGY

#include "gto-common.hpp"
#include "types.hpp"

#include <cmath>

namespace Poker {
  
  namespace Gto {

    enum StrategyActionT { ActionFold, ActionCall, ActionRaise };

    StrategyActionT get_max_p_action(double fold_p, double call_p, double raise_p) {
      // Bias towards fold if fold and call are equally probable
      if(fold_p < call_p) {
	// Bias towards call if call and raise are equally probable
	if(call_p < raise_p) {
	  return ActionRaise;
	} else {
	  return ActionCall;
	}
      } else {
	// Bias towards fold if fold and raise are equally probable
	if(fold_p < raise_p) {
	  return ActionRaise;
	} else {
	  return ActionFold;
	}
      }
    }

    StrategyActionT get_max_p_action(double fold_p, double call_p) {
      // Bias towards fold if fold and call are equally probable
      if(fold_p < call_p) {
	return ActionCall;
      } else {
	return ActionFold;
      }
    }

    struct StrategyAdjustStatsT {
      int n_max_p_action_changes;
    };

    // Policy for adjusting strategies - either converge slowly, or immediately
    //   clamp to the "best" strategy.
    enum StrategyAdjustT { AdjustConverge, AdjustToMax };
    
    // When aadjusting strategies, how do we treat very small strategy values?
    enum StrategyClampT { NoClamp, ClampToMin, ClampToZero };
    
    // Configuration options for adjusting strategy values.
    struct StrategyAdjustPolicyT {
      StrategyAdjustT adjust;
      double leeway;
      double min_strategy;
      StrategyClampT strategy_clamp;
    };

    // Clamp a  (small) strategy probability to min_strategy by stealing from the max value
    static inline void clamp_to_min(double& p, double& max_p, double min_strategy) {
      if(p < min_strategy) {
	double diff = min_strategy - p;
	max_p -= diff;
	p = min_strategy;
      }
    }
    
    // Clamp a (small) strategy probability to 0.0 by donating to the max value.
    // We do this to ignore clearly useless strategies, which improves efficiency.
    static inline void clamp_to_zero(double& p, double& max_p, double min_strategy) {
      if(p < min_strategy) {
	max_p += p;
	p = 0.0;
      }
    }
    
    // Normalise values to sum to the given total
    static inline void normalise_to_total(double& fold_v, double& call_v, double& raise_v, double total) {
      double sum_vs = fold_v + call_v + raise_v;
      if(sum_vs == 0.0) {
	fold_v += 1.0/3;
	call_v += 1.0/3;
	raise_v += 1.0/3;
	sum_vs = 1.0;
      }
      double norm = sum_vs/total;
      fold_v /= norm; call_v /= norm; raise_v /= norm;
    }

    // Normalise values to sum to 1.0
    static inline void normalise_to_unit_sum(double& fold_v, double& call_v, double& raise_v) {
      normalise_to_total(fold_v, call_v, raise_v, 1.0);
    }

    // Normalise values to sum to the given total
    static inline void normalise_to_total(double& fold_v, double& call_v, double total) {
      double sum_vs = fold_v + call_v;
      if(sum_vs == 0.0) {
	fold_v += 1.0/2;
	call_v += 1.0/2;
	sum_vs = 1.0;
      }
      double norm = sum_vs/total;
      fold_v /= norm; call_v /= norm;
    }

    // Normalise values to sum to the given total
    static inline void normalise_to_unit_sum(double& fold_v, double& call_v) {
      normalise_to_total(fold_v, call_v, 1.0);
    }
    
    // Adjust strategy according to empirical outcomes - reward the more profitable options and
    //   penalise the less profitable options.
    // @param leeway is in [0.0, +infinity) - the smaller the leeway the more aggressively we adjust
    //    leeway of 0.0 is not a good idea since it will leave the strategy of the worst option at 0.0.
    void adjust_strategy(double fold_profit, double call_profit, double& fold_p, double& call_p, const StrategyAdjustPolicyT& policy) {
      // We will get NaN (from 0.0/0.0) if we don't have any coverage of this path.
      // If there's no coverage we don't have any data for adjustment so just bail.
      if(std::isnan(fold_profit) || std::isnan(call_profit)) {
	return;
      }
      
      // In this case total_p might not be 1.0 in case we get here from the fold/call/raise path
      //   where one of them is NaN.
      double total_p = fold_p + call_p;

      if(total_p == 0.0) {
	// Can't adjust two 0.0 paths and there is no point
	return;
      }

      if(policy.adjust == AdjustToMax) {
	fold_p = 0.0;
	call_p = 0.0;
	
	// Bias towards fold if they're equal
	if (fold_profit < call_profit) {
	  call_p = 1.0;
	} else {
	  fold_p = 1.0;
	}
      } else {
	// default to AdjustConverge

	// Normalise profits to be 0-based.
	double min_profit = std::min(fold_profit, call_profit);
	// All positive...
	fold_profit -= min_profit; call_profit -= min_profit;
	
	// Normalise profits to sum to 1.0
	normalise_to_unit_sum(fold_profit, call_profit);
	
	// Give some leeway
	fold_profit += policy.leeway; call_profit += policy.leeway;
	
	// Adjust strategies...
	fold_p *= fold_profit; call_p *= call_profit;
	
	// Strategies must sum to same total as before
	normalise_to_total(fold_p, call_p, total_p);
      }

      // Find the maximum strategy so we can adjust it against the clamped small values
      double& max_p = fold_p > call_p ? fold_p : call_p;
      
      // Apply clamping policy
      if(policy.strategy_clamp == ClampToMin) {
	clamp_to_min(fold_p, max_p, policy.min_strategy);
	clamp_to_min(call_p, max_p, policy.min_strategy);
      } else if(policy.strategy_clamp == ClampToZero) {
	clamp_to_zero(fold_p, max_p, policy.min_strategy);
	clamp_to_zero(call_p, max_p, policy.min_strategy);
      }
    }
    
    // Adjust strategy according to empirical outcomes - reward the more profitable options and
    //   penalise the less profitable options.
    // @param leeway is in [0.0, +infinity) - the smaller the leeway the more aggressively we adjust
    //    leeway of 0.0 is not a good idea since it will leave the strategy of the worst option at 0.0.
    void adjust_strategy(double fold_profit, double call_profit, double raise_profit, double& fold_p, double& call_p, double& raise_p, const StrategyAdjustPolicyT& policy) {
      
      // We will get NaN (from 0.0/0.0) if we don't have any coverage of this path.
      // If there's no coverage of one of the paths (e.g. p = 0.0), we need to adjust the
      //   other two paths, otherwise we get stuck.
      if(std::isnan(fold_profit)) {
	adjust_strategy(call_profit, raise_profit, call_p, raise_p, policy);
	return;
      } else if(std::isnan(call_profit)) {
	adjust_strategy(fold_profit, raise_profit, fold_p, raise_p, policy);
	return;
      } else if(std::isnan(raise_profit)) {
	adjust_strategy(fold_profit, call_profit, fold_p, call_p, policy);
	return;
      }
      
      if(policy.adjust == AdjustToMax) {
	fold_p = 0.0;
	call_p = 0.0;
	raise_p = 0.0;
	
	// Bias towards fold if fold and call are equally profitable
	if(fold_profit < call_profit) {
	  // Bias towards call if call and raise are equally profitable
	  if(call_profit < raise_profit) {
	    raise_p = 1.0;
	  } else {
	    call_p = 1.0;
	  }
	} else {
	  // Bias towards fold if fold and raise are equally profitable
	  if(fold_profit < raise_profit) {
	    raise_p = 1.0;
	  } else {
	    fold_p = 1.0;
	  }
	}
      } else {
	// default to AdjustConverge
	
	// In this case total_p MUST sum to 1.0
	double total_p = fold_p + call_p + raise_p;
	
	// Normalise profits to be 0-based.
	double min_profit = std::min(fold_profit, std::min(call_profit, raise_profit));
	// All positive...
	fold_profit -= min_profit; call_profit -= min_profit; raise_profit -= min_profit;
	
	// Normalise profits to sum to 1.0
	normalise_to_unit_sum(fold_profit, call_profit, raise_profit);
	
	// Give some leeway
	fold_profit += policy.leeway; call_profit += policy.leeway; raise_profit += policy.leeway;
	
	// Adjust strategies...
	fold_p *= fold_profit; call_p *= call_profit; raise_p *= raise_profit;
	
	// Strategies must sum to the same total as before adjustment - always 1.0 in this case
	normalise_to_total(fold_p, call_p, raise_p, total_p);
      }
      
      // Find the maximum strategy so we can adjust it against the clamped small values
      double& fold_call_max_p = fold_p > call_p ? fold_p : call_p;
      double& max_p = fold_call_max_p > raise_p ? fold_call_max_p : raise_p;
      
      // Apply clamping policy
      if(policy.strategy_clamp == ClampToMin) {
	clamp_to_min(fold_p, max_p, policy.min_strategy);
	clamp_to_min(call_p, max_p, policy.min_strategy);
	clamp_to_min(raise_p, max_p, policy.min_strategy);
      } else if(policy.strategy_clamp == ClampToZero) {
	clamp_to_zero(fold_p, max_p, policy.min_strategy);
	clamp_to_zero(call_p, max_p, policy.min_strategy);
	clamp_to_zero(raise_p, max_p, policy.min_strategy);
      }
    }
    
    // GTO strategy - two variants depending on whether we can raise or not.
    template <bool CAN_RAISE>
    struct GtoStrategy;

    // Used for fold/call/raise and fold/check/raise nodes.
    //   fold, call, raise each in [0.0, 1.0]
    //   fold + call + raise == 1.0
    template <>
    struct GtoStrategy</*CAN_RAISE*/true> {
      double fold_p;
      double call_p;
      double raise_p;
      
      GtoStrategy() :
	fold_p(1.0/3.0), call_p(1.0/3.0), raise_p(1.0/3.0) {}

      // Adjust strategy according to empirical outcomes - reward the more profitable options and
      //   penalise the less profitable options.
      // @param leeway is in [0.0, +infinity) - the smaller the leeway the more aggressively we adjust
      //    leeway of 0.0 is not a good idea since it will leave the strategy of the worst option at 0.0.
      void adjust(double fold_profit, double call_profit, double raise_profit, const StrategyAdjustPolicyT& policy, StrategyAdjustStatsT& stats) {
	StrategyActionT prev_max_p_action = get_max_p_action(fold_p, call_p, raise_p);
      	adjust_strategy(fold_profit, call_profit, raise_profit, fold_p, call_p, raise_p, policy);
	StrategyActionT max_p_action = get_max_p_action(fold_p, call_p, raise_p);
	if(prev_max_p_action != max_p_action) {
	  stats.n_max_p_action_changes++;
	}
      }
      
    }; // struct GtoStrategy</*CAN_RAISE*/true>

    // Used for fold/call and fold/check nodes.
    //   fold, call each in [0.0, 1.0]
    //   fold + call == 1.0
    template <>
    struct GtoStrategy</*CAN_RAISE*/false> {
      double fold_p;
      double call_p;
      
      GtoStrategy() :
	fold_p(1.0/2.0), call_p(1.0/2.0) {}

      // Adjust strategy according to empirical outcomes - reward the more profitable options and
      //   penalise the less profitable options.
      // @param leeway is in [0.0, +infinity) - the smaller the leeway the more aggressively we adjust
      //    leeway of 0.0 is not a good idea since it will leave the strategy of the worst option at 0.0.
      void adjust(double fold_profit, double call_profit, const StrategyAdjustPolicyT& policy, StrategyAdjustStatsT& stats) {
	StrategyActionT prev_max_p_action = get_max_p_action(fold_p, call_p);
	adjust_strategy(fold_profit, call_profit, fold_p, call_p, policy);
	StrategyActionT max_p_action = get_max_p_action(fold_p, call_p);
	if(prev_max_p_action != max_p_action) {
	  stats.n_max_p_action_changes++;
	}
      }
      
    }; // struct GtoStrategy</*CAN_RAISE*/false>
    
    // Declaration of LimitHandStrategy.
    // Note that this includes the strategies of all players for a given set of hole cards.
    template <
      // Small blind
      int SMALL_BLIND,
      // Big blind - also raise amount
      int BIG_BLIND,
      // How many players are playing
      int N_PLAYERS,
      // Bitmap of active players.
      u8 ACTIVE_BM,
      // How many _active_ players left until everyone has called.
      // When this is zero then the round of betting is over and
      //   goes to showdown or next stage.
      int N_TO_CALL,
      // The player currently betting.
      // Small blind (SB) is player 0, big blind (BB) is player 1.
      // For hole card betting we start at player 2 (or SB if heads up).
      int PLAYER_NO,
      // How many raises can still be made. Typically limit poker allows
      //   a maximum bet total per player of 4 x BB, which means that at
      //   the start of a betting round there are 3 raises still allowed.
      int N_RAISES_LEFT,
      // The current bet amount of all players, stuffed into a u64, with
      //   one byte per player - giving a max bet of 255 per player and
      //   a max of 8 players. This is enough for limit holdem.
      u64 PLAYER_POTS
      >
    struct LimitHandStrategy;

    // Declaration of LimitHandStrategySpecialised.
    template <
      // Small blind
      int SMALL_BLIND,
      // Big blind - also raise amount
      int BIG_BLIND,
      // How many players are playing
      int N_PLAYERS,
      // Bitmap of active players.
      u8 ACTIVE_BM,
      // How many _active_ players left until everyone has called.
      // When this is zero then the round of betting is over and
      //   goes to showdown or next stage.
      int N_TO_CALL,
      // The player currently betting.
      // Small blind (SB) is player 0, big blind (BB) is player 1.
      // For hole card betting we start at player 2 (or SB if heads up).
      int PLAYER_NO,
      // How many raises can still be made. Typically limit poker allows
      //   a maximum bet total per player of 4 x BB, which means that at
      //   the start of a betting round there are 3 raises still allowed.
      int N_RAISES_LEFT,
      // The current bet amount of all players, stuffed into a u64, with
      //   one byte per player - giving a max bet of 255 per player and
      //   a max of 8 players. This is enough for limit holdem.
      u64 PLAYER_POTS,
      // Specialised node type
      LimitHandNodeType NODE_TYPE
      >
    struct LimitHandStrategySpecialised;

    // Definition of LimitHandStrategy as one of four specialisations
    template <
      int SMALL_BLIND,
      int BIG_BLIND,
      int N_PLAYERS,
      u8 ACTIVE_BM,
      int N_TO_CALL,
      int PLAYER_NO,
      int N_RAISES_LEFT,
      u64 PLAYER_POTS
      >
    struct LimitHandStrategy :
      LimitHandStrategySpecialised<
        SMALL_BLIND,
        BIG_BLIND,
        N_PLAYERS,
        ACTIVE_BM,
        N_TO_CALL,
        PLAYER_NO,
        N_RAISES_LEFT,
        PLAYER_POTS,
        get_node_type(PLAYER_NO, ACTIVE_BM, N_TO_CALL, N_RAISES_LEFT)
      >
    {};
    
    // Child node of this strategy tree node for current player folding
    template <
      int SMALL_BLIND,
      int BIG_BLIND,
      int N_PLAYERS,
      u8 ACTIVE_BM,
      int N_TO_CALL,
      int PLAYER_NO,
      int N_RAISES_LEFT,
      u64 PLAYER_POTS
      >
    struct LimitHandStrategyFoldChild {
      typedef LimitHandStrategy<
	SMALL_BLIND,
	BIG_BLIND,
	N_PLAYERS,
	remove_player_from_active_bm(PLAYER_NO, ACTIVE_BM),
	N_TO_CALL-1,
	next_player(PLAYER_NO, N_PLAYERS),
	N_RAISES_LEFT,
	PLAYER_POTS
	> fold_t;
      
      fold_t fold;
    };
    
    // Child node of this strategy tree node for current player calling (or checking).
    template <
      int SMALL_BLIND,
      int BIG_BLIND,
      int N_PLAYERS,
      u8 ACTIVE_BM,
      int N_TO_CALL,
      int PLAYER_NO,
      int N_RAISES_LEFT,
      u64 PLAYER_POTS
      >
    struct LimitHandStrategyCallChild {
      typedef LimitHandStrategy<
	SMALL_BLIND,
	BIG_BLIND,
	N_PLAYERS,
	ACTIVE_BM,
	N_TO_CALL-1,
	next_player(PLAYER_NO, N_PLAYERS),
	N_RAISES_LEFT,
	update_player_pots(PLAYER_NO, get_curr_max_bet(N_PLAYERS, PLAYER_POTS), PLAYER_POTS)
	> call_t;
      
      call_t call;
    };
    
    // Child node of this strategy tree node for current player raising.
    template <
      int SMALL_BLIND,
      int BIG_BLIND,
      int N_PLAYERS,
      u8 ACTIVE_BM,
      int N_TO_CALL,
      int PLAYER_NO,
      int N_RAISES_LEFT,
      u64 PLAYER_POTS
      >
    struct LimitHandStrategyRaiseChild {
      typedef LimitHandStrategy<
	SMALL_BLIND,
	BIG_BLIND,
	N_PLAYERS,
	ACTIVE_BM,
	/*N_TO_CALL*/get_n_active(ACTIVE_BM)-1, // Since we raised, we go all the way round the table again...
	next_player(PLAYER_NO, N_PLAYERS),
	N_RAISES_LEFT-1,
	update_player_pots(PLAYER_NO, get_curr_max_bet(N_PLAYERS, PLAYER_POTS) + BIG_BLIND, PLAYER_POTS)
	> raise_t;
      
      raise_t raise;
    };
    
    // Default specialisation of LimitHandStrategy.
    //   The current active player can fold, call or raise.
    template <
      int SMALL_BLIND,
      int BIG_BLIND,
      int N_PLAYERS,
      u8 ACTIVE_BM,
      int N_TO_CALL,
      int PLAYER_NO,
      int N_RAISES_LEFT,
      u64 PLAYER_POTS
      >
    struct LimitHandStrategySpecialised<SMALL_BLIND, BIG_BLIND, N_PLAYERS, ACTIVE_BM, N_TO_CALL, PLAYER_NO, N_RAISES_LEFT, PLAYER_POTS, FoldCallRaiseNodeType> :
      LimitHandNodeConsts<SMALL_BLIND, BIG_BLIND, N_PLAYERS, ACTIVE_BM, N_TO_CALL, PLAYER_NO, N_RAISES_LEFT, PLAYER_POTS>,
      LimitHandStrategyFoldChild<SMALL_BLIND, BIG_BLIND, N_PLAYERS, ACTIVE_BM, N_TO_CALL, PLAYER_NO, N_RAISES_LEFT, PLAYER_POTS>,
      LimitHandStrategyCallChild<SMALL_BLIND, BIG_BLIND, N_PLAYERS, ACTIVE_BM, N_TO_CALL, PLAYER_NO, N_RAISES_LEFT, PLAYER_POTS>,
      LimitHandStrategyRaiseChild<SMALL_BLIND, BIG_BLIND, N_PLAYERS, ACTIVE_BM, N_TO_CALL, PLAYER_NO, N_RAISES_LEFT, PLAYER_POTS>
    {
      static const bool is_leaf = false;
      static const bool can_raise = true;
      
      GtoStrategy</*CAN_RAISE*/true> strategy;

      template <typename PlayerEvalT>
      void adjust(PlayerEvals<N_PLAYERS, PlayerEvalT> player_evals, const StrategyAdjustPolicyT& policy, StrategyAdjustStatsT& stats) {
	const PlayerEvalT& curr_player_eval = player_evals.get_player_eval(PLAYER_NO);

	double rel_fold_profit = curr_player_eval.fold.eval.rel_player_profit(PLAYER_NO);
	double rel_call_profit = curr_player_eval.call.eval.rel_player_profit(PLAYER_NO);
	double rel_raise_profit = curr_player_eval.raise.eval.rel_player_profit(PLAYER_NO);

	strategy.adjust(rel_fold_profit, rel_call_profit, rel_raise_profit, policy, stats);

	auto fold_evals = PlayerEvalsFoldGetter<N_PLAYERS, PlayerEvalT>::get_fold_evals(player_evals);
	this->fold.adjust(fold_evals, policy, stats);

	auto call_evals = PlayerEvalsCallGetter<N_PLAYERS, PlayerEvalT>::get_call_evals(player_evals);
	this->call.adjust(call_evals, policy, stats);

	auto raise_evals = PlayerEvalsRaiseGetter<N_PLAYERS, PlayerEvalT>::get_raise_evals(player_evals);
	this->raise.adjust(raise_evals, policy, stats);
      }
    };
    
    // Specialisation for one player left in the hand.
    // This is the highest priority specialisation since the hand is over
    //   immediately - there are no children.
    template <
      int SMALL_BLIND,
      int BIG_BLIND,
      int N_PLAYERS,
      u8 ACTIVE_BM,
      int N_TO_CALL,
      int PLAYER_NO,
      int N_RAISES_LEFT,
      u64 PLAYER_POTS
      >
    struct LimitHandStrategySpecialised<SMALL_BLIND, BIG_BLIND, N_PLAYERS, ACTIVE_BM, N_TO_CALL, PLAYER_NO, N_RAISES_LEFT, PLAYER_POTS, AllButOneFoldNodeType>
      : LimitHandNodeConsts<SMALL_BLIND, BIG_BLIND, N_PLAYERS, ACTIVE_BM, N_TO_CALL, PLAYER_NO, N_RAISES_LEFT, PLAYER_POTS>
    {
      static const bool is_leaf = true;

      template <typename PlayerEvalT>
      void adjust(PlayerEvals<N_PLAYERS, PlayerEvalT> player_evals, const StrategyAdjustPolicyT& policy, StrategyAdjustStatsT& stats) { /*noop*/ }
    };
    
    // Specialisation for all active players called.
    // Betting is now over and we go to showdown or the next stage.
    template <
      int SMALL_BLIND,
      int BIG_BLIND,
      int N_PLAYERS,
      u8 ACTIVE_BM,
      int N_TO_CALL,
      int PLAYER_NO,
      int N_RAISES_LEFT,
      u64 PLAYER_POTS
      >
    struct LimitHandStrategySpecialised<SMALL_BLIND, BIG_BLIND, N_PLAYERS, ACTIVE_BM, N_TO_CALL, PLAYER_NO, N_RAISES_LEFT, PLAYER_POTS, ShowdownNodeType>
      : LimitHandNodeConsts<SMALL_BLIND, BIG_BLIND, N_PLAYERS, ACTIVE_BM, N_TO_CALL, PLAYER_NO, N_RAISES_LEFT, PLAYER_POTS>
    {
      static const bool is_leaf = true;

      template <typename PlayerEvalT>
      void adjust(PlayerEvals<N_PLAYERS, PlayerEvalT> player_evals, const StrategyAdjustPolicyT& policy, StrategyAdjustStatsT& stats) { /*noop*/ }
    };
    
    // Specialisation for current player already folded
    // This is just a dummy node but it is more comprehensible to include '_' for inactive players.
    template <
      int SMALL_BLIND,
      int BIG_BLIND,
      int N_PLAYERS,
      u8 ACTIVE_BM,
      int N_TO_CALL,
      int PLAYER_NO,
      int N_RAISES_LEFT,
      u64 PLAYER_POTS
      >
    struct LimitHandStrategySpecialised<SMALL_BLIND, BIG_BLIND, N_PLAYERS, ACTIVE_BM, N_TO_CALL, PLAYER_NO, N_RAISES_LEFT, PLAYER_POTS, AlreadyFoldedNodeType>
      : LimitHandNodeConsts<SMALL_BLIND, BIG_BLIND, N_PLAYERS, ACTIVE_BM, N_TO_CALL, PLAYER_NO, N_RAISES_LEFT, PLAYER_POTS>
    {
      static const bool is_leaf = false;
      
      typedef LimitHandStrategy<
	SMALL_BLIND,
	BIG_BLIND,
	N_PLAYERS,
	ACTIVE_BM,
	N_TO_CALL,
	next_player(PLAYER_NO, N_PLAYERS),
	N_RAISES_LEFT,
	PLAYER_POTS
	> dead_t;
      
      dead_t _;

      template <typename PlayerEvalT>
      void adjust(PlayerEvals<N_PLAYERS, PlayerEvalT> player_evals, const StrategyAdjustPolicyT& policy, StrategyAdjustStatsT& stats) {
	auto dead_evals = PlayerEvalsDeadGetter<N_PLAYERS, PlayerEvalT>::get_dead_evals(player_evals);
	this->_.adjust(dead_evals, policy, stats);
      }
      
    };
    
    // Specialisation for when we're at maximum bet level.
    // This is lowest priority specialisation since terminal conditions should be considered first.
    // Only fold or call (check) is allowed now.
    template <
      int SMALL_BLIND,
      int BIG_BLIND,
      int N_PLAYERS,
      u8 ACTIVE_BM,
      int N_TO_CALL,
      int PLAYER_NO,
      int N_RAISES_LEFT,
      u64 PLAYER_POTS
      >
    struct LimitHandStrategySpecialised<SMALL_BLIND, BIG_BLIND, N_PLAYERS, ACTIVE_BM, N_TO_CALL, PLAYER_NO, N_RAISES_LEFT, PLAYER_POTS, FoldCallNodeType> :
      LimitHandNodeConsts<SMALL_BLIND, BIG_BLIND, N_PLAYERS, ACTIVE_BM, N_TO_CALL, PLAYER_NO, N_RAISES_LEFT, PLAYER_POTS>,
      LimitHandStrategyFoldChild<SMALL_BLIND, BIG_BLIND, N_PLAYERS, ACTIVE_BM, N_TO_CALL, PLAYER_NO, N_RAISES_LEFT, PLAYER_POTS>,
      LimitHandStrategyCallChild<SMALL_BLIND, BIG_BLIND, N_PLAYERS, ACTIVE_BM, N_TO_CALL, PLAYER_NO, N_RAISES_LEFT, PLAYER_POTS>
    {
      static const bool is_leaf = false;
      static const bool can_raise = false;
      
      GtoStrategy</*CAN_RAISE*/false> strategy;


      template <typename PlayerEvalT>
      void adjust(PlayerEvals<N_PLAYERS, PlayerEvalT> player_evals, const StrategyAdjustPolicyT& policy, StrategyAdjustStatsT& stats) {
	const PlayerEvalT& curr_player_eval = player_evals.get_player_eval(PLAYER_NO);

	double rel_fold_profit = curr_player_eval.fold.eval.rel_player_profit(PLAYER_NO);
	double rel_call_profit = curr_player_eval.call.eval.rel_player_profit(PLAYER_NO);

	strategy.adjust(rel_fold_profit, rel_call_profit, policy, stats);

	auto fold_evals = PlayerEvalsFoldGetter<N_PLAYERS, PlayerEvalT>::get_fold_evals(player_evals);
	this->fold.adjust(fold_evals, policy, stats);

	auto call_evals = PlayerEvalsCallGetter<N_PLAYERS, PlayerEvalT>::get_call_evals(player_evals);
	this->call.adjust(call_evals, policy, stats);
      }
    };
    
    template <
      int N_PLAYERS,
      int N_RAISES = 3,
      int SMALL_BLIND = 1,
      int BIG_BLIND = 2
      >
    struct LimitRootHandStrategy {
      typedef LimitHandStrategy<
	SMALL_BLIND,
	BIG_BLIND,
	N_PLAYERS,
	make_root_active_bm(N_PLAYERS),
	/*N_TO_CALL*/N_PLAYERS,     // Note BB is allowed to still raise
	/*PLAYER_NO*/2 % N_PLAYERS,
	/*N_RAISES_LEFT*/N_RAISES,
	/*PLAYER_POTS*/make_root_player_pots(SMALL_BLIND, BIG_BLIND)
	> type_t;
    };

    // Let's see if we and gcc got this right...
    
    namespace StaticAsserts {

      namespace TwoPlayerZeroRaises {
	
	typedef LimitRootHandStrategy</*N_PLAYERS*/2, /*N_RAISES*/0>::type_t limit_2p_0r_strategy_t;
	
	// Root node
	static_assert(!limit_2p_0r_strategy_t::is_leaf);
	static_assert(!limit_2p_0r_strategy_t::can_raise);
	static_assert(limit_2p_0r_strategy_t::small_blind == 1);
	static_assert(limit_2p_0r_strategy_t::big_blind == 2);
	static_assert(limit_2p_0r_strategy_t::n_players == 2);
	static_assert(limit_2p_0r_strategy_t::n_active == 2);
	static_assert(limit_2p_0r_strategy_t::active_bm == 0x3);
	static_assert(limit_2p_0r_strategy_t::n_to_call == 2);
	static_assert(limit_2p_0r_strategy_t::player_no == 0); // SB to bet
	static_assert(limit_2p_0r_strategy_t::n_raises_left == 0);
	static_assert(limit_2p_0r_strategy_t::player_pots.pots[0] == 1);
	static_assert(limit_2p_0r_strategy_t::player_pots.pots[1] == 2);
	static_assert(limit_2p_0r_strategy_t::curr_max_bet == 2);
	static_assert(limit_2p_0r_strategy_t::total_pot == 3);
	
	// SB folds
	static_assert(limit_2p_0r_strategy_t::fold_t::is_leaf); // Only BB left
	static_assert(limit_2p_0r_strategy_t::fold_t::small_blind == 1);
	static_assert(limit_2p_0r_strategy_t::fold_t::big_blind == 2);
	static_assert(limit_2p_0r_strategy_t::fold_t::n_players == 2);
	static_assert(limit_2p_0r_strategy_t::fold_t::n_active == 1);
	static_assert(limit_2p_0r_strategy_t::fold_t::active_bm == 0x2);
	static_assert(limit_2p_0r_strategy_t::fold_t::n_to_call == 1);
	static_assert(limit_2p_0r_strategy_t::fold_t::player_no == 1); // BB to bet
	static_assert(limit_2p_0r_strategy_t::fold_t::n_raises_left == 0);
	static_assert(limit_2p_0r_strategy_t::fold_t::player_pots.pots[0] == 1);
	static_assert(limit_2p_0r_strategy_t::fold_t::player_pots.pots[1] == 2);
	static_assert(limit_2p_0r_strategy_t::fold_t::curr_max_bet == 2);
	static_assert(limit_2p_0r_strategy_t::fold_t::total_pot == 3);
	
	// SB calls
	static_assert(!limit_2p_0r_strategy_t::call_t::is_leaf);
	static_assert(!limit_2p_0r_strategy_t::call_t::can_raise);
	static_assert(limit_2p_0r_strategy_t::call_t::small_blind == 1);
	static_assert(limit_2p_0r_strategy_t::call_t::big_blind == 2);
	static_assert(limit_2p_0r_strategy_t::call_t::n_players == 2);
	static_assert(limit_2p_0r_strategy_t::call_t::n_active == 2);
	static_assert(limit_2p_0r_strategy_t::call_t::active_bm == 0x3);
	static_assert(limit_2p_0r_strategy_t::call_t::n_to_call == 1);
	static_assert(limit_2p_0r_strategy_t::call_t::player_no == 1); // BB to bet
	static_assert(limit_2p_0r_strategy_t::call_t::n_raises_left == 0);
	static_assert(limit_2p_0r_strategy_t::call_t::player_pots.pots[0] == 2);
	static_assert(limit_2p_0r_strategy_t::call_t::player_pots.pots[1] == 2);
	static_assert(limit_2p_0r_strategy_t::call_t::curr_max_bet == 2);
	static_assert(limit_2p_0r_strategy_t::call_t::total_pot == 4);
	
	// SB calls, BB folds
	static_assert(limit_2p_0r_strategy_t::call_t::fold_t::is_leaf);
	static_assert(limit_2p_0r_strategy_t::call_t::fold_t::small_blind == 1);
	static_assert(limit_2p_0r_strategy_t::call_t::fold_t::big_blind == 2);
	static_assert(limit_2p_0r_strategy_t::call_t::fold_t::n_players == 2);
	static_assert(limit_2p_0r_strategy_t::call_t::fold_t::n_active == 1);
	static_assert(limit_2p_0r_strategy_t::call_t::fold_t::active_bm == 0x1);
	static_assert(limit_2p_0r_strategy_t::call_t::fold_t::n_to_call == 0);
	static_assert(limit_2p_0r_strategy_t::call_t::fold_t::player_no == 0);
	static_assert(limit_2p_0r_strategy_t::call_t::fold_t::n_raises_left == 0);
	static_assert(limit_2p_0r_strategy_t::call_t::fold_t::player_pots.pots[0] == 2);
	static_assert(limit_2p_0r_strategy_t::call_t::fold_t::player_pots.pots[1] == 2);
	static_assert(limit_2p_0r_strategy_t::call_t::fold_t::curr_max_bet == 2);
	static_assert(limit_2p_0r_strategy_t::call_t::fold_t::total_pot == 4);
	
	// SB calls, BB calls
	static_assert(limit_2p_0r_strategy_t::call_t::call_t::is_leaf);
	static_assert(limit_2p_0r_strategy_t::call_t::call_t::small_blind == 1);
	static_assert(limit_2p_0r_strategy_t::call_t::call_t::big_blind == 2);
	static_assert(limit_2p_0r_strategy_t::call_t::call_t::n_players == 2);
	static_assert(limit_2p_0r_strategy_t::call_t::call_t::n_active == 2);
	static_assert(limit_2p_0r_strategy_t::call_t::call_t::active_bm == 0x3);
	static_assert(limit_2p_0r_strategy_t::call_t::call_t::n_to_call == 0);
	static_assert(limit_2p_0r_strategy_t::call_t::call_t::player_no == 0);
	static_assert(limit_2p_0r_strategy_t::call_t::call_t::n_raises_left == 0);
	static_assert(limit_2p_0r_strategy_t::call_t::call_t::player_pots.pots[0] == 2);
	static_assert(limit_2p_0r_strategy_t::call_t::call_t::player_pots.pots[1] == 2);
	static_assert(limit_2p_0r_strategy_t::call_t::call_t::curr_max_bet == 2);
	static_assert(limit_2p_0r_strategy_t::call_t::call_t::total_pot == 4);

      } // namespace TwoPlayerZeroRaises

      namespace TwoPlayerOneRaise {
	
	typedef LimitRootHandStrategy</*N_PLAYERS*/2, /*N_RAISES*/1>::type_t limit_2p_1r_strategy_t;
	
	// Root node
	static_assert(!limit_2p_1r_strategy_t::is_leaf);
	static_assert(limit_2p_1r_strategy_t::can_raise);
	static_assert(limit_2p_1r_strategy_t::small_blind == 1);
	static_assert(limit_2p_1r_strategy_t::big_blind == 2);
	static_assert(limit_2p_1r_strategy_t::n_players == 2);
	static_assert(limit_2p_1r_strategy_t::n_active == 2);
	static_assert(limit_2p_1r_strategy_t::active_bm == 0x3);
	static_assert(limit_2p_1r_strategy_t::n_to_call == 2);
	static_assert(limit_2p_1r_strategy_t::player_no == 0); // SB to bet
	static_assert(limit_2p_1r_strategy_t::n_raises_left == 1);
	static_assert(limit_2p_1r_strategy_t::player_pots.pots[0] == 1);
	static_assert(limit_2p_1r_strategy_t::player_pots.pots[1] == 2);
	static_assert(limit_2p_1r_strategy_t::curr_max_bet == 2);
	static_assert(limit_2p_1r_strategy_t::total_pot == 3);
	
	// SB folds
	static_assert(limit_2p_1r_strategy_t::fold_t::is_leaf); // Only BB left
	static_assert(limit_2p_1r_strategy_t::fold_t::small_blind == 1);
	static_assert(limit_2p_1r_strategy_t::fold_t::big_blind == 2);
	static_assert(limit_2p_1r_strategy_t::fold_t::n_players == 2);
	static_assert(limit_2p_1r_strategy_t::fold_t::n_active == 1);
	static_assert(limit_2p_1r_strategy_t::fold_t::active_bm == 0x2);
	static_assert(limit_2p_1r_strategy_t::fold_t::n_to_call == 1);
	static_assert(limit_2p_1r_strategy_t::fold_t::player_no == 1); // BB to bet
	static_assert(limit_2p_1r_strategy_t::fold_t::n_raises_left == 1);
	static_assert(limit_2p_1r_strategy_t::fold_t::player_pots.pots[0] == 1);
	static_assert(limit_2p_1r_strategy_t::fold_t::player_pots.pots[1] == 2);
	static_assert(limit_2p_1r_strategy_t::fold_t::curr_max_bet == 2);
	static_assert(limit_2p_1r_strategy_t::fold_t::total_pot == 3);
	
	// SB calls
	static_assert(!limit_2p_1r_strategy_t::call_t::is_leaf);
	static_assert(limit_2p_1r_strategy_t::call_t::can_raise);
	static_assert(limit_2p_1r_strategy_t::call_t::small_blind == 1);
	static_assert(limit_2p_1r_strategy_t::call_t::big_blind == 2);
	static_assert(limit_2p_1r_strategy_t::call_t::n_players == 2);
	static_assert(limit_2p_1r_strategy_t::call_t::n_active == 2);
	static_assert(limit_2p_1r_strategy_t::call_t::active_bm == 0x3);
	static_assert(limit_2p_1r_strategy_t::call_t::n_to_call == 1);
	static_assert(limit_2p_1r_strategy_t::call_t::player_no == 1); // BB to bet
	static_assert(limit_2p_1r_strategy_t::call_t::n_raises_left == 1);
	static_assert(limit_2p_1r_strategy_t::call_t::player_pots.pots[0] == 2);
	static_assert(limit_2p_1r_strategy_t::call_t::player_pots.pots[1] == 2);
	static_assert(limit_2p_1r_strategy_t::call_t::curr_max_bet == 2);
	static_assert(limit_2p_1r_strategy_t::call_t::total_pot == 4);
	
	// SB calls, BB folds
	static_assert(limit_2p_1r_strategy_t::call_t::fold_t::is_leaf);
	static_assert(limit_2p_1r_strategy_t::call_t::fold_t::small_blind == 1);
	static_assert(limit_2p_1r_strategy_t::call_t::fold_t::big_blind == 2);
	static_assert(limit_2p_1r_strategy_t::call_t::fold_t::n_players == 2);
	static_assert(limit_2p_1r_strategy_t::call_t::fold_t::n_active == 1);
	static_assert(limit_2p_1r_strategy_t::call_t::fold_t::active_bm == 0x1);
	static_assert(limit_2p_1r_strategy_t::call_t::fold_t::n_to_call == 0);
	static_assert(limit_2p_1r_strategy_t::call_t::fold_t::player_no == 0);
	static_assert(limit_2p_1r_strategy_t::call_t::fold_t::n_raises_left == 1);
	static_assert(limit_2p_1r_strategy_t::call_t::fold_t::player_pots.pots[0] == 2);
	static_assert(limit_2p_1r_strategy_t::call_t::fold_t::player_pots.pots[1] == 2);
	static_assert(limit_2p_1r_strategy_t::call_t::fold_t::curr_max_bet == 2);
	static_assert(limit_2p_1r_strategy_t::call_t::fold_t::total_pot == 4);
	
	// SB calls, BB calls
	static_assert(limit_2p_1r_strategy_t::call_t::call_t::is_leaf);
	static_assert(limit_2p_1r_strategy_t::call_t::call_t::small_blind == 1);
	static_assert(limit_2p_1r_strategy_t::call_t::call_t::big_blind == 2);
	static_assert(limit_2p_1r_strategy_t::call_t::call_t::n_players == 2);
	static_assert(limit_2p_1r_strategy_t::call_t::call_t::n_active == 2);
	static_assert(limit_2p_1r_strategy_t::call_t::call_t::active_bm == 0x3);
	static_assert(limit_2p_1r_strategy_t::call_t::call_t::n_to_call == 0);
	static_assert(limit_2p_1r_strategy_t::call_t::call_t::player_no == 0);
	static_assert(limit_2p_1r_strategy_t::call_t::call_t::n_raises_left == 1);
	static_assert(limit_2p_1r_strategy_t::call_t::call_t::player_pots.pots[0] == 2);
	static_assert(limit_2p_1r_strategy_t::call_t::call_t::player_pots.pots[1] == 2);
	static_assert(limit_2p_1r_strategy_t::call_t::call_t::curr_max_bet == 2);
	static_assert(limit_2p_1r_strategy_t::call_t::call_t::total_pot == 4);
	
	// SB calls, BB raises
	static_assert(!limit_2p_1r_strategy_t::call_t::raise_t::is_leaf);
	static_assert(!limit_2p_1r_strategy_t::call_t::raise_t::can_raise);
	static_assert(limit_2p_1r_strategy_t::call_t::raise_t::small_blind == 1);
	static_assert(limit_2p_1r_strategy_t::call_t::raise_t::big_blind == 2);
	static_assert(limit_2p_1r_strategy_t::call_t::raise_t::n_players == 2);
	static_assert(limit_2p_1r_strategy_t::call_t::raise_t::n_active == 2);
	static_assert(limit_2p_1r_strategy_t::call_t::raise_t::active_bm == 0x3);
	static_assert(limit_2p_1r_strategy_t::call_t::raise_t::n_to_call == 1);
	static_assert(limit_2p_1r_strategy_t::call_t::raise_t::player_no == 0);
	static_assert(limit_2p_1r_strategy_t::call_t::raise_t::n_raises_left == 0);
	static_assert(limit_2p_1r_strategy_t::call_t::raise_t::player_pots.pots[0] == 2);
	static_assert(limit_2p_1r_strategy_t::call_t::raise_t::player_pots.pots[1] == 4);
	static_assert(limit_2p_1r_strategy_t::call_t::raise_t::curr_max_bet == 4);
	static_assert(limit_2p_1r_strategy_t::call_t::raise_t::total_pot == 6);
	
	// SB calls, BB raises, SB folds
	static_assert(limit_2p_1r_strategy_t::call_t::raise_t::fold_t::is_leaf);
	static_assert(limit_2p_1r_strategy_t::call_t::raise_t::fold_t::small_blind == 1);
	static_assert(limit_2p_1r_strategy_t::call_t::raise_t::fold_t::big_blind == 2);
	static_assert(limit_2p_1r_strategy_t::call_t::raise_t::fold_t::n_players == 2);
	static_assert(limit_2p_1r_strategy_t::call_t::raise_t::fold_t::n_active == 1);
	static_assert(limit_2p_1r_strategy_t::call_t::raise_t::fold_t::active_bm == 0x2);
	static_assert(limit_2p_1r_strategy_t::call_t::raise_t::fold_t::n_to_call == 0);
	static_assert(limit_2p_1r_strategy_t::call_t::raise_t::fold_t::player_no == 1);
	static_assert(limit_2p_1r_strategy_t::call_t::raise_t::fold_t::n_raises_left == 0);
	static_assert(limit_2p_1r_strategy_t::call_t::raise_t::fold_t::player_pots.pots[0] == 2);
	static_assert(limit_2p_1r_strategy_t::call_t::raise_t::fold_t::player_pots.pots[1] == 4);
	static_assert(limit_2p_1r_strategy_t::call_t::raise_t::fold_t::curr_max_bet == 4);
	static_assert(limit_2p_1r_strategy_t::call_t::raise_t::fold_t::total_pot == 6);
	
	// SB calls, BB raises, SB calls
	static_assert(limit_2p_1r_strategy_t::call_t::raise_t::call_t::is_leaf);
	static_assert(limit_2p_1r_strategy_t::call_t::raise_t::call_t::small_blind == 1);
	static_assert(limit_2p_1r_strategy_t::call_t::raise_t::call_t::big_blind == 2);
	static_assert(limit_2p_1r_strategy_t::call_t::raise_t::call_t::n_players == 2);
	static_assert(limit_2p_1r_strategy_t::call_t::raise_t::call_t::n_active == 2);
	static_assert(limit_2p_1r_strategy_t::call_t::raise_t::call_t::active_bm == 0x3);
	static_assert(limit_2p_1r_strategy_t::call_t::raise_t::call_t::n_to_call == 0);
	static_assert(limit_2p_1r_strategy_t::call_t::raise_t::call_t::player_no == 1);
	static_assert(limit_2p_1r_strategy_t::call_t::raise_t::call_t::n_raises_left == 0);
	static_assert(limit_2p_1r_strategy_t::call_t::raise_t::call_t::player_pots.pots[0] == 4);
	static_assert(limit_2p_1r_strategy_t::call_t::raise_t::call_t::player_pots.pots[1] == 4);
	static_assert(limit_2p_1r_strategy_t::call_t::raise_t::call_t::curr_max_bet == 4);
	static_assert(limit_2p_1r_strategy_t::call_t::raise_t::call_t::total_pot == 8);
	
	// SB raises
	static_assert(!limit_2p_1r_strategy_t::raise_t::is_leaf);
	static_assert(!limit_2p_1r_strategy_t::raise_t::can_raise);
	static_assert(limit_2p_1r_strategy_t::raise_t::small_blind == 1);
	static_assert(limit_2p_1r_strategy_t::raise_t::big_blind == 2);
	static_assert(limit_2p_1r_strategy_t::raise_t::n_players == 2);
	static_assert(limit_2p_1r_strategy_t::raise_t::n_active == 2);
	static_assert(limit_2p_1r_strategy_t::raise_t::active_bm == 0x3);
	static_assert(limit_2p_1r_strategy_t::raise_t::n_to_call == 1);
	static_assert(limit_2p_1r_strategy_t::raise_t::player_no == 1); // BB to bet
	static_assert(limit_2p_1r_strategy_t::raise_t::n_raises_left == 0);
	static_assert(limit_2p_1r_strategy_t::raise_t::player_pots.pots[0] == 4);
	static_assert(limit_2p_1r_strategy_t::raise_t::player_pots.pots[1] == 2);
	static_assert(limit_2p_1r_strategy_t::raise_t::curr_max_bet == 4);
	static_assert(limit_2p_1r_strategy_t::raise_t::total_pot == 6);
	
	// SB raises, BB folds
	static_assert(limit_2p_1r_strategy_t::raise_t::fold_t::is_leaf);
	static_assert(limit_2p_1r_strategy_t::raise_t::fold_t::small_blind == 1);
	static_assert(limit_2p_1r_strategy_t::raise_t::fold_t::big_blind == 2);
	static_assert(limit_2p_1r_strategy_t::raise_t::fold_t::n_players == 2);
	static_assert(limit_2p_1r_strategy_t::raise_t::fold_t::n_active == 1);
	static_assert(limit_2p_1r_strategy_t::raise_t::fold_t::active_bm == 0x1);
	static_assert(limit_2p_1r_strategy_t::raise_t::fold_t::n_to_call == 0);
	static_assert(limit_2p_1r_strategy_t::raise_t::fold_t::player_no == 0);
	static_assert(limit_2p_1r_strategy_t::raise_t::fold_t::n_raises_left == 0);
	static_assert(limit_2p_1r_strategy_t::raise_t::fold_t::player_pots.pots[0] == 4);
	static_assert(limit_2p_1r_strategy_t::raise_t::fold_t::player_pots.pots[1] == 2);
	static_assert(limit_2p_1r_strategy_t::raise_t::fold_t::curr_max_bet == 4);
	static_assert(limit_2p_1r_strategy_t::raise_t::fold_t::total_pot == 6);
	
	// SB raises, BB calls
	static_assert(limit_2p_1r_strategy_t::raise_t::call_t::is_leaf);
	static_assert(limit_2p_1r_strategy_t::raise_t::call_t::small_blind == 1);
	static_assert(limit_2p_1r_strategy_t::raise_t::call_t::big_blind == 2);
	static_assert(limit_2p_1r_strategy_t::raise_t::call_t::n_players == 2);
	static_assert(limit_2p_1r_strategy_t::raise_t::call_t::n_active == 2);
	static_assert(limit_2p_1r_strategy_t::raise_t::call_t::active_bm == 0x3);
	static_assert(limit_2p_1r_strategy_t::raise_t::call_t::n_to_call == 0);
	static_assert(limit_2p_1r_strategy_t::raise_t::call_t::player_no == 0);
	static_assert(limit_2p_1r_strategy_t::raise_t::call_t::n_raises_left == 0);
	static_assert(limit_2p_1r_strategy_t::raise_t::call_t::player_pots.pots[0] == 4);
	static_assert(limit_2p_1r_strategy_t::raise_t::call_t::player_pots.pots[1] == 4);
	static_assert(limit_2p_1r_strategy_t::raise_t::call_t::curr_max_bet == 4);
	static_assert(limit_2p_1r_strategy_t::raise_t::call_t::total_pot == 8);
	
      } // namespace TwoPlayerOneRaise
      
      namespace ThreePlayerZeroRaises {
	
	typedef LimitRootHandStrategy</*N_PLAYERS*/3, /*N_RAISES*/0>::type_t limit_3p_0r_strategy_t;
	
	// Root node
	static_assert(!limit_3p_0r_strategy_t::is_leaf);
	static_assert(!limit_3p_0r_strategy_t::can_raise);
	static_assert(limit_3p_0r_strategy_t::small_blind == 1);
	static_assert(limit_3p_0r_strategy_t::big_blind == 2);
	static_assert(limit_3p_0r_strategy_t::n_players == 3);
	static_assert(limit_3p_0r_strategy_t::n_active == 3);
	static_assert(limit_3p_0r_strategy_t::active_bm == 0x7);
	static_assert(limit_3p_0r_strategy_t::n_to_call == 3);
	static_assert(limit_3p_0r_strategy_t::player_no == 2); // UTG to bet
	static_assert(limit_3p_0r_strategy_t::n_raises_left == 0);
	static_assert(limit_3p_0r_strategy_t::player_pots.pots[0] == 1);
	static_assert(limit_3p_0r_strategy_t::player_pots.pots[1] == 2);
	static_assert(limit_3p_0r_strategy_t::player_pots.pots[2] == 0);
	static_assert(limit_3p_0r_strategy_t::curr_max_bet == 2);
	static_assert(limit_3p_0r_strategy_t::total_pot == 3);
	
	// UTG folds
	static_assert(!limit_3p_0r_strategy_t::fold_t::is_leaf); // SB and BB left
	static_assert(!limit_3p_0r_strategy_t::fold_t::can_raise);
	static_assert(limit_3p_0r_strategy_t::fold_t::small_blind == 1);
	static_assert(limit_3p_0r_strategy_t::fold_t::big_blind == 2);
	static_assert(limit_3p_0r_strategy_t::fold_t::n_players == 3);
	static_assert(limit_3p_0r_strategy_t::fold_t::n_active == 2);
	static_assert(limit_3p_0r_strategy_t::fold_t::active_bm == 0x3);
	static_assert(limit_3p_0r_strategy_t::fold_t::n_to_call == 2);
	static_assert(limit_3p_0r_strategy_t::fold_t::player_no == 0); // SB to bet
	static_assert(limit_3p_0r_strategy_t::fold_t::n_raises_left == 0);
	static_assert(limit_3p_0r_strategy_t::fold_t::player_pots.pots[0] == 1);
	static_assert(limit_3p_0r_strategy_t::fold_t::player_pots.pots[1] == 2);
	static_assert(limit_3p_0r_strategy_t::fold_t::player_pots.pots[2] == 0);
	static_assert(limit_3p_0r_strategy_t::fold_t::curr_max_bet == 2);
	static_assert(limit_3p_0r_strategy_t::fold_t::total_pot == 3);

	// UTG folds, SB folds
	static_assert(limit_3p_0r_strategy_t::fold_t::fold_t::is_leaf); // BB left
	static_assert(limit_3p_0r_strategy_t::fold_t::fold_t::small_blind == 1);
	static_assert(limit_3p_0r_strategy_t::fold_t::fold_t::big_blind == 2);
	static_assert(limit_3p_0r_strategy_t::fold_t::fold_t::n_players == 3);
	static_assert(limit_3p_0r_strategy_t::fold_t::fold_t::n_active == 1);
	static_assert(limit_3p_0r_strategy_t::fold_t::fold_t::active_bm == 0x2);
	static_assert(limit_3p_0r_strategy_t::fold_t::fold_t::n_to_call == 1);
	static_assert(limit_3p_0r_strategy_t::fold_t::fold_t::player_no == 1); // BB to bet
	static_assert(limit_3p_0r_strategy_t::fold_t::fold_t::n_raises_left == 0);
	static_assert(limit_3p_0r_strategy_t::fold_t::fold_t::player_pots.pots[0] == 1);
	static_assert(limit_3p_0r_strategy_t::fold_t::fold_t::player_pots.pots[1] == 2);
	static_assert(limit_3p_0r_strategy_t::fold_t::fold_t::player_pots.pots[2] == 0);
	static_assert(limit_3p_0r_strategy_t::fold_t::fold_t::curr_max_bet == 2);
	static_assert(limit_3p_0r_strategy_t::fold_t::fold_t::total_pot == 3);

	// UTG folds, SB calls
	static_assert(!limit_3p_0r_strategy_t::fold_t::call_t::is_leaf); // SB, BB left
	static_assert(!limit_3p_0r_strategy_t::fold_t::call_t::can_raise);
	static_assert(limit_3p_0r_strategy_t::fold_t::call_t::small_blind == 1);
	static_assert(limit_3p_0r_strategy_t::fold_t::call_t::big_blind == 2);
	static_assert(limit_3p_0r_strategy_t::fold_t::call_t::n_players == 3);
	static_assert(limit_3p_0r_strategy_t::fold_t::call_t::n_active == 2);
	static_assert(limit_3p_0r_strategy_t::fold_t::call_t::active_bm == 0x3);
	static_assert(limit_3p_0r_strategy_t::fold_t::call_t::n_to_call == 1);
	static_assert(limit_3p_0r_strategy_t::fold_t::call_t::player_no == 1); // BB to bet
	static_assert(limit_3p_0r_strategy_t::fold_t::call_t::n_raises_left == 0);
	static_assert(limit_3p_0r_strategy_t::fold_t::call_t::player_pots.pots[0] == 2);
	static_assert(limit_3p_0r_strategy_t::fold_t::call_t::player_pots.pots[1] == 2);
	static_assert(limit_3p_0r_strategy_t::fold_t::call_t::player_pots.pots[2] == 0);
	static_assert(limit_3p_0r_strategy_t::fold_t::call_t::curr_max_bet == 2);
	static_assert(limit_3p_0r_strategy_t::fold_t::call_t::total_pot == 4);

	// UTG folds, SB calls, BB calls
	static_assert(limit_3p_0r_strategy_t::fold_t::call_t::call_t::is_leaf);
	static_assert(limit_3p_0r_strategy_t::fold_t::call_t::call_t::small_blind == 1);
	static_assert(limit_3p_0r_strategy_t::fold_t::call_t::call_t::big_blind == 2);
	static_assert(limit_3p_0r_strategy_t::fold_t::call_t::call_t::n_players == 3);
	static_assert(limit_3p_0r_strategy_t::fold_t::call_t::call_t::n_active == 2);
	static_assert(limit_3p_0r_strategy_t::fold_t::call_t::call_t::active_bm == 0x3);
	static_assert(limit_3p_0r_strategy_t::fold_t::call_t::call_t::n_to_call == 0);
	static_assert(limit_3p_0r_strategy_t::fold_t::call_t::call_t::player_no == 2);
	static_assert(limit_3p_0r_strategy_t::fold_t::call_t::call_t::n_raises_left == 0);
	static_assert(limit_3p_0r_strategy_t::fold_t::call_t::call_t::player_pots.pots[0] == 2);
	static_assert(limit_3p_0r_strategy_t::fold_t::call_t::call_t::player_pots.pots[1] == 2);
	static_assert(limit_3p_0r_strategy_t::fold_t::call_t::call_t::player_pots.pots[2] == 0);
	static_assert(limit_3p_0r_strategy_t::fold_t::call_t::call_t::curr_max_bet == 2);
	static_assert(limit_3p_0r_strategy_t::fold_t::call_t::call_t::total_pot == 4);
	
	// UTG calls
	static_assert(!limit_3p_0r_strategy_t::call_t::is_leaf); // all 3 active
	static_assert(!limit_3p_0r_strategy_t::call_t::can_raise);
	static_assert(limit_3p_0r_strategy_t::call_t::small_blind == 1);
	static_assert(limit_3p_0r_strategy_t::call_t::big_blind == 2);
	static_assert(limit_3p_0r_strategy_t::call_t::n_players == 3);
	static_assert(limit_3p_0r_strategy_t::call_t::n_active == 3);
	static_assert(limit_3p_0r_strategy_t::call_t::active_bm == 0x7);
	static_assert(limit_3p_0r_strategy_t::call_t::n_to_call == 2);
	static_assert(limit_3p_0r_strategy_t::call_t::player_no == 0); // SB to bet
	static_assert(limit_3p_0r_strategy_t::call_t::n_raises_left == 0);
	static_assert(limit_3p_0r_strategy_t::call_t::player_pots.pots[0] == 1);
	static_assert(limit_3p_0r_strategy_t::call_t::player_pots.pots[1] == 2);
	static_assert(limit_3p_0r_strategy_t::call_t::player_pots.pots[2] == 2);
	static_assert(limit_3p_0r_strategy_t::call_t::curr_max_bet == 2);
	static_assert(limit_3p_0r_strategy_t::call_t::total_pot == 5);
	
	// UTG calls, SB folds
	static_assert(!limit_3p_0r_strategy_t::call_t::fold_t::is_leaf); // UTG, BB active
	static_assert(!limit_3p_0r_strategy_t::call_t::fold_t::can_raise);
	static_assert(limit_3p_0r_strategy_t::call_t::fold_t::small_blind == 1);
	static_assert(limit_3p_0r_strategy_t::call_t::fold_t::big_blind == 2);
	static_assert(limit_3p_0r_strategy_t::call_t::fold_t::n_players == 3);
	static_assert(limit_3p_0r_strategy_t::call_t::fold_t::n_active == 2);
	static_assert(limit_3p_0r_strategy_t::call_t::fold_t::active_bm == 0x6);
	static_assert(limit_3p_0r_strategy_t::call_t::fold_t::n_to_call == 1);
	static_assert(limit_3p_0r_strategy_t::call_t::fold_t::player_no == 1); // BB to bet
	static_assert(limit_3p_0r_strategy_t::call_t::fold_t::n_raises_left == 0);
	static_assert(limit_3p_0r_strategy_t::call_t::fold_t::player_pots.pots[0] == 1);
	static_assert(limit_3p_0r_strategy_t::call_t::fold_t::player_pots.pots[1] == 2);
	static_assert(limit_3p_0r_strategy_t::call_t::fold_t::player_pots.pots[2] == 2);
	static_assert(limit_3p_0r_strategy_t::call_t::fold_t::curr_max_bet == 2);
	static_assert(limit_3p_0r_strategy_t::call_t::fold_t::total_pot == 5);
	
	// UTG calls, SB folds, BB folds
	static_assert(limit_3p_0r_strategy_t::call_t::fold_t::fold_t::is_leaf); // only UTG active
	static_assert(limit_3p_0r_strategy_t::call_t::fold_t::fold_t::small_blind == 1);
	static_assert(limit_3p_0r_strategy_t::call_t::fold_t::fold_t::big_blind == 2);
	static_assert(limit_3p_0r_strategy_t::call_t::fold_t::fold_t::n_players == 3);
	static_assert(limit_3p_0r_strategy_t::call_t::fold_t::fold_t::n_active == 1);
	static_assert(limit_3p_0r_strategy_t::call_t::fold_t::fold_t::active_bm == 0x4);
	static_assert(limit_3p_0r_strategy_t::call_t::fold_t::fold_t::n_to_call == 0);
	static_assert(limit_3p_0r_strategy_t::call_t::fold_t::fold_t::player_no == 2); // UTG to bet
	static_assert(limit_3p_0r_strategy_t::call_t::fold_t::fold_t::n_raises_left == 0);
	static_assert(limit_3p_0r_strategy_t::call_t::fold_t::fold_t::player_pots.pots[0] == 1);
	static_assert(limit_3p_0r_strategy_t::call_t::fold_t::fold_t::player_pots.pots[1] == 2);
	static_assert(limit_3p_0r_strategy_t::call_t::fold_t::fold_t::player_pots.pots[2] == 2);
	static_assert(limit_3p_0r_strategy_t::call_t::fold_t::fold_t::curr_max_bet == 2);
	static_assert(limit_3p_0r_strategy_t::call_t::fold_t::fold_t::total_pot == 5);
	
      } // namespace ThreePlayerZeroRaises
      
      namespace FourPlayerTwoRaises {
	
	typedef LimitRootHandStrategy</*N_PLAYERS*/4, /*N_RAISES*/2>::type_t limit_4p_2r_strategy_t;
	
	// Root node
	static_assert(!limit_4p_2r_strategy_t::is_leaf);
	static_assert(limit_4p_2r_strategy_t::can_raise);
	static_assert(limit_4p_2r_strategy_t::small_blind == 1);
	static_assert(limit_4p_2r_strategy_t::big_blind == 2);
	static_assert(limit_4p_2r_strategy_t::n_players == 4);
	static_assert(limit_4p_2r_strategy_t::n_active == 4);
	static_assert(limit_4p_2r_strategy_t::active_bm == 0xf);
	static_assert(limit_4p_2r_strategy_t::n_to_call == 4);
	static_assert(limit_4p_2r_strategy_t::player_no == 2); // UTG to bet
	static_assert(limit_4p_2r_strategy_t::n_raises_left == 2);
	static_assert(limit_4p_2r_strategy_t::player_pots.pots[0] == 1);
	static_assert(limit_4p_2r_strategy_t::player_pots.pots[1] == 2);
	static_assert(limit_4p_2r_strategy_t::player_pots.pots[2] == 0);
	static_assert(limit_4p_2r_strategy_t::player_pots.pots[3] == 0);
	static_assert(limit_4p_2r_strategy_t::curr_max_bet == 2);
	static_assert(limit_4p_2r_strategy_t::total_pot == 3);

	// UTG folds, P3 calls, SB calls, BB raises
	static_assert(!limit_4p_2r_strategy_t::fold_t::call_t::call_t::raise_t::is_leaf);
	static_assert(limit_4p_2r_strategy_t::fold_t::call_t::call_t::raise_t::small_blind == 1);
	static_assert(limit_4p_2r_strategy_t::fold_t::call_t::call_t::raise_t::big_blind == 2);
	static_assert(limit_4p_2r_strategy_t::fold_t::call_t::call_t::raise_t::n_players == 4);
	static_assert(limit_4p_2r_strategy_t::fold_t::call_t::call_t::raise_t::n_active == 3);
	static_assert(limit_4p_2r_strategy_t::fold_t::call_t::call_t::raise_t::active_bm == 0xb);
	static_assert(limit_4p_2r_strategy_t::fold_t::call_t::call_t::raise_t::n_to_call == 2);
	static_assert(limit_4p_2r_strategy_t::fold_t::call_t::call_t::raise_t::player_no == 2); // UTG to bet
	static_assert(limit_4p_2r_strategy_t::fold_t::call_t::call_t::raise_t::n_raises_left == 1);
	static_assert(limit_4p_2r_strategy_t::fold_t::call_t::call_t::raise_t::player_pots.pots[0] == 2);
	static_assert(limit_4p_2r_strategy_t::fold_t::call_t::call_t::raise_t::player_pots.pots[1] == 4);
	static_assert(limit_4p_2r_strategy_t::fold_t::call_t::call_t::raise_t::player_pots.pots[2] == 0);
	static_assert(limit_4p_2r_strategy_t::fold_t::call_t::call_t::raise_t::player_pots.pots[3] == 2);
	static_assert(limit_4p_2r_strategy_t::fold_t::call_t::call_t::raise_t::curr_max_bet == 4);
	static_assert(limit_4p_2r_strategy_t::fold_t::call_t::call_t::raise_t::total_pot == 8);

	// UTG folds, P3 calls, SB calls, BB raises, UTG is dead
	static_assert(!limit_4p_2r_strategy_t::fold_t::call_t::call_t::raise_t::dead_t::is_leaf);
	static_assert(limit_4p_2r_strategy_t::fold_t::call_t::call_t::raise_t::dead_t::small_blind == 1);
	static_assert(limit_4p_2r_strategy_t::fold_t::call_t::call_t::raise_t::dead_t::big_blind == 2);
	static_assert(limit_4p_2r_strategy_t::fold_t::call_t::call_t::raise_t::dead_t::n_players == 4);
	static_assert(limit_4p_2r_strategy_t::fold_t::call_t::call_t::raise_t::dead_t::n_active == 3);
	static_assert(limit_4p_2r_strategy_t::fold_t::call_t::call_t::raise_t::dead_t::active_bm == 0xb);
	static_assert(limit_4p_2r_strategy_t::fold_t::call_t::call_t::raise_t::dead_t::n_to_call == 2);
	static_assert(limit_4p_2r_strategy_t::fold_t::call_t::call_t::raise_t::dead_t::player_no == 3);
	static_assert(limit_4p_2r_strategy_t::fold_t::call_t::call_t::raise_t::dead_t::n_raises_left == 1);
	static_assert(limit_4p_2r_strategy_t::fold_t::call_t::call_t::raise_t::dead_t::player_pots.pots[0] == 2);
	static_assert(limit_4p_2r_strategy_t::fold_t::call_t::call_t::raise_t::dead_t::player_pots.pots[1] == 4);
	static_assert(limit_4p_2r_strategy_t::fold_t::call_t::call_t::raise_t::dead_t::player_pots.pots[2] == 0);
	static_assert(limit_4p_2r_strategy_t::fold_t::call_t::call_t::raise_t::dead_t::player_pots.pots[3] == 2);
	static_assert(limit_4p_2r_strategy_t::fold_t::call_t::call_t::raise_t::dead_t::curr_max_bet == 4);
	static_assert(limit_4p_2r_strategy_t::fold_t::call_t::call_t::raise_t::dead_t::total_pot == 8);

	// UTG folds, P3 calls, SB calls, BB raises, UTG is dead, P3 raises, SB folds
	static_assert(!limit_4p_2r_strategy_t::fold_t::call_t::call_t::raise_t::dead_t::raise_t::fold_t::is_leaf);
	static_assert(!limit_4p_2r_strategy_t::fold_t::call_t::call_t::raise_t::dead_t::raise_t::fold_t::can_raise);
	static_assert(limit_4p_2r_strategy_t::fold_t::call_t::call_t::raise_t::dead_t::raise_t::fold_t::small_blind == 1);
	static_assert(limit_4p_2r_strategy_t::fold_t::call_t::call_t::raise_t::dead_t::raise_t::fold_t::big_blind == 2);
	static_assert(limit_4p_2r_strategy_t::fold_t::call_t::call_t::raise_t::dead_t::raise_t::fold_t::n_players == 4);
	static_assert(limit_4p_2r_strategy_t::fold_t::call_t::call_t::raise_t::dead_t::raise_t::fold_t::n_active == 2);
	static_assert(limit_4p_2r_strategy_t::fold_t::call_t::call_t::raise_t::dead_t::raise_t::fold_t::active_bm == 0xa);
	static_assert(limit_4p_2r_strategy_t::fold_t::call_t::call_t::raise_t::dead_t::raise_t::fold_t::n_to_call == 1);
	static_assert(limit_4p_2r_strategy_t::fold_t::call_t::call_t::raise_t::dead_t::raise_t::fold_t::player_no == 1);
	static_assert(limit_4p_2r_strategy_t::fold_t::call_t::call_t::raise_t::dead_t::raise_t::fold_t::n_raises_left == 0);
	static_assert(limit_4p_2r_strategy_t::fold_t::call_t::call_t::raise_t::dead_t::raise_t::fold_t::player_pots.pots[0] == 2);
	static_assert(limit_4p_2r_strategy_t::fold_t::call_t::call_t::raise_t::dead_t::raise_t::fold_t::player_pots.pots[1] == 4);
	static_assert(limit_4p_2r_strategy_t::fold_t::call_t::call_t::raise_t::dead_t::raise_t::fold_t::player_pots.pots[2] == 0);
	static_assert(limit_4p_2r_strategy_t::fold_t::call_t::call_t::raise_t::dead_t::raise_t::fold_t::player_pots.pots[3] == 6);
	static_assert(limit_4p_2r_strategy_t::fold_t::call_t::call_t::raise_t::dead_t::raise_t::fold_t::curr_max_bet == 6);
	static_assert(limit_4p_2r_strategy_t::fold_t::call_t::call_t::raise_t::dead_t::raise_t::fold_t::total_pot == 12);
	
	// UTG folds, P3 calls, SB calls, BB raises, UTG is dead, P3 raises, SB folds, BB folds
	static_assert(limit_4p_2r_strategy_t::fold_t::call_t::call_t::raise_t::dead_t::raise_t::fold_t::fold_t::is_leaf);
	static_assert(limit_4p_2r_strategy_t::fold_t::call_t::call_t::raise_t::dead_t::raise_t::fold_t::fold_t::small_blind == 1);
	static_assert(limit_4p_2r_strategy_t::fold_t::call_t::call_t::raise_t::dead_t::raise_t::fold_t::fold_t::big_blind == 2);
	static_assert(limit_4p_2r_strategy_t::fold_t::call_t::call_t::raise_t::dead_t::raise_t::fold_t::fold_t::n_players == 4);
	static_assert(limit_4p_2r_strategy_t::fold_t::call_t::call_t::raise_t::dead_t::raise_t::fold_t::fold_t::n_active == 1);
	static_assert(limit_4p_2r_strategy_t::fold_t::call_t::call_t::raise_t::dead_t::raise_t::fold_t::fold_t::active_bm == 0x8);
	static_assert(limit_4p_2r_strategy_t::fold_t::call_t::call_t::raise_t::dead_t::raise_t::fold_t::fold_t::n_to_call == 0);
	static_assert(limit_4p_2r_strategy_t::fold_t::call_t::call_t::raise_t::dead_t::raise_t::fold_t::fold_t::player_no == 2);
	static_assert(limit_4p_2r_strategy_t::fold_t::call_t::call_t::raise_t::dead_t::raise_t::fold_t::fold_t::n_raises_left == 0);
	static_assert(limit_4p_2r_strategy_t::fold_t::call_t::call_t::raise_t::dead_t::raise_t::fold_t::fold_t::player_pots.pots[0] == 2);
	static_assert(limit_4p_2r_strategy_t::fold_t::call_t::call_t::raise_t::dead_t::raise_t::fold_t::fold_t::player_pots.pots[1] == 4);
	static_assert(limit_4p_2r_strategy_t::fold_t::call_t::call_t::raise_t::dead_t::raise_t::fold_t::fold_t::player_pots.pots[2] == 0);
	static_assert(limit_4p_2r_strategy_t::fold_t::call_t::call_t::raise_t::dead_t::raise_t::fold_t::fold_t::player_pots.pots[3] == 6);
	static_assert(limit_4p_2r_strategy_t::fold_t::call_t::call_t::raise_t::dead_t::raise_t::fold_t::fold_t::curr_max_bet == 6);
	static_assert(limit_4p_2r_strategy_t::fold_t::call_t::call_t::raise_t::dead_t::raise_t::fold_t::fold_t::total_pot == 12);
	
	// UTG folds, P3 calls, SB calls, BB raises, UTG is dead, P3 raises, SB folds, BB calls
	static_assert(limit_4p_2r_strategy_t::fold_t::call_t::call_t::raise_t::dead_t::raise_t::fold_t::call_t::is_leaf);
	static_assert(limit_4p_2r_strategy_t::fold_t::call_t::call_t::raise_t::dead_t::raise_t::fold_t::call_t::small_blind == 1);
	static_assert(limit_4p_2r_strategy_t::fold_t::call_t::call_t::raise_t::dead_t::raise_t::fold_t::call_t::big_blind == 2);
	static_assert(limit_4p_2r_strategy_t::fold_t::call_t::call_t::raise_t::dead_t::raise_t::fold_t::call_t::n_players == 4);
	static_assert(limit_4p_2r_strategy_t::fold_t::call_t::call_t::raise_t::dead_t::raise_t::fold_t::call_t::n_active == 2);
	static_assert(limit_4p_2r_strategy_t::fold_t::call_t::call_t::raise_t::dead_t::raise_t::fold_t::call_t::active_bm == 0xa);
	static_assert(limit_4p_2r_strategy_t::fold_t::call_t::call_t::raise_t::dead_t::raise_t::fold_t::call_t::n_to_call == 0);
	static_assert(limit_4p_2r_strategy_t::fold_t::call_t::call_t::raise_t::dead_t::raise_t::fold_t::call_t::player_no == 2);
	static_assert(limit_4p_2r_strategy_t::fold_t::call_t::call_t::raise_t::dead_t::raise_t::fold_t::call_t::n_raises_left == 0);
	static_assert(limit_4p_2r_strategy_t::fold_t::call_t::call_t::raise_t::dead_t::raise_t::fold_t::call_t::player_pots.pots[0] == 2);
	static_assert(limit_4p_2r_strategy_t::fold_t::call_t::call_t::raise_t::dead_t::raise_t::fold_t::call_t::player_pots.pots[1] == 6);
	static_assert(limit_4p_2r_strategy_t::fold_t::call_t::call_t::raise_t::dead_t::raise_t::fold_t::call_t::player_pots.pots[2] == 0);
	static_assert(limit_4p_2r_strategy_t::fold_t::call_t::call_t::raise_t::dead_t::raise_t::fold_t::call_t::player_pots.pots[3] == 6);
	static_assert(limit_4p_2r_strategy_t::fold_t::call_t::call_t::raise_t::dead_t::raise_t::fold_t::call_t::curr_max_bet == 6);
	static_assert(limit_4p_2r_strategy_t::fold_t::call_t::call_t::raise_t::dead_t::raise_t::fold_t::call_t::total_pot == 14);
	
      } // namespace FourPlayerTwoRaises
      
    } // namespace StaticAsserts
    
  } // namespace Gto
  
} // namespace Poker

#endif //def GTO_STRATEGY
