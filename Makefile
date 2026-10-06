CXX ?= c++
CXXFLAGS ?= -std=c++17 -Wall -Wextra -Wpedantic

TARGET := build/ray-tracing
SOURCE := src/main.cpp
OUTPUT := output/image.ppm

.PHONY: all render run clean

all: $(TARGET)

$(TARGET): $(SOURCE)
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $< -o $@

render: $(TARGET)
	@mkdir -p $(dir $(OUTPUT))
	./$(TARGET) > $(OUTPUT)

run: render

clean:
	rm -rf build output
