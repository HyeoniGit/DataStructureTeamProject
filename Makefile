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

# 테스트: tests/*.cpp 를 main.cpp 를 뺀 모듈과 함께 빌드해서 실행
MODULE_OBJS := $(filter-out build/main.o,$(OBJS))
TEST_SRCS   := $(wildcard tests/*.cpp)
TEST_BINS   := $(TEST_SRCS:tests/%.cpp=build/tests/%)

test: $(TEST_BINS)
	@for t in $(TEST_BINS); do echo "== $$t"; ./$$t || exit 1; done

build/tests/%: tests/%.cpp $(MODULE_OBJS)
	@mkdir -p $(dir $@)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $< $(MODULE_OBJS) -o $@

clean:
	rm -rf build $(TARGET) $(TARGET).exe

-include $(DEPS)

.PHONY: all run clean test
