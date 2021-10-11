server: 
	gcc -Wall -o server IO.c command.c core.c server.c  -lpthread
clean:
	rm server
