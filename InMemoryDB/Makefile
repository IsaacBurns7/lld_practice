CXX      := g++
CXXFLAGS := -std=c++20 -Wall -Wextra -O2 -MMD -MP
SRC      := $(wildcard src/*.cpp)
OBJ      := $(SRC:src/%.cpp=build/%.o)
TARGET   := app

$(TARGET): $(OBJ)
	$(CXX) $(CXXFLAGS) $^ -o $@

build/%.o: src/%.cpp | build
	$(CXX) $(CXXFLAGS) -c $< -o $@

build:
	mkdir -p $@

clean:
	rm -rf build $(TARGET)

.PHONY: clean
-include $(OBJ:.o=.d)
