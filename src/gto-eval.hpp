#ifndef GTO_EVAL
#define GTO_EVAL

#include "types.hpp"

namespace Poker {
  namespace Gto {
    template <int N_PLAYERS>
    struct NodeEvalPerPlayerProfit {
      double profits[N_PLAYERS];
    };
    
    template <int N_PLAYERS>
    struct NodeEval {
      // Sum over all hands evaluated of per-hand 'probability of reaching this node'.
      double activity;
      // Sum over all hands of per-hand (product of) 'probability of reaching this node' times 'player outcome for the hand'.
      NodeEvalPerPlayerProfit<N_PLAYERS> player_profits;
    };
    
    template <int N_PLAYERS>
    struct PlayerPots {
      int pots[N_PLAYERS];
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
    
    template <int N_PLAYERS>
    constexpr inline PlayerPots<N_PLAYERS> make_player_pots(u64 player_pots_u64) {
      PlayerPots<N_PLAYERS> player_pots();
      
      for(int n = 0; n < N_PLAYERS; n++) {
	player_pots[n] = get_player_pot(n, player_pots_u64);
      }
      
      return player_pots;
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
    
    constexpr inline bool is_active(int player_no, u8 active_bm) {
      return (active_bm & active_bm_u8_mask(player_no)) != 0;
    }
    
    constexpr inline u8 remove_player_from_active_bm(int player_no, u8 active_bm) {
      return active_bm & ~active_bm_u8_mask(player_no);
    }
    
    template <
      // Small blind
      int SMALL_BLIND,
      // Big blind - also raise amount
      int BIG_BLIND,
      // How many players are playing
      int N_PLAYERS,
      // How many players are still active - i.e. have _not_ folded.
      // When this reaches 1 then that player takes the pot cos
      //   everyone else has folded.
      int N_ACTIVE,
      // Bitmap of active players.
      u8 ACTIVE_BM,
      // Is the current player active?
      bool IS_ACTIVE,
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
      // The current highest bet - this increases at every raise.
      int CURR_MAX_BET,
      // Current total pot
      int TOTAL_POT
      >
    struct LimitHandEvalConsts {
      static constexpr int small_blind = SMALL_BLIND;
      static constexpr int big_blind = BIG_BLIND;
      static constexpr int n_players = N_PLAYERS;
      static constexpr int n_active = N_ACTIVE;
      static constexpr u8 active_bm = ACTIVE_BM;
      static constexpr bool is_active = IS_ACTIVE;
      static constexpr int n_to_call = N_TO_CALL;
      static constexpr int player_no = PLAYER_NO;
      static constexpr int n_raises_left = N_RAISES_LEFT;
      static constexpr PlayerPots<N_PLAYERS> player_pots = make_player_pots<N_PLAYERS>(PLAYER_POTS);
      static constexpr int curr_max_bet = CURR_MAX_BET;
      static constexpr int total_pot = TOTAL_POT;
    };
    
    // Declaration of LimitHandEval.
    template <
      // Small blind
      int SMALL_BLIND,
      // Big blind - also raise amount
      int BIG_BLIND,
      // How many players are playing
      int N_PLAYERS,
      // How many players are still active - i.e. have _not_ folded.
      // When this reaches 1 then that player takes the pot cos
      //   everyone else has folded.
      int N_ACTIVE,
      // Bitmap of active players.
      u8 ACTIVE_BM,
      // Is the current player active?
      bool IS_ACTIVE,
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
      // The current highest bet - this increases at every raise.
      int CURR_MAX_BET,
      // Current total pot
      int TOTAL_POT
      >
    struct LimitHandEval;

    enum LimitHandEvalNodeType {
      FoldCallRaiseNodeType,
      FoldCallNodeType,
      AllButOneFoldNodeType,
      ShowdownNodeType,
      AlreadyFoldedNodeType
    };

    constexpr inline LimitHandEvalNodeType get_node_type(int n_active, int n_to_call, bool is_active, int n_raises_left) {
      // Highest priority specialisation - only one player left
      if(n_active == 1) {
	return AllButOneFoldNodeType;
      }
      // 2nd highest priority specialisation - multiple players still in and all called
      if(n_to_call == 0) {
	return ShowdownNodeType;
      }
      // Has the current player already folded - if so this is a dummy node
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
    
    // Declaration of LimitHandEvalSpecialised.
    template <
      // Small blind
      int SMALL_BLIND,
      // Big blind - also raise amount
      int BIG_BLIND,
      // How many players are playing
      int N_PLAYERS,
      // How many players are still active - i.e. have _not_ folded.
      // When this reaches 1 then that player takes the pot cos
      //   everyone else has folded.
      int N_ACTIVE,
      // Bitmap of active players.
      u8 ACTIVE_BM,
      // Is the current player active?
      bool IS_ACTIVE,
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
      // The current highest bet - this increases at every raise.
      int CURR_MAX_BET,
      // Current total pot
      int TOTAL_POT,
      // Specialised node type
      LimitHandEvalNodeType NODE_TYPE
      >
    struct LimitHandEvalSpecialised;

    // Definition of LimitHandEval as one of four specialisations
    template <
      int SMALL_BLIND,
      int BIG_BLIND,
      int N_PLAYERS,
      int N_ACTIVE,
      u8 ACTIVE_BM,
      bool IS_ACTIVE,
      int N_TO_CALL,
      int PLAYER_NO,
      int N_RAISES_LEFT,
      u64 PLAYER_POTS,
      int CURR_MAX_BET,
      int TOTAL_POT
      >
    struct LimitHandEval :
      LimitHandEvalSpecialised<
      SMALL_BLIND,
      BIG_BLIND,
      N_PLAYERS,
      N_ACTIVE,
      ACTIVE_BM,
      IS_ACTIVE,
      N_TO_CALL,
      PLAYER_NO,
      N_RAISES_LEFT,
      PLAYER_POTS,
      CURR_MAX_BET,
      TOTAL_POT,
      get_node_type(N_ACTIVE, N_TO_CALL, IS_ACTIVE, N_RAISES_LEFT)
      >
    {};
    
    // Child node of this eval tree node for current player folding
    template <
      int SMALL_BLIND,
      int BIG_BLIND,
      int N_PLAYERS,
      int N_ACTIVE,
      u8 ACTIVE_BM,
      bool IS_ACTIVE,
      int N_TO_CALL,
      int PLAYER_NO,
      int N_RAISES_LEFT,
      u64 PLAYER_POTS,
      int CURR_MAX_BET,
      int TOTAL_POT
      >
    struct LimitHandEvalFoldChild {
      typedef LimitHandEval<
	SMALL_BLIND,
	BIG_BLIND,
	N_PLAYERS,
	N_ACTIVE-1,
	remove_player_from_active_bm(PLAYER_NO, ACTIVE_BM),
	is_active(next_player(PLAYER_NO, N_PLAYERS), ACTIVE_BM),
	N_TO_CALL-1,
	next_player(PLAYER_NO, N_PLAYERS),
	N_RAISES_LEFT,
	PLAYER_POTS,
	CURR_MAX_BET,
	TOTAL_POT
	> fold_t;
      
      fold_t fold;
    };
    
    // Child node of this eval tree node for current player calling (or checking).
    template <
      int SMALL_BLIND,
      int BIG_BLIND,
      int N_PLAYERS,
      int N_ACTIVE,
      u8 ACTIVE_BM,
      bool IS_ACTIVE,
      int N_TO_CALL,
      int PLAYER_NO,
      int N_RAISES_LEFT,
      u64 PLAYER_POTS,
      int CURR_MAX_BET,
      int TOTAL_POT
      >
    struct LimitHandEvalCallChild {
      typedef LimitHandEval<
	SMALL_BLIND,
	BIG_BLIND,
	N_PLAYERS,
	N_ACTIVE,
	ACTIVE_BM,
	is_active(next_player(PLAYER_NO, N_PLAYERS), ACTIVE_BM),
	N_TO_CALL-1,
	next_player(PLAYER_NO, N_PLAYERS),
	N_RAISES_LEFT,
	update_player_pots(PLAYER_NO, CURR_MAX_BET, PLAYER_POTS),
	CURR_MAX_BET,
	TOTAL_POT + player_bet(PLAYER_NO, PLAYER_POTS, CURR_MAX_BET)
	> call_t;
      
      call_t call;
    };
    
    // Child node of this eval tree node for current player raising.
    template <
      int SMALL_BLIND,
      int BIG_BLIND,
      int N_PLAYERS,
      int N_ACTIVE,
      u8 ACTIVE_BM,
      bool IS_ACTIVE,
      int N_TO_CALL,
      int PLAYER_NO,
      int N_RAISES_LEFT,
      u64 PLAYER_POTS,
      int CURR_MAX_BET,
      int TOTAL_POT
      >
    struct LimitHandEvalRaiseChild {
      typedef LimitHandEval<
	SMALL_BLIND,
	BIG_BLIND,
	N_PLAYERS,
	N_ACTIVE,
	ACTIVE_BM,
	is_active(next_player(PLAYER_NO, N_PLAYERS), ACTIVE_BM),
	N_TO_CALL-1,
	next_player(PLAYER_NO, N_PLAYERS),
	N_RAISES_LEFT-1,
	update_player_pots(PLAYER_NO, CURR_MAX_BET + BIG_BLIND, PLAYER_POTS),
	CURR_MAX_BET + BIG_BLIND,
	TOTAL_POT + player_bet(PLAYER_NO, PLAYER_POTS, CURR_MAX_BET + BIG_BLIND)
	> raise_t;
      
      raise_t raise;
    };
    
    // Default specialisation of LimitHandEval.
    //   The current active player can fold, call or raise.
    template <
      int SMALL_BLIND,
      int BIG_BLIND,
      int N_PLAYERS,
      int N_ACTIVE,
      u8 ACTIVE_BM,
      bool IS_ACTIVE,
      int N_TO_CALL,
      int PLAYER_NO,
      int N_RAISES_LEFT,
      u64 PLAYER_POTS,
      int CURR_MAX_BET,
      int TOTAL_POT
      >
    struct LimitHandEvalSpecialised<SMALL_BLIND, BIG_BLIND, N_PLAYERS, N_ACTIVE, ACTIVE_BM, IS_ACTIVE, N_TO_CALL, PLAYER_NO, N_RAISES_LEFT, PLAYER_POTS, CURR_MAX_BET, TOTAL_POT, FoldCallRaiseNodeType> :
      LimitHandEvalConsts<SMALL_BLIND, BIG_BLIND, N_PLAYERS, N_ACTIVE, ACTIVE_BM, IS_ACTIVE, N_TO_CALL, PLAYER_NO, N_RAISES_LEFT, PLAYER_POTS, CURR_MAX_BET, TOTAL_POT>,
      LimitHandEvalFoldChild<SMALL_BLIND, BIG_BLIND, N_PLAYERS, N_ACTIVE, ACTIVE_BM, IS_ACTIVE, N_TO_CALL, PLAYER_NO, N_RAISES_LEFT, PLAYER_POTS, CURR_MAX_BET, TOTAL_POT>,
      LimitHandEvalCallChild<SMALL_BLIND, BIG_BLIND, N_PLAYERS, N_ACTIVE, ACTIVE_BM, IS_ACTIVE, N_TO_CALL, PLAYER_NO, N_RAISES_LEFT, PLAYER_POTS, CURR_MAX_BET, TOTAL_POT>,
      LimitHandEvalRaiseChild<SMALL_BLIND, BIG_BLIND, N_PLAYERS, N_ACTIVE, ACTIVE_BM, IS_ACTIVE, N_TO_CALL, PLAYER_NO, N_RAISES_LEFT, PLAYER_POTS, CURR_MAX_BET, TOTAL_POT>
    {
      static const bool is_leaf = false;
      static const bool can_raise = true;
      
      NodeEval<N_PLAYERS> eval;
    };
    
    // Specialisation for one player left in the hand.
    // This is the highest priority specialisation since the hand is over
    //   immediately - there are no children.
    template <
      int SMALL_BLIND,
      int BIG_BLIND,
      int N_PLAYERS,
      int N_ACTIVE,
      u8 ACTIVE_BM,
      bool IS_ACTIVE,
      int N_TO_CALL,
      int PLAYER_NO,
      int N_RAISES_LEFT,
      u64 PLAYER_POTS,
      int CURR_MAX_BET,
      int TOTAL_POT
      >
    struct LimitHandEvalSpecialised<SMALL_BLIND, BIG_BLIND, N_PLAYERS, N_ACTIVE, ACTIVE_BM, IS_ACTIVE, N_TO_CALL, PLAYER_NO, N_RAISES_LEFT, PLAYER_POTS, CURR_MAX_BET, TOTAL_POT, AllButOneFoldNodeType>
      : LimitHandEvalConsts<SMALL_BLIND, BIG_BLIND, N_PLAYERS, N_ACTIVE, ACTIVE_BM, IS_ACTIVE, N_TO_CALL, PLAYER_NO, N_RAISES_LEFT, PLAYER_POTS, CURR_MAX_BET, TOTAL_POT>
    {
      static const bool is_leaf = true;
      
      NodeEval<N_PLAYERS> eval;
    };
    
    // Specialisation for all active players called.
    // Betting is now over and we go to showdown or the next stage.
    template <
      int SMALL_BLIND,
      int BIG_BLIND,
      int N_PLAYERS,
      int N_ACTIVE,
      u8 ACTIVE_BM,
      bool IS_ACTIVE,
      int N_TO_CALL,
      int PLAYER_NO,
      int N_RAISES_LEFT,
      u64 PLAYER_POTS,
      int CURR_MAX_BET,
      int TOTAL_POT
      >
    struct LimitHandEvalSpecialised<SMALL_BLIND, BIG_BLIND, N_PLAYERS, N_ACTIVE, ACTIVE_BM, IS_ACTIVE, N_TO_CALL, PLAYER_NO, N_RAISES_LEFT, PLAYER_POTS, CURR_MAX_BET, TOTAL_POT, ShowdownNodeType>
      : LimitHandEvalConsts<SMALL_BLIND, BIG_BLIND, N_PLAYERS, N_ACTIVE, ACTIVE_BM, IS_ACTIVE, N_TO_CALL, PLAYER_NO, N_RAISES_LEFT, PLAYER_POTS, CURR_MAX_BET, TOTAL_POT>
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
      int N_ACTIVE,
      u8 ACTIVE_BM,
      bool IS_ACTIVE,
      int N_TO_CALL,
      int PLAYER_NO,
      int N_RAISES_LEFT,
      u64 PLAYER_POTS,
      int CURR_MAX_BET,
      int TOTAL_POT
      >
    struct LimitHandEvalSpecialised<SMALL_BLIND, BIG_BLIND, N_PLAYERS, N_ACTIVE, ACTIVE_BM, IS_ACTIVE, N_TO_CALL, PLAYER_NO, N_RAISES_LEFT, PLAYER_POTS, CURR_MAX_BET, TOTAL_POT, AlreadyFoldedNodeType>
      : LimitHandEvalConsts<SMALL_BLIND, BIG_BLIND, N_PLAYERS, N_ACTIVE, ACTIVE_BM, IS_ACTIVE, N_TO_CALL, PLAYER_NO, N_RAISES_LEFT, PLAYER_POTS, CURR_MAX_BET, TOTAL_POT>
    {
      static const bool is_leaf = false;
      
      typedef LimitHandEval<
	SMALL_BLIND,
	BIG_BLIND,
	N_PLAYERS,
	N_ACTIVE,
	ACTIVE_BM,
	is_active(next_player(PLAYER_NO, N_PLAYERS), ACTIVE_BM),
	N_TO_CALL,
	next_player(PLAYER_NO, N_PLAYERS),
	N_RAISES_LEFT,
	PLAYER_POTS,
	CURR_MAX_BET,
	TOTAL_POT
	> inactive_child_t;
      
      inactive_child_t _;
    };
    
    // Specialisation for when we're at maximum bet level.
    // This is lowest priority specialisation since terminal conditions should be considered first.
    // Only fold or call (check) is allowed now.
    template <
      int SMALL_BLIND,
      int BIG_BLIND,
      int N_PLAYERS,
      int N_ACTIVE,
      u8 ACTIVE_BM,
      bool IS_ACTIVE,
      int N_TO_CALL,
      int PLAYER_NO,
      u64 PLAYER_POTS,
      int CURR_MAX_BET,
      int TOTAL_POT
      >
    struct LimitHandEvalSpecialised<SMALL_BLIND, BIG_BLIND, N_PLAYERS, N_ACTIVE, ACTIVE_BM, IS_ACTIVE, N_TO_CALL, PLAYER_NO, /*N_RAISES_LEFT*/0, PLAYER_POTS, CURR_MAX_BET, TOTAL_POT, FoldCallNodeType> :
      LimitHandEvalConsts<SMALL_BLIND, BIG_BLIND, N_PLAYERS, N_ACTIVE, ACTIVE_BM, IS_ACTIVE, N_TO_CALL, PLAYER_NO, /*N_RAISES_LEFT*/0, PLAYER_POTS, CURR_MAX_BET, TOTAL_POT>,
      LimitHandEvalFoldChild<SMALL_BLIND, BIG_BLIND, N_PLAYERS, N_ACTIVE, ACTIVE_BM, IS_ACTIVE, N_TO_CALL, PLAYER_NO, /*N_RAISES_LEFT*/0, PLAYER_POTS, CURR_MAX_BET, TOTAL_POT>,
      LimitHandEvalCallChild<SMALL_BLIND, BIG_BLIND, N_PLAYERS, N_ACTIVE, ACTIVE_BM, IS_ACTIVE, N_TO_CALL, PLAYER_NO, /*N_RAISES_LEFT*/0, PLAYER_POTS, CURR_MAX_BET, TOTAL_POT>
    {
      static const bool is_leaf = false;
      static const bool can_raise = false;
      
      NodeEval<N_PLAYERS> eval;
    };
    
    constexpr inline u8 make_root_active_bm(int n_players) {
      return (u8)((1 << n_players) - 1);
    }
    
    constexpr inline u64 make_root_player_pots(int small_blind, int big_blind) {
      return make_player_pot(0, small_blind) | make_player_pot(1, big_blind);
    }
    
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
	/*N_ACTIVE*/N_PLAYERS,
	make_root_active_bm(N_PLAYERS),
	/*IS_ACTIVE*/true,
	/*N_TO_CALL*/N_PLAYERS,     // Note BB is allowed to still raise
	/*PLAYER_NO*/2 % N_PLAYERS,
	/*N_RAISES_LEFT*/N_RAISES,
	/*PLAYER_POTS*/make_root_player_pots(SMALL_BLIND, BIG_BLIND),
	/*CURR_MAX_BET*/BIG_BLIND,
	/*TOTAL_POT*/BIG_BLIND + SMALL_BLIND
	> type_t;
    };

    typedef LimitRootHandEval</*N_PLAYERS*/2, /*N_RAISES*/0>::type_t limit_root_2p_0raise_t;

    // Let's check that things are as expected
    static_assert(!limit_root_2p_0raise_t::is_leaf);
    
  } // namespace Gto
  
} // namespace Poker

#endif //def GTO_EVAL
