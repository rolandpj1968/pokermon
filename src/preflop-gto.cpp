#include <cstdio>
#include <utility>

#include "dealer.hpp"
#include "hand-eval.hpp"
#include "normal.hpp"
#include "types.hpp"

using namespace Poker;

// Used for fold/call/raise and fold/check/raise nodes.
//   fold, call, raise each in [0.0, 1.0]
//   fold + call + raise == 1.0
struct FoldCallRaiseStrategy {
  double fold_p;
  double call_p;
  double raise_p;

  FoldCallRaiseStrategy() :
    fold_p(1.0/3.0), call_p(1.0/3.0), raise_p(1.0/3.0) {}
};

// Used for fold/call and fold/check nodes.
//   fold, call each in [0.0, 1.0]
//   fold + call == 1.0
struct FoldCallStrategy {
  double fold_p;
  double call_p;

  FoldCallStrategy() :
    fold_p(1.0/2.0), call_p(1.0/2.0) {}
};

// Player 0 (Small Blind) Strategy in heads-up for a particular hole card hand.
// Fold/Call/Check/Raise probabilities for Player 0 heads-up.
// Player 0 is Small Blind (SB) and goes first in heads-up.
// Assume pre-flop pot limit of 4 BB's.
struct HeadsUpP0HoleHandStrategy {
  FoldCallRaiseStrategy open;
  
  FoldCallRaiseStrategy p0_called_p1_raised;

  FoldCallStrategy      p0_called_p1_raised_p0_raised_p1_raised;
  
  FoldCallRaiseStrategy p0_raised_p1_raised;
};

// Player 0 (Small Blind) Strategy
struct HeadsUpP0PreflopStrategy {
  // Note we actually only use hand_strategies[i][j][k] where j >(=) k.
  HeadsUpP0HoleHandStrategy hand_strategies[2][13][13];
};

// Player 1 (Big Blind) Strategy in heads-up for a particular hole card hand.
// Strategy - Fold/Call/Check/Raise probabilities for Player 1 heads-up.
// Player 1 is Small Blind (SB) and goes first in heads-up.
// Assume pre-flop pot limit of 4 BB's.
struct HeadsUpP1HoleHandStrategy {
  FoldCallRaiseStrategy p0_called;

  FoldCallRaiseStrategy p0_called_p1_raised_p0_raised;
  
  FoldCallRaiseStrategy p0_raised;

  FoldCallStrategy      p0_raised_p1_raised_p0_raised;
};

// Player 1 (Small Blind) Strategy
struct HeadsUpP1PreflopStrategy {
  // Note we actually only use hand_strategies[i][j][k] where j >(=) k.
  HeadsUpP1HoleHandStrategy hand_strategies[2][13][13];
};

// Evaluation of a game-tree node.
// p0_profit + p1_profit MUST be 0, so this is somewhat redundant
struct HeadsUpNodeEval {
  // Sum over all hands evaluated of per-hand 'probability of reaching this node'.
  double activity;
  // Sum over all hands of per-hand (product of) 'probability of reaching this node' times 'p0 outcome for the hand'.
  double p0_profit;
  // Sum over all hands of per-hand (product of) 'probability of reaching this node' times 'p1 outcome for the hand'.
  double p1_profit;
};

struct HeadsUpPlayerHoleHandEval {
  HeadsUpNodeEval eval;

  struct { HeadsUpNodeEval eval; } p0_folded;


  struct {
    HeadsUpNodeEval eval;

    struct { HeadsUpNodeEval eval; } p1_folded;
    struct { HeadsUpNodeEval eval; } p1_called;
    
    struct {
      HeadsUpNodeEval eval;
      
      struct { HeadsUpNodeEval eval; } p0_folded;
      struct { HeadsUpNodeEval eval; } p0_called;
	
      struct {
	HeadsUpNodeEval eval;

	struct { HeadsUpNodeEval eval; } p1_folded;
	struct { HeadsUpNodeEval eval; } p1_called;

	struct {
	  HeadsUpNodeEval eval;

	  struct { HeadsUpNodeEval eval; } p0_folded;
	  struct { HeadsUpNodeEval eval; } p0_called;
	  
	} p1_raised;
	
      } p0_raised;
      
    } p1_raised;
    
  } p0_called;

  struct {
    HeadsUpNodeEval eval;

    struct { HeadsUpNodeEval eval; } p1_folded;
    struct { HeadsUpNodeEval eval; } p1_called;

    struct {
      HeadsUpNodeEval eval;
    
      struct { HeadsUpNodeEval eval; } p0_folded;
      struct { HeadsUpNodeEval eval; } p0_called;

      struct {
	HeadsUpNodeEval eval;
      
	struct { HeadsUpNodeEval eval; } p1_folded;
	struct { HeadsUpNodeEval eval; } p1_called;
	
      } p0_raised;
      
    } p1_raised;
    
  } p0_raised;
};

// Player 0 (Small Blind) Strategy
struct HeadsUpPlayerPreflopEval {
  // Note we actually only use hand_evals[i][j][k] where j >(=) k.
  HeadsUpPlayerHoleHandEval hand_evals[2][13][13];
};

enum HeadsUpWinner { P0Wins, P1Wins, P0P1Push };

static const char* WINNER[] = { "P0", "P1", "Push" };

static void update_eval(HeadsUpNodeEval& eval, double p, double p0_profit, double p1_profit) {
  eval.activity += p;
  eval.p0_profit += p * p0_profit;
  eval.p1_profit += p * p1_profit;
}
	     
static void update_evals(HeadsUpNodeEval& p0_eval, HeadsUpNodeEval& p1_eval, double p, double p0_profit, double p1_profit) {
  update_eval(p0_eval, p, p0_profit, p1_profit);
  update_eval(p1_eval, p, p0_profit, p1_profit);
}

