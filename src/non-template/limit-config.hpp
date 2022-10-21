#ifndef LIMIT_CONFIG_HPP
#define LIMIT_CONFIG_HPP

// std::size_t
#include <array>

namespace Limit {

  namespace Config {

    struct ConfigT {
      const std::size_t n_players;
      const double small_blind;
      const double big_blind;
      const double preflop_raise;
      const std::size_t max_n_preflop_raises;
      const double flop_raise;
      const std::size_t max_n_flop_raises;
      const double turn_raise;
      const std::size_t max_n_turn_raises;
      const double river_raise;
      const std::size_t max_n_river_raises;
    };
    
  } // namespace Config

} // namespace Limit


#endif //def LIMIT_CONFIG_HPP
