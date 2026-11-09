CXX      = g++
CXXFLAGS = -g -Wall -std=c++11
LDFLAGS  = -pthread
 
all: lizards
 
lizards: lizards.cpp
	$(CXX) $(CXXFLAGS) lizards.cpp -o lizards $(LDFLAGS)
 
lizardsUni: lizardsUni.cpp
	$(CXX) $(CXXFLAGS) lizardsUni.cpp -o lizardsUni $(LDFLAGS)

clean:
	rm -f lizards lizardsUni
 