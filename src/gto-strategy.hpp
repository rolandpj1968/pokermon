#ifndef GTO_STRATEGY
#define GTO_STRATEGY

#include "types.hpp"

namespace Poker {
  
  namespace Gto {

    // Clamp minimum strategy probability in order to avoid underflow deep in the tree.
    // This should (tm) have minimal impact on final outcome.
    static const double MIN_STRATEGY = 0.000001;

    // Clamp a value to MIN_STRATEGY by stealing from the max value
    static inline void clamp_to_min(double& p, double& max_p) {
      if(p < MIN_STRATEGY) {
	double diff = MIN_STRATEGY - p;
	max_p -= diff;
	p = MIN_STRATEGY;
      }
    }
    
    // Normalise values to sum to 1.0
    static inline void normalise_to_unit_sum(double& fold_v, double& call_v, double& raise_v) {
      double sum_vs = fold_v + call_v + raise_v;
      fold_v /= sum_vs; call_v /= sum_vs; raise_v /= sum_vs;
    }

    // Normalise values to sum to 1.0
    static inline void normalise_to_unit_sum(double& fold_v, double& call_v) {
      double sum_vs = fold_v + call_v;
      fold_v /= sum_vs; call_v /= sum_vs;
    }

    // GTO strategy - two variants depending on whether we can raise or not.
    template <bool CAN_RAISE>
    struct GtoStrategy;

    // Used for fold/call/raise and fold/check/raise nodes.
    //   fold, call, raise each in [0.0, 1.0]
    //   fold + call + raise == 1.0
    template <>
    struct GtoStrategy</*CAN_RAISE*/true> {
      double fold_p;
      double call_p;
      double raise_p;
      
      GtoStrategy() :
	fold_p(1.0/3.0), call_p(1.0/3.0), raise_p(1.0/3.0) {}

      // Adjust strategy according to empirical outcomes - reward the more profitable options and
      //   penalise the less profitable options.
      // @param leeway is in [0.0, +infinity) - the smaller the leeway the more aggressively we adjust
      //    leeway of 0.0 is not a good idea since it will leave the strategy of the worst option at 0.0.
      void adjust(double fold_profit, double call_profit, double raise_profit, double leeway) {
	// Normalise profits to be 0-based.
	double min_profit = std::min(fold_profit, std::min(call_profit, raise_profit));
	// All positive...
	fold_profit -= min_profit; call_profit -= min_profit; raise_profit -= min_profit;
	
	// Normalise profits to sum to 1.0
	normalise_to_unit_sum(fold_profit, call_profit, raise_profit);
	
	// Give some leeway
	fold_profit += leeway; call_profit += leeway; raise_profit += leeway;
	
	// Adjust strategies...
	fold_p *= fold_profit; call_p *= call_profit; raise_p *= raise_profit;
	
	// Strategies must sum to 1.0
	normalise_to_unit_sum(fold_p, call_p, raise_p);
	
	// Clamp minimum probability
	double& fold_call_max_p = fold_p > call_p ? fold_p : call_p;
	double& max_p = fold_call_max_p > raise_p ? fold_call_max_p : raise_p;
	clamp_to_min(fold_p, max_p); clamp_to_min(call_p, max_p); clamp_to_min(raise_p, max_p);
      }
      
    }; // struct GtoStrategy</*CAN_RAISE*/true>

    // Used for fold/call and fold/check nodes.
    //   fold, call each in [0.0, 1.0]
    //   fold + call == 1.0
    template <>
    struct GtoStrategy</*CAN_RAISE*/false> {
      double fold_p;
      double call_p;
      
      GtoStrategy() :
	fold_p(1.0/2.0), call_p(1.0/2.0) {}

      // Adjust strategy according to empirical outcomes - reward the more profitable options and
      //   penalise the less profitable options.
      // @param leeway is in [0.0, +infinity) - the smaller the leeway the more aggressively we adjust
      //    leeway of 0.0 is not a good idea since it will leave the strategy of the worst option at 0.0.
      void adjust(double fold_profit, double call_profit, double leeway) {
	// Normalise profits to be 0-based.
	double min_profit = std::min(fold_profit, call_profit);
	// All positive...
	fold_profit -= min_profit; call_profit -= min_profit;
	
	// Normalise profits to sum to 1.0
	normalise_to_unit_sum(fold_profit, call_profit);
	
	// Give some leeway
	fold_profit += leeway; call_profit += leeway;
	
	// Adjust strategies...
	fold_p *= fold_profit; call_p *= call_profit;
	
	// Strategies must sum to 1.0
	normalise_to_unit_sum(fold_p, call_p);
	
	// Clamp minimum probability
	double& max_p = fold_p > call_p ? fold_p : call_p;
	clamp_to_min(fold_p, max_p); clamp_to_min(call_p, max_p);
      }

    }; // struct GtoStrategy</*CAN_RAISE*/false>
    
  } // namespace Gto
  
} // namespace Poker

#endif //def GTO_STRATEGY
