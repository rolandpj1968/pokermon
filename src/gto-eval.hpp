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

      template <typename PerPlayerStrategyListT, typename PerPlayerHandListT>
      inline NodeEvalPerPlayerProfit<N_PLAYERS> evaluate(double node_prob, const PerPlayerStrategyListT& playerStrategies, const PerPlayerHandListT& playerHands) {
	NodeEvalPerPlayerProfit<N_PLAYERS> player_profits = {};
	
	auto currPlayerStrategy = PerPlayerStrategyListT::get<PLAYER_NO>(playerStrategies);

	auto foldStrategies = playerStrategies.fold();
	double fold_p = currPlayerStrategy.fold_p;
	NodeEvalPerPlayerProfit<N_PLAYERS> fold_profits = this.fold.evaluate(node_prob*fold_p, foldStrategies, playerHands);
	player_profits.accumulate(fold_p, fold_profits);

	auto callStrategies = playerStrategies.call();
	double call_p = currPlayerStrategy.call_p;
	NodeEvalPerPlayerProfit<N_PLAYERS> call_profits = this.call.evaluate(node_prob*call_p, callStrategies, playerHands);
	player_profits.accumulate(call_p, call_profits);

	auto raiseStrategies = playerStrategies.raise();
	double raise_p = currPlayerStrategy.raise_p;
	NodeEvalPerPlayerProfit<N_PLAYERS> raise_profits = this.raise.evaluate(node_prob*raise_p, raiseStrategies, playerHands);
	player_profits.accumulate(raise_p, raise_profits);

	eval.accumulate(node_prob, player_profits);

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

      template <typename PerPlayerStrategyListT, typename PerPlayerHandListT>
      inline NodeEvalPerPlayerProfit<N_PLAYERS> evaluate(double node_prob, const PerPlayerStrategyListT& playerStrategies, const PerPlayerHandListT& playerHands) {
	// The single player remaining takes the pot.
	NodeEvalPerPlayerProfit<N_PLAYERS> player_profits = make_player_profits_for_one_winner<N_PLAYERS>(ACTIVE_BM, PLAYER_POTS);
	
	eval.accumulate(node_prob, player_profits);

	return player_profits;
      }
      
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
    struct LimitHandEvalSpecialised<SMALL_BLIND, BIG_BLIND, N_PLAYERS, ACTIVE_BM, N_TO_CALL, PLAYER_NO, N_RAISES_LEFT, PLAYER_POTS, ShowdownNodeType>
      : LimitHandNodeConsts<SMALL_BLIND, BIG_BLIND, N_PLAYERS, ACTIVE_BM, N_TO_CALL, PLAYER_NO, N_RAISES_LEFT, PLAYER_POTS>
    {
      static const bool is_leaf = true;
      
      NodeEval<N_PLAYERS> eval;

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

      template <typename PerPlayerStrategyListT, typename PerPlayerHandListT>
      inline NodeEvalPerPlayerProfit<N_PLAYERS> evaluate(double node_prob, const PerPlayerStrategyListT& playerStrategies, const PerPlayerHandListT& playerHands) {
	// Skip this node - it's just a dummy.
	auto deadStrategies = playerStrategies.dead();
	return this._.evaluate(node_prob, deadStrategies, playerHands);
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

      template <typename PerPlayerStrategyListT, typename PerPlayerHandListT>
      inline NodeEvalPerPlayerProfit<N_PLAYERS> evaluate(double node_prob, const PerPlayerStrategyListT& playerStrategies, const PerPlayerHandListT& playerHands) {
	NodeEvalPerPlayerProfit<N_PLAYERS> player_profits = {};
	
	auto currPlayerStrategy = PerPlayerStrategyListT::get<PLAYER_NO>(playerStrategies);

	auto foldStrategies = playerStrategies.fold();
	double fold_p = currPlayerStrategy.fold_p;
	NodeEvalPerPlayerProfit<N_PLAYERS> fold_profits = this.fold.evaluate(node_prob*fold_p, foldStrategies, playerHands);
	player_profits.accumulate(fold_p, fold_profits);

	auto callStrategies = playerStrategies.call();
	double call_p = currPlayerStrategy.call_p;
	NodeEvalPerPlayerProfit<N_PLAYERS> call_profits = this.call.evaluate(node_prob*call_p, callStrategies, playerHands);
	player_profits.accumulate(call_p, call_profits);

	eval.accumulate(node_prob, player_profits);

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
