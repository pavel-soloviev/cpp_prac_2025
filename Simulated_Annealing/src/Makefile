CXX = g++
CXXFLAGS = -std=c++20 -Wall -Wextra -Wpedantic -O3 -DNDEBUG -MMD -MP
OBJS = main.o core.o parallel.o
DEPS = $(OBJS:.o=.d)
TARGET = sa_sched

all: $(TARGET)
$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) $(OBJS) -o $@

%.o: %.cpp sa.hpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

run-seq: $(TARGET)
	./$(TARGET) --mode=seq --input sample.csv --T0 1000 --Tmin 1e-3 --iters 100 --patience 1000 --law logoverlinear --seed 42

run-par: $(TARGET)
	./$(TARGET) --mode=par --input sample.csv --nproc 4 --outer 10 --T0 100 --Tmin 1e-2 --iters 1 --patience 10000 --law logoverlinear --seed 42

clean:
	rm -f $(TARGET) $(OBJS) $(DEPS)

-include $(DEPS)

