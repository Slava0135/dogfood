CXX=g++
CXXFLAGS=-std=c++11

BUILD=$(CXX) $(CXXFLAGS)

all: executor.o test.o syscall.o trace.o utils.o
	${BUILD} executor.o test.o syscall.o trace.o utils.o -o test.out

executor.o: executor.cc
	${BUILD} -c executor.cc

test.o: test.c
	${BUILD} -c test.c

syscall.o: syscall.cc
	${BUILD} -c syscall.cc

trace.o: trace.cc
	${BUILD} -c trace.cc

utils.o: utils.cc
	${BUILD} -c utils.cc

clean:
	rm *.o *.out
