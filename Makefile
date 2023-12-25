CC = g++
LIBS = -lzmq

all: control calculation

control: ./src/control.cpp
	$(CC) $^ -o $@ $(LIBS)

calculation: ./src/calculation_node.cpp
	$(CC) $^ -o $@ $(LIBS)