static std::pair<double, double> eval_showdown_profits(HeadsUpWinner winner, double bet) {
  double p0_profit = 0.0;
  double p1_profit = 0.0;
  if(winner == P0Wins) {
    p0_profit = +bet;
    p1_profit = -bet;
  } else if(winner == P1Wins) {
    p0_profit = -bet;
    p1_profit = +bet;
  }
  return std::make_pair(p0_profit, p1_profit);
}

static void eval_heads_up_preflop_deal(const HeadsUpP0HoleHandStrategy& p0_strategy, HeadsUpPlayerHoleHandEval& p0_eval, const HeadsUpP1HoleHandStrategy& p1_strategy, HeadsUpPlayerHoleHandEval& p1_eval, HeadsUpWinner winner) {
  double p = 1.0; 
  double p0_profit = 0.0;
  double p1_profit = 0.0;
  
  { // p0_fold
    
    double p0_fold_p = p0_strategy.open.fold_p;
    
    // P0 loses small blind - i.e. 0.5; P1 wins that SB of 0.5
    double p0_fold_p0_profit = -0.5;
    double p0_fold_p1_profit = +0.5;

    update_evals(p0_eval.p0_folded.eval,
		 p1_eval.p0_folded.eval,
		 p0_fold_p,
		 p0_fold_p0_profit,
		 p0_fold_p1_profit);
	       
    p0_profit += p0_fold_p * p0_fold_p0_profit;
    p1_profit += p0_fold_p * p0_fold_p1_profit;
    
  } // p0_fold

  { // p0_call
    
    double p0_call_p = p0_strategy.open.call_p;
    double p0_call_p0_profit = 0.0;
    double p0_call_p1_profit = 0.0;
      
    { // p1_fold
      
      double p0_call_p1_fold_p = p0_call_p * p1_strategy.p0_called.fold_p;
      
      // P1 folds BB - i.e. 1.0, P0 wins that 1.0
      double p0_call_p1_fold_p0_profit = +1.0;
      double p0_call_p1_fold_p1_profit = -1.0;
								       
      update_evals(p0_eval.p0_called.p1_folded.eval,
		   p1_eval.p0_called.p1_folded.eval,
		   p0_call_p1_fold_p,
		   p0_call_p1_fold_p0_profit,
		   p0_call_p1_fold_p1_profit);
								       
      p0_call_p0_profit += p0_call_p1_fold_p * p0_call_p1_fold_p0_profit;
      p0_call_p1_profit += p0_call_p1_fold_p * p0_call_p1_fold_p1_profit;
      
    } // p1_fold

    { // p1_call
      
      double p0_call_p1_call_p = p0_call_p * p1_strategy.p0_called.call_p;

      // Both players have 1.0 in the pot
      auto p0_call_p1_call_p0_p1_profit = eval_showdown_profits(winner, 1.0);
      double p0_call_p1_call_p0_profit = p0_call_p1_call_p0_p1_profit.first;
      double p0_call_p1_call_p1_profit = p0_call_p1_call_p0_p1_profit.second;
      
      update_evals(p0_eval.p0_called.p1_called.eval,
		   p1_eval.p0_called.p1_called.eval,
		   p0_call_p1_call_p,
		   p0_call_p1_call_p0_profit,
		   p0_call_p1_call_p1_profit);
								       
      p0_call_p0_profit += p0_call_p1_call_p * p0_call_p1_call_p0_profit;
      p0_call_p1_profit += p0_call_p1_call_p * p0_call_p1_call_p1_profit;
      
    } // p1_call

    //#ifdef no_p1_raise
    
    { // p1_raise
      double p0_call_p1_raise_p = p0_call_p * p1_strategy.p0_called.raise_p;

      double p0_call_p1_raise_p0_profit = 0.0;
      double p0_call_p1_raise_p1_profit = 0.0;

      { // p0_fold
	
        double p0_call_p1_raise_p0_fold_p = p0_call_p1_raise_p * p0_strategy.p0_called_p1_raised.fold_p;

	// P0 loses current bet of 1.0; P1 wins it
	double p0_call_p1_raise_p0_fold_p0_profit = -1.0;
	double p0_call_p1_raise_p0_fold_p1_profit = +1.0;
	
	update_evals(p0_eval.p0_called.p1_raised.p0_folded.eval,
		     p1_eval.p0_called.p1_raised.p0_folded.eval,
		     p0_call_p1_raise_p0_fold_p,
		     p0_call_p1_raise_p0_fold_p0_profit,
		     p0_call_p1_raise_p0_fold_p1_profit);
								       
	p0_call_p1_raise_p0_profit += p0_call_p1_raise_p0_fold_p * p0_call_p1_raise_p0_fold_p0_profit;
	p0_call_p1_raise_p1_profit += p0_call_p1_raise_p0_fold_p * p0_call_p1_raise_p0_fold_p1_profit;
	  
      } // p0_fold

      { // p0_call
	
        double p0_call_p1_raise_p0_call_p = p0_call_p1_raise_p * p0_strategy.p0_called_p1_raised.call_p;

	// Both players have 2.0 in the pot
	auto p0_call_p1_raise_p0_call_p0_p1_profit = eval_showdown_profits(winner, 2.0);
	double p0_call_p1_raise_p0_call_p0_profit = p0_call_p1_raise_p0_call_p0_p1_profit.first;
	double p0_call_p1_raise_p0_call_p1_profit = p0_call_p1_raise_p0_call_p0_p1_profit.second;
	
	update_evals(p0_eval.p0_called.p1_raised.p0_called.eval,
		     p1_eval.p0_called.p1_raised.p0_called.eval,
		     p0_call_p1_raise_p0_call_p,
		     p0_call_p1_raise_p0_call_p0_profit,
		     p0_call_p1_raise_p0_call_p1_profit);
								       
	p0_call_p1_raise_p0_profit += p0_call_p1_raise_p0_call_p * p0_call_p1_raise_p0_call_p0_profit;
	p0_call_p1_raise_p1_profit += p0_call_p1_raise_p0_call_p * p0_call_p1_raise_p0_call_p1_profit;
	  
      } // p0_call

      { // p0_raise
	
        double p0_call_p1_raise_p0_raise_p = p0_call_p1_raise_p * p0_strategy.p0_called_p1_raised.raise_p;

	double p0_call_p1_raise_p0_raise_p0_profit = 0.0;
	double p0_call_p1_raise_p0_raise_p1_profit = 0.0;

	{ // p1_fold
	  
	  double p0_call_p1_raise_p0_raise_p1_fold_p = p0_call_p1_raise_p0_raise_p * p1_strategy.p0_called_p1_raised_p0_raised.fold_p;

	  // P1 has 2.0 in the pot; P0 wins this; P1 loses it.
	  double p0_call_p1_raise_p0_raise_p1_fold_p0_profit = +2.0;
	  double p0_call_p1_raise_p0_raise_p1_fold_p1_profit = -2.0;

	  update_evals(p0_eval.p0_called.p1_raised.p0_raised.p1_folded.eval,
		       p1_eval.p0_called.p1_raised.p0_raised.p1_folded.eval,
		       p0_call_p1_raise_p0_raise_p1_fold_p,
		       p0_call_p1_raise_p0_raise_p1_fold_p0_profit,
		       p0_call_p1_raise_p0_raise_p1_fold_p1_profit);
								       
	  p0_call_p1_raise_p0_raise_p0_profit += p0_call_p1_raise_p0_raise_p1_fold_p * p0_call_p1_raise_p0_raise_p1_fold_p0_profit;
	  p0_call_p1_raise_p0_raise_p1_profit += p0_call_p1_raise_p0_raise_p1_fold_p * p0_call_p1_raise_p0_raise_p1_fold_p1_profit;
	  
	} // p1_fold

	{ // p1_call
	  
	  double p0_call_p1_raise_p0_raise_p1_call_p = p0_call_p1_raise_p0_raise_p * p1_strategy.p0_called_p1_raised_p0_raised.call_p;

	  // Both players have 3.0 in the pot
	  auto p0_call_p1_raise_p0_raise_p1_call_p0_p1_profit = eval_showdown_profits(winner, 3.0);
	  double p0_call_p1_raise_p0_raise_p1_call_p0_profit = p0_call_p1_raise_p0_raise_p1_call_p0_p1_profit.first;
	  double p0_call_p1_raise_p0_raise_p1_call_p1_profit = p0_call_p1_raise_p0_raise_p1_call_p0_p1_profit.second;

	  update_evals(p0_eval.p0_called.p1_raised.p0_raised.p1_called.eval,
		       p1_eval.p0_called.p1_raised.p0_raised.p1_called.eval,
		       p0_call_p1_raise_p0_raise_p1_call_p,
		       p0_call_p1_raise_p0_raise_p1_call_p0_profit,
		       p0_call_p1_raise_p0_raise_p1_call_p1_profit);
								       
	  p0_call_p1_raise_p0_raise_p0_profit += p0_call_p1_raise_p0_raise_p1_call_p * p0_call_p1_raise_p0_raise_p1_call_p0_profit;
	  p0_call_p1_raise_p0_raise_p1_profit += p0_call_p1_raise_p0_raise_p1_call_p * p0_call_p1_raise_p0_raise_p1_call_p1_profit;
	  
	} // p1_call

	{ // p1_raise
	  
	  double p0_call_p1_raise_p0_raise_p1_raise_p = p0_call_p1_raise_p0_raise_p * p1_strategy.p0_called_p1_raised_p0_raised.raise_p;

	  double p0_call_p1_raise_p0_raise_p1_raise_p0_profit = 0.0;
	  double p0_call_p1_raise_p0_raise_p1_raise_p1_profit = 0.0;

	  { // p0_fold
	    
	    double p0_call_p1_raise_p0_raise_p1_raise_p0_fold_p = p0_call_p1_raise_p0_raise_p1_raise_p * p0_strategy.p0_called_p1_raised_p0_raised_p1_raised.fold_p;

	    // P0 has 3.0 in the pot; he wins this; P1 wins it.
	    double p0_call_p1_raise_p0_raise_p1_raise_p0_fold_p0_profit = -3.0;
	    double p0_call_p1_raise_p0_raise_p1_raise_p0_fold_p1_profit = +3.0;
	    
	    update_evals(p0_eval.p0_called.p1_raised.p0_raised.p1_raised.p0_folded.eval,
			 p1_eval.p0_called.p1_raised.p0_raised.p1_raised.p0_folded.eval,
			 p0_call_p1_raise_p0_raise_p1_raise_p0_fold_p,
			 p0_call_p1_raise_p0_raise_p1_raise_p0_fold_p0_profit,
			 p0_call_p1_raise_p0_raise_p1_raise_p0_fold_p1_profit);
								       
	    p0_call_p1_raise_p0_raise_p1_raise_p0_profit += p0_call_p1_raise_p0_raise_p1_raise_p0_fold_p * p0_call_p1_raise_p0_raise_p1_raise_p0_fold_p0_profit;
	    p0_call_p1_raise_p0_raise_p1_raise_p1_profit += p0_call_p1_raise_p0_raise_p1_raise_p0_fold_p * p0_call_p1_raise_p0_raise_p1_raise_p0_fold_p1_profit;
	  
	  } // p0_fold
	  
	  { // p0_call
	    
	    double p0_call_p1_raise_p0_raise_p1_raise_p0_call_p = p0_call_p1_raise_p0_raise_p1_raise_p * p0_strategy.p0_called_p1_raised_p0_raised_p1_raised.call_p;

	    // Both players have 4.0 in the pot
	    auto p0_call_p1_raise_p0_raise_p1_raise_p0_call_p0_p1_profit = eval_showdown_profits(winner, 4.0);
	    double p0_call_p1_raise_p0_raise_p1_raise_p0_call_p0_profit = p0_call_p1_raise_p0_raise_p1_raise_p0_call_p0_p1_profit.first;
	    double p0_call_p1_raise_p0_raise_p1_raise_p0_call_p1_profit = p0_call_p1_raise_p0_raise_p1_raise_p0_call_p0_p1_profit.second;
	    
	    update_evals(p0_eval.p0_called.p1_raised.p0_raised.p1_raised.p0_called.eval,
			 p1_eval.p0_called.p1_raised.p0_raised.p1_raised.p0_called.eval,
			 p0_call_p1_raise_p0_raise_p1_raise_p0_call_p,
			 p0_call_p1_raise_p0_raise_p1_raise_p0_call_p0_profit,
			 p0_call_p1_raise_p0_raise_p1_raise_p0_call_p1_profit);
								       
	    p0_call_p1_raise_p0_raise_p1_raise_p0_profit += p0_call_p1_raise_p0_raise_p1_raise_p0_call_p * p0_call_p1_raise_p0_raise_p1_raise_p0_call_p0_profit;
	    p0_call_p1_raise_p0_raise_p1_raise_p1_profit += p0_call_p1_raise_p0_raise_p1_raise_p0_call_p * p0_call_p1_raise_p0_raise_p1_raise_p0_call_p1_profit;
	  
	  } // p0_call
	  
	  update_evals(p0_eval.p0_called.p1_raised.p0_raised.p1_raised.eval,
		       p1_eval.p0_called.p1_raised.p0_raised.p1_raised.eval,
		       p0_call_p1_raise_p0_raise_p1_raise_p,
		       p0_call_p1_raise_p0_raise_p1_raise_p0_profit,
		       p0_call_p1_raise_p0_raise_p1_raise_p1_profit);
								       
	  p0_call_p1_raise_p0_raise_p0_profit += p0_call_p1_raise_p0_raise_p1_raise_p * p0_call_p1_raise_p0_raise_p1_raise_p0_profit;
	  p0_call_p1_raise_p0_raise_p1_profit += p0_call_p1_raise_p0_raise_p1_raise_p * p0_call_p1_raise_p0_raise_p1_raise_p1_profit;
	  
	} // p1_raise

	update_evals(p0_eval.p0_called.p1_raised.p0_raised.eval,
		     p1_eval.p0_called.p1_raised.p0_raised.eval,
		     p0_call_p1_raise_p0_raise_p,
		     p0_call_p1_raise_p0_raise_p0_profit,
		     p0_call_p1_raise_p0_raise_p1_profit);
								       
	p0_call_p1_raise_p0_profit += p0_call_p1_raise_p0_raise_p * p0_call_p1_raise_p0_raise_p0_profit;
	p0_call_p1_raise_p1_profit += p0_call_p1_raise_p0_raise_p * p0_call_p1_raise_p0_raise_p1_profit;
	  
      } // p0_raise

      update_evals(p0_eval.p0_called.p1_raised.eval,
		   p1_eval.p0_called.p1_raised.eval,
		   p0_call_p1_raise_p,
		   p0_call_p1_raise_p0_profit,
		   p0_call_p1_raise_p1_profit);
								       
      p0_call_p0_profit += p0_call_p1_raise_p * p0_call_p1_raise_p0_profit;
      p0_call_p1_profit += p0_call_p1_raise_p * p0_call_p1_raise_p1_profit;
      
    } // p1_call

    //#endif // no_p1_raise
    
    update_evals(p0_eval.p0_called.eval,
		 p1_eval.p0_called.eval,
		 p0_call_p,
		 p0_call_p0_profit,
		 p0_call_p1_profit);
	       
    p0_profit += p0_call_p * p0_call_p0_profit;
    p1_profit += p0_call_p * p0_call_p1_profit;
    
  } // p0_call
  
  //#if 0
  
  // p0_raise
  {
    double p0_raise_p = p0_strategy.open.raise_p;
    double p0_raise_p0_profit = 0.0;
    double p0_raise_p1_profit = 0.0;

    { // p1_fold
      
      double p0_raise_p1_fold_p = p0_raise_p * p1_strategy.p0_raised.fold_p;
      
      // P1 folds BB - i.e. 1.0, P0 wins that 1.0
      double p0_raise_p1_fold_p0_profit = +1.0;
      double p0_raise_p1_fold_p1_profit = -1.0;
								       
      update_evals(p0_eval.p0_raised.p1_folded.eval,
		   p1_eval.p0_raised.p1_folded.eval,
		   p0_raise_p1_fold_p,
		   p0_raise_p1_fold_p0_profit,
		   p0_raise_p1_fold_p1_profit);
								       
      p0_raise_p0_profit += p0_raise_p1_fold_p * p0_raise_p1_fold_p0_profit;
      p0_raise_p1_profit += p0_raise_p1_fold_p * p0_raise_p1_fold_p1_profit;
      
    } // p1_fold
      
    { // p1_call
      
      double p0_raise_p1_call_p = p0_raise_p * p1_strategy.p0_raised.call_p;

      // Both players have 2.0 in the pot
      auto p0_raise_p1_call_p0_p1_profit = eval_showdown_profits(winner, 2.0);
      double p0_raise_p1_call_p0_profit = p0_raise_p1_call_p0_p1_profit.first;
      double p0_raise_p1_call_p1_profit = p0_raise_p1_call_p0_p1_profit.second;
      
      update_evals(p0_eval.p0_raised.p1_called.eval,
		   p1_eval.p0_raised.p1_called.eval,
		   p0_raise_p1_call_p,
		   p0_raise_p1_call_p0_profit,
		   p0_raise_p1_call_p1_profit);
								       
      p0_raise_p0_profit += p0_raise_p1_call_p * p0_raise_p1_call_p0_profit;
      p0_raise_p1_profit += p0_raise_p1_call_p * p0_raise_p1_call_p1_profit;
      
    } // p1_call
      
    { // p1_raise
      
      double p0_raise_p1_raise_p = p0_raise_p * p1_strategy.p0_raised.raise_p;

      double p0_raise_p1_raise_p0_profit = 0.0;
      double p0_raise_p1_raise_p1_profit = 0.0;

      { // p0_fold
	
        double p0_raise_p1_raise_p0_fold_p = p0_raise_p1_raise_p * p0_strategy.p0_raised_p1_raised.fold_p;

	// P0 loses current bet of 2.0; P1 wins it
	double p0_raise_p1_raise_p0_fold_p0_profit = -2.0;
	double p0_raise_p1_raise_p0_fold_p1_profit = +2.0;
	
	update_evals(p0_eval.p0_raised.p1_raised.p0_folded.eval,
		     p1_eval.p0_raised.p1_raised.p0_folded.eval,
		     p0_raise_p1_raise_p0_fold_p,
		     p0_raise_p1_raise_p0_fold_p0_profit,
		     p0_raise_p1_raise_p0_fold_p1_profit);
								       
	p0_raise_p1_raise_p0_profit += p0_raise_p1_raise_p0_fold_p * p0_raise_p1_raise_p0_fold_p0_profit;
	p0_raise_p1_raise_p1_profit += p0_raise_p1_raise_p0_fold_p * p0_raise_p1_raise_p0_fold_p1_profit;
	  
      } // p0_fold

      { // p0_call
	
        double p0_raise_p1_raise_p0_call_p = p0_raise_p1_raise_p * p0_strategy.p0_raised_p1_raised.call_p;

	// Both players have 3.0 in the pot
	auto p0_raise_p1_raise_p0_call_p0_p1_profit = eval_showdown_profits(winner, 3.0);
	double p0_raise_p1_raise_p0_call_p0_profit = p0_raise_p1_raise_p0_call_p0_p1_profit.first;
	double p0_raise_p1_raise_p0_call_p1_profit = p0_raise_p1_raise_p0_call_p0_p1_profit.second;
	
	update_evals(p0_eval.p0_raised.p1_raised.p0_called.eval,
		     p1_eval.p0_raised.p1_raised.p0_called.eval,
		     p0_raise_p1_raise_p0_call_p,
		     p0_raise_p1_raise_p0_call_p0_profit,
		     p0_raise_p1_raise_p0_call_p1_profit);
								       
	p0_raise_p1_raise_p0_profit += p0_raise_p1_raise_p0_call_p * p0_raise_p1_raise_p0_call_p0_profit;
	p0_raise_p1_raise_p1_profit += p0_raise_p1_raise_p0_call_p * p0_raise_p1_raise_p0_call_p1_profit;
	  
      } // p0_call

      { // p0_raise
	
        double p0_raise_p1_raise_p0_raise_p = p0_raise_p1_raise_p * p0_strategy.p0_raised_p1_raised.raise_p;

	double p0_raise_p1_raise_p0_raise_p0_profit = 0.0;
	double p0_raise_p1_raise_p0_raise_p1_profit = 0.0;

	{ // p1_fold
	  
	  double p0_raise_p1_raise_p0_raise_p1_fold_p = p0_raise_p1_raise_p0_raise_p * p1_strategy.p0_raised_p1_raised_p0_raised.fold_p;

	  // P1 has 3.0 in the pot; P0 wins this; P1 loses it.
	  double p0_raise_p1_raise_p0_raise_p1_fold_p0_profit = +3.0;
	  double p0_raise_p1_raise_p0_raise_p1_fold_p1_profit = -3.0;

	  update_evals(p0_eval.p0_raised.p1_raised.p0_raised.p1_folded.eval,
		       p1_eval.p0_raised.p1_raised.p0_raised.p1_folded.eval,
		       p0_raise_p1_raise_p0_raise_p1_fold_p,
		       p0_raise_p1_raise_p0_raise_p1_fold_p0_profit,
		       p0_raise_p1_raise_p0_raise_p1_fold_p1_profit);
								       
	  p0_raise_p1_raise_p0_raise_p0_profit += p0_raise_p1_raise_p0_raise_p1_fold_p * p0_raise_p1_raise_p0_raise_p1_fold_p0_profit;
	  p0_raise_p1_raise_p0_raise_p1_profit += p0_raise_p1_raise_p0_raise_p1_fold_p * p0_raise_p1_raise_p0_raise_p1_fold_p1_profit;
	  
	} // p1_fold

	{ // p1_call
	  
	  double p0_raise_p1_raise_p0_raise_p1_call_p = p0_raise_p1_raise_p0_raise_p * p1_strategy.p0_raised_p1_raised_p0_raised.call_p;

	  // Both players have 4.0 in the pot
	  auto p0_raise_p1_raise_p0_raise_p1_call_p0_p1_profit = eval_showdown_profits(winner, 4.0);
	  double p0_raise_p1_raise_p0_raise_p1_call_p0_profit = p0_raise_p1_raise_p0_raise_p1_call_p0_p1_profit.first;
	  double p0_raise_p1_raise_p0_raise_p1_call_p1_profit = p0_raise_p1_raise_p0_raise_p1_call_p0_p1_profit.second;

	  update_evals(p0_eval.p0_raised.p1_raised.p0_raised.p1_called.eval,
		       p1_eval.p0_raised.p1_raised.p0_raised.p1_called.eval,
		       p0_raise_p1_raise_p0_raise_p1_call_p,
		       p0_raise_p1_raise_p0_raise_p1_call_p0_profit,
		       p0_raise_p1_raise_p0_raise_p1_call_p1_profit);
								       
	  p0_raise_p1_raise_p0_raise_p0_profit += p0_raise_p1_raise_p0_raise_p1_call_p * p0_raise_p1_raise_p0_raise_p1_call_p0_profit;
	  p0_raise_p1_raise_p0_raise_p1_profit += p0_raise_p1_raise_p0_raise_p1_call_p * p0_raise_p1_raise_p0_raise_p1_call_p1_profit;
	  
	} // p1_call

	update_evals(p0_eval.p0_raised.p1_raised.p0_raised.eval,
		     p1_eval.p0_raised.p1_raised.p0_raised.eval,
		     p0_raise_p1_raise_p0_raise_p,
		     p0_raise_p1_raise_p0_raise_p0_profit,
		     p0_raise_p1_raise_p0_raise_p1_profit);
								       
	p0_raise_p1_raise_p0_profit += p0_raise_p1_raise_p0_raise_p * p0_raise_p1_raise_p0_raise_p0_profit;
	p0_raise_p1_raise_p1_profit += p0_raise_p1_raise_p0_raise_p * p0_raise_p1_raise_p0_raise_p1_profit;
	  
      } // p0_raise

      update_evals(p0_eval.p0_raised.p1_raised.eval,
		   p1_eval.p0_raised.p1_raised.eval,
		   p0_raise_p1_raise_p,
		   p0_raise_p1_raise_p0_profit,
		   p0_raise_p1_raise_p1_profit);
								       
      p0_raise_p0_profit += p0_raise_p1_raise_p * p0_raise_p1_raise_p0_profit;
      p0_raise_p1_profit += p0_raise_p1_raise_p * p0_raise_p1_raise_p1_profit;
      
    } // p1_raise
      
    update_evals(p0_eval.p0_raised.eval,
		 p1_eval.p0_raised.eval,
		 p0_raise_p,
		 p0_raise_p0_profit,
		 p0_raise_p1_profit);
	       
    p0_profit += p0_raise_p * p0_raise_p0_profit;
    p1_profit += p0_raise_p * p0_raise_p1_profit;
    
  } // p0_raise

  //#endif // 0
  
  update_evals(p0_eval.eval,
	       p1_eval.eval,
	       p,
	       p0_profit,
	       p1_profit);
}

