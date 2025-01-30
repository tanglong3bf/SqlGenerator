.PHONY: run clean
run: test
	./test

objects = SqlGenerator.o test.o

vpath % ../src:.

test: $(objects)
	g++ -o $@ $^ -ljsoncpp -ldrogon -ltrantor -luuid -lz -lcrypto
$(objects): ../src/SqlGenerator.h

clean:
	rm -f $(objects) test
