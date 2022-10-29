#ifndef LIMIT_GAME_TREE_HPP
#define LIMIT_GAME_TREE_HPP

#include <algorithm>
#include <array>
#include <cassert>

#include "limit-config.hpp"

namespace Limit {

  namespace GameTree {

    enum street_t { PREFLOP_STREET, FLOP_STREET, TURN_STREET, RIVER_STREET, RESULT_STREET, N_STREETS };

    [[maybe_unused]]
    static const char *const STREET_NAMES[N_STREETS] = { "preflop", "flop", "turn", "river", "result" };
    
    static inline street_t next_street(street_t street) {
      if (street == RESULT_STREET) {
	assert(0 && "next_street(RESULT_STREET) is invalid");
      } else {
	return (street_t) (street+1);
      }
    }

    static inline double street_raise(street_t street, const Config::ConfigT& config) {
      switch (street) {
      case PREFLOP_STREET: return config.preflop_raise;
      case FLOP_STREET:    return config.flop_raise;
      case TURN_STREET:    return config.turn_raise;
      case RIVER_STREET:   return config.river_raise;
      default:             assert(0 && "invalid street for street_raise()");
      }
      
      return 0.0;
    }

    enum node_t { DEAL_NODE, BETTING_NODE, SHOWDOWN_NODE, STEAL_NODE, N_NODE_TYPES };
    [[maybe_unused]]
    static const char *const NODE_TYPE_NAMES[N_NODE_TYPES] = { "deal", "bet", "showdown", "steal" };

    const std::size_t SB = 0;
    const std::size_t BB = 1;
    const std::size_t UTG = 2;

    // Player 0 is small blind (SB)
    // Player 1 is big blind (BB)
    template <std::size_t N_PLAYERS>
    struct GameTreeNodeT {

      const Config::ConfigT& config;

      const street_t street;
      const node_t node_type;
      
      // The count of active players - whenever this is 1 then the hand is finished.
      const std::size_t n_players_active;
      
      // Each player's current total wager, including blinds.
      const std::array<double, N_PLAYERS> players_bets;

      // Maximum player bet
      const double max_bet;
      
      // Sum of all players_bets.
      const double pot;
      
      // For each player, false iff the player has not yet folded.
      const std::array<bool, N_PLAYERS> players_folded;
      
      // Player to go for betting (or folded) nodes.
      const std::size_t player_no;
      
      // The number of allowed raises remaining in this street.
      const std::size_t n_raises_left;

      // The number of calls (or checks) remaining until everyone has called.
      //   This includes folded players.
      const std::size_t n_calls_left;

      // The winning player for STEAL_NODE's where all but one player has folded.
      const std::size_t steal_player_no;

      /////////////////////////// Child Nodes ////////////////////////////////////

      // Quick test for whether this node has child nodes.
      bool is_expanded;
      
      // Child node for all non-betting nodes including where player_no has already folded.
      GameTreeNodeT<N_PLAYERS>* child;
      
      // Child nodes of BETTING nodes.
      GameTreeNodeT<N_PLAYERS>* fold;
      GameTreeNodeT<N_PLAYERS>* call;
      GameTreeNodeT<N_PLAYERS>* raise;

      // For compiler to check uninitialised members.
      bool init_check;

      static GameTreeNodeT<N_PLAYERS>* new_root(const Config::ConfigT& config) {

	std::array<double, N_PLAYERS> new_players_bets{};
	new_players_bets[SB] = config.small_blind;
	new_players_bets[BB] = config.big_blind;

	return new GameTreeNodeT<N_PLAYERS> {
	  .config = config,
	    .street = PREFLOP_STREET,
	    .node_type = DEAL_NODE,
	    .n_players_active = N_PLAYERS,
	    .players_bets = new_players_bets,
	    .max_bet = std::max(config.small_blind, config.big_blind),
	    .pot = config.small_blind + config.big_blind,
	    .players_folded = std::array<bool, N_PLAYERS>{},
	    .player_no = 0,
	       .n_raises_left = 0,
	       .n_calls_left = 0,
	       .steal_player_no = 0,
	       .is_expanded = false,
	       .child = 0,
	       .fold = 0,
	       .call = 0,
	       .raise = 0,
	       .init_check = true,
	};
      }

