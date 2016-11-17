CC = gcc
CFLAGS = -Wall -pedantic -std=gnu99
DEBUG = -g
TARGETS = server client

all: clubhub clubber

# A debug target to update flags before cleaning and compiling all targets
debug: CFLAGS += $(DEBUG)
debug: clean $(TARGETS)

# Create a shared object for inclusion in our programs
clubhub: clubhub.o
	$(CC) $(CFLAGS) clubhub.o -o clubhub

clubber: clubber.o
	$(CC) $(CFLAGS)  clubber.o -o clubber

clubhub.o: clubhub.c
	$(CC) $(CFLAGS) -c clubhub.c -o clubhub.o

clubber.o: clubber.c clubber.h
	$(CC) $(CFLAGS) -c clubber.c -o clubber.o

# Clean up our directory - remove objects and binaries
clean:
	rm -f $(TARGETS) *.o
