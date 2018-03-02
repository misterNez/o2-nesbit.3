CC = gcc -g
PROGS = oss user
OBJ1 = oss.o
OBJ2 = user.o

all: $(PROGS)

oss: $(OBJ1)
	$(CC) -o $@ $^

user: $(OBJ2)
	$(CC) -o $@ $^

clean:
	rm -rf *.o *.log $(PROGS)
