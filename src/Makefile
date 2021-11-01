preflop-gto: preflop-gto.cpp dealer.hpp types.hpp hand-eval.hpp hand-eval.cpp normal.hpp gto-eval.hpp Makefile
	g++ -Wall -O3 -o preflop-gto preflop-gto.cpp hand-eval.cpp

dealer: dealer.cpp dealer.hpp types.hpp hand-eval.hpp hand-eval.cpp Makefile
	g++ -O3 -o dealer dealer.cpp hand-eval.cpp

hand-eval: hand-eval.cpp hand-eval.cpp types.hpp Makefile
	g++ -c hand-eval.cpp

eval-10: eval-p0-at-2-to-10-up.cpp dealer.hpp types.hpp hand-eval.hpp hand-eval.cpp Makefile
	g++ -O3 -o eval-10 eval-p0-at-2-to-10-up.cpp hand-eval.cpp