static void dump_fold_call_raise_strategy(const FoldCallRaiseStrategy& strategy) {
  printf("fold  %.4f call  %.4f raise %.4f", strategy.fold_p, strategy.call_p, strategy.raise_p);
}

static void dump_fold_call_strategy(const FoldCallStrategy& strategy) {
  printf("fold  %.4f call  %.4f", strategy.fold_p, strategy.call_p);
}

static void dump_p0_hand_strategy(int rank1, int rank2, bool suited, const HeadsUpP0HoleHandStrategy& hand_strategy) {
  printf("%c%c%c\n", RANK_CHARS[rank1], RANK_CHARS[rank2], (suited ? 's' : 'o'));
  printf("  open:                   "); dump_fold_call_raise_strategy(hand_strategy.open); printf("\n");
  printf("  call-raise:             "); dump_fold_call_raise_strategy(hand_strategy.p0_called_p1_raised); printf("\n");
  printf("  call-raise-raise-raise: "); dump_fold_call_strategy(hand_strategy.p0_called_p1_raised_p0_raised_p1_raised); printf("\n");
  printf("  raise-raise:            "); dump_fold_call_raise_strategy(hand_strategy.p0_raised_p1_raised); printf("\n");
}

static void dump_p1_hand_strategy(int rank1, int rank2, bool suited, const HeadsUpP1HoleHandStrategy& hand_strategy) {
  printf("%c%c%c\n", RANK_CHARS[rank1], RANK_CHARS[rank2], (suited ? 's' : 'o'));
  printf("  call:                   "); dump_fold_call_raise_strategy(hand_strategy.p0_called); printf("\n");
  printf("  call-raise-raise:       "); dump_fold_call_raise_strategy(hand_strategy.p0_called_p1_raised_p0_raised); printf("\n");
  printf("  raise:                  "); dump_fold_call_raise_strategy(hand_strategy.p0_raised); printf("\n");
  printf("  raise-raise-raise:      "); dump_fold_call_strategy(hand_strategy.p0_raised_p1_raised_p0_raised); printf("\n");
}

