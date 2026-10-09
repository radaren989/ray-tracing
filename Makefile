CXX ?= c++
CXXFLAGS ?= -std=c++17 -Wall -Wextra -Wpedantic
CPPFLAGS ?= -Isrc/math -Isrc/renderer -Isrc/scene

TARGET := build/ray-tracing
BENCHMARK_TARGET := build/benchmark
TEST_TARGET := build/bvh-test
COMMON_SOURCES := $(filter-out src/app/main.cpp src/benchmark/main.cpp src/tests/bvh_test.cpp,$(wildcard src/**/*.cpp))
APP_SOURCES := src/app/main.cpp $(COMMON_SOURCES)
BENCHMARK_SOURCES := src/benchmark/main.cpp $(COMMON_SOURCES)
TEST_SOURCES := src/tests/bvh_test.cpp $(COMMON_SOURCES)
APP_OBJECTS := $(patsubst %.cpp,build/%.o,$(APP_SOURCES))
BENCHMARK_OBJECTS := $(patsubst %.cpp,build/%.o,$(BENCHMARK_SOURCES))
TEST_OBJECTS := $(patsubst %.cpp,build/%.o,$(TEST_SOURCES))
DEPENDENCIES := $(APP_OBJECTS:.o=.d) $(BENCHMARK_OBJECTS:.o=.d) $(TEST_OBJECTS:.o=.d)
OUTPUT := output/image.ppm

.PHONY: all render run benchmark benchmark-build test clean

all: $(TARGET)

$(TARGET): $(APP_OBJECTS)
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $^ -o $@

$(BENCHMARK_TARGET): $(BENCHMARK_OBJECTS)
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

test: $(TEST_TARGET)
	./$(TEST_TARGET)

clean:
	rm -rf build output
