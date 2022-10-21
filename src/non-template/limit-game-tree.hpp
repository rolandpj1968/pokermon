#ifndef LIMIT_GAME_TREE_HPP
#define LIMIT_GAME_TREE_HPP

#include <algorithm>
#include <array>
#include <cassert>

#include "limit-config.hpp"

namespace Limit {

  namespace GameTree {

    enum stage_t { ROOT_STAGE, PREFLOP_STAGE, FLOP_STAGE, TURN_STAGE, RIVER_STAGE, LAST_MAN_STANDING_STAGE };

    enum node_t { ROOT_NODE, HOLE_DEAL_NODE, PREFLOP_BETTING_NODE, FLOP_DEAL_NODE, FLOP_BETTING_NODE, TURN_DEAL_NODE, TURN_BETTING_NODE, RIVER_DEAL_NODE, RIVER_BETTING_NODE, SHOWDOWN_NODE, LAST_MAN_STANDING_NODE };

    const std::size_t SB = 0;
    const std::size_t BB = 1;
    const std::size_t UTG = 2;

    // Player 0 is small blind (SB)
    // Player 1 is big blind (BB)
    template <std::size_t N_PLAYERS>
    struct GameTreeNodeT {

      const Config::ConfigT& config;

      const stage_t stage;
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
      
      // The number of allowed raises remaining in this stage.
      const std::size_t n_raises_left;

      // The number of checks remaining until everyone has checked.
      //   This includes folded players.
      const std::size_t n_checks_left;

      // The winning player for LAST_MAN_STANDING_NODE's
      const std::size_t last_man_standing_player_no;

      /////////////////////////// Child Nodes ////////////////////////////////////

      // Quick check for whether this node has child nodes.
      bool is_expanded;
      
      // Child node for all non-betting nodes including where player_no has already folded.
      GameTreeNodeT<N_PLAYERS>* child;
      
      // Child nodes of BETTING nodes.
      GameTreeNodeT<N_PLAYERS>* fold;
      GameTreeNodeT<N_PLAYERS>* check;
      GameTreeNodeT<N_PLAYERS>* raise;

      static GameTreeNodeT<N_PLAYERS>* new_root(const Config::ConfigT& config) {

	std::array<double, N_PLAYERS> players_bets;
	players_bets[SB] = config.small_blind;
	players_bets[BB] = config.big_blind;

	return new GameTreeNodeT<N_PLAYERS> {
	  .config = config,
	    .stage = ROOT_STAGE,
	    .node_type = ROOT_NODE,
	    .n_players_active = N_PLAYERS,
	    .players_bets = players_bets,
	    .max_bet = std::max(config.small_blind, config.big_blind),
	    .pot = config.small_blind + config.big_blind,
	};
      }

      void expand_root_node() {
	assert(!this->is_expanded);
	assert(this->child == 0);
	assert(this->stage == ROOT_STAGE);
	assert(this->node_type == ROOT_NODE);

	// Child node of root node is the hole-card deal.

	child = new GameTreeNodeT<N_PLAYERS> {
					      .config = this->config,
					      .stage = PREFLOP_STAGE,
					      .node_type = HOLE_DEAL_NODE,
					      .n_players_active = N_PLAYERS,
					      .players_bets = this->players_bets,
					      .max_bet = this->max_bet,
					      .pot = this->pot,
	};
      }

      void expand_hole_deal_node() {
      	assert(!this->is_expanded);
      	assert(this->child == 0);
      	assert(this->stage == PREFLOP_STAGE);
      	assert(this->node_type == HOLE_DEAL_NODE);

      	// Child node of hole-card deal node is the first betting round.

      	// In heads-up, SB starts - otherwise under-the-gun (UTG) player 2.
      	std::size_t new_player_no = N_PLAYERS <= 2 ? SB : UTG;

      	child = new GameTreeNodeT<N_PLAYERS> {
					      .config = this->config,
					      .stage = PREFLOP_STAGE,
					      .node_type = PREFLOP_BETTING_NODE,
					      .n_players_active = N_PLAYERS,
					      .players_bets = this->players_bets,
					      .max_bet = this->max_bet,
					      .pot = this->pot,
					      .players_folded = this->players_folded,
					      .player_no = new_player_no,
					      .n_raises_left = this->config.max_n_preflop_raises,
					      .n_checks_left = N_PLAYERS - 1,
      	};
      }

      GameTreeNodeT<N_PLAYERS>* new_fold_child(stage_t next_stage, node_t next_node_type) {

	// Most often the fold child will be another betting node of the same stage.
	stage_t new_stage = this->stage;
	node_t new_node_type = this->node_type;

	std::array<bool, N_PLAYERS> new_players_folded = this->players_folded;
	new_players_folded[this->player_no] = true;

	const std::size_t new_n_players_active = this->n_players_active - 1;

	const std::size_t new_n_checks_left = this->n_checks_left - 1;

	// Only valid if there will be only one player left in the pot.
	std::size_t new_last_man_standing_player_no = this->last_man_standing_player_no;

	// Is there only one man standing?
	if (new_n_players_active == 1) {
	  new_stage = LAST_MAN_STANDING_STAGE;
	  new_node_type = LAST_MAN_STANDING_NODE;

	  new_last_man_standing_player_no = std::find(new_players_folded.begin(), new_players_folded.end(), true) - new_players_folded.begin();
	  assert(new_last_man_standing_player_no < N_PLAYERS);
	} else if (new_n_checks_left == 0) {
	  new_stage = next_stage;
	  new_node_type = next_node_type;
	}

	return new GameTreeNodeT<N_PLAYERS> {
	  .config = config,
	    .stage = new_stage,
	    .node_type = new_node_type,
	    .n_players_active = new_n_players_active,
	    .players_bets = players_bets,
	    .max_bet = this->max_bet,
	    .pot = this->pot,
	    .players_folded = new_players_folded,
	    .player_no = (this->player_no + 1) % N_PLAYERS,
	    .n_raises_left = this->n_raises_left,
	    .n_checks_left = new_n_checks_left,
	    .last_man_standing_player_no = new_last_man_standing_player_no,
	    };
      }