static void dump_p0_strategy(const HeadsUpP0PreflopStrategy& p0_strategy) {

  printf("Player 0 - Small Blind - Strategy:\n\n");
  
  // Pocket pairs
  bool suited = false;
  for(RankT rank = Ace; rank > AceLow; rank = (RankT)(rank-1)) {
    int rank1 = rank == Ace ? AceLow : rank;

    dump_p0_hand_strategy(rank1, rank1, suited, p0_strategy.hand_strategies[suited][rank1][rank1]);
  }
  
  printf("\n\n");
  
  // Suited
  suited = true;
  for(RankT rank_hi = Ace; rank_hi > AceLow; rank_hi = (RankT)(rank_hi-1)) {
    int rank1 = rank_hi == Ace ? 0 : rank_hi;
    
    for(RankT rank_lo = (RankT)(rank_hi-1); rank_lo > AceLow; rank_lo = (RankT)(rank_lo-1)) {
      int rank2 = rank_lo == Ace ? 0 : rank_lo;
  
      dump_p0_hand_strategy(rank1, rank2, suited, p0_strategy.hand_strategies[suited][rank1][rank2]);
    }

    printf("\n");
  }

  printf("\n\n");
  
  // Off-suit
  suited = false;
  for(RankT rank_hi = Ace; rank_hi > AceLow; rank_hi = (RankT)(rank_hi-1)) {
    int rank1 = rank_hi == Ace ? 0 : rank_hi;
    
    for(RankT rank_lo = (RankT)(rank_hi-1); rank_lo > AceLow; rank_lo = (RankT)(rank_lo-1)) {
      int rank2 = rank_lo == Ace ? 0 : rank_lo;
  
      dump_p0_hand_strategy(rank1, rank2, suited, p0_strategy.hand_strategies[suited][rank1][rank2]);
    }

    printf("\n");
  }
}

