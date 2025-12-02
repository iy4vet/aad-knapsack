# Makefile for Knapsack Algorithms
# Compatible with Unix/WSL

CXX = g++
CXXFLAGS = -std=c++17 -O3 -Wall -Wextra
TARGET_DIR = bin

# Explicit targets we currently build
TARGETS = $(TARGET_DIR)/bruteforce \
          $(TARGET_DIR)/memoization \
          $(TARGET_DIR)/dynamicprogramming \
          $(TARGET_DIR)/combo \
          $(TARGET_DIR)/randompermutation \
          $(TARGET_DIR)/branchandbound \
          $(TARGET_DIR)/meetinthemiddle \
          $(TARGET_DIR)/greedy \
          $(TARGET_DIR)/efficient \
          $(TARGET_DIR)/billionscale \
          $(TARGET_DIR)/genetic \
          $(TARGET_DIR)/memetic

# Create bin directory if it doesn't exist
$(shell mkdir -p $(TARGET_DIR))

.PHONY: all clean help bruteforce memoization dynamicprogramming combo \
	randompermutation branchandbound meetinthemiddle greedy \
	efficient billionscale genetic memetic

# Default: build all explicit targets
all: $(TARGETS)

# 01-bruteforce
$(TARGET_DIR)/bruteforce: 01-bruteforce/bruteforce.cpp
	@echo "Compiling Brute Force algorithm..."
	$(CXX) $(CXXFLAGS) -o $@ $<
	@echo "Brute Force compiled successfully!"

# 02-memoization
$(TARGET_DIR)/memoization: 02-memoization/memoization.cpp
	@echo "Compiling Memoization algorithm..."
	$(CXX) $(CXXFLAGS) -o $@ $<
	@echo "Memoization compiled successfully!"

# 03-dynamicprogramming
$(TARGET_DIR)/dynamicprogramming: 03-dynamicprogramming/dynamicprogramming.cpp
	@echo "Compiling Dynamic Programming algorithm..."
	$(CXX) $(CXXFLAGS) -o $@ $<
	@echo "Dynamic Programming compiled successfully!"

# 04-combo
$(TARGET_DIR)/combo: 04-combo/combo.cpp
	@echo "Compiling Combo algorithm..."
	$(CXX) $(CXXFLAGS) -o $@ $<
	@echo "Combo compiled successfully!"

# 05-randompermutation
$(TARGET_DIR)/randompermutation: 05-randompermutation/randompermutation.cpp
	@echo "Compiling Random Permutation algorithm..."
	$(CXX) $(CXXFLAGS) -o $@ $<
	@echo "Random Permutation compiled successfully!"

# 06-branchandbound
$(TARGET_DIR)/branchandbound: 06-branchandbound/branchandbound.cpp
	@echo "Compiling Branch and Bound algorithm..."
	$(CXX) $(CXXFLAGS) -o $@ $<
	@echo "Branch and Bound compiled successfully!"

# 07-meetinthemiddle
$(TARGET_DIR)/meetinthemiddle: 07-meetinthemiddle/meetinthemiddle.cpp
	@echo "Compiling Meet in the Middle algorithm..."
	$(CXX) $(CXXFLAGS) -o $@ $<
	@echo "Meet in the Middle compiled successfully!"

# 08-greedy
$(TARGET_DIR)/greedy: 08-greedy/greedy.cpp
	@echo "Compiling Greedy algorithm..."
	$(CXX) $(CXXFLAGS) -o $@ $<
	@echo "Greedy compiled successfully!"

# 09-efficient
$(TARGET_DIR)/efficient: 09-efficient/efficient.cpp
	@echo "Compiling Efficient algorithm..."
	$(CXX) $(CXXFLAGS) -o $@ $<
	@echo "Efficient compiled successfully!"

# 10-billionscale
$(TARGET_DIR)/billionscale: 10-billionscale/billionscale.cpp
	@echo "Compiling Billionscale algorithm..."
	$(CXX) $(CXXFLAGS) -o $@ $<
	@echo "Billionscale compiled successfully!"

# 11-genetic
$(TARGET_DIR)/genetic: 11-genetic/genetic.cpp
	@echo "Compiling Genetic algorithm..."
	$(CXX) $(CXXFLAGS) -o $@ $<
	@echo "Genetic compiled successfully!"

# 12-memetic
$(TARGET_DIR)/memetic: 12-memetic/memetic.cpp
	@echo "Compiling Memetic algorithm..."
	$(CXX) $(CXXFLAGS) -o $@ $<
	@echo "Memetic compiled successfully!"

# Convenience single-target aliases
bruteforce: $(TARGET_DIR)/bruteforce
memoization: $(TARGET_DIR)/memoization
dynamicprogramming: $(TARGET_DIR)/dynamicprogramming
combo: $(TARGET_DIR)/combo
randompermutation: $(TARGET_DIR)/randompermutation
branchandbound: $(TARGET_DIR)/branchandbound
meetinthemiddle: $(TARGET_DIR)/meetinthemiddle
greedy: $(TARGET_DIR)/greedy
efficient: $(TARGET_DIR)/efficient
billionscale: $(TARGET_DIR)/billionscale
genetic: $(TARGET_DIR)/genetic
memetic: $(TARGET_DIR)/memetic

clean:
	@echo "Cleaning compiled binaries..."
	rm -rf $(TARGET_DIR)
	@echo "Clean complete!"

# Help target
help:
	@echo "Knapsack Algorithms Makefile"
	@echo "Usage:"
	@echo "  make all       - Compile all algorithms"
	@echo "  make <name>    - Compile a single algorithm (e.g., make bruteforce)"
	@echo "  make clean     - Remove all compiled binaries"
	@echo "  make help      - Display this help message"
	@echo ""
	@echo "Available algorithms: bruteforce, memoization, dynamicprogramming, combo,"
	@echo "                      randompermutation, branchandbound, meetinthemiddle, greedy,"
	@echo "                      efficient, billionscale, genetic, memetic"
