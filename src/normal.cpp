#include <algorithm>
#include <cassert>
#include <map>
#include <utility>

#include "types.hpp"
#include "normal.hpp"
#include "util.hpp"

using namespace Poker;

static bool OMAHA_HOLD_NORMAL_INDEX_INITED = false;
static int OMAHA_HOLD_NORMAL_INDEX[52][52][52][52];
static std::tuple<CardT, CardT, CardT, CardT> OMAHA_HOLE_NORMAL_FROM_INDEX[Poker::Normal::N_OMAHA_HOLE_NORMALS];

void Poker::Normal::init_omaha_hold_normal_index() {
  std::map<std::tuple<CardT, CardT, CardT, CardT>, int> omaha_hole_normal_to_index;
  int next_index = 0;

  for (u8 c0 = 0; c0 < 52; c0++) {
    U8CardT c0_u8 = U8CardT(c0);
    CardT card0 = CardT(c0_u8.suit(), c0_u8.rank());

    for (u8 c1 = 0; c1 < 52; c1++) {
      bool c1_invalid = (c1 == c0);
      
      U8CardT c1_u8 = U8CardT(c1);
      CardT card1 = CardT(c1_u8.suit(), c1_u8.rank());
    
      for (u8 c2 = 0; c2 < 52; c2++) {
	bool c2_invalid = (c2 == c0 || c2 == c1);
	
	U8CardT c2_u8 = U8CardT(c2);
	CardT card2 = CardT(c2_u8.suit(), c2_u8.rank());
    
	for (u8 c3 = 0; c3 < 52; c3++) {
	  bool c3_invalid = (c3 == c0 || c3 == c1 || c3 == c2);

	  U8CardT c3_u8 = U8CardT(c3);
	  CardT card3 = CardT(c3_u8.suit(), c3_u8.rank());

	  int index = Poker::Normal::INVALID_OMAHA_HOLE_NORMAL_INDEX;
	  if (!c1_invalid && !c2_invalid && !c3_invalid) {
	    auto hole_normal = Normal::omaha_hole_normal(card0, card1, card2, card3);

	    auto it = omaha_hole_normal_to_index.find(hole_normal);

	    if (it != omaha_hole_normal_to_index.end()) {
	      index = it->second;
	    } else {
	      index = next_index++;

	      omaha_hole_normal_to_index[hole_normal] = index;

	      OMAHA_HOLE_NORMAL_FROM_INDEX[index] = hole_normal;
	    }
	  }

	  OMAHA_HOLD_NORMAL_INDEX[c0][c1][c2][c3] = index;
	}
      }
    }
  }

  assert(next_index == N_OMAHA_HOLE_NORMALS);
  OMAHA_HOLD_NORMAL_INDEX_INITED = true;
}

int Poker::Normal::omaha_hole_normal_index(CardT card0, CardT card1, CardT card2, CardT card3) {
  assert(OMAHA_HOLD_NORMAL_INDEX_INITED);

  u8 c0 = to_u8card(card0).u8_card;
  u8 c1 = to_u8card(card1).u8_card;
  u8 c2 = to_u8card(card1).u8_card;
  u8 c3 = to_u8card(card3).u8_card;

  return OMAHA_HOLD_NORMAL_INDEX[c0][c1][c2][c3];
}

std::tuple<CardT, CardT, CardT, CardT> Poker::Normal::omaha_hole_normal_from_index(int index) {
  assert(OMAHA_HOLD_NORMAL_INDEX_INITED);
  assert(0 <= index && (std::size_t)index < Poker::Normal::N_OMAHA_HOLE_NORMALS);

  return OMAHA_HOLE_NORMAL_FROM_INDEX[index];
}
