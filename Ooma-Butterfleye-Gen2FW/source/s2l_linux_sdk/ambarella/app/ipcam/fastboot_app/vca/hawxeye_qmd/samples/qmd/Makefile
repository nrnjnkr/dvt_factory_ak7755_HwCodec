# Copyright 2017 HawXeye, Inc.  All rights reserved.
S2LM_CXXFLAGS+=-I../../include -I../../opencv/include/

BIN=sample

SRCS=sample.cpp
OBJS=$(SRCS:.cpp=.o)

LIBFLAGS=-L../../lib/ -lhawxeyeqmd -lpthread -Wl,-rpath,../../lib


all: $(BIN)

$(OBJS):

%.o: %.cpp
	$(S2LM_CXX) -c $(S2LM_CXXFLAGS) $< -o $@

$(BIN): $(OBJS)
	$(S2LM_CXX) $^ $(LIBFLAGS) -o $@

clean:
	rm -f $(BIN) *.o
