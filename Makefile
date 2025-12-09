# Compiler settings
CXX = g++
CXXFLAGS = -std=c++20 -Wall -Wextra -pedantic -fPIC

# Robot plugins (.so files)
ROBOTS = Robot_Toland.so Robot_Ratboy.so

# Arena executable
TARGET = RobotWarz

# Source files
ARENA_SRC = Arena.cpp
ROBOTBASE_SRC = RobotBase.cpp

# Build everything
all: $(ROBOTS) $(TARGET)

# Build a robot shared object (.so)
%.so: %.cpp RobotBase.o
	$(CXX) $(CXXFLAGS) -shared $< RobotBase.o -o $@

# Build RobotBase.o (used by robots and arena)
RobotBase.o: RobotBase.cpp RobotBase.h
	$(CXX) $(CXXFLAGS) -c RobotBase.cpp -o RobotBase.o

# Build the arena executable
$(TARGET): $(ARENA_SRC) RobotBase.o
	$(CXX) $(CXXFLAGS) $(ARENA_SRC) RobotBase.o -ldl -o $(TARGET)

# Clean everything
clean:
	rm -f *.o *.so $(TARGET)
