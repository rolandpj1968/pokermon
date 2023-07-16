#ifndef DEALER_HPP
#define DEALER_HPP

#include <random>
#include <vector>

#include "types.hpp"

namespace Poker {
  namespace Dealer {
    // Random card dealer

    struct DealerT {
      std::mt19937 rng;
      std::uniform_int_distribution<int> single_pack_dist;

      DealerT(std::seed_seq& seed):
	rng(seed), single_pack_dist(0, 51) {}

      inline std::vector<Poker::U8CardT> deal(size_t n) {
	std::vector<Poker::U8CardT> cards;
	cards.reserve(n);

	bool dealt[52] = {};

	while(cards.size() < n) {
	  int card = single_pack_dist(rng);

	  if(!dealt[card]) {
	    dealt[card] = true;

	    cards.push_back(Poker::U8CardT((u8)card));
	  }
	}

	return cards;
      }
      
      inline void deal(Poker::U8CardT* cards, size_t n) {
	bool dealt[52] = {};

	size_t n_dealt = 0;
	while(n_dealt < n) {
	  int card = single_pack_dist(rng);

	  if(!dealt[card]) {
	    dealt[card] = true;

	    cards[n_dealt++] = Poker::U8CardT((u8)card);
	  }
	}
      }
    }; // struct DealerT

  }// namespace Dealer
} // namespace Poker

#endif //ndef DEALER_HPP

