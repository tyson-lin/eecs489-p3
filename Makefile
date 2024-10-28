# Variables
CXX = g++
CXXFLAGS = -Wall -O2
LDFLAGS =

# Targets
all: wSender-base wReceiver-base wSender-opt wReceiver-opt

# Rule to compile wSender-base
wSender-base: wSender-base.o
	$(CXX) $(LDFLAGS) -o wSender-base wSender-base.o

# Object file for wSender-base
wSender-base.o: wSender-base.cpp
	$(CXX) $(CXXFLAGS) -c wSender-base.cpp

# Rule to compile wReceiver-base
wReceiver-base: wReceiver-base.o
	$(CXX) $(LDFLAGS) -o wReceiver-base wReceiver-base.o

# Object file for wReceiver-base
wReceiver-base.o: wReceiver-base.cpp
	$(CXX) $(CXXFLAGS) -c wReceiver-base.cpp

# Rule to compile wSender-opt 
wSender-opt: wwSender-opt.o
	$(CXX) $(LDFLAGS) -o wSender-opt wSender-opt.o

# Object file for wSender-opt 
wSender-opt.o: wSender-opt.cpp
	$(CXX) $(CXXFLAGS) -c wSender-opt.cpp

# Rule to compile wReceiver-opt
wReceiver-opt: wReceiver-opt.o
	$(CXX) $(LDFLAGS) -o wReceiver-opt wReceiver-opt.o

# Object file for wReceiver-opt
wReceiver-opt.o: wReceiver-opt.cpp
	$(CXX) $(CXXFLAGS) -c wReceiver-opt.cpp

# Clean rule
clean:
	rm -f *.o wSender_base
