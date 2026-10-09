CXX ?= c++
CXXFLAGS ?= -std=c++17 -Wall -Wextra -Wpedantic
CPPFLAGS ?= -Isrc/math -Isrc/renderer -Isrc/scene

TARGET := build/ray-tracing
BENCHMARK_TARGET := build/benchmark
PACKET_BENCHMARK_TARGET := build/packet-benchmark
TEST_TARGET := build/bvh-test
COMMON_SOURCES := $(filter-out src/app/main.cpp src/benchmark/main.cpp src/benchmark/packet_main.cpp src/tests/bvh_test.cpp,$(wildcard src/**/*.cpp))
APP_SOURCES := src/app/main.cpp $(COMMON_SOURCES)
BENCHMARK_SOURCES := src/benchmark/main.cpp $(COMMON_SOURCES)
PACKET_BENCHMARK_SOURCES := src/benchmark/packet_main.cpp $(COMMON_SOURCES)
TEST_SOURCES := src/tests/bvh_test.cpp $(COMMON_SOURCES)
APP_OBJECTS := $(patsubst %.cpp,build/%.o,$(APP_SOURCES))
BENCHMARK_OBJECTS := $(patsubst %.cpp,build/%.o,$(BENCHMARK_SOURCES))
PACKET_BENCHMARK_OBJECTS := $(patsubst %.cpp,build/%.o,$(PACKET_BENCHMARK_SOURCES))
TEST_OBJECTS := $(patsubst %.cpp,build/%.o,$(TEST_SOURCES))
DEPENDENCIES := $(APP_OBJECTS:.o=.d) $(BENCHMARK_OBJECTS:.o=.d) $(TEST_OBJECTS:.o=.d)
OUTPUT := output/image.ppm
THREAD_SCALING_CSV := build/thread-scaling.csv
THREAD_SCALING_WIDTH ?= 320
THREAD_SCALING_SAMPLES ?= 10
THREAD_SCALING_DEPTH ?= 20
THREAD_SCALING_ITERATIONS ?= 3

.PHONY: all render run benchmark benchmark-build packet-benchmark thread-scaling test avx2-test clean

all: $(TARGET)

$(TARGET): $(APP_OBJECTS)
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $^ -o $@

$(BENCHMARK_TARGET): $(BENCHMARK_OBJECTS)
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $^ -o $@

$(PACKET_BENCHMARK_TARGET): $(PACKET_BENCHMARK_OBJECTS)
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $^ -o $@

$(TEST_TARGET): $(TEST_OBJECTS)
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $^ -o $@

build/%.o: %.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -MMD -MP -c $< -o $@

-include $(DEPENDENCIES)

render: $(TARGET)
	@mkdir -p $(dir $(OUTPUT))
	./$(TARGET) > $(OUTPUT)

run: render

benchmark: $(BENCHMARK_TARGET)
	./$(BENCHMARK_TARGET) $(BENCHMARK_ARGS)

benchmark-build: $(BENCHMARK_TARGET)

packet-benchmark: $(PACKET_BENCHMARK_TARGET)
	./$(PACKET_BENCHMARK_TARGET)

thread-scaling:
	$(MAKE) clean
	$(MAKE) benchmark-build
	@mkdir -p $(dir $(THREAD_SCALING_CSV))
	@{ \
		./$(BENCHMARK_TARGET) $(THREAD_SCALING_WIDTH) $(THREAD_SCALING_SAMPLES) $(THREAD_SCALING_DEPTH) $(THREAD_SCALING_ITERATIONS) 1; \
		for threads in 2 4 6 12; do \
			./$(BENCHMARK_TARGET) $(THREAD_SCALING_WIDTH) $(THREAD_SCALING_SAMPLES) $(THREAD_SCALING_DEPTH) $(THREAD_SCALING_ITERATIONS) $$threads | tail -n +2; \
		done; \
	} | tee $(THREAD_SCALING_CSV)
	@awk -F, '\
	NR == 1 { next } \
	{ seconds[$$4] += $$6; count[$$4]++ } \
	END { \
		for (threads in seconds) average[threads] = seconds[threads] / count[threads]; \
		baseline = average[1]; \
		printf "\n%-8s %-14s %-10s %-12s %-12s %-12s\n", "Threads", "Avg seconds", "Speedup(x)", "Efficiency(%)", "Cost", "Overhead"; \
		for (i = 1; i <= 5; ++i) { \
			threads = (i == 1 ? 1 : i == 2 ? 2 : i == 3 ? 4 : i == 4 ? 6 : 12); \
			if (!(threads in average)) continue; \
			speedup = baseline / average[threads]; \
			efficiency = speedup / threads * 100; \
			cost = threads * average[threads]; \
			printf "%-8d %-14.6f %-10.2f %-12.2f %-12.6f %-12.6f\n", threads, average[threads], speedup, efficiency, cost, cost - baseline; \
		} \
	}' $(THREAD_SCALING_CSV)

test: $(TEST_TARGET)
	./$(TEST_TARGET)

avx2-test:
	$(MAKE) clean
	$(MAKE) test CXXFLAGS="$(CXXFLAGS) -mavx2"

clean:
	rm -rf build output