static void dump_p1_strategy(const HeadsUpP1PreflopStrategy& p1_strategy) {

  printf("Player 1 - Big Blind - Strategy:\n\n");
  
  // Pocket pairs
  bool suited = false;
  for(RankT rank = Ace; rank > AceLow; rank = (RankT)(rank-1)) {
    int rank1 = rank == Ace ? AceLow : rank;

    dump_p1_hand_strategy(rank1, rank1, suited, p1_strategy.hand_strategies[suited][rank1][rank1]);
  }
  
  printf("\n\n");
  
  // Suited
  suited = true;
  for(RankT rank_hi = Ace; rank_hi > AceLow; rank_hi = (RankT)(rank_hi-1)) {
    int rank1 = rank_hi == Ace ? 0 : rank_hi;
    
    for(RankT rank_lo = (RankT)(rank_hi-1); rank_lo > AceLow; rank_lo = (RankT)(rank_lo-1)) {
      int rank2 = rank_lo == Ace ? 0 : rank_lo;
  
      dump_p1_hand_strategy(rank1, rank2, suited, p1_strategy.hand_strategies[suited][rank1][rank2]);
    }

    printf("\n");
  }

  printf("\n\n");
  
  // Off-suit
  suited = false;
  for(RankT rank_hi = Ace; rank_hi > AceLow; rank_hi = (RankT)(rank_hi-1)) {
    int rank1 = rank_hi == Ace ? 0 : rank_hi;
    
    for(RankT rank_lo = (RankT)(rank_hi-1); rank_lo > AceLow; rank_lo = (RankT)(rank_lo-1)) {
      int rank2 = rank_lo == Ace ? 0 : rank_lo;
  
      dump_p1_hand_strategy(rank1, rank2, suited, p1_strategy.hand_strategies[suited][rank1][rank2]);
    }

    printf("\n");
  }
}

