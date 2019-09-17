

all: cachesim.o

cachesim.o: CacheSimulator.cpp
	g++ -o cachesim CacheSimulator.cpp

clean:
	rm -rf *.o *~ *.out cachesim # Remove the .o and the executable
