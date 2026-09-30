CXX      ?= g++
CXXFLAGS ?= -std=c++17 -Wall -Wextra -O2
CPPFLAGS := -Iinclude -MMD -MP
SRCS     := $(wildcard src/*.cpp src/modules/*.cpp)
OBJS     := $(SRCS:src/%.cpp=build/%.o)
DEPS     := $(OBJS:.o=.d)
TARGET   := ddareungi

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^

build/%.o: src/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@

run: $(TARGET)
	./$(TARGET) data/sample.csv

clean:
	rm -rf build $(TARGET) $(TARGET).exe

-include $(DEPS)

.PHONY: all run clean
