check-hand-eval: check-hand-eval.cpp dealer.hpp ./include/types.hpp hand-eval.hpp hand-eval.cpp normal.hpp Makefile
	g++ -I ./include -Wall -O3 -o check-hand-eval check-hand-eval.cpp hand-eval.cpp

omaha-ev: omaha-ev.cpp dealer.hpp ./include/types.hpp hand-eval.hpp hand-eval.cpp normal.hpp normal.cpp Makefile
	g++ -I ./include -Wall -O3 -o omaha-ev omaha-ev.cpp hand-eval.cpp normal.cpp -lpthread

holdem-ev: holdem-ev.cpp dealer.hpp ./include/types.hpp hand-eval.hpp hand-eval.cpp normal.hpp Makefile
	g++ -I ./include -Wall -O3 -o holdem-ev holdem-ev.cpp hand-eval.cpp

preflop-gto: preflop-gto.cpp dealer.hpp ./include/types.hpp hand-eval.hpp hand-eval.cpp normal.hpp gto-common.hpp gto-eval.hpp gto-strategy.hpp Makefile
	g++ -I ./include -Wall -O3 -o preflop-gto preflop-gto.cpp hand-eval.cpp
	#g++ -Wall -g -o preflop-gto preflop-gto.cpp hand-eval.cpp

dealer: dealer.cpp dealer.hpp ./include/types.hpp hand-eval.hpp hand-eval.cpp Makefile
	g++ -I ./include -O3 -o dealer dealer.cpp hand-eval.cpp

hand-eval: hand-eval.cpp hand-eval.cpp ./include/types.hpp Makefile
	g++ -I ./include -c hand-eval.cpp

eval-10: eval-p0-at-2-to-10-up.cpp dealer.hpp ./include/types.hpp hand-eval.hpp hand-eval.cpp Makefile
	g++ -I ./include -O3 -o eval-10 eval-p0-at-2-to-10-up.cpp hand-eval.cpp

all: omaha-ev holdem-ev preflop-gto dealer hand-eval eval-10