      void expand_deal_node() {
      	assert(!this->is_expanded);
      	assert(this->child == 0);
      	assert(this->fold == 0);
      	assert(this->call == 0);
      	assert(this->raise == 0);
      	assert(this->node_type == DEAL_NODE);

      	// Child node of a deal node is the first betting round.

      	// In heads-up, SB starts pre-flop - otherwise under-the-gun (UTG) player 2.
	// In heads-up, BB starts post-flop - otherwise SB;

	// Oops, needs to be an active player! !$#%@#$%@#$%@#$%
      	std::size_t new_player_no = this->street == PREFLOP_STREET
	  ? (N_PLAYERS <= 2 ? SB : UTG)
	  : (N_PLAYERS <= 2 ? BB : SB);

      	child = new GameTreeNodeT<N_PLAYERS> {
					      .config = this->config,
					      .street = this->street,
					      .node_type = BETTING_NODE,
					      .n_players_active = this->n_players_active,
					      .players_bets = this->players_bets,
					      .max_bet = this->max_bet,
					      .pot = this->pot,
					      .players_folded = this->players_folded,
					      .player_no = new_player_no,
					      .n_raises_left = this->config.max_n_preflop_raises, // @#$@#$ wrong
					      .n_calls_left = N_PLAYERS - 1,
					      .steal_player_no = 0,
					      .is_expanded = false,
					      .child = 0,
					      .fold = 0,
					      .call = 0,
					      .raise = 0,
					      .init_check = true,
      	};
      }

      GameTreeNodeT<N_PLAYERS>* new_fold_child() {

	// Most often the fold child will be another betting node of the same street.
	street_t new_street = this->street;
	node_t new_node_type = this->node_type;

	std::array<bool, N_PLAYERS> new_players_folded = this->players_folded;
	new_players_folded[this->player_no] = true;

	const std::size_t new_n_players_active = this->n_players_active - 1;

	const std::size_t new_n_calls_left = this->n_calls_left - 1;

	// Only valid if there will be only one player left in the pot.
	std::size_t new_steal_player_no = this->steal_player_no;

	// Max bet doesn't change, except if this is the last betting node on this street - then max_bet is reset to 0.0.
	double new_max_bet = this->max_bet;

	// Is there only one man standing?
	if (new_n_players_active == 1) {
	  new_street = RESULT_STREET;
	  new_node_type = STEAL_NODE;

	  new_steal_player_no = std::find(new_players_folded.begin(), new_players_folded.end(), false) - new_players_folded.begin();
	  assert(new_steal_player_no < N_PLAYERS);
	} else if (new_n_calls_left == 0) {
	  new_street = next_street(street);
	  new_node_type = this->street == RIVER_STREET ? SHOWDOWN_NODE : BETTING_NODE;
	  new_max_bet = 0.0;
	}

	return new GameTreeNodeT<N_PLAYERS> {
	  .config = this->config,
	    .street = new_street,
	    .node_type = new_node_type,
	    .n_players_active = new_n_players_active,
	    .players_bets = this->players_bets,
	    .max_bet = new_max_bet,
	    .pot = this->pot,
	    .players_folded = new_players_folded,
	    .player_no = (this->player_no + 1) % N_PLAYERS,
	    .n_raises_left = this->n_raises_left,
	    .n_calls_left = new_n_calls_left,
	    .steal_player_no = new_steal_player_no,
	    .is_expanded = false,
	    .child = 0,
	    .fold = 0,
	    .call = 0,
	    .raise = 0,
	    .init_check = true,
	    };
      }

      GameTreeNodeT<N_PLAYERS>* new_call_child() {
	
	// Most often the call child will be another betting node of the same street.
	street_t new_street = this->street;
	node_t new_node_type = this->node_type;

	const double new_pot = this->pot + (this->max_bet - this->players_bets[this->player_no]);
	  
	std::array<double, N_PLAYERS> new_players_bets = this->players_bets;
	new_players_bets[this->player_no] = this->max_bet;

	const std::size_t new_n_calls_left = this->n_calls_left - 1;

	// Max bet doesn't change, except if this is the last betting node on this street - then max_bet is reset to 0.0.
	double new_max_bet = this->max_bet;

	// Has everyone left called?
	if (new_n_calls_left == 0) {
	  new_street = next_street(this->street);
	  new_node_type = this->street == RIVER_STREET ? SHOWDOWN_NODE : DEAL_NODE;
	  new_max_bet = 0.0;
	}

	return new GameTreeNodeT<N_PLAYERS> {
	  .config = this->config,
	    .street = new_street,
	    .node_type = new_node_type,
	    .n_players_active = this->n_players_active,
	    .players_bets = new_players_bets,
	    .max_bet = new_max_bet,
	    .pot = new_pot,
	    .players_folded = this->players_folded,
	    .player_no = (this->player_no + 1) % N_PLAYERS,
	    .n_raises_left = this->n_raises_left,
	    .n_calls_left = new_n_calls_left,
	    .steal_player_no = 0,
	    .is_expanded = false,
	    .child = 0,
	    .fold = 0,
	    .call = 0,
	    .raise = 0,
	    .init_check = true,
	    };
      }

