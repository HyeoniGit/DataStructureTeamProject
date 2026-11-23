

CXX      ?= g++
CXXFLAGS ?= -std=c++17 -Wall -Wextra -O2
CPPFLAGS := -Iinclude -MMD -MP

SRCS     := $(wildcard src/*.cpp src/modules/*.cpp)
OBJS     := $(SRCS:src/%.cpp=build/%.o)
DEPS     := $(OBJS:.o=.d)
TARGET   := ddareungi

# Exclude main.cpp when building test programs
MODULE_OBJS := $(filter-out build/main.o,$(OBJS))

# Discover functional tests automatically
TEST_SRCS := $(filter-out tests/benchmark_%.cpp,$(wildcard tests/*.cpp))
TEST_BINS := $(TEST_SRCS:tests/%.cpp=build/tests/%)
TEST_DEPS := $(TEST_BINS:=.d)

# Graph benchmark configuration
GRAPH_BENCH_SRC := tests/benchmark_graph.cpp
GRAPH_BENCH := build/benchmark_graph
GRAPH_BENCH_DEP := $(GRAPH_BENCH).d

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^

build/%.o: src/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@

run: $(TARGET)
	./$(TARGET) data/sample_stations.csv data/sample.csv

# Run all functional tests
test: $(TEST_BINS)
	@for t in $(TEST_BINS); do \
		echo "== $$t"; \
		./$$t || exit 1; \
	done

# Build each functional test
build/tests/%: tests/%.cpp $(MODULE_OBJS)
	@mkdir -p $(dir $@)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) \
		-MF $@.d \
		$< $(MODULE_OBJS) -o $@

# Run only Graph functional tests
test-graph: build/tests/test_graph
	./build/tests/test_graph

# Run Graph benchmark separately
benchmark-graph: $(GRAPH_BENCH)
	./$(GRAPH_BENCH) 1000 2789 8 100000

$(GRAPH_BENCH): $(GRAPH_BENCH_SRC) $(MODULE_OBJS)
	@mkdir -p $(dir $@)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) \
		-MF $(GRAPH_BENCH_DEP) \
		$(GRAPH_BENCH_SRC) $(MODULE_OBJS) -o $@

# Run sort / binary search benchmark on real data
#   make benchmark-analysis CSV="data/서울특별시 공공자전거 대여이력 정보_2606.csv"
CSV ?= data/sample.csv
benchmark-analysis: build/tests/benchmark_analysis
	./build/tests/benchmark_analysis "$(CSV)"

clean:
	rm -rf build $(TARGET) $(TARGET).exe

-include $(DEPS) $(TEST_DEPS) $(GRAPH_BENCH_DEP)

.PHONY: all run clean test test-graph benchmark-graph benchmark-analysis
