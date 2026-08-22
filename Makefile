BUILD_DIR := build
TYPE      := Release

.PHONY: all build run clean

all: build

build:
	cmake -B $(BUILD_DIR) -DCMAKE_BUILD_TYPE=$(TYPE)
	cmake --build $(BUILD_DIR) --parallel

run: build
	./$(BUILD_DIR)/mnk_game

clean:
	rm -rf $(BUILD_DIR)
