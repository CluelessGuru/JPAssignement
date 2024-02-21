CXX = g++
CXXFLAGS = -std=c++17 -O3 
CPPFLAGS = -MMD -MP
LDFLAGS = 

EXEC = test

SOURCES= $(wildcard *.cpp)
OBJECTS= $(SOURCES:.cpp=.o) 
DEPS= $(SOURCES:.cpp=.d) 

$(EXEC):$(OBJECTS)
	$(CXX) $(LDFLAGS) $(OBJECTS) -o $(EXEC)

all: $(OBJS) $(EXEC)

clean:
	rm $(EXEC) $(OBJECTS) $(DEPS)

-include $(DEPS)
