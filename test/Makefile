objects = SqlGenerator.o test.o
FLAGS = -g -Wall -Wextra -std=c++2a

vpath % ../src:.

test: $(objects)
	g++ -o $@ $^ -ljsoncpp -ldrogon -ltrantor -luuid -lz -lcrypto
SqlGenerator.o: SqlGenerator.cc SqlGenerator.h
	g++ $(FLAGS) -c $<
test.o: test.cc SqlGenerator.h
	g++ $(FLAGS) -c $<

.PHONY: run clean
run: test
	./test
clean:
	rm -f $(objects) test
