micro: micro.c input.o output.o bufferOperations.o search.o fileIO.o microOperations.o rowOperations.o syntaxHighlighting.o terminalManipulation.o
	$(CC) -o micro micro.c input.o output.o bufferOperations.o search.o fileIO.o microOperations.o rowOperations.o syntaxHighlighting.o terminalManipulation.o -Wall -Werror -Wextra -pedantic -std=c99
	rm -f *.o

input.o: input.h input.c
	$(CC) -c input.c

output.o: output.h output.c
	$(CC) -c output.c

bufferOperations.o: bufferOperations.h bufferOperations.c
	$(CC) -c bufferOperations.c

search.o: search.h search.c
	$(CC) -c search.c

fileIO.o: fileIO.h fileIO.c micro.h
	$(CC) -c fileIO.c

microOperations.o: microOperations.h microOperations.c
	$(CC) -c microOperations.c

syntaxHighlighting.o: syntaxHighlighting.h syntaxHighlighting.c
	$(CC) -c syntaxHighlighting.c

rowOperations.o: rowOperations.h rowOperations.c
	$(CC) -c rowOperations.c

terminalManipulation.o: terminalManipulation.h terminalManipulation.c
	$(CC) -c terminalManipulation.c

clean:
	rm -f *.o