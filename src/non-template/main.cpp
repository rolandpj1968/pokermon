#include <cstdio>

#include "limit-config.hpp"
#include "limit-strategy.hpp"

template <std::size_t N_PLAYERS>
void count_nodes(Limit::GameTree::GameTreeNodeT<N_PLAYERS>* node, int& count) {
  
  if (node == 0) { return; }
  
  count++;

  count_nodes(node->child, count);

  count_nodes(node->fold, count);
  
  count_nodes(node->call, count);
  
  count_nodes(node->raise, count);
  
}

template <std::size_t N_PLAYERS>
void dump_nodes(Limit::GameTree::GameTreeNodeT<N_PLAYERS>* node, const char* label, int indent) {
  
  if (node == 0) { return; }

  if (label) {
    printf("%*s", indent, "");
    printf("%s:\n", label);
  }

  printf("%*s", indent, "");
  printf("%s %s player %lu %s pot %.2lf\n", Limit::GameTree::STREET_NAMES[node->street], Limit::GameTree::NODE_TYPE_NAMES[node->node_type], node->player_no, (node->players_folded[node->player_no] ? "folded" : "active"), node->pot);

  dump_nodes(node->child, 0, indent+2);

  dump_nodes(node->fold, "fold", indent+2);
  
  dump_nodes(node->call, "call", indent+2);
  
  dump_nodes(node->raise, "raise", indent+2);
  
}

template <std::size_t N_PLAYERS>
void expand_all_to_street(Limit::GameTree::GameTreeNodeT<N_PLAYERS>* node, Limit::GameTree::street_t street) {

  if (node == 0) { return; }

  if (node->street == Limit::GameTree::RESULT_STREET) { return; }
  
  if (node->street == street) { return; }
  
  node->expand();

  expand_all_to_street(node->child, street);

  expand_all_to_street(node->fold, street);
  
  expand_all_to_street(node->call, street);
  
  expand_all_to_street(node->raise, street);
  
}

int main() {
  printf("Hallo RPJ\n");

  const Limit::Config::ConfigT<2> config {
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
  
  auto root = Limit::GameTree::GameTreeNodeT<2>::new_root(config);

  expand_all_to_street(root, Limit::GameTree::TURN_STREET);

  int count = 0;

  count_nodes(root, count);

  printf("Found %d nodes\n", count);

  // printf("\n");
  // dump_nodes(root, 0, 0);
}
