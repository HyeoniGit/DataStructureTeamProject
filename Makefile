CXX      ?= g++
CXXFLAGS ?= -std=c++17 -Wall -Wextra -O2 -Iinclude
SRCS     := $(wildcard src/*.cpp src/modules/*.cpp)
OBJS     := $(SRCS:src/%.cpp=build/%.o)
TARGET   := ddareungi

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^

build/%.o: src/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

run: $(TARGET)
	./$(TARGET) data/sample.csv

clean:
	rm -rf build $(TARGET)

.PHONY: all run clean