static void dump_hand_eval(int rank1, int rank2, bool suited, const HeadsUpPlayerHoleHandEval& hand_eval) {
  if(hand_eval.eval.activity != 0.0) {
  printf("%c%c%c", RANK_CHARS[rank1], RANK_CHARS[rank2], (suited ? 's' : 'o'));
  printf(" activity: %9.4lf p0 %9.4lf p1 %9.4lf\n", hand_eval.eval.activity, hand_eval.eval.p0_profit, hand_eval.eval.p1_profit);
  }
}

static void dump_player_eval(const HeadsUpPlayerPreflopEval& eval) {
  // Pocket pairs
  bool suited = false;
  for(RankT rank = Ace; rank > AceLow; rank = (RankT)(rank-1)) {
    int rank1 = rank == Ace ? AceLow : rank;

    dump_hand_eval(rank1, rank1, suited, eval.hand_evals[suited][rank1][rank1]);
  }
  
  printf("\n\n");
  
  // Suited
  suited = true;
  for(RankT rank_hi = Ace; rank_hi > AceLow; rank_hi = (RankT)(rank_hi-1)) {
    int rank1 = rank_hi == Ace ? 0 : rank_hi;
    
    for(RankT rank_lo = (RankT)(rank_hi-1); rank_lo > AceLow; rank_lo = (RankT)(rank_lo-1)) {
      int rank2 = rank_lo == Ace ? 0 : rank_lo;
  
      dump_hand_eval(rank1, rank2, suited, eval.hand_evals[suited][rank1][rank2]);
    }

    printf("\n");
  }

  printf("\n\n");
  
  // Off-suit
  suited = false;
  for(RankT rank_hi = Ace; rank_hi > AceLow; rank_hi = (RankT)(rank_hi-1)) {
    int rank1 = rank_hi == Ace ? 0 : rank_hi;
    
    for(RankT rank_lo = (RankT)(rank_hi-1); rank_lo > AceLow; rank_lo = (RankT)(rank_lo-1)) {
      int rank2 = rank_lo == Ace ? 0 : rank_lo;
  
      dump_hand_eval(rank1, rank2, suited, eval.hand_evals[suited][rank1][rank2]);
    }

    printf("\n");
  }
}

