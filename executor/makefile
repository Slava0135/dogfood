CXX=g++
CXXFLAGS=-std=c++11

BUILD=$(CXX) $(CXXFLAGS)
TESTNAME=test

all: executor.o test.o syscall.o trace.o utils.o
	${BUILD} executor.o ${TESTNAME}.o syscall.o trace.o utils.o -o ${TESTNAME}.out

executor.o: executor.cc
	${BUILD} -c executor.cc

test.o: ${TESTNAME}.c
	${BUILD} -c ${TESTNAME}.c

syscall.o: syscall.cc
	${BUILD} -c syscall.cc

trace.o: trace.cc
	${BUILD} -c trace.cc

utils.o: utils.cc
	${BUILD} -c utils.cc

clean:
	rm *.o *.out
