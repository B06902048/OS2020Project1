
all:
	gcc -std=c99 main.c scheduler.c -o main
run:
	./main
clean:
	rm -rf main
