CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -Werror -Iinclude

TARGET = tests
TEST_SRC = test/test_skip_list.cpp

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(TEST_SRC)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(TEST_SRC) -lgtest -lgtest_main -pthread

clean:
	rm -f $(TARGET)