#ifndef LIMIT_STRATEGY_HPP
#define LIMIT_STRATEGY_HPP

#include <algorithm>
#include <array>

#include "types.hpp"
#include "limit-game-tree.hpp"

namespace Limit {
  
  namespace Strategy {

    using Poker::CardT;
    using Poker::RankT;
    using Poker::SuitT;
  
    template <std::size_t N_PLAYERS>
    struct StrategyNodeT;
    
    template <std::size_t N_PLAYERS>
    struct HoleDealStrategyNodeT {

      // Base StrategyNodeT for this hole deal
      struct StrategyNodeT<N_PLAYERS> *const base;

      // [i][i] for pocket pairs
      // [i][j] with i > j for suited
      // [i][j] with i < j for off-suit
      std::array<std::array<StrategyNodeT<N_PLAYERS>*, 13>, 13> hole_cards_strategies;

      StrategyNodeT<N_PLAYERS>* hole_cards_strategy(CardT card1, CardT card2) {
	RankT rank1_ace_low = to_ace_low(card1.rank);
	RankT rank2_ace_low = to_ace_low(card2.rank);

	RankT min_rank = std::min(rank1_ace_low, rank2_ace_low);
	RankT max_rank = std::max(rank1_ace_low, rank2_ace_low);

	std::size_t i = min_rank;
	std::size_t j = max_rank;

	if (card1.suit == card2.suit) {
	  std::swap(i, j);
	}

	return hole_cards_strategies[i][j];
      }
    };

    template <std::size_t N_PLAYERS>
    struct StrategyNodeT {

      const GameTree::GameTreeNodeT<N_PLAYERS>& game_tree_node;

      // The player whose strategy this is.
      const std::size_t my_player_no;
      
      //////////////////// SubNodes for Special Nodes ////////////////////////////

      // For hole card deal node.
      HoleDealStrategyNodeT<N_PLAYERS>* hole_deal_subnode;

      // TODO flop, river, turn

      /////////////////////////// Evaluation /////////////////////////////////////

      // Used only for monte-carlo evaluation

      // Sum of probability of reaching this node over all hands evaluated
      double activity;
      // Sum of node probability x EV over all hands evaluated
      double value;

      /////////////////////////// Child Nodes ////////////////////////////////////
      
      // Child node for all nodes where player_no is not active.
      StrategyNodeT<N_PLAYERS>* passthru_strategy;
      
      // Probability of folding - only for BETTING nodes of my_player_no.
      double fold_p;
      // Probability of checking/seeing - only for BETTING nodes of my_player_no.
      double check_p;
      // Probability of raising - only for BETTING nodes of my_player_no.
      // Not used when n_raises_left == 0.
      double raise_p;
      
      // Child nodes of BETTING nodes.
      StrategyNodeT<N_PLAYERS>* fold_strategy;
      StrategyNodeT<N_PLAYERS>* check_strategy;
      StrategyNodeT<N_PLAYERS>* raise_strategy;
    
      // For compiler to check uninitialised members.
      bool init_check;
    };
    
  } // namespace Strategy

} // namespace Limit

#endif //LIMIT_STRATEGY_HPP
