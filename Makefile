# Makefile for Authentication Server and Password Cracker

CC = gcc
CFLAGS = -Wall -Wextra -O2
LIBS = -lssl -lcrypto

# Targets
all: server cracker

server: server.c
	$(CC) $(CFLAGS) -o server server.c $(LIBS)
	@echo "✓ Server compiled successfully"

cracker: cracker.c
	$(CC) $(CFLAGS) -o cracker cracker.c
	@echo "✓ Cracker compiled successfully"

clean:
	rm -f server cracker
	@echo "✓ Cleaned build files"

run-server: server
	./server

run-cracker-mode1: cracker
	./cracker 1

run-cracker-mode2: cracker
	@echo "Usage: make run-cracker-mode2 USERNAME=<username>"
	@if [ -z "$(USERNAME)" ]; then \
		echo "Error: Please specify USERNAME=<username>"; \
		exit 1; \
	fi
	./cracker 2 $(USERNAME)

help:
	@echo "Available targets:"
	@echo "  make all              - Compile server and cracker"
	@echo "  make server           - Compile only server"
	@echo "  make cracker          - Compile only cracker"
	@echo "  make run-server       - Run the authentication server"
	@echo "  make run-cracker-mode1 - Run cracker (mode 1: both username & password)"
	@echo "  make run-cracker-mode2 USERNAME=admin - Run cracker (mode 2: known username)"
	@echo "  make clean            - Remove compiled files"

.PHONY: all clean run-server run-cracker-mode1 run-cracker-mode2 help
