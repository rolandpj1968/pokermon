#include <algorithm>
#include <cstdio>
#include <utility>
#include <set>

#include "dealer.hpp"
#include "hand-eval.hpp"
#include "normal.hpp"
#include "types.hpp"

using namespace Poker;

int main() {
  std::seed_seq seed{2, 3, 5, 7, 13};
  Dealer::DealerT dealer(seed);

  Normal::init_omaha_hold_normal_index();

  const int N_DEALS = 16;

  for(int deal_no = 0; deal_no < N_DEALS; deal_no++) {
    auto cards = dealer.deal(4+3+1+1);
    
    auto p0_hole = std::make_tuple(CardT(cards[0+0]), CardT(cards[0+1]), CardT(cards[0+2]), CardT(cards[0+3]));
				   
    // auto flop = std::make_tuple(CardT(cards[4]), CardT(cards[4 + 1]), CardT(cards[4 + 2]));
    // auto turn = CardT(cards[4 + 3]);
    // auto river = CardT(cards[4 + 4]);

    auto p0_hole_normal = Normal::omaha_hole_normal(std::get<0>(p0_hole), std::get<1>(p0_hole), std::get<2>(p0_hole), std::get<3>(p0_hole));

    printf("Hole: %c%c/%c%c/%c%c/%c%c - normal %c%c/%c%c/%c%c/%c%c\n",
	   RANK_CHARS[std::get<0>(p0_hole).rank], SUIT_CHARS[std::get<0>(p0_hole).suit], RANK_CHARS[std::get<1>(p0_hole).rank], SUIT_CHARS[std::get<1>(p0_hole).suit], RANK_CHARS[std::get<2>(p0_hole).rank], SUIT_CHARS[std::get<2>(p0_hole).suit], RANK_CHARS[std::get<3>(p0_hole).rank], SUIT_CHARS[std::get<3>(p0_hole).suit],
	   RANK_CHARS[std::get<0>(p0_hole_normal).rank], SUIT_CHARS[std::get<0>(p0_hole_normal).suit], RANK_CHARS[std::get<1>(p0_hole_normal).rank], SUIT_CHARS[std::get<1>(p0_hole_normal).suit], RANK_CHARS[std::get<2>(p0_hole_normal).rank], SUIT_CHARS[std::get<2>(p0_hole_normal).suit], RANK_CHARS[std::get<3>(p0_hole_normal).rank], SUIT_CHARS[std::get<3>(p0_hole_normal).suit]);
    
  }

  printf("\n");

  std::set<std::tuple<CardT, CardT, CardT, CardT>> omaha_hole_normals;
  size_t n_omaha_holes = 0;

  for (u8 c0 = 0; c0 < 52; c0++) {
    U8CardT c0_u8 = U8CardT(c0);
    CardT card0 = CardT(c0_u8.suit(), c0_u8.rank());

    for (u8 c1 = 0; c1 < 52; c1++) {
      if (c1 == c0) {
	continue;
      }
      
      U8CardT c1_u8 = U8CardT(c1);
      CardT card1 = CardT(c1_u8.suit(), c1_u8.rank());
    
      for (u8 c2 = 0; c2 < 52; c2++) {
	if (c2 == c0 || c2 == c1) {
	  continue;
	}
	
	U8CardT c2_u8 = U8CardT(c2);
	CardT card2 = CardT(c2_u8.suit(), c2_u8.rank());
    
	for (u8 c3 = 0; c3 < 52; c3++) {
	  if (c3 == c0 || c3 == c1 || c3 == c2) {
	    continue;
	  }

	  n_omaha_holes++;
	  
	  U8CardT c3_u8 = U8CardT(c3);
	  CardT card3 = CardT(c3_u8.suit(), c3_u8.rank());

	  auto hole_normal = Normal::omaha_hole_normal(card0, card1, card2, card3);

	  int hole_index = Normal::omaha_hole_normal_index(card0, card1, card2, card3);
	  auto hole_normal2 = Normal::omaha_hole_normal_from_index(hole_index);

	  assert(hole_normal == hole_normal2);

	  omaha_hole_normals.insert(hole_normal);
	}
      }
    }
  }

  printf("%zu omaha holes / %zu omaha normal holes\n", n_omaha_holes, omaha_hole_normals.size());
}