      GameTreeNodeT<N_PLAYERS>* new_raise_child() {
	assert(this->n_raises_left > 0);

	double new_max_bet = this->max_bet + street_raise(this->street, this->config);

	const double new_pot = this->pot + (new_max_bet - this->players_bets[this->player_no]);
	  
	std::array<double, N_PLAYERS> new_players_bets = this->players_bets;
	new_players_bets[this->player_no] = new_max_bet;

	return new GameTreeNodeT<N_PLAYERS> {
	  .config = this->config,
	    .street = this->street,
	    .node_type = this->node_type,
	    .n_players_active = this->n_players_active,
	    .players_bets = new_players_bets,
	    .max_bet = new_max_bet,
	    .pot = new_pot,
	    .players_folded = this->players_folded,
	    .player_no = (this->player_no + 1) % N_PLAYERS,
	    .n_raises_left = this->n_raises_left - 1,
	    .n_calls_left = N_PLAYERS - 1,
	    .steal_player_no = 0,
	    .is_expanded = false,
	    .child = 0,
	    .fold = 0,
	    .call = 0,
	    .raise = 0,
	    .init_check = true,
	    };
      }

      GameTreeNodeT<N_PLAYERS>* new_folded_child() {
	assert(this->players_folded[this->player_no]);
	
	// Most often the child will be another betting node of the same street.
	street_t new_street = this->street;
	node_t new_node_type = this->node_type;

	const std::size_t new_n_calls_left = this->n_calls_left - 1;

	// Max bet doesn't change, except if this is the last betting node on this street - then max_bet is reset to 0.0.
	double new_max_bet = this->max_bet;

	// Has everyone still active called?
	if (new_n_calls_left == 0) {
	  new_street = next_street(this->street);
	  new_node_type = this->street == RIVER_STREET ? SHOWDOWN_NODE : DEAL_NODE;
	  new_max_bet = 0.0;
	}

	return new GameTreeNodeT<N_PLAYERS> {
	  .config = this->config,
	    .street = new_street,
	    .node_type = new_node_type,
	    .n_players_active = this->n_players_active,
	    .players_bets = this->players_bets,
	    .max_bet = new_max_bet,
	    .pot = this->pot,
	    .players_folded = this->players_folded,
	    .player_no = (this->player_no + 1) % N_PLAYERS,
	    .n_raises_left = this->n_raises_left,
	    .n_calls_left = new_n_calls_left,
	    .steal_player_no = 0,
	    .is_expanded = false,
	    .child = 0,
	    .fold = 0,
	    .call = 0,
	    .raise = 0,
	    .init_check = true,
	    };
      }

      void expand_betting_node() {
	
      	assert(!this->is_expanded);
      	assert(this->child == 0);
      	assert(this->fold == 0);
      	assert(this->call == 0);
      	assert(this->raise == 0);
      	assert(this->street != RESULT_STREET);
      	assert(this->node_type == BETTING_NODE);

	
	assert(this->n_players_active > 1);

	// If this player has already folded, then this is just a pass-thru node
	if (this->players_folded[this->player_no]) {
	  this->child = new_folded_child();
	  return;
	}

	// Create the fold child.
	this->fold = new_fold_child();

	// Create the call child.
	this->call = new_call_child();

	// Create the raise child if raise is still an option
	if (n_raises_left > 0) {
	  this->raise = new_raise_child();
	}
      }
      
      // Expand one level down, if we've not yet expanded.
      // $%^#$%^#$% scrap betting nodes where player is inactive
      void expand() {
	if (this->is_expanded) { return; }

	//@#$%@#$% terminal nodes like steal

	switch (this->node_type) {
	case DEAL_NODE:    expand_deal_node(); break;
	case BETTING_NODE: expand_betting_node(); break;
	  //, FLOP_DEAL_CLUMPED_NODE, FLOP_DEAL_EXACT_NODE, FLOP_BETTING_NODE, TURN_DEAL_NODE, TURN_BETTING_NODE, RIVER_DEAL_NODE, RIVER_BETTING_NODE };

	default:
	  //printf("RPJ - %s node type unrecognised\n", STREET_NAMES[this->street]);
	  assert(0 && "unrecognised node_type");
	  break;
	  
	}

	this->is_expanded = true;
      }
      
    }; // struct GameTreeNodeT
    
  } // namespace GameTree

} // namespace Limit

#endif //def LIMIT_GAME_TREE_HPP