      GameTreeNodeT<N_PLAYERS>* new_check_child(stage_t next_stage, node_t next_node_type) {
	
	// Most often the check child will be another betting node of the same stage.
	stage_t new_stage = this->stage;
	node_t new_node_type = this->node_type;

	assert(this->players_bets[this->player_no] < this->max_bet);
	const double new_pot = this->pot + (this->max_bet - this->players_bets[this->player_no]);
	  
	std::array<double, N_PLAYERS> new_players_bets = this->players_bets;
	new_players_bets[this->player_no] = this->max_bet;

	const std::size_t new_n_checks_left = this->n_checks_left - 1;

	// Has everyone left checked?
	if (new_n_checks_left == 0) {
	  new_stage = next_stage;
	  new_node_type = next_node_type;
	}

	return new GameTreeNodeT<N_PLAYERS> {
	  .config = config,
	    .stage = new_stage,
	    .node_type = new_node_type,
	    .n_players_active = this->n_players_active,
	    .players_bets = new_players_bets,
	    .max_bet = this->max_bet,
	    .pot = new_pot,
	    .players_folded = this->players_folded,
	    .player_no = (this->player_no + 1) % N_PLAYERS,
	    .n_raises_left = this->n_raises_left,
	    .n_checks_left = new_n_checks_left,
	    };
      }

      GameTreeNodeT<N_PLAYERS>* new_raise_child(double stage_raise) {

	double new_max_bet = this->max_bet + stage_raise;

	const double new_pot = this->pot + (new_max_bet - this->players_bets[this->player_no]);
	  
	std::array<double, N_PLAYERS> new_players_bets = this->players_bets;
	new_players_bets[this->player_no] = new_max_bet;

	return new GameTreeNodeT<N_PLAYERS> {
	  .config = config,
	    .stage = this->stage,
	    .node_type = this->node_type,
	    .n_players_active = this->n_players_active,
	    .players_bets = new_players_bets,
	    .max_bet = new_max_bet,
	    .pot = new_pot,
	    .players_folded = this->players_folded,
	    .player_no = (this->player_no + 1) % N_PLAYERS,
	    .n_raises_left = this->n_raises_left - 1,
	    .n_checks_left = N_PLAYERS - 1,
	    };
      }

      GameTreeNodeT<N_PLAYERS>* new_folded_child(stage_t next_stage, node_t next_node_type) {
	assert(this->players_folded[this->player_no]);
	
	// Most often the check child will be another betting node of the same stage.
	stage_t new_stage = this->stage;
	node_t new_node_type = this->node_type;

	const std::size_t new_n_checks_left = this->n_checks_left - 1;

	// Has everyone still active checked?
	if (new_n_checks_left == 0) {
	  new_stage = next_stage;
	  new_node_type = next_node_type;
	}

	return new GameTreeNodeT<N_PLAYERS> {
	  .config = config,
	    .stage = new_stage,
	    .node_type = new_node_type,
	    .n_players_active = this->n_players_active,
	    .players_bets = this->players_bets,
	    .max_bet = this->max_bet,
	    .pot = this->pot,
	    .players_folded = this->players_folded,
	    .player_no = (this->player_no + 1) % N_PLAYERS,
	    .n_raises_left = this->n_raises_left,
	    .n_checks_left = new_n_checks_left,
	    };
      }

      void expand_betting_node(stage_t next_stage, node_t next_node_type, double raise) {
	
	assert(this->n_players_active > 1);

	// If this player has already folded, then this is just a pass-thru node
	if (this->players_folded[this->player_no]) {
	  this->child = new_folded_child(next_stage, next_node_type);
	  return;
	}

	// Create the fold child.
	this->fold = new_fold_child(next_stage, next_node_type);

	// Create the check child.
	this->check = new_check_child(next_stage, next_node_type);

	// Create the raise child if raise is still an option
	if (n_raises_left > 0) {
	  this->raise = new_raise_child(raise);
	}
      }
      
      void expand_preflop_betting_node() {
      	assert(!this->is_expanded);
      	assert(this->child == 0);
      	assert(this->fold == 0);
      	assert(this->check == 0);
      	assert(this->raise == 0);
      	assert(this->stage == PREFLOP_STAGE);
      	assert(this->node_type == PREFLOP_BETTING_NODE);

	expand_betting_node(FLOP_STAGE, FLOP_DEAL_NODE, config.preflop_raise);
      }
      
      // Expand one level down, if we've not yet expanded.
      void expand() {
	if (this->is_expanded) { return; }

	switch (this->node_type) {
	case ROOT_NODE:            expand_root_node(); break;
	case HOLE_DEAL_NODE:       expand_hole_deal_node(); break;
	case PREFLOP_BETTING_NODE: expand_preflop_betting_node(); break;
	  //, FLOP_DEAL_CLUMPED_NODE, FLOP_DEAL_EXACT_NODE, FLOP_BETTING_NODE, TURN_DEAL_NODE, TURN_BETTING_NODE, RIVER_DEAL_NODE, RIVER_BETTING_NODE };

	default: assert(0 && "unrecognised node_type"); break;
	  
	}

	this->is_expanded = true;
      }
      
    }; // struct GameTreeNodeT
    
  } // namespace GameTree

} // namespace Limit

#endif //def LIMIT_GAME_TREE_HPP
