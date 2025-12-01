# Makefile for Knapsack Algorithms
# Compatible with Unix/WSL

CXX = g++
CXXFLAGS = -std=c++17 -O3 -Wall -Wextra
TARGET_DIR = bin

# Algorithm directories
ALGO_DIRS = 01-bruteforce 02-memoization 03-dynamicprogramming 04-branchandbound \
            05-meetinthemiddle 06-greedyheuristic 07-randompremutation 08-efficientalgo \
            09-billionscale 10-geneticalgorithm 11-customalgorithm 12-customtestbed

# Explicit targets we currently build (following ALGO_DIRS order)
TARGETS = $(TARGET_DIR)/bruteforce $(TARGET_DIR)/memoization $(TARGET_DIR)/dynamicprogramming \
	$(TARGET_DIR)/branchandbound $(TARGET_DIR)/meetinthemiddle $(TARGET_DIR)/greedyheuristic \
	$(TARGET_DIR)/randompermutation $(TARGET_DIR)/efficientalgo $(TARGET_DIR)/billionscale \
	$(TARGET_DIR)/geneticalgorithm $(TARGET_DIR)/customalgorithm $(TARGET_DIR)/customtestbed

# Create bin directory if it doesn't exist
$(shell mkdir -p $(TARGET_DIR))

.PHONY: all clean help bruteforce memoization dynamicprogramming \
    branchandbound meetinthemiddle greedyheuristic \
    randompermutation efficientalgo billionscale \
    geneticalgorithm customalgorithm customtestbed

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

# 04-branchandbound
$(TARGET_DIR)/branchandbound: 04-branchandbound/branchandbound.cpp
	@echo "Compiling Branch and Bound algorithm..."
	$(CXX) $(CXXFLAGS) -o $@ $<
	@echo "Branch and Bound compiled successfully!"

# 05-meetinthemiddle
$(TARGET_DIR)/meetinthemiddle: 05-meetinthemiddle/algo.cpp
	@echo "Compiling Meet in the Middle algorithm..."
	$(CXX) $(CXXFLAGS) -o $@ $<
	@echo "Meet in the Middle compiled successfully!"

# 06-greedyheuristic
$(TARGET_DIR)/greedyheuristic: 06-greedyheuristic/greedyheuristic.cpp
	@echo "Compiling Greedy Heuristic algorithm..."
	$(CXX) $(CXXFLAGS) -o $@ $<
	@echo "Greedy Heuristic compiled successfully!"

# 07-randompermutation
$(TARGET_DIR)/randompermutation: 07-randompremutation/randompermutation.cpp
	@echo "Compiling Random Permutation algorithm..."
	$(CXX) $(CXXFLAGS) -o $@ $<
	@echo "Random Permutation compiled successfully!"

# 08-efficientalgo
$(TARGET_DIR)/efficientalgo: 08-efficientalgo/efficientalgo.cpp
	@echo "Compiling Efficient Algorithm..."
	$(CXX) $(CXXFLAGS) -o $@ $<
	@echo "Efficient Algorithm compiled successfully!"

# 09-billionscale
$(TARGET_DIR)/billionscale: 09-billionscale/algo.cpp
	@echo "Compiling Billionscale algorithm..."
	$(CXX) $(CXXFLAGS) -o $@ $<
	@echo "Billionscale compiled successfully!"

# 10-geneticalgorithm
$(TARGET_DIR)/geneticalgorithm: 10-geneticalgorithm/geneticalgorithm.cpp
	@echo "Compiling Genetic Algorithm..."
	$(CXX) $(CXXFLAGS) -o $@ $<
	@echo "Genetic Algorithm compiled successfully!"

# 11-customalgorithm
$(TARGET_DIR)/customalgorithm: 11-customalgorithm/customalgorithm.cpp
	@echo "Compiling Custom Algorithm..."
	$(CXX) $(CXXFLAGS) -o $@ $<
	@echo "Custom Algorithm compiled successfully!"

# 12-customtestbed
$(TARGET_DIR)/customtestbed: 12-customtestbed/customtestbed.cpp
	@echo "Compiling Custom Testbed algorithm..."
	$(CXX) $(CXXFLAGS) -o $@ $<
	@echo "Custom Testbed compiled successfully!"

# Convenience single-target aliases
bruteforce: $(TARGET_DIR)/bruteforce
memoization: $(TARGET_DIR)/memoization
dynamicprogramming: $(TARGET_DIR)/dynamicprogramming
branchandbound: $(TARGET_DIR)/branchandbound
meetinthemiddle: $(TARGET_DIR)/meetinthemiddle
greedyheuristic: $(TARGET_DIR)/greedyheuristic
randompermutation: $(TARGET_DIR)/randompermutation
efficientalgo: $(TARGET_DIR)/efficientalgo
billionscale: $(TARGET_DIR)/billionscale
geneticalgorithm: $(TARGET_DIR)/geneticalgorithm
customalgorithm: $(TARGET_DIR)/customalgorithm
customtestbed: $(TARGET_DIR)/customtestbed

clean:
	@echo "Cleaning compiled binaries..."
	rm -rf $(TARGET_DIR)
	@echo "Clean complete!"

# Help target
help:
	@echo "Knapsack Algorithms Makefile"
	@echo "Usage:"
	@echo "  make all              - Compile all algorithms"
	@echo "  make <name>           - Compile a single algorithm (e.g., make bruteforce)"
	@echo "  make clean            - Remove all compiled binaries"
	@echo "  make help             - Display this help message"
	@echo ""
	@echo "Available algorithms: bruteforce, memoization, dynamicprogramming, branchandbound,"
	@echo "                      meetinthemiddle, greedyheuristic, randompermutation, efficientalgo,"
	@echo "                      billionscale, geneticalgorithm, customalgorithm, customtestbed"
