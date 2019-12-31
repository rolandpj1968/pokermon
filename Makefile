CXX ?= g++

SRC_DIR = $(shell pwd)/src

CPP_FILES = $(wildcard src/*.cpp)

HPP_FILES = $(wildcard src/*.hpp)

OBJ_FILES = $(addprefix obj/,$(notdir $(CPP_FILES:.cpp=.o)))

# don't seem to get much better than plain old -O3

CC_FLAGS = -Wall -std=c++11 -fshort-enums -fno-exceptions -fno-rtti -finline-limit=2000 -flto -march=native -Ofast -I$(SRC_DIR)
#CC_FLAGS = -fprofile-generate -Wall -std=c++11 -fshort-enums -fno-exceptions -fno-rtti -finline-limit=2000 -flto -march=native -Ofast -I$(SRC_DIR)
#CC_FLAGS = -fprofile-use -Wall -std=c++11 -fshort-enums -fno-exceptions -fno-rtti -finline-limit=2000 -flto -march=native -Ofast -I$(SRC_DIR)
#CC_FLAGS = -Wall -std=c++11 -flto -march=native -O3 -I$(SRC_DIR)
#CC_FLAGS = -Wall -std=c++11 -flto -O3 -I$(SRC_DIR)
#CC_FLAGS = -Wall -std=c++11 -fshort-enums -fno-exceptions -fno-rtti -flto -march=native -O3 -I$(SRC_DIR)
#CC_FLAGS = -Wall -std=c++11 -fshort-enums -fno-exceptions -fno-rtti -flto -march=native -O2 -I$(SRC_DIR)
#CC_FLAGS = -Wall -std=c++11 -I$(SRC_DIR)

LD_FLAGS = -fshort-enums -fno-exceptions -fno-rtti -finline-limit=2000 -flto -march=native -Ofast
#LD_FLAGS = -fprofile-generate -fshort-enums -fno-exceptions -fno-rtti -finline-limit=2000 -flto -march=native -Ofast
#LD_FLAGS = -fprofile-use -fshort-enums -fno-exceptions -fno-rtti -finline-limit=2000 -flto -march=native -Ofast
#LD_FLAGS = -flto -march=native -O3
#LD_FLAGS = -flto -O3
#LD_FLAGS = -flto -march=native -O2
#LD_FLAGS = 

OBJ_DIR = obj

POKERMON_CPP_FILES = $(wildcard src/pokermon.cpp)
POKERMON_OBJ_FILES = $(addprefix obj/,$(notdir $(POKERMON_CPP_FILES:.cpp=.o)))

POKERMON_BIN_NAME = pokermon

all: $(OBJ_DIR) $(POKERMON_BIN_NAME)

$(POKERMON_BIN_NAME): $(POKERMON_OBJ_FILES) $(OBJ_FILES)
	$(CXX) $(LD_FLAGS) -o $@ $^

obj/%.o: src/%.cpp $(HPP_FILES) Makefile
	$(CXX) $(CC_FLAGS) -c -o $@ $<

$(OBJ_DIR):
	mkdir $(OBJ_DIR)

clean:
	rm -rf $(OBJ_DIR)
	rm -f $(POKERMON_BIN_NAME)



