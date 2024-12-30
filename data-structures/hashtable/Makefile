build: src/hash_table.c src/main.c
	gcc -Wall src/hash_table.c src/main.c -o build/main -lm

run: build
	./build/main

clean:
	rm -rf build/main