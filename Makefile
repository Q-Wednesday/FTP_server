server: 
	gcc -Wall -std=c11 -o server IO.c command.c core.c server.c  utils.c  -lpthread 
clean:
	rm server
