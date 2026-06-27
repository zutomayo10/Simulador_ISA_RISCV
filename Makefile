CXX      = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -O2
SRC_DIR  = src
BUILD    = build

SRCS = $(SRC_DIR)/main.cpp $(SRC_DIR)/memory.cpp $(SRC_DIR)/cpu.cpp $(SRC_DIR)/disasm.cpp
OBJS = $(SRCS:$(SRC_DIR)/%.cpp=$(BUILD)/%.o)
TARGET = $(BUILD)/riscv-sim

.PHONY: all clean

all: $(BUILD) $(TARGET)
	@echo "Compilacion exitosa: $(TARGET)"

$(BUILD):
	mkdir -p $(BUILD)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^

$(BUILD)/%.o: $(SRC_DIR)/%.cpp
	$(CXX) $(CXXFLAGS) -I$(SRC_DIR) -c -o $@ $<

clean:
	rm -rf $(BUILD)
