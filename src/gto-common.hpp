#ifndef GTO_COMMON
#define GTO_COMMON

#include "types.hpp"

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
    
    constexpr inline bool get_is_active(int player_no, u8 active_bm) {
      return (active_bm & active_bm_u8_mask(player_no)) != 0;
    }
    
    constexpr inline u8 remove_player_from_active_bm(int player_no, u8 active_bm) {
      return active_bm & ~active_bm_u8_mask(player_no);
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
    };
    
    template <int N_PLAYERS>
    struct NodeEval {
      // Sum over all hands evaluated of per-hand 'probability of reaching this node'.
      double activity;
      // Sum over all hands of per-hand (product of) 'probability of reaching this node' times 'player outcome for the hand'.
      NodeEvalPerPlayerProfit<N_PLAYERS> player_profits;
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
    
  } // namespace Gto
  
} // namespace Poker




#endif //def GTO_COMMON