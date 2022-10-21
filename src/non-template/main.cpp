#include <cstdio>

#include "limit-config.hpp"
#include "limit-strategy.hpp"

template <std::size_t N_PLAYERS>
void expand_all(Limit::GameTree::GameTreeNodeT<N_PLAYERS>* node) {
  node->expand();

  if (node->child) {
    expand_all(node->child);
  }

  if (node->fold) {
    expand_all(node->fold);
  }
  
  if (node->check) {
    expand_all(node->check);
  }
  
  if (node->raise) {
    expand_all(node->raise);
  }
  
}

int main() {
  printf("Hallo RPJ\n");

  const std::size_t n_players = 5;
  const Limit::Config::ConfigT config {
      .n_players = n_players,
      .small_blind = 1.0,
      .big_blind = 2.0,
      .preflop_raise = 2.0,
      .max_n_preflop_raises = 4,
      .flop_raise = 2.0,
      .max_n_flop_raises = 4,
      .turn_raise = 4.0,
      .max_n_turn_raises = 4,
      .river_raise = 4.0,
      .max_n_river_raises = 4,
  };
  
  auto root = Limit::GameTree::GameTreeNodeT<5>::new_root(config);

  expand_all(root);
}
