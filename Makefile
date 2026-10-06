CXX ?= c++
CXXFLAGS ?= -std=c++17 -Wall -Wextra -Wpedantic
CPPFLAGS ?= -Isrc/math -Isrc/renderer -Isrc/scene

TARGET := build/ray-tracing
BENCHMARK_TARGET := build/benchmark
COMMON_SOURCES := $(filter-out src/app/main.cpp src/benchmark/main.cpp,$(wildcard src/**/*.cpp))
APP_SOURCES := src/app/main.cpp $(COMMON_SOURCES)
BENCHMARK_SOURCES := src/benchmark/main.cpp $(COMMON_SOURCES)
APP_OBJECTS := $(patsubst %.cpp,build/%.o,$(APP_SOURCES))
BENCHMARK_OBJECTS := $(patsubst %.cpp,build/%.o,$(BENCHMARK_SOURCES))
DEPENDENCIES := $(APP_OBJECTS:.o=.d) $(BENCHMARK_OBJECTS:.o=.d)
OUTPUT := output/image.ppm

.PHONY: all render run benchmark clean

all: $(TARGET)

$(TARGET): $(APP_OBJECTS)
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $^ -o $@

$(BENCHMARK_TARGET): $(BENCHMARK_OBJECTS)
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

clean:
	rm -rf build output
