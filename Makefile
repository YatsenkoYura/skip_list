CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -Werror -Iinclude

TARGET = tests
TEST_SRC = test/test_skip_list.cpp
DOXYFILE = Doxyfile

.PHONY: all clean docs clean-docs

all: $(TARGET)

$(TARGET): $(TEST_SRC)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(TEST_SRC) \
	        -lgtest -lgtest_main -pthread

docs:
	doxygen $(DOXYFILE)

clean:
	rm -f $(TARGET)

clean-docs:
	rm -rf docs