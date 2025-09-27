GENERATES = main.out
MY_OBG_FILES = *.o
TRASH = logs/*.log
CC = g++
CPPFLAGS = -fconcepts-diagnostics-depth=2 -fsanitize=address,undefined,signed-integer-overflow,pointer-compare,pointer-subtract,leak,bounds,pointer-overflow -O2 -Wall -Wextra -Wpedantic -std=c++23 -lm


.PHONY: all run clean

all: main.out

run: main.out
	./main.out

main.out: main.o smart_ptr.o logger.o formatter.o
	$(CC) $(CPPFLAGS) -o $@ $^

main.o: main.cpp
	$(CC) $(CPPFLAGS) -c -o $@ $^

smart_ptr.o: smart_ptr.cpp
	$(CC) $(CPPFLAGS) -c -o $@ $^

logger.o: logger.cpp
	$(CC) $(CPPFLAGS) -c -o $@ $^

formatter.o: formatter.cpp
	$(CC) $(CPPFLAGS) -c -o $@ $^

trashclean:
	rm -v $(TRASH)

objclean:
	rm -v $(MY_OBG_FILES)

distclean:
	rm -v $(GENERATES)

clean: trashclean objclean distclean