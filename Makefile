C = gcc
CXX = g++
C_LIB = network.c nl.c
C_LINK = network.o nl.o
MAIN = main.cpp
LD = -std=c++14
OUT = docker-run

all:
	make container
container:
	$(C) -c $(C_LIB)
	$(CXX) $(LD) -o $(OUT) $(MAIN) $(C_LINK) $(LD)
clean:
	rm *.o $(OUT)
