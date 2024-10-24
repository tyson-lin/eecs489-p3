# Variables
CXX = g++
CXXFLAGS = -Wall -O2
LDFLAGS =

# Targets
all: miProxy nameserver

# Rule to compile unoptimized sender
wSender-base: wSender-base.o
	$(CXX) $(LDFLAGS) -o wSender-base wSender-base.o

# Object file for wSender-base
wSender-base.o: wSender-base.cpp
	$(CXX) $(CXXFLAGS) -c wSender-base.cpp

# Clean rule
clean:
	rm -f *.o miProxy nameserver
