#ifndef LIMIT_GAME_TREE_HPP
#define LIMIT_GAME_TREE_HPP

#include <array>
#include "limit-config.hpp"

namespace Limit {

  namespace GameTree {

    enum stage_t { ROOT_STAGE, PREFLOP_STAGE, FLOP_STAGE, TURN_STAGE, RIVER_STAGE };

    enum node_t { ROOT_NODE, HOLE_DEAL_NODE, FOLDED_NODE, PREFLOP_BETTING_NODE, FLOP_DEAL_CLUMPED_NODE, FLOP_DEAL_EXACT_NODE, FLOP_BETTING_NODE, TURN_DEAL_NODE, TURN_BETTING_NODE, RIVER_DEAL_NODE, RIVER_BETTING_NODE };

    template <std::size_t N_PLAYERS>
    struct GameTreeNodeT {
      
      const stage_t stage;
      const node_t node_type;
      
      // The player whose strategy this is.
      const std::size_t my_player_no;
      
      // Player to go for betting (or folded) nodes.
      const std::size_t player_no;
      
      // Each player's current total wager, including blinds.
      const std::array<double, N_PLAYERS> players_bets;
      
      // Sum of all players_bets.
      const double pot;
      
      // For each player, true iff the player has not yet folded.
      const std::array<bool, N_PLAYERS> players_active;
      
      // The count of active players - whenever this is 1 then the hand is finished.
      const std::size_t n_players_active;
      
      // The number of allowed raises remaining in this stage.
      const std::size_t n_raises_left;

      /////////////////////////// Child Nodes ////////////////////////////////////
      
      // Child node for all nodes where player_no is not betting.
      GameTreeNodeT<N_PLAYERS>* passthru_strategy;
      
      // Child nodes of BETTING nodes.
      GameTreeNodeT<N_PLAYERS>* fold_strategy;
      GameTreeNodeT<N_PLAYERS>* check_strategy;
      GameTreeNodeT<N_PLAYERS>* raise_strategy;
      
    }; // struct GameTreeNodeT
    
  } // namespace GameTree

} // namespace Limit

#endif //def LIMIT_GAME_TREE_HPP
