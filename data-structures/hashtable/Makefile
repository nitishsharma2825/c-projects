build: src/prime.c src/hash_table.c src/main.c
	gcc -Wall src/prime.c src/hash_table.c src/main.c -o main -lm

run: build
	./main

clean:
	rm -rf ./main