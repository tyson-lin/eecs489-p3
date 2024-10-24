# Variables
CXX = g++
CXXFLAGS = -Wall -O2
LDFLAGS =

# Targets
all: miProxy nameserver

# Rule to compile miProxy executable
miProxy: miProxy.o
	$(CXX) $(LDFLAGS) -o miProxy miProxy.o

# Rule to compile nameserver executable
nameserver: nameserver.o
	$(CXX) $(LDFLAGS) -o nameserver nameserver.o

# Object file for miProxy
miProxy.o: miProxy.cpp
	$(CXX) $(CXXFLAGS) -c miProxy.cpp

# Object file for nameserver
nameserver.o: nameserver.cpp
	$(CXX) $(CXXFLAGS) -c nameserver.cpp

# Clean rule
clean:
	rm -f *.o miProxy nameserver
