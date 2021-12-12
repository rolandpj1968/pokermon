#ifndef GTO_COMMON
#define GTO_COMMON

#include "types.hpp"
#include "hand-eval.hpp"

namespace Poker {
  
  namespace Gto {
    
    template <int N_PLAYERS>
    struct PlayerPots {
      int pots[N_PLAYERS];

      constexpr inline int get_total_pot() const {
	int total_pot = 0;
	for(int i = 0; i < N_PLAYERS; i++) {
	  total_pot += pots[i];
	}
	return total_pot;
      }
    };
    
    constexpr inline int player_pot_u64_shift(int player_no) {
      return player_no * 8/*bits*/;
    }
    
    static const u64 U8_MASK = 0xff;
    
    constexpr inline u64 player_pot_u64_mask(int player_no) {
      return U8_MASK << player_pot_u64_shift(player_no);
    }
    
    constexpr inline int get_player_pot(int player_no, u64 player_pots_u64) {
      return (player_pots_u64 & player_pot_u64_mask(player_no)) >> player_pot_u64_shift(player_no);
    }
    
    constexpr inline u64 make_player_pot(int player_no, int player_pot) {
      return (u64)player_pot << player_pot_u64_shift(player_no);
    }
    
    constexpr inline u64 update_player_pots(int player_no, int player_pot, u64 player_pots_u64) {
      return
	(player_pots_u64 & ~player_pot_u64_mask(player_no))
	| make_player_pot(player_no, player_pot);
      
    }
    
    constexpr inline u64 make_root_player_pots(int small_blind, int big_blind) {
      return make_player_pot(0, small_blind) | make_player_pot(1, big_blind);
    }
    
    template <int N_PLAYERS>
    constexpr inline PlayerPots<N_PLAYERS> make_player_pots(u64 player_pots_u64) {
      PlayerPots<N_PLAYERS> player_pots = {};
      
      for(int n = 0; n < N_PLAYERS; n++) {
	player_pots.pots[n] = get_player_pot(n, player_pots_u64);
      }
      
      return player_pots;
    }

    constexpr inline int get_curr_max_bet(int n_players, u64 player_pots_u64) {
      int curr_max_bet = -1;
      
      for(int n = 0; n < n_players; n++) {
	curr_max_bet = std::max(curr_max_bet, get_player_pot(n, player_pots_u64));
      }
      
      return curr_max_bet;
    }
    
    constexpr inline int next_player(int player_no, int n_players) {
      return (player_no+1) % n_players;
    }
    
    constexpr inline int player_bet(int player_no, u64 player_pots_u64, int target_bet) {
      return target_bet - get_player_pot(player_no, player_pots_u64);
    }
    
    constexpr inline u8 active_bm_u8_mask(int player_no) {
      return (u8) (1 << player_no);
    }
    
    constexpr inline bool get_is_active(int player_no, u8 active_bm_u8) {
      return (active_bm_u8 & active_bm_u8_mask(player_no)) != 0;
    }
    
    constexpr inline u8 remove_player_from_active_bm(int player_no, u8 active_bm_u8) {
      return active_bm_u8 & ~active_bm_u8_mask(player_no);
    }
    
    constexpr inline int get_n_active(u8 active_bm_u8) {
      return __builtin_popcount(active_bm_u8);
    }

    constexpr inline u8 make_root_active_bm(int n_players) {
      return (u8)((1 << n_players) - 1);
    }
    
    enum LimitHandNodeType {
      FoldCallRaiseNodeType,
      FoldCallNodeType,
      AllButOneFoldNodeType,
      ShowdownNodeType,
      AlreadyFoldedNodeType
    };

    constexpr inline LimitHandNodeType get_node_type(int player_no, u8 active_bm_u8, int n_to_call, int n_raises_left) {
      // Highest priority specialisation - only one player left
      int n_active = get_n_active(active_bm_u8);
      if(n_active == 1) {
	return AllButOneFoldNodeType;
      }
      // 2nd highest priority specialisation - multiple players still in and all called
      if(n_to_call == 0) {
	return ShowdownNodeType;
      }
      // Has the current player already folded - if so this is a dummy node
      bool is_active = get_is_active(player_no, active_bm_u8);
      if(!is_active) {
	return AlreadyFoldedNodeType;
      }
      // If we're at maximum raise level then no further raises allowed
      if(n_raises_left == 0) {
	return FoldCallNodeType;
      }
      // Default node - player can fold, call/check or raise
      return FoldCallRaiseNodeType;
    }
    
    template <int N_PLAYERS>
    struct NodeEvalPerPlayerProfit {
      double profits[N_PLAYERS];

      inline void accumulate(double hand_activity, const NodeEvalPerPlayerProfit<N_PLAYERS> hand_profits) {
	for(int i = 0; i < N_PLAYERS; i++) {
	  profits[i] += hand_activity*hand_profits.profits[i];
	}
      }
      
    };

    // Only valid if active_bm_u8 has only a single bit set
    template <int N_PLAYERS>
    constexpr inline NodeEvalPerPlayerProfit<N_PLAYERS> make_player_profits_for_one_winner(u8 active_bm_u8, u64 player_pots_u64) {
      PlayerPots<N_PLAYERS> player_pots = make_player_pots<N_PLAYERS>(player_pots_u64);
      int total_pot = player_pots.get_total_pot();

      // All players lose their pots except the (single) winner who wins the total pot minus its player pot
      NodeEvalPerPlayerProfit<N_PLAYERS> player_profits = {};
      for(int n = 0; n < N_PLAYERS; n++) {
	if(get_is_active(n, active_bm_u8)) {
	  player_profits.profits[n] = (double) (total_pot - player_pots.pots[n]);
	} else {
	  player_profits.profits[n] = (double) -player_pots.pots[n];
	}
      }
      
      return player_profits;
    }
    
    template <int N_PLAYERS>
    struct NodeEval {
      // Sum over all hands evaluated of per-hand 'probability of reaching this node'.
      double activity;
      // Sum over all hands of per-hand (product of) 'probability of reaching this node' times 'player outcome for the hand'.
      NodeEvalPerPlayerProfit<N_PLAYERS> player_profits;

      inline double rel_player_profit(int player_no) const {
	return activity == 0.0 ? 0.0 : player_profits.profits[player_no]/activity;
      }
      
      // Accumulate results of a hand eval at this node
      inline void accumulate(double hand_activity, const NodeEvalPerPlayerProfit<N_PLAYERS> hand_profits) {
	activity += hand_activity;

	player_profits.accumulate(hand_activity, hand_profits);
      }
    };
    
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
    struct LimitHandNodeConsts {
      static constexpr int small_blind = SMALL_BLIND;
      static constexpr int big_blind = BIG_BLIND;
      static constexpr int n_players = N_PLAYERS;
      static constexpr int n_active = get_n_active(ACTIVE_BM);
      static constexpr u8 active_bm = ACTIVE_BM;
      static constexpr bool is_active = get_is_active(PLAYER_NO, ACTIVE_BM);
      static constexpr int n_to_call = N_TO_CALL;
      static constexpr int player_no = PLAYER_NO;
      static constexpr int n_raises_left = N_RAISES_LEFT;
      static constexpr PlayerPots<N_PLAYERS> player_pots = make_player_pots<N_PLAYERS>(PLAYER_POTS);
      static constexpr int curr_max_bet = get_curr_max_bet(N_PLAYERS, PLAYER_POTS);
      static constexpr int total_pot = player_pots.get_total_pot();
    };

    template <int N_PLAYERS, typename PlayerStrategyT>
    struct PlayerStrategies {
      PlayerStrategyT* strategies[N_PLAYERS];

      constexpr inline PlayerStrategyT& get_player_strategy(int player_no) const {
	return *strategies[player_no];
      }
    };
    
    template <int N_PLAYERS, typename PlayerStrategyT>
    struct PlayerStrategiesFoldGetter {
      typedef typename PlayerStrategyT::fold_t fold_t;

      static constexpr inline PlayerStrategies<N_PLAYERS, fold_t> get_fold_strategies(const PlayerStrategies<N_PLAYERS, PlayerStrategyT> strategies) {
	PlayerStrategies<N_PLAYERS, fold_t> fold_strategies = {};
	
	for(int n = 0; n < N_PLAYERS; n++) {
	  fold_strategies.strategies[n] = &strategies.strategies[n]->fold;
	}
	
	return fold_strategies;
      }
    };
    
    template <int N_PLAYERS, typename PlayerStrategyT>
    struct PlayerStrategiesCallGetter {
      typedef typename PlayerStrategyT::call_t call_t;

      static constexpr inline PlayerStrategies<N_PLAYERS, call_t> get_call_strategies(const PlayerStrategies<N_PLAYERS, PlayerStrategyT> strategies) {
	PlayerStrategies<N_PLAYERS, call_t> call_strategies = {};
	
	for(int n = 0; n < N_PLAYERS; n++) {
	  call_strategies.strategies[n] = &strategies.strategies[n]->call;
	}
	
	return call_strategies;
      }
    };
    
    template <int N_PLAYERS, typename PlayerStrategyT>
    struct PlayerStrategiesRaiseGetter {
      typedef typename PlayerStrategyT::raise_t raise_t;