static void evaluate_heads_up_preflop_strategies(const HeadsUpP0PreflopStrategy& p0_strategy, const HeadsUpP1PreflopStrategy p1_strategy, int N_DEALS) {
  if(false) {
    printf("Evaluating preflop strategies\n\n");
    dump_p0_strategy(p0_strategy);
    printf("\n\n");
    dump_p1_strategy(p1_strategy);
  }

  HeadsUpPlayerPreflopEval p0_eval;
  HeadsUpPlayerPreflopEval p1_eval;

  std::seed_seq seed{1, 2, 3, 4, 5};
  Poker::Dealer::DealerT dealer(seed);
  
  for(int deal_no = 0; deal_no < N_DEALS; deal_no++) {
    auto cards = dealer.deal(2+2+3+1+1);

    auto p0_hole = std::make_pair(Poker::CardT(cards[0+0]), Poker::CardT(cards[0+1]));
    auto p1_hole = std::make_pair(Poker::CardT(cards[2+0]), Poker::CardT(cards[2+1]));

    auto p0_hole_norm = Poker::holdem_normal(p0_hole.first, p0_hole.second);
    auto p1_hole_norm = Poker::holdem_normal(p1_hole.first, p1_hole.second);

    if(false) {
      printf("Deal: p0 %c%c+%c%c p0-norm %c%c+%c%c p1 %c%c+%c%c p1-norm %c%c+%c%c\n",
	     RANK_CHARS[p0_hole.first.rank], SUIT_CHARS[p0_hole.first.suit], RANK_CHARS[p0_hole.second.rank], SUIT_CHARS[p0_hole.second.suit], 
	     RANK_CHARS[p0_hole_norm.first.rank], SUIT_CHARS[p0_hole_norm.first.suit], RANK_CHARS[p0_hole_norm.second.rank], SUIT_CHARS[p0_hole_norm.second.suit], 
	     RANK_CHARS[p1_hole.first.rank], SUIT_CHARS[p1_hole.first.suit], RANK_CHARS[p1_hole.second.rank], SUIT_CHARS[p1_hole.second.suit], 
	     RANK_CHARS[p1_hole_norm.first.rank], SUIT_CHARS[p1_hole_norm.first.suit], RANK_CHARS[p1_hole_norm.second.rank], SUIT_CHARS[p1_hole_norm.second.suit]);
    }
    
    HeadsUpWinner winner;
    {
      auto flop = std::make_tuple(Poker::CardT(cards[2*2]), Poker::CardT(cards[2*2 + 1]), Poker::CardT(cards[2*2 + 2]));
      auto turn = Poker::CardT(cards[2*2 + 3]);
      auto river = Poker::CardT(cards[2*2 + 4]);

      auto p0_hand_eval = Poker::HandEval::eval_hand(p0_hole, flop, turn, river);
      auto p1_hand_eval = Poker::HandEval::eval_hand(p1_hole, flop, turn, river);
      
      if(p0_hand_eval > p1_hand_eval) {
	winner = P0Wins;
      } else if(p1_hand_eval > p0_hand_eval) {
	winner = P1Wins;
      } else {
	winner = P0P1Push;
      }

      if(false) {
	printf("           flop %c%c+%c%c+%c%c turn %c%c river %c%c\n",
	       RANK_CHARS[std::get<0>(flop).rank], SUIT_CHARS[std::get<0>(flop).suit], RANK_CHARS[std::get<1>(flop).rank], SUIT_CHARS[std::get<1>(flop).suit], RANK_CHARS[std::get<2>(flop).rank], SUIT_CHARS[std::get<2>(flop).suit],
	       RANK_CHARS[turn.rank], SUIT_CHARS[turn.suit],
	       RANK_CHARS[river.rank], SUIT_CHARS[river.suit]
	       );
	printf("  p0 hand %s p1 hand %s winner %s\n", HAND_EVALS[p0_hand_eval.first], HAND_EVALS[p1_hand_eval.first], WINNER[winner]);
      }
    }

    bool p0_is_suited = p0_hole_norm.first.suit == p0_hole_norm.second.suit;
    RankT p0_rank1 = p0_hole_norm.first.rank == Ace ? AceLow : p0_hole_norm.first.rank;
    RankT p0_rank2 = p0_hole_norm.second.rank == Ace ? AceLow : p0_hole_norm.second.rank;

    const HeadsUpP0HoleHandStrategy& p0_hand_strategy = p0_strategy.hand_strategies[p0_is_suited][p0_rank1][p0_rank2];
    HeadsUpPlayerHoleHandEval& p0_hand_eval = p0_eval.hand_evals[p0_is_suited][p0_rank1][p0_rank2];
    
    bool p1_is_suited = p1_hole_norm.first.suit == p1_hole_norm.second.suit;
    RankT p1_rank1 = p1_hole_norm.first.rank == Ace ? AceLow : p1_hole_norm.first.rank;
    RankT p1_rank2 = p1_hole_norm.second.rank == Ace ? AceLow : p1_hole_norm.second.rank;

    const HeadsUpP1HoleHandStrategy& p1_hand_strategy = p1_strategy.hand_strategies[p1_is_suited][p1_rank1][p1_rank2];
    HeadsUpPlayerHoleHandEval& p1_hand_eval = p1_eval.hand_evals[p1_is_suited][p1_rank1][p1_rank2];

    eval_heads_up_preflop_deal(p0_hand_strategy, p0_hand_eval, p1_hand_strategy, p1_hand_eval, winner);
  }

  if(true) {
    // What is the outcome
    printf("Player 0 - Small Blind - outcomes\n\n");
    dump_player_eval(p0_eval);
    printf("\n\n");
    printf("Player 1 - Big Blind - outcomes\n\n");
    dump_player_eval(p1_eval);
    printf("\n\n");
  }
}

int main() {
  int N_DEALS = 100000; //10608/*52*51*4*/;
  
  HeadsUpP0PreflopStrategy p0_strategy;
  HeadsUpP1PreflopStrategy p1_strategy;

  evaluate_heads_up_preflop_strategies(p0_strategy, p1_strategy, N_DEALS);
  
  return 0;
}
