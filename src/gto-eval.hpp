#ifndef GTO_EVAL
#define GTO_EVAL

#include "gto-common.hpp"
#include "types.hpp"

namespace Poker {
  
  namespace Gto {
    
    // Declaration of LimitHandEval.
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
    struct LimitHandEval;

    // Declaration of LimitHandEvalSpecialised.
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
    struct LimitHandEvalSpecialised;

    // Definition of LimitHandEval as one of four specialisations
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
    struct LimitHandEval :
      LimitHandEvalSpecialised<
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
    
    // Child node of this eval tree node for current player folding
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
    struct LimitHandEvalFoldChild {
      typedef LimitHandEval<
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
    
    // Child node of this eval tree node for current player calling (or checking).
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
    struct LimitHandEvalCallChild {
      typedef LimitHandEval<
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
    
    // Child node of this eval tree node for current player raising.
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
    struct LimitHandEvalRaiseChild {
      typedef LimitHandEval<
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
    
    // Default specialisation of LimitHandEval.
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
    struct LimitHandEvalSpecialised<SMALL_BLIND, BIG_BLIND, N_PLAYERS, ACTIVE_BM, N_TO_CALL, PLAYER_NO, N_RAISES_LEFT, PLAYER_POTS, FoldCallRaiseNodeType> :
      LimitHandNodeConsts<SMALL_BLIND, BIG_BLIND, N_PLAYERS, ACTIVE_BM, N_TO_CALL, PLAYER_NO, N_RAISES_LEFT, PLAYER_POTS>,
      LimitHandEvalFoldChild<SMALL_BLIND, BIG_BLIND, N_PLAYERS, ACTIVE_BM, N_TO_CALL, PLAYER_NO, N_RAISES_LEFT, PLAYER_POTS>,
      LimitHandEvalCallChild<SMALL_BLIND, BIG_BLIND, N_PLAYERS, ACTIVE_BM, N_TO_CALL, PLAYER_NO, N_RAISES_LEFT, PLAYER_POTS>,
      LimitHandEvalRaiseChild<SMALL_BLIND, BIG_BLIND, N_PLAYERS, ACTIVE_BM, N_TO_CALL, PLAYER_NO, N_RAISES_LEFT, PLAYER_POTS>
    {
      static const bool is_leaf = false;
      static const bool can_raise = true;
      
      NodeEval<N_PLAYERS> eval;

      template <typename PlayerEvalT, typename PlayerStrategyT>
      static inline NodeEvalPerPlayerProfit<N_PLAYERS> evaluate_hand(double node_prob, PlayerEvals<N_PLAYERS, PlayerEvalT> player_evals, const PlayerStrategies<N_PLAYERS, PlayerStrategyT>& player_strategies, const PlayerHandEvals<N_PLAYERS>& player_hand_evals) {
	NodeEvalPerPlayerProfit<N_PLAYERS> player_profits = {};

	const PlayerStrategyT& curr_player_strategy = player_strategies.get_player_strategy(PLAYER_NO);

	double fold_p = curr_player_strategy.strategy.fold_p;
	if(fold_p != 0.0) {
	  auto fold_evals = PlayerEvalsFoldGetter<N_PLAYERS, PlayerEvalT>::get_fold_evals(player_evals);
	  auto fold_strategies = PlayerStrategiesFoldGetter<N_PLAYERS, PlayerStrategyT>::get_fold_strategies(player_strategies);
	  typedef typename PlayerEvalT::fold_t eval_fold_t;
	  NodeEvalPerPlayerProfit<N_PLAYERS> fold_profits = eval_fold_t::evaluate_hand(node_prob*fold_p, fold_evals, fold_strategies, player_hand_evals);
	  player_profits.accumulate(fold_p, fold_profits);
	}

	double call_p = curr_player_strategy.strategy.call_p;
	if(call_p != 0.0) {
	  auto call_evals = PlayerEvalsCallGetter<N_PLAYERS, PlayerEvalT>::get_call_evals(player_evals);
	  auto call_strategies = PlayerStrategiesCallGetter<N_PLAYERS, PlayerStrategyT>::get_call_strategies(player_strategies);
	  typedef typename PlayerEvalT::call_t eval_call_t;
	  NodeEvalPerPlayerProfit<N_PLAYERS> call_profits = eval_call_t::evaluate_hand(node_prob*call_p, call_evals, call_strategies, player_hand_evals);
	  player_profits.accumulate(call_p, call_profits);
	}

	double raise_p = curr_player_strategy.strategy.raise_p;
	if(raise_p != 0.0) {
	  auto raise_evals = PlayerEvalsRaiseGetter<N_PLAYERS, PlayerEvalT>::get_raise_evals(player_evals);
	  auto raise_strategies = PlayerStrategiesRaiseGetter<N_PLAYERS, PlayerStrategyT>::get_raise_strategies(player_strategies);
	  typedef typename PlayerEvalT::raise_t eval_raise_t;
	  NodeEvalPerPlayerProfit<N_PLAYERS> raise_profits = eval_raise_t::evaluate_hand(node_prob*raise_p, raise_evals, raise_strategies, player_hand_evals);
	  player_profits.accumulate(raise_p, raise_profits);
	}
	
	player_evals.accumulate(node_prob, player_profits);

	return player_profits;
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
    struct LimitHandEvalSpecialised<SMALL_BLIND, BIG_BLIND, N_PLAYERS, ACTIVE_BM, N_TO_CALL, PLAYER_NO, N_RAISES_LEFT, PLAYER_POTS, AllButOneFoldNodeType>
      : LimitHandNodeConsts<SMALL_BLIND, BIG_BLIND, N_PLAYERS, ACTIVE_BM, N_TO_CALL, PLAYER_NO, N_RAISES_LEFT, PLAYER_POTS>
    {
      static const bool is_leaf = true;
      
      NodeEval<N_PLAYERS> eval;

      template <typename PlayerEvalT, typename PlayerStrategyT>
      static inline NodeEvalPerPlayerProfit<N_PLAYERS> evaluate_hand(double node_prob, PlayerEvals<N_PLAYERS, PlayerEvalT> player_evals, const PlayerStrategies<N_PLAYERS, PlayerStrategyT>& player_strategies, const PlayerHandEvals<N_PLAYERS>& player_hand_evals) {
	// The single player remaining takes the pot.
	NodeEvalPerPlayerProfit<N_PLAYERS> player_profits = make_player_profits_for_one_winner<N_PLAYERS>(ACTIVE_BM, PLAYER_POTS);
	
	player_evals.accumulate(node_prob, player_profits);

	return player_profits;
      }
    
    };

    template <int N_PLAYERS>
    inline u8 get_active_winners_bm(u8 active_bm_u8, const PlayerHandEvals<N_PLAYERS>& player_hand_evals) {
      bool have_active = false;
      u8 active_winners_bm_u8 = 0;
      HandEval::HandEvalT winners_eval;

      for(int n = 0; n < N_PLAYERS; n++) {
	const HandEval::HandEvalT curr_player_eval = player_hand_evals.evals[n];

	// Ignore inactive (folded) players.
	if(!get_is_active(n, active_bm_u8)) {
	  continue;
	}

	// Is this the first active hand - if so it is the winner so far
	if(!have_active) {
	  have_active = true;

	  active_winners_bm_u8 = active_bm_u8_mask(n);
	  winners_eval = curr_player_eval;

	  continue;
	}

	// Most common case - current hand is not the best
	if(curr_player_eval < winners_eval) {
	  continue;
	}

	if(curr_player_eval == winners_eval) {
	  // Tie with the winners - add current player to winners bm
	  active_winners_bm_u8 |= active_bm_u8_mask(n);
	} else {
	  // New solo winner
	  active_winners_bm_u8 = active_bm_u8_mask(n);
	  winners_eval = curr_player_eval;
	}
      }

      return active_winners_bm_u8;
    }
    
    // Only valid if active_bm_u8 has at least one bit set
    template <int N_PLAYERS>
    inline NodeEvalPerPlayerProfit<N_PLAYERS> make_player_profits_for_showdown(u8 active_bm_u8, u64 player_pots_u64, const PlayerHandEvals<N_PLAYERS>& player_hand_evals) {
      PlayerPots<N_PLAYERS> player_pots = make_player_pots<N_PLAYERS>(player_pots_u64);
      int total_pot = player_pots.get_total_pot();

      u8 winners_bm_u8 = get_active_winners_bm<N_PLAYERS>(active_bm_u8, player_hand_evals);

      // First pass - accumulate the winners' total pots
      int winners_total_pot = 0;
      for(int n = 0; n < N_PLAYERS; n++) {
	if(get_is_active(n, winners_bm_u8)) {
	  winners_total_pot += player_pots.pots[n];
	}
      }

      int losers_total_pot = total_pot - winners_total_pot;
      int n_winners = get_n_active(winners_bm_u8);

      // Second pass.
      // All players lose their pots except the active winners who share the total pot minus the winners' total pots.
      NodeEvalPerPlayerProfit<N_PLAYERS> player_profits = {};
      for(int n = 0; n < N_PLAYERS; n++) {
	if(get_is_active(n, winners_bm_u8)) {
	  player_profits.profits[n] = (double)losers_total_pot / (double)n_winners;
	} else {
	  player_profits.profits[n] = (double) -player_pots.pots[n];
	}
      }
      
      return player_profits;
    }
    
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
    struct LimitHandEvalSpecialised<SMALL_BLIND, BIG_BLIND, N_PLAYERS, ACTIVE_BM, N_TO_CALL, PLAYER_NO, N_RAISES_LEFT, PLAYER_POTS, ShowdownNodeType>
      : LimitHandNodeConsts<SMALL_BLIND, BIG_BLIND, N_PLAYERS, ACTIVE_BM, N_TO_CALL, PLAYER_NO, N_RAISES_LEFT, PLAYER_POTS>
    {
      static const bool is_leaf = true;
      
      NodeEval<N_PLAYERS> eval;

      template <typename PlayerEvalT, typename PlayerStrategyT>
      static inline NodeEvalPerPlayerProfit<N_PLAYERS> evaluate_hand(double node_prob, PlayerEvals<N_PLAYERS, PlayerEvalT> player_evals, const PlayerStrategies<N_PLAYERS, PlayerStrategyT>& player_strategies, const PlayerHandEvals<N_PLAYERS>& player_hand_evals) {
	// The active top-ranked players share the pot
	NodeEvalPerPlayerProfit<N_PLAYERS> player_profits = make_player_profits_for_showdown<N_PLAYERS>(ACTIVE_BM, PLAYER_POTS, player_hand_evals);
	
	player_evals.accumulate(node_prob, player_profits);

	return player_profits;
      }
      
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
    struct LimitHandEvalSpecialised<SMALL_BLIND, BIG_BLIND, N_PLAYERS, ACTIVE_BM, N_TO_CALL, PLAYER_NO, N_RAISES_LEFT, PLAYER_POTS, AlreadyFoldedNodeType>
      : LimitHandNodeConsts<SMALL_BLIND, BIG_BLIND, N_PLAYERS, ACTIVE_BM, N_TO_CALL, PLAYER_NO, N_RAISES_LEFT, PLAYER_POTS>
    {
      static const bool is_leaf = false;
      
      typedef LimitHandEval<
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

      template <typename PlayerEvalT, typename PlayerStrategyT>
      static inline NodeEvalPerPlayerProfit<N_PLAYERS> evaluate_hand(double node_prob, PlayerEvals<N_PLAYERS, PlayerEvalT> player_evals, const PlayerStrategies<N_PLAYERS, PlayerStrategyT>& player_strategies, const PlayerHandEvals<N_PLAYERS>& player_hand_evals) {
	auto dead_evals = PlayerEvalsDeadGetter<N_PLAYERS, PlayerEvalT>::get_dead_evals(player_evals);
	auto dead_strategies = PlayerStrategiesDeadGetter<N_PLAYERS, PlayerStrategyT>::get_dead_strategies(player_strategies);
	typedef typename PlayerEvalT::dead_t eval_dead_t;
	
	return eval_dead_t::evaluate_hand(node_prob, dead_evals, dead_strategies, player_hand_evals);
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
    struct LimitHandEvalSpecialised<SMALL_BLIND, BIG_BLIND, N_PLAYERS, ACTIVE_BM, N_TO_CALL, PLAYER_NO, N_RAISES_LEFT, PLAYER_POTS, FoldCallNodeType> :
      LimitHandNodeConsts<SMALL_BLIND, BIG_BLIND, N_PLAYERS, ACTIVE_BM, N_TO_CALL, PLAYER_NO, N_RAISES_LEFT, PLAYER_POTS>,
      LimitHandEvalFoldChild<SMALL_BLIND, BIG_BLIND, N_PLAYERS, ACTIVE_BM, N_TO_CALL, PLAYER_NO, N_RAISES_LEFT, PLAYER_POTS>,
      LimitHandEvalCallChild<SMALL_BLIND, BIG_BLIND, N_PLAYERS, ACTIVE_BM, N_TO_CALL, PLAYER_NO, N_RAISES_LEFT, PLAYER_POTS>
    {
      static const bool is_leaf = false;
      static const bool can_raise = false;
      
      NodeEval<N_PLAYERS> eval;

      template <typename PlayerEvalT, typename PlayerStrategyT>
      static inline NodeEvalPerPlayerProfit<N_PLAYERS> evaluate_hand(double node_prob, PlayerEvals<N_PLAYERS, PlayerEvalT> player_evals, const PlayerStrategies<N_PLAYERS, PlayerStrategyT>& player_strategies, const PlayerHandEvals<N_PLAYERS>& player_hand_evals) {
	NodeEvalPerPlayerProfit<N_PLAYERS> player_profits = {};

	const PlayerStrategyT& curr_player_strategy = player_strategies.get_player_strategy(PLAYER_NO);

	double fold_p = curr_player_strategy.strategy.fold_p;
	if(fold_p != 0.0) {
	  auto fold_evals = PlayerEvalsFoldGetter<N_PLAYERS, PlayerEvalT>::get_fold_evals(player_evals);
	  auto fold_strategies = PlayerStrategiesFoldGetter<N_PLAYERS, PlayerStrategyT>::get_fold_strategies(player_strategies);
	  typedef typename PlayerEvalT::fold_t eval_fold_t;
	  NodeEvalPerPlayerProfit<N_PLAYERS> fold_profits = eval_fold_t::evaluate_hand(node_prob*fold_p, fold_evals, fold_strategies, player_hand_evals);
	  player_profits.accumulate(fold_p, fold_profits);
	}

	double call_p = curr_player_strategy.strategy.call_p;
	if(call_p != 0.0) {
	  auto call_evals = PlayerEvalsCallGetter<N_PLAYERS, PlayerEvalT>::get_call_evals(player_evals);
	  auto call_strategies = PlayerStrategiesCallGetter<N_PLAYERS, PlayerStrategyT>::get_call_strategies(player_strategies);
	  typedef typename PlayerEvalT::call_t eval_call_t;
	  NodeEvalPerPlayerProfit<N_PLAYERS> call_profits = eval_call_t::evaluate_hand(node_prob*call_p, call_evals, call_strategies, player_hand_evals);
	  player_profits.accumulate(call_p, call_profits);
	}

	player_evals.accumulate(node_prob, player_profits);

	return player_profits;
      }
      
    };
    
    template <
      int N_PLAYERS,
      int N_RAISES = 3,
      int SMALL_BLIND = 1,
      int BIG_BLIND = 2
      >
    struct LimitRootHandEval {
      typedef LimitHandEval<
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

    template <typename T>
    struct PerHoleHandContainer {
      // Pocket pair hole hands
      // Indexed AceLow (0) ... King (12)
      T pocket_pairs[13];

      // Suited non-pairs
      // Indexed densely
      T suited[13*12/2];

      // Offsuit non-pairs
      // Indexed densely
      T offsuit[13*12/2];

      // rank0 > rank1
      // rank0, rank1 in [0..13)
      static constexpr size_t get_non_pair_index(size_t rank0, size_t rank1) {
	size_t total_size = 13*12/2;
	size_t rank0_remaining = (rank0+1)*rank0/2;
	size_t rank0_offset = total_size - rank0_remaining;

	size_t index = rank0_offset + rank1;
	if(index >= total_size) {
	  printf("Boooooooooooooooooooooo get_non_pair_index(%zu, %zu) -> %zu limit is %zu\n", rank0, rank1, index, total_size);
	}
	
	return rank0_offset + rank1;
      }

      
      // Demote Ace to AceLow
      inline void normalise_rank(RankT& rank) {
	rank = to_ace_low(rank);
      }

      inline T& get_pocket_pair_value(RankT rank) {
	normalise_rank(rank);

	return pocket_pairs[rank];
      }

      // Demote Ace to AceLow and order rank0, rank1 so that:
      //   rank0 > rank1
      //   rank0, rank1 in [0..13)
      inline void normalise_ranks(RankT& rank0, RankT& rank1) {
	normalise_rank(rank0);
	normalise_rank(rank1);

	if(rank0 < rank1) {
	  RankT tmp = rank0; rank0 = rank1; rank1 = tmp;
	}
      }

      inline T& get_suited_value(RankT rank0, RankT rank1) {
	normalise_ranks(rank0, rank1);

	return suited[get_non_pair_index(rank0, rank1)];
      }
	    
      inline T& get_offsuit_value(RankT rank0, RankT rank1) {
	normalise_ranks(rank0, rank1);

	return offsuit[get_non_pair_index(rank0, rank1)];
      }
	    
      inline T& get_value(const CardT card0, const CardT card1) {
	// Ensure AceLow - i.e. ranks in [0..13)
	RankT rank0 = to_ace_low(card0.rank);
	RankT rank1 = to_ace_low(card1.rank);

	if(rank0 == rank1) {
	  return pocket_pairs[rank0];
	}

	// Ensure rank0 >= rank1
	if(rank0 < rank1) {
	  RankT tmp = rank0; rank0 = rank1; rank1 = tmp;
	}

	size_t non_pair_index = get_non_pair_index((size_t)rank0, (size_t)rank1);

	if(card0.suit == card1.suit) {
	  return suited[non_pair_index];
	} else {
	  return offsuit[non_pair_index];
	}
      }
    };

    // Let's see if we and gcc got this right...
    
    namespace StaticAsserts {

      namespace TwoPlayerZeroRaises {
	
	typedef LimitRootHandEval</*N_PLAYERS*/2, /*N_RAISES*/0>::type_t limit_2p_0r_eval_t;
	
	// Root node
	static_assert(!limit_2p_0r_eval_t::is_leaf);
	static_assert(!limit_2p_0r_eval_t::can_raise);
	static_assert(limit_2p_0r_eval_t::small_blind == 1);
	static_assert(limit_2p_0r_eval_t::big_blind == 2);
	static_assert(limit_2p_0r_eval_t::n_players == 2);
	static_assert(limit_2p_0r_eval_t::n_active == 2);
	static_assert(limit_2p_0r_eval_t::active_bm == 0x3);
	static_assert(limit_2p_0r_eval_t::n_to_call == 2);
	static_assert(limit_2p_0r_eval_t::player_no == 0); // SB to bet
	static_assert(limit_2p_0r_eval_t::n_raises_left == 0);
	static_assert(limit_2p_0r_eval_t::player_pots.pots[0] == 1);
	static_assert(limit_2p_0r_eval_t::player_pots.pots[1] == 2);
	static_assert(limit_2p_0r_eval_t::curr_max_bet == 2);
	static_assert(limit_2p_0r_eval_t::total_pot == 3);
	
	// SB folds
	static_assert(limit_2p_0r_eval_t::fold_t::is_leaf); // Only BB left
	static_assert(limit_2p_0r_eval_t::fold_t::small_blind == 1);
	static_assert(limit_2p_0r_eval_t::fold_t::big_blind == 2);
	static_assert(limit_2p_0r_eval_t::fold_t::n_players == 2);
	static_assert(limit_2p_0r_eval_t::fold_t::n_active == 1);
	static_assert(limit_2p_0r_eval_t::fold_t::active_bm == 0x2);
	static_assert(limit_2p_0r_eval_t::fold_t::n_to_call == 1);
	static_assert(limit_2p_0r_eval_t::fold_t::player_no == 1); // BB to bet
	static_assert(limit_2p_0r_eval_t::fold_t::n_raises_left == 0);
	static_assert(limit_2p_0r_eval_t::fold_t::player_pots.pots[0] == 1);
	static_assert(limit_2p_0r_eval_t::fold_t::player_pots.pots[1] == 2);
	static_assert(limit_2p_0r_eval_t::fold_t::curr_max_bet == 2);
	static_assert(limit_2p_0r_eval_t::fold_t::total_pot == 3);
	
	// SB calls
	static_assert(!limit_2p_0r_eval_t::call_t::is_leaf);
	static_assert(!limit_2p_0r_eval_t::call_t::can_raise);
	static_assert(limit_2p_0r_eval_t::call_t::small_blind == 1);
	static_assert(limit_2p_0r_eval_t::call_t::big_blind == 2);
	static_assert(limit_2p_0r_eval_t::call_t::n_players == 2);
	static_assert(limit_2p_0r_eval_t::call_t::n_active == 2);
	static_assert(limit_2p_0r_eval_t::call_t::active_bm == 0x3);
	static_assert(limit_2p_0r_eval_t::call_t::n_to_call == 1);
	static_assert(limit_2p_0r_eval_t::call_t::player_no == 1); // BB to bet
	static_assert(limit_2p_0r_eval_t::call_t::n_raises_left == 0);
	static_assert(limit_2p_0r_eval_t::call_t::player_pots.pots[0] == 2);
	static_assert(limit_2p_0r_eval_t::call_t::player_pots.pots[1] == 2);
	static_assert(limit_2p_0r_eval_t::call_t::curr_max_bet == 2);
	static_assert(limit_2p_0r_eval_t::call_t::total_pot == 4);
	
	// SB calls, BB folds
	static_assert(limit_2p_0r_eval_t::call_t::fold_t::is_leaf);
	static_assert(limit_2p_0r_eval_t::call_t::fold_t::small_blind == 1);
	static_assert(limit_2p_0r_eval_t::call_t::fold_t::big_blind == 2);
	static_assert(limit_2p_0r_eval_t::call_t::fold_t::n_players == 2);
	static_assert(limit_2p_0r_eval_t::call_t::fold_t::n_active == 1);
	static_assert(limit_2p_0r_eval_t::call_t::fold_t::active_bm == 0x1);
	static_assert(limit_2p_0r_eval_t::call_t::fold_t::n_to_call == 0);
	static_assert(limit_2p_0r_eval_t::call_t::fold_t::player_no == 0);
	static_assert(limit_2p_0r_eval_t::call_t::fold_t::n_raises_left == 0);
	static_assert(limit_2p_0r_eval_t::call_t::fold_t::player_pots.pots[0] == 2);
	static_assert(limit_2p_0r_eval_t::call_t::fold_t::player_pots.pots[1] == 2);
	static_assert(limit_2p_0r_eval_t::call_t::fold_t::curr_max_bet == 2);
	static_assert(limit_2p_0r_eval_t::call_t::fold_t::total_pot == 4);
	
	// SB calls, BB calls
	static_assert(limit_2p_0r_eval_t::call_t::call_t::is_leaf);
	static_assert(limit_2p_0r_eval_t::call_t::call_t::small_blind == 1);
	static_assert(limit_2p_0r_eval_t::call_t::call_t::big_blind == 2);
	static_assert(limit_2p_0r_eval_t::call_t::call_t::n_players == 2);
	static_assert(limit_2p_0r_eval_t::call_t::call_t::n_active == 2);
	static_assert(limit_2p_0r_eval_t::call_t::call_t::active_bm == 0x3);
	static_assert(limit_2p_0r_eval_t::call_t::call_t::n_to_call == 0);
	static_assert(limit_2p_0r_eval_t::call_t::call_t::player_no == 0);
	static_assert(limit_2p_0r_eval_t::call_t::call_t::n_raises_left == 0);
	static_assert(limit_2p_0r_eval_t::call_t::call_t::player_pots.pots[0] == 2);
	static_assert(limit_2p_0r_eval_t::call_t::call_t::player_pots.pots[1] == 2);
	static_assert(limit_2p_0r_eval_t::call_t::call_t::curr_max_bet == 2);
	static_assert(limit_2p_0r_eval_t::call_t::call_t::total_pot == 4);

      } // namespace TwoPlayerZeroRaises

      namespace TwoPlayerOneRaise {
	
	typedef LimitRootHandEval</*N_PLAYERS*/2, /*N_RAISES*/1>::type_t limit_2p_1r_eval_t;
	
	// Root node
	static_assert(!limit_2p_1r_eval_t::is_leaf);
	static_assert(limit_2p_1r_eval_t::can_raise);
	static_assert(limit_2p_1r_eval_t::small_blind == 1);
	static_assert(limit_2p_1r_eval_t::big_blind == 2);
	static_assert(limit_2p_1r_eval_t::n_players == 2);
	static_assert(limit_2p_1r_eval_t::n_active == 2);
	static_assert(limit_2p_1r_eval_t::active_bm == 0x3);
	static_assert(limit_2p_1r_eval_t::n_to_call == 2);
	static_assert(limit_2p_1r_eval_t::player_no == 0); // SB to bet
	static_assert(limit_2p_1r_eval_t::n_raises_left == 1);
	static_assert(limit_2p_1r_eval_t::player_pots.pots[0] == 1);
	static_assert(limit_2p_1r_eval_t::player_pots.pots[1] == 2);
	static_assert(limit_2p_1r_eval_t::curr_max_bet == 2);
	static_assert(limit_2p_1r_eval_t::total_pot == 3);
	
	// SB folds
	static_assert(limit_2p_1r_eval_t::fold_t::is_leaf); // Only BB left
	static_assert(limit_2p_1r_eval_t::fold_t::small_blind == 1);
	static_assert(limit_2p_1r_eval_t::fold_t::big_blind == 2);
	static_assert(limit_2p_1r_eval_t::fold_t::n_players == 2);
	static_assert(limit_2p_1r_eval_t::fold_t::n_active == 1);
	static_assert(limit_2p_1r_eval_t::fold_t::active_bm == 0x2);
	static_assert(limit_2p_1r_eval_t::fold_t::n_to_call == 1);
	static_assert(limit_2p_1r_eval_t::fold_t::player_no == 1); // BB to bet
	static_assert(limit_2p_1r_eval_t::fold_t::n_raises_left == 1);
	static_assert(limit_2p_1r_eval_t::fold_t::player_pots.pots[0] == 1);
	static_assert(limit_2p_1r_eval_t::fold_t::player_pots.pots[1] == 2);
	static_assert(limit_2p_1r_eval_t::fold_t::curr_max_bet == 2);
	static_assert(limit_2p_1r_eval_t::fold_t::total_pot == 3);
	
	// SB calls
	static_assert(!limit_2p_1r_eval_t::call_t::is_leaf);
	static_assert(limit_2p_1r_eval_t::call_t::can_raise);
	static_assert(limit_2p_1r_eval_t::call_t::small_blind == 1);
	static_assert(limit_2p_1r_eval_t::call_t::big_blind == 2);
	static_assert(limit_2p_1r_eval_t::call_t::n_players == 2);
	static_assert(limit_2p_1r_eval_t::call_t::n_active == 2);
	static_assert(limit_2p_1r_eval_t::call_t::active_bm == 0x3);
	static_assert(limit_2p_1r_eval_t::call_t::n_to_call == 1);
	static_assert(limit_2p_1r_eval_t::call_t::player_no == 1); // BB to bet
	static_assert(limit_2p_1r_eval_t::call_t::n_raises_left == 1);
	static_assert(limit_2p_1r_eval_t::call_t::player_pots.pots[0] == 2);
	static_assert(limit_2p_1r_eval_t::call_t::player_pots.pots[1] == 2);
	static_assert(limit_2p_1r_eval_t::call_t::curr_max_bet == 2);
	static_assert(limit_2p_1r_eval_t::call_t::total_pot == 4);
	
	// SB calls, BB folds
	static_assert(limit_2p_1r_eval_t::call_t::fold_t::is_leaf);
	static_assert(limit_2p_1r_eval_t::call_t::fold_t::small_blind == 1);
	static_assert(limit_2p_1r_eval_t::call_t::fold_t::big_blind == 2);
	static_assert(limit_2p_1r_eval_t::call_t::fold_t::n_players == 2);
	static_assert(limit_2p_1r_eval_t::call_t::fold_t::n_active == 1);
	static_assert(limit_2p_1r_eval_t::call_t::fold_t::active_bm == 0x1);
	static_assert(limit_2p_1r_eval_t::call_t::fold_t::n_to_call == 0);
	static_assert(limit_2p_1r_eval_t::call_t::fold_t::player_no == 0);
	static_assert(limit_2p_1r_eval_t::call_t::fold_t::n_raises_left == 1);
	static_assert(limit_2p_1r_eval_t::call_t::fold_t::player_pots.pots[0] == 2);
	static_assert(limit_2p_1r_eval_t::call_t::fold_t::player_pots.pots[1] == 2);
	static_assert(limit_2p_1r_eval_t::call_t::fold_t::curr_max_bet == 2);
	static_assert(limit_2p_1r_eval_t::call_t::fold_t::total_pot == 4);
	
	// SB calls, BB calls
	static_assert(limit_2p_1r_eval_t::call_t::call_t::is_leaf);
	static_assert(limit_2p_1r_eval_t::call_t::call_t::small_blind == 1);
	static_assert(limit_2p_1r_eval_t::call_t::call_t::big_blind == 2);
	static_assert(limit_2p_1r_eval_t::call_t::call_t::n_players == 2);
	static_assert(limit_2p_1r_eval_t::call_t::call_t::n_active == 2);
	static_assert(limit_2p_1r_eval_t::call_t::call_t::active_bm == 0x3);
	static_assert(limit_2p_1r_eval_t::call_t::call_t::n_to_call == 0);
	static_assert(limit_2p_1r_eval_t::call_t::call_t::player_no == 0);
	static_assert(limit_2p_1r_eval_t::call_t::call_t::n_raises_left == 1);
	static_assert(limit_2p_1r_eval_t::call_t::call_t::player_pots.pots[0] == 2);
	static_assert(limit_2p_1r_eval_t::call_t::call_t::player_pots.pots[1] == 2);
	static_assert(limit_2p_1r_eval_t::call_t::call_t::curr_max_bet == 2);
	static_assert(limit_2p_1r_eval_t::call_t::call_t::total_pot == 4);
	
	// SB calls, BB raises
	static_assert(!limit_2p_1r_eval_t::call_t::raise_t::is_leaf);
	static_assert(!limit_2p_1r_eval_t::call_t::raise_t::can_raise);
	static_assert(limit_2p_1r_eval_t::call_t::raise_t::small_blind == 1);
	static_assert(limit_2p_1r_eval_t::call_t::raise_t::big_blind == 2);
	static_assert(limit_2p_1r_eval_t::call_t::raise_t::n_players == 2);
	static_assert(limit_2p_1r_eval_t::call_t::raise_t::n_active == 2);
	static_assert(limit_2p_1r_eval_t::call_t::raise_t::active_bm == 0x3);
	static_assert(limit_2p_1r_eval_t::call_t::raise_t::n_to_call == 1);
	static_assert(limit_2p_1r_eval_t::call_t::raise_t::player_no == 0);
	static_assert(limit_2p_1r_eval_t::call_t::raise_t::n_raises_left == 0);
	static_assert(limit_2p_1r_eval_t::call_t::raise_t::player_pots.pots[0] == 2);
	static_assert(limit_2p_1r_eval_t::call_t::raise_t::player_pots.pots[1] == 4);
	static_assert(limit_2p_1r_eval_t::call_t::raise_t::curr_max_bet == 4);
	static_assert(limit_2p_1r_eval_t::call_t::raise_t::total_pot == 6);
	
	// SB calls, BB raises, SB folds
	static_assert(limit_2p_1r_eval_t::call_t::raise_t::fold_t::is_leaf);
	static_assert(limit_2p_1r_eval_t::call_t::raise_t::fold_t::small_blind == 1);
	static_assert(limit_2p_1r_eval_t::call_t::raise_t::fold_t::big_blind == 2);
	static_assert(limit_2p_1r_eval_t::call_t::raise_t::fold_t::n_players == 2);
	static_assert(limit_2p_1r_eval_t::call_t::raise_t::fold_t::n_active == 1);
	static_assert(limit_2p_1r_eval_t::call_t::raise_t::fold_t::active_bm == 0x2);
	static_assert(limit_2p_1r_eval_t::call_t::raise_t::fold_t::n_to_call == 0);
	static_assert(limit_2p_1r_eval_t::call_t::raise_t::fold_t::player_no == 1);
	static_assert(limit_2p_1r_eval_t::call_t::raise_t::fold_t::n_raises_left == 0);
	static_assert(limit_2p_1r_eval_t::call_t::raise_t::fold_t::player_pots.pots[0] == 2);
	static_assert(limit_2p_1r_eval_t::call_t::raise_t::fold_t::player_pots.pots[1] == 4);
	static_assert(limit_2p_1r_eval_t::call_t::raise_t::fold_t::curr_max_bet == 4);
	static_assert(limit_2p_1r_eval_t::call_t::raise_t::fold_t::total_pot == 6);
	
	// SB calls, BB raises, SB calls
	static_assert(limit_2p_1r_eval_t::call_t::raise_t::call_t::is_leaf);
	static_assert(limit_2p_1r_eval_t::call_t::raise_t::call_t::small_blind == 1);
	static_assert(limit_2p_1r_eval_t::call_t::raise_t::call_t::big_blind == 2);
	static_assert(limit_2p_1r_eval_t::call_t::raise_t::call_t::n_players == 2);
	static_assert(limit_2p_1r_eval_t::call_t::raise_t::call_t::n_active == 2);
	static_assert(limit_2p_1r_eval_t::call_t::raise_t::call_t::active_bm == 0x3);
	static_assert(limit_2p_1r_eval_t::call_t::raise_t::call_t::n_to_call == 0);
	static_assert(limit_2p_1r_eval_t::call_t::raise_t::call_t::player_no == 1);
	static_assert(limit_2p_1r_eval_t::call_t::raise_t::call_t::n_raises_left == 0);
	static_assert(limit_2p_1r_eval_t::call_t::raise_t::call_t::player_pots.pots[0] == 4);
	static_assert(limit_2p_1r_eval_t::call_t::raise_t::call_t::player_pots.pots[1] == 4);
	static_assert(limit_2p_1r_eval_t::call_t::raise_t::call_t::curr_max_bet == 4);
	static_assert(limit_2p_1r_eval_t::call_t::raise_t::call_t::total_pot == 8);
	
	// SB raises
	static_assert(!limit_2p_1r_eval_t::raise_t::is_leaf);
	static_assert(!limit_2p_1r_eval_t::raise_t::can_raise);
	static_assert(limit_2p_1r_eval_t::raise_t::small_blind == 1);
	static_assert(limit_2p_1r_eval_t::raise_t::big_blind == 2);
	static_assert(limit_2p_1r_eval_t::raise_t::n_players == 2);
	static_assert(limit_2p_1r_eval_t::raise_t::n_active == 2);
	static_assert(limit_2p_1r_eval_t::raise_t::active_bm == 0x3);
	static_assert(limit_2p_1r_eval_t::raise_t::n_to_call == 1);
	static_assert(limit_2p_1r_eval_t::raise_t::player_no == 1); // BB to bet
	static_assert(limit_2p_1r_eval_t::raise_t::n_raises_left == 0);
	static_assert(limit_2p_1r_eval_t::raise_t::player_pots.pots[0] == 4);
	static_assert(limit_2p_1r_eval_t::raise_t::player_pots.pots[1] == 2);
	static_assert(limit_2p_1r_eval_t::raise_t::curr_max_bet == 4);
	static_assert(limit_2p_1r_eval_t::raise_t::total_pot == 6);
	
	// SB raises, BB folds
	static_assert(limit_2p_1r_eval_t::raise_t::fold_t::is_leaf);
	static_assert(limit_2p_1r_eval_t::raise_t::fold_t::small_blind == 1);
	static_assert(limit_2p_1r_eval_t::raise_t::fold_t::big_blind == 2);
	static_assert(limit_2p_1r_eval_t::raise_t::fold_t::n_players == 2);
	static_assert(limit_2p_1r_eval_t::raise_t::fold_t::n_active == 1);
	static_assert(limit_2p_1r_eval_t::raise_t::fold_t::active_bm == 0x1);
	static_assert(limit_2p_1r_eval_t::raise_t::fold_t::n_to_call == 0);
	static_assert(limit_2p_1r_eval_t::raise_t::fold_t::player_no == 0);
	static_assert(limit_2p_1r_eval_t::raise_t::fold_t::n_raises_left == 0);
	static_assert(limit_2p_1r_eval_t::raise_t::fold_t::player_pots.pots[0] == 4);
	static_assert(limit_2p_1r_eval_t::raise_t::fold_t::player_pots.pots[1] == 2);
	static_assert(limit_2p_1r_eval_t::raise_t::fold_t::curr_max_bet == 4);
	static_assert(limit_2p_1r_eval_t::raise_t::fold_t::total_pot == 6);
	
	// SB raises, BB calls
	static_assert(limit_2p_1r_eval_t::raise_t::call_t::is_leaf);
	static_assert(limit_2p_1r_eval_t::raise_t::call_t::small_blind == 1);
	static_assert(limit_2p_1r_eval_t::raise_t::call_t::big_blind == 2);
	static_assert(limit_2p_1r_eval_t::raise_t::call_t::n_players == 2);
	static_assert(limit_2p_1r_eval_t::raise_t::call_t::n_active == 2);
	static_assert(limit_2p_1r_eval_t::raise_t::call_t::active_bm == 0x3);
	static_assert(limit_2p_1r_eval_t::raise_t::call_t::n_to_call == 0);
	static_assert(limit_2p_1r_eval_t::raise_t::call_t::player_no == 0);
	static_assert(limit_2p_1r_eval_t::raise_t::call_t::n_raises_left == 0);
	static_assert(limit_2p_1r_eval_t::raise_t::call_t::player_pots.pots[0] == 4);
	static_assert(limit_2p_1r_eval_t::raise_t::call_t::player_pots.pots[1] == 4);
	static_assert(limit_2p_1r_eval_t::raise_t::call_t::curr_max_bet == 4);
	static_assert(limit_2p_1r_eval_t::raise_t::call_t::total_pot == 8);
	
      } // namespace TwoPlayerOneRaise
      
      namespace ThreePlayerZeroRaises {
	
	typedef LimitRootHandEval</*N_PLAYERS*/3, /*N_RAISES*/0>::type_t limit_3p_0r_eval_t;
	
	// Root node
	static_assert(!limit_3p_0r_eval_t::is_leaf);
	static_assert(!limit_3p_0r_eval_t::can_raise);
	static_assert(limit_3p_0r_eval_t::small_blind == 1);
	static_assert(limit_3p_0r_eval_t::big_blind == 2);
	static_assert(limit_3p_0r_eval_t::n_players == 3);
	static_assert(limit_3p_0r_eval_t::n_active == 3);
	static_assert(limit_3p_0r_eval_t::active_bm == 0x7);
	static_assert(limit_3p_0r_eval_t::n_to_call == 3);
	static_assert(limit_3p_0r_eval_t::player_no == 2); // UTG to bet
	static_assert(limit_3p_0r_eval_t::n_raises_left == 0);
	static_assert(limit_3p_0r_eval_t::player_pots.pots[0] == 1);
	static_assert(limit_3p_0r_eval_t::player_pots.pots[1] == 2);
	static_assert(limit_3p_0r_eval_t::player_pots.pots[2] == 0);
	static_assert(limit_3p_0r_eval_t::curr_max_bet == 2);
	static_assert(limit_3p_0r_eval_t::total_pot == 3);
	
	// UTG folds
	static_assert(!limit_3p_0r_eval_t::fold_t::is_leaf); // SB and BB left
	static_assert(!limit_3p_0r_eval_t::fold_t::can_raise);
	static_assert(limit_3p_0r_eval_t::fold_t::small_blind == 1);
	static_assert(limit_3p_0r_eval_t::fold_t::big_blind == 2);
	static_assert(limit_3p_0r_eval_t::fold_t::n_players == 3);
	static_assert(limit_3p_0r_eval_t::fold_t::n_active == 2);
	static_assert(limit_3p_0r_eval_t::fold_t::active_bm == 0x3);
	static_assert(limit_3p_0r_eval_t::fold_t::n_to_call == 2);
	static_assert(limit_3p_0r_eval_t::fold_t::player_no == 0); // SB to bet
	static_assert(limit_3p_0r_eval_t::fold_t::n_raises_left == 0);
	static_assert(limit_3p_0r_eval_t::fold_t::player_pots.pots[0] == 1);
	static_assert(limit_3p_0r_eval_t::fold_t::player_pots.pots[1] == 2);
	static_assert(limit_3p_0r_eval_t::fold_t::player_pots.pots[2] == 0);
	static_assert(limit_3p_0r_eval_t::fold_t::curr_max_bet == 2);
	static_assert(limit_3p_0r_eval_t::fold_t::total_pot == 3);

	// UTG folds, SB folds
	static_assert(limit_3p_0r_eval_t::fold_t::fold_t::is_leaf); // BB left
	static_assert(limit_3p_0r_eval_t::fold_t::fold_t::small_blind == 1);
	static_assert(limit_3p_0r_eval_t::fold_t::fold_t::big_blind == 2);
	static_assert(limit_3p_0r_eval_t::fold_t::fold_t::n_players == 3);
	static_assert(limit_3p_0r_eval_t::fold_t::fold_t::n_active == 1);
	static_assert(limit_3p_0r_eval_t::fold_t::fold_t::active_bm == 0x2);
	static_assert(limit_3p_0r_eval_t::fold_t::fold_t::n_to_call == 1);
	static_assert(limit_3p_0r_eval_t::fold_t::fold_t::player_no == 1); // BB to bet
	static_assert(limit_3p_0r_eval_t::fold_t::fold_t::n_raises_left == 0);
	static_assert(limit_3p_0r_eval_t::fold_t::fold_t::player_pots.pots[0] == 1);
	static_assert(limit_3p_0r_eval_t::fold_t::fold_t::player_pots.pots[1] == 2);
	static_assert(limit_3p_0r_eval_t::fold_t::fold_t::player_pots.pots[2] == 0);
	static_assert(limit_3p_0r_eval_t::fold_t::fold_t::curr_max_bet == 2);
	static_assert(limit_3p_0r_eval_t::fold_t::fold_t::total_pot == 3);

	// UTG folds, SB calls
	static_assert(!limit_3p_0r_eval_t::fold_t::call_t::is_leaf); // SB, BB left
	static_assert(!limit_3p_0r_eval_t::fold_t::call_t::can_raise);
	static_assert(limit_3p_0r_eval_t::fold_t::call_t::small_blind == 1);
	static_assert(limit_3p_0r_eval_t::fold_t::call_t::big_blind == 2);
	static_assert(limit_3p_0r_eval_t::fold_t::call_t::n_players == 3);
	static_assert(limit_3p_0r_eval_t::fold_t::call_t::n_active == 2);
	static_assert(limit_3p_0r_eval_t::fold_t::call_t::active_bm == 0x3);
	static_assert(limit_3p_0r_eval_t::fold_t::call_t::n_to_call == 1);
	static_assert(limit_3p_0r_eval_t::fold_t::call_t::player_no == 1); // BB to bet
	static_assert(limit_3p_0r_eval_t::fold_t::call_t::n_raises_left == 0);
	static_assert(limit_3p_0r_eval_t::fold_t::call_t::player_pots.pots[0] == 2);
	static_assert(limit_3p_0r_eval_t::fold_t::call_t::player_pots.pots[1] == 2);
	static_assert(limit_3p_0r_eval_t::fold_t::call_t::player_pots.pots[2] == 0);
	static_assert(limit_3p_0r_eval_t::fold_t::call_t::curr_max_bet == 2);
	static_assert(limit_3p_0r_eval_t::fold_t::call_t::total_pot == 4);

	// UTG folds, SB calls, BB calls
	static_assert(limit_3p_0r_eval_t::fold_t::call_t::call_t::is_leaf);
	static_assert(limit_3p_0r_eval_t::fold_t::call_t::call_t::small_blind == 1);
	static_assert(limit_3p_0r_eval_t::fold_t::call_t::call_t::big_blind == 2);
	static_assert(limit_3p_0r_eval_t::fold_t::call_t::call_t::n_players == 3);
	static_assert(limit_3p_0r_eval_t::fold_t::call_t::call_t::n_active == 2);
	static_assert(limit_3p_0r_eval_t::fold_t::call_t::call_t::active_bm == 0x3);
	static_assert(limit_3p_0r_eval_t::fold_t::call_t::call_t::n_to_call == 0);
	static_assert(limit_3p_0r_eval_t::fold_t::call_t::call_t::player_no == 2);
	static_assert(limit_3p_0r_eval_t::fold_t::call_t::call_t::n_raises_left == 0);
	static_assert(limit_3p_0r_eval_t::fold_t::call_t::call_t::player_pots.pots[0] == 2);
	static_assert(limit_3p_0r_eval_t::fold_t::call_t::call_t::player_pots.pots[1] == 2);
	static_assert(limit_3p_0r_eval_t::fold_t::call_t::call_t::player_pots.pots[2] == 0);
	static_assert(limit_3p_0r_eval_t::fold_t::call_t::call_t::curr_max_bet == 2);
	static_assert(limit_3p_0r_eval_t::fold_t::call_t::call_t::total_pot == 4);
	
	// UTG calls
	static_assert(!limit_3p_0r_eval_t::call_t::is_leaf); // all 3 active
	static_assert(!limit_3p_0r_eval_t::call_t::can_raise);
	static_assert(limit_3p_0r_eval_t::call_t::small_blind == 1);
	static_assert(limit_3p_0r_eval_t::call_t::big_blind == 2);
	static_assert(limit_3p_0r_eval_t::call_t::n_players == 3);
	static_assert(limit_3p_0r_eval_t::call_t::n_active == 3);
	static_assert(limit_3p_0r_eval_t::call_t::active_bm == 0x7);
	static_assert(limit_3p_0r_eval_t::call_t::n_to_call == 2);
	static_assert(limit_3p_0r_eval_t::call_t::player_no == 0); // SB to bet
	static_assert(limit_3p_0r_eval_t::call_t::n_raises_left == 0);
	static_assert(limit_3p_0r_eval_t::call_t::player_pots.pots[0] == 1);
	static_assert(limit_3p_0r_eval_t::call_t::player_pots.pots[1] == 2);
	static_assert(limit_3p_0r_eval_t::call_t::player_pots.pots[2] == 2);
	static_assert(limit_3p_0r_eval_t::call_t::curr_max_bet == 2);
	static_assert(limit_3p_0r_eval_t::call_t::total_pot == 5);
	
	// UTG calls, SB folds
	static_assert(!limit_3p_0r_eval_t::call_t::fold_t::is_leaf); // UTG, BB active
	static_assert(!limit_3p_0r_eval_t::call_t::fold_t::can_raise);
	static_assert(limit_3p_0r_eval_t::call_t::fold_t::small_blind == 1);
	static_assert(limit_3p_0r_eval_t::call_t::fold_t::big_blind == 2);
	static_assert(limit_3p_0r_eval_t::call_t::fold_t::n_players == 3);
	static_assert(limit_3p_0r_eval_t::call_t::fold_t::n_active == 2);
	static_assert(limit_3p_0r_eval_t::call_t::fold_t::active_bm == 0x6);
	static_assert(limit_3p_0r_eval_t::call_t::fold_t::n_to_call == 1);
	static_assert(limit_3p_0r_eval_t::call_t::fold_t::player_no == 1); // BB to bet
	static_assert(limit_3p_0r_eval_t::call_t::fold_t::n_raises_left == 0);
	static_assert(limit_3p_0r_eval_t::call_t::fold_t::player_pots.pots[0] == 1);
	static_assert(limit_3p_0r_eval_t::call_t::fold_t::player_pots.pots[1] == 2);
	static_assert(limit_3p_0r_eval_t::call_t::fold_t::player_pots.pots[2] == 2);
	static_assert(limit_3p_0r_eval_t::call_t::fold_t::curr_max_bet == 2);
	static_assert(limit_3p_0r_eval_t::call_t::fold_t::total_pot == 5);
	
	// UTG calls, SB folds, BB folds
	static_assert(limit_3p_0r_eval_t::call_t::fold_t::fold_t::is_leaf); // only UTG active
	static_assert(limit_3p_0r_eval_t::call_t::fold_t::fold_t::small_blind == 1);
	static_assert(limit_3p_0r_eval_t::call_t::fold_t::fold_t::big_blind == 2);
	static_assert(limit_3p_0r_eval_t::call_t::fold_t::fold_t::n_players == 3);
	static_assert(limit_3p_0r_eval_t::call_t::fold_t::fold_t::n_active == 1);
	static_assert(limit_3p_0r_eval_t::call_t::fold_t::fold_t::active_bm == 0x4);
	static_assert(limit_3p_0r_eval_t::call_t::fold_t::fold_t::n_to_call == 0);
	static_assert(limit_3p_0r_eval_t::call_t::fold_t::fold_t::player_no == 2); // UTG to bet
	static_assert(limit_3p_0r_eval_t::call_t::fold_t::fold_t::n_raises_left == 0);
	static_assert(limit_3p_0r_eval_t::call_t::fold_t::fold_t::player_pots.pots[0] == 1);
	static_assert(limit_3p_0r_eval_t::call_t::fold_t::fold_t::player_pots.pots[1] == 2);
	static_assert(limit_3p_0r_eval_t::call_t::fold_t::fold_t::player_pots.pots[2] == 2);
	static_assert(limit_3p_0r_eval_t::call_t::fold_t::fold_t::curr_max_bet == 2);
	static_assert(limit_3p_0r_eval_t::call_t::fold_t::fold_t::total_pot == 5);
	
      } // namespace ThreePlayerZeroRaises
      
      namespace FourPlayerTwoRaises {
	
	typedef LimitRootHandEval</*N_PLAYERS*/4, /*N_RAISES*/2>::type_t limit_4p_2r_eval_t;
	
	// Root node
	static_assert(!limit_4p_2r_eval_t::is_leaf);
	static_assert(limit_4p_2r_eval_t::can_raise);
	static_assert(limit_4p_2r_eval_t::small_blind == 1);
	static_assert(limit_4p_2r_eval_t::big_blind == 2);
	static_assert(limit_4p_2r_eval_t::n_players == 4);
	static_assert(limit_4p_2r_eval_t::n_active == 4);
	static_assert(limit_4p_2r_eval_t::active_bm == 0xf);
	static_assert(limit_4p_2r_eval_t::n_to_call == 4);
	static_assert(limit_4p_2r_eval_t::player_no == 2); // UTG to bet
	static_assert(limit_4p_2r_eval_t::n_raises_left == 2);
	static_assert(limit_4p_2r_eval_t::player_pots.pots[0] == 1);
	static_assert(limit_4p_2r_eval_t::player_pots.pots[1] == 2);
	static_assert(limit_4p_2r_eval_t::player_pots.pots[2] == 0);
	static_assert(limit_4p_2r_eval_t::player_pots.pots[3] == 0);
	static_assert(limit_4p_2r_eval_t::curr_max_bet == 2);
	static_assert(limit_4p_2r_eval_t::total_pot == 3);

	// UTG folds, P3 calls, SB calls, BB raises
	static_assert(!limit_4p_2r_eval_t::fold_t::call_t::call_t::raise_t::is_leaf);
	static_assert(limit_4p_2r_eval_t::fold_t::call_t::call_t::raise_t::small_blind == 1);
	static_assert(limit_4p_2r_eval_t::fold_t::call_t::call_t::raise_t::big_blind == 2);
	static_assert(limit_4p_2r_eval_t::fold_t::call_t::call_t::raise_t::n_players == 4);
	static_assert(limit_4p_2r_eval_t::fold_t::call_t::call_t::raise_t::n_active == 3);
	static_assert(limit_4p_2r_eval_t::fold_t::call_t::call_t::raise_t::active_bm == 0xb);
	static_assert(limit_4p_2r_eval_t::fold_t::call_t::call_t::raise_t::n_to_call == 2);
	static_assert(limit_4p_2r_eval_t::fold_t::call_t::call_t::raise_t::player_no == 2); // UTG to bet
	static_assert(limit_4p_2r_eval_t::fold_t::call_t::call_t::raise_t::n_raises_left == 1);
	static_assert(limit_4p_2r_eval_t::fold_t::call_t::call_t::raise_t::player_pots.pots[0] == 2);
	static_assert(limit_4p_2r_eval_t::fold_t::call_t::call_t::raise_t::player_pots.pots[1] == 4);
	static_assert(limit_4p_2r_eval_t::fold_t::call_t::call_t::raise_t::player_pots.pots[2] == 0);
	static_assert(limit_4p_2r_eval_t::fold_t::call_t::call_t::raise_t::player_pots.pots[3] == 2);
	static_assert(limit_4p_2r_eval_t::fold_t::call_t::call_t::raise_t::curr_max_bet == 4);
	static_assert(limit_4p_2r_eval_t::fold_t::call_t::call_t::raise_t::total_pot == 8);

	// UTG folds, P3 calls, SB calls, BB raises, UTG is dead
	static_assert(!limit_4p_2r_eval_t::fold_t::call_t::call_t::raise_t::dead_t::is_leaf);
	static_assert(limit_4p_2r_eval_t::fold_t::call_t::call_t::raise_t::dead_t::small_blind == 1);
	static_assert(limit_4p_2r_eval_t::fold_t::call_t::call_t::raise_t::dead_t::big_blind == 2);
	static_assert(limit_4p_2r_eval_t::fold_t::call_t::call_t::raise_t::dead_t::n_players == 4);
	static_assert(limit_4p_2r_eval_t::fold_t::call_t::call_t::raise_t::dead_t::n_active == 3);
	static_assert(limit_4p_2r_eval_t::fold_t::call_t::call_t::raise_t::dead_t::active_bm == 0xb);
	static_assert(limit_4p_2r_eval_t::fold_t::call_t::call_t::raise_t::dead_t::n_to_call == 2);
	static_assert(limit_4p_2r_eval_t::fold_t::call_t::call_t::raise_t::dead_t::player_no == 3);
	static_assert(limit_4p_2r_eval_t::fold_t::call_t::call_t::raise_t::dead_t::n_raises_left == 1);
	static_assert(limit_4p_2r_eval_t::fold_t::call_t::call_t::raise_t::dead_t::player_pots.pots[0] == 2);
	static_assert(limit_4p_2r_eval_t::fold_t::call_t::call_t::raise_t::dead_t::player_pots.pots[1] == 4);
	static_assert(limit_4p_2r_eval_t::fold_t::call_t::call_t::raise_t::dead_t::player_pots.pots[2] == 0);
	static_assert(limit_4p_2r_eval_t::fold_t::call_t::call_t::raise_t::dead_t::player_pots.pots[3] == 2);
	static_assert(limit_4p_2r_eval_t::fold_t::call_t::call_t::raise_t::dead_t::curr_max_bet == 4);
	static_assert(limit_4p_2r_eval_t::fold_t::call_t::call_t::raise_t::dead_t::total_pot == 8);

	// UTG folds, P3 calls, SB calls, BB raises, UTG is dead, P3 raises, SB folds
	static_assert(!limit_4p_2r_eval_t::fold_t::call_t::call_t::raise_t::dead_t::raise_t::fold_t::is_leaf);
	static_assert(!limit_4p_2r_eval_t::fold_t::call_t::call_t::raise_t::dead_t::raise_t::fold_t::can_raise);
	static_assert(limit_4p_2r_eval_t::fold_t::call_t::call_t::raise_t::dead_t::raise_t::fold_t::small_blind == 1);
	static_assert(limit_4p_2r_eval_t::fold_t::call_t::call_t::raise_t::dead_t::raise_t::fold_t::big_blind == 2);
	static_assert(limit_4p_2r_eval_t::fold_t::call_t::call_t::raise_t::dead_t::raise_t::fold_t::n_players == 4);
	static_assert(limit_4p_2r_eval_t::fold_t::call_t::call_t::raise_t::dead_t::raise_t::fold_t::n_active == 2);
	static_assert(limit_4p_2r_eval_t::fold_t::call_t::call_t::raise_t::dead_t::raise_t::fold_t::active_bm == 0xa);
	static_assert(limit_4p_2r_eval_t::fold_t::call_t::call_t::raise_t::dead_t::raise_t::fold_t::n_to_call == 1);
	static_assert(limit_4p_2r_eval_t::fold_t::call_t::call_t::raise_t::dead_t::raise_t::fold_t::player_no == 1);
	static_assert(limit_4p_2r_eval_t::fold_t::call_t::call_t::raise_t::dead_t::raise_t::fold_t::n_raises_left == 0);
	static_assert(limit_4p_2r_eval_t::fold_t::call_t::call_t::raise_t::dead_t::raise_t::fold_t::player_pots.pots[0] == 2);
	static_assert(limit_4p_2r_eval_t::fold_t::call_t::call_t::raise_t::dead_t::raise_t::fold_t::player_pots.pots[1] == 4);
	static_assert(limit_4p_2r_eval_t::fold_t::call_t::call_t::raise_t::dead_t::raise_t::fold_t::player_pots.pots[2] == 0);
	static_assert(limit_4p_2r_eval_t::fold_t::call_t::call_t::raise_t::dead_t::raise_t::fold_t::player_pots.pots[3] == 6);
	static_assert(limit_4p_2r_eval_t::fold_t::call_t::call_t::raise_t::dead_t::raise_t::fold_t::curr_max_bet == 6);
	static_assert(limit_4p_2r_eval_t::fold_t::call_t::call_t::raise_t::dead_t::raise_t::fold_t::total_pot == 12);
	
	// UTG folds, P3 calls, SB calls, BB raises, UTG is dead, P3 raises, SB folds, BB folds
	static_assert(limit_4p_2r_eval_t::fold_t::call_t::call_t::raise_t::dead_t::raise_t::fold_t::fold_t::is_leaf);
	static_assert(limit_4p_2r_eval_t::fold_t::call_t::call_t::raise_t::dead_t::raise_t::fold_t::fold_t::small_blind == 1);
	static_assert(limit_4p_2r_eval_t::fold_t::call_t::call_t::raise_t::dead_t::raise_t::fold_t::fold_t::big_blind == 2);
	static_assert(limit_4p_2r_eval_t::fold_t::call_t::call_t::raise_t::dead_t::raise_t::fold_t::fold_t::n_players == 4);
	static_assert(limit_4p_2r_eval_t::fold_t::call_t::call_t::raise_t::dead_t::raise_t::fold_t::fold_t::n_active == 1);
	static_assert(limit_4p_2r_eval_t::fold_t::call_t::call_t::raise_t::dead_t::raise_t::fold_t::fold_t::active_bm == 0x8);
	static_assert(limit_4p_2r_eval_t::fold_t::call_t::call_t::raise_t::dead_t::raise_t::fold_t::fold_t::n_to_call == 0);
	static_assert(limit_4p_2r_eval_t::fold_t::call_t::call_t::raise_t::dead_t::raise_t::fold_t::fold_t::player_no == 2);
	static_assert(limit_4p_2r_eval_t::fold_t::call_t::call_t::raise_t::dead_t::raise_t::fold_t::fold_t::n_raises_left == 0);
	static_assert(limit_4p_2r_eval_t::fold_t::call_t::call_t::raise_t::dead_t::raise_t::fold_t::fold_t::player_pots.pots[0] == 2);
	static_assert(limit_4p_2r_eval_t::fold_t::call_t::call_t::raise_t::dead_t::raise_t::fold_t::fold_t::player_pots.pots[1] == 4);
	static_assert(limit_4p_2r_eval_t::fold_t::call_t::call_t::raise_t::dead_t::raise_t::fold_t::fold_t::player_pots.pots[2] == 0);
	static_assert(limit_4p_2r_eval_t::fold_t::call_t::call_t::raise_t::dead_t::raise_t::fold_t::fold_t::player_pots.pots[3] == 6);
	static_assert(limit_4p_2r_eval_t::fold_t::call_t::call_t::raise_t::dead_t::raise_t::fold_t::fold_t::curr_max_bet == 6);
	static_assert(limit_4p_2r_eval_t::fold_t::call_t::call_t::raise_t::dead_t::raise_t::fold_t::fold_t::total_pot == 12);
	
	// UTG folds, P3 calls, SB calls, BB raises, UTG is dead, P3 raises, SB folds, BB calls
	static_assert(limit_4p_2r_eval_t::fold_t::call_t::call_t::raise_t::dead_t::raise_t::fold_t::call_t::is_leaf);
	static_assert(limit_4p_2r_eval_t::fold_t::call_t::call_t::raise_t::dead_t::raise_t::fold_t::call_t::small_blind == 1);
	static_assert(limit_4p_2r_eval_t::fold_t::call_t::call_t::raise_t::dead_t::raise_t::fold_t::call_t::big_blind == 2);
	static_assert(limit_4p_2r_eval_t::fold_t::call_t::call_t::raise_t::dead_t::raise_t::fold_t::call_t::n_players == 4);
	static_assert(limit_4p_2r_eval_t::fold_t::call_t::call_t::raise_t::dead_t::raise_t::fold_t::call_t::n_active == 2);
	static_assert(limit_4p_2r_eval_t::fold_t::call_t::call_t::raise_t::dead_t::raise_t::fold_t::call_t::active_bm == 0xa);
	static_assert(limit_4p_2r_eval_t::fold_t::call_t::call_t::raise_t::dead_t::raise_t::fold_t::call_t::n_to_call == 0);
	static_assert(limit_4p_2r_eval_t::fold_t::call_t::call_t::raise_t::dead_t::raise_t::fold_t::call_t::player_no == 2);
	static_assert(limit_4p_2r_eval_t::fold_t::call_t::call_t::raise_t::dead_t::raise_t::fold_t::call_t::n_raises_left == 0);
	static_assert(limit_4p_2r_eval_t::fold_t::call_t::call_t::raise_t::dead_t::raise_t::fold_t::call_t::player_pots.pots[0] == 2);
	static_assert(limit_4p_2r_eval_t::fold_t::call_t::call_t::raise_t::dead_t::raise_t::fold_t::call_t::player_pots.pots[1] == 6);
	static_assert(limit_4p_2r_eval_t::fold_t::call_t::call_t::raise_t::dead_t::raise_t::fold_t::call_t::player_pots.pots[2] == 0);
	static_assert(limit_4p_2r_eval_t::fold_t::call_t::call_t::raise_t::dead_t::raise_t::fold_t::call_t::player_pots.pots[3] == 6);
	static_assert(limit_4p_2r_eval_t::fold_t::call_t::call_t::raise_t::dead_t::raise_t::fold_t::call_t::curr_max_bet == 6);
	static_assert(limit_4p_2r_eval_t::fold_t::call_t::call_t::raise_t::dead_t::raise_t::fold_t::call_t::total_pot == 14);
	
      } // namespace FourPlayerTwoRaises
      
    } // namespace StaticAsserts
    
  } // namespace Gto
  
} // namespace Poker

#endif //def GTO_EVAL