      static constexpr inline PlayerStrategies<N_PLAYERS, raise_t> get_raise_strategies(const PlayerStrategies<N_PLAYERS, PlayerStrategyT> strategies) {
	PlayerStrategies<N_PLAYERS, raise_t> raise_strategies = {};
	
	for(int n = 0; n < N_PLAYERS; n++) {
	  raise_strategies.strategies[n] = &strategies.strategies[n]->raise;
	}
	
	return raise_strategies;
      }
    };
    
    template <int N_PLAYERS, typename PlayerStrategyT>
    struct PlayerStrategiesDeadGetter {
      typedef typename PlayerStrategyT::dead_t dead_t;

      static constexpr inline PlayerStrategies<N_PLAYERS, dead_t> get_dead_strategies(const PlayerStrategies<N_PLAYERS, PlayerStrategyT> strategies) {
	PlayerStrategies<N_PLAYERS, dead_t> dead_strategies = {};
	
	for(int n = 0; n < N_PLAYERS; n++) {
	  dead_strategies.strategies[n] = &strategies.strategies[n]->dead;
	}
	
	return dead_strategies;
      }
    };
    
    template <int N_PLAYERS, typename PlayerEvalT>
    struct PlayerEvals {
      PlayerEvalT* evals[N_PLAYERS];

      constexpr inline PlayerEvalT& get_player_eval(int player_no) const {
	return *evals[player_no];
      }

      inline void accumulate(double node_prob, const NodeEvalPerPlayerProfit<N_PLAYERS> player_profits) {
	for(int n = 0; n < N_PLAYERS; n++) {
	  evals[n]->eval.accumulate(node_prob, player_profits);
	}
      }
    };
    
    template <int N_PLAYERS, typename PlayerEvalT>
    struct PlayerEvalsFoldGetter {
      typedef typename PlayerEvalT::fold_t fold_t;

      static constexpr inline PlayerEvals<N_PLAYERS, fold_t> get_fold_evals(const PlayerEvals<N_PLAYERS, PlayerEvalT> evals) {
	PlayerEvals<N_PLAYERS, fold_t> fold_evals = {};
	
	for(int n = 0; n < N_PLAYERS; n++) {
	  fold_evals.evals[n] = &evals.evals[n]->fold;
	}
	
	return fold_evals;
      }
    };
    
    template <int N_PLAYERS, typename PlayerEvalT>
    struct PlayerEvalsCallGetter {
      typedef typename PlayerEvalT::call_t call_t;

      static constexpr inline PlayerEvals<N_PLAYERS, call_t> get_call_evals(const PlayerEvals<N_PLAYERS, PlayerEvalT> evals) {
	PlayerEvals<N_PLAYERS, call_t> call_evals = {};
	
	for(int n = 0; n < N_PLAYERS; n++) {
	  call_evals.evals[n] = &evals.evals[n]->call;
	}
	
	return call_evals;
      }
    };
    
    template <int N_PLAYERS, typename PlayerEvalT>
    struct PlayerEvalsRaiseGetter {
      typedef typename PlayerEvalT::raise_t raise_t;

      static constexpr inline PlayerEvals<N_PLAYERS, raise_t> get_raise_evals(const PlayerEvals<N_PLAYERS, PlayerEvalT> evals) {
	PlayerEvals<N_PLAYERS, raise_t> raise_evals = {};
	
	for(int n = 0; n < N_PLAYERS; n++) {
	  raise_evals.evals[n] = &evals.evals[n]->raise;
	}
	
	return raise_evals;
      }
    };
    
    template <int N_PLAYERS, typename PlayerEvalT>
    struct PlayerEvalsDeadGetter {
      typedef typename PlayerEvalT::dead_t dead_t;

      static constexpr inline PlayerEvals<N_PLAYERS, dead_t> get_dead_evals(const PlayerEvals<N_PLAYERS, PlayerEvalT> evals) {
	PlayerEvals<N_PLAYERS, dead_t> dead_evals = {};
	
	for(int n = 0; n < N_PLAYERS; n++) {
	  dead_evals.evals[n] = &evals.evals[n]->dead;
	}
	
	return dead_evals;
      }
    };
    
    template <int N_PLAYERS>
    struct PlayerHandRankings {
      HandRankingT rankings[N_PLAYERS];
    };
    
    template <int N_PLAYERS>
    struct PlayerHandValues {
      HandValueT values[N_PLAYERS];
    };
    
    template <int N_PLAYERS>
    struct PlayerHandEvals {
      HandEval::HandEvalT evals[N_PLAYERS];
    };
  } // namespace Gto
  
} // namespace Poker




#endif //def GTO_COMMON
