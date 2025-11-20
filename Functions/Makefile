# Компилятор и флаги
CXX      := g++
CXXFLAGS := -std=gnu++17 -Wall -Wextra -O2 -I.

LDFLAGS  :=
GTEST_LIBS := -lgtest -lgtest_main -pthread

MAIN_TARGET  := main
MAIN_SOURCES := main.cpp
MAIN_OBJECTS := $(MAIN_SOURCES:.cpp=.o)

# Тесты
TEST_TARGET   := funclib_tests
TEST_SOURCES  := funclib_tests.cpp
TEST_OBJECTS  := $(TEST_SOURCES:.cpp=.o)

.PHONY: all clean run test

# Собрать всё: и бинарник, и тесты
all: $(MAIN_TARGET) $(TEST_TARGET)


$(MAIN_TARGET): $(MAIN_OBJECTS)
	$(CXX) $(CXXFLAGS) $^ -o $@ $(LDFLAGS)


$(TEST_TARGET): $(TEST_OBJECTS)
	$(CXX) $(CXXFLAGS) $^ -o $@ $(LDFLAGS) $(GTEST_LIBS)

%.o: %.cpp funclib.h
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Запустить основную программу
run: $(MAIN_TARGET)
	./$(MAIN_TARGET)

# Запустить тесты
test: $(TEST_TARGET)
	./$(TEST_TARGET)

# Удалить все генераты
clean:
	rm -f $(MAIN_OBJECTS) $(TEST_OBJECTS) $(MAIN_TARGET) $(TEST_TARGET)
