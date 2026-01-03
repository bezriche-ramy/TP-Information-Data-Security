# Makefile for Information Data Security Assignment

CC = gcc
NVCC = nvcc
CFLAGS = -Wall -Wextra -std=c99
NVFLAGS = -O3 -arch=sm_86
TARGET = auth_system
GPU_TARGET = gpu_cracker
SRC = auth_system.c
GPU_SRC = gpu_cracker.cu

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC)

# GPU Cracker (requires CUDA)
gpu: $(GPU_SRC)
	$(NVCC) $(NVFLAGS) -o $(GPU_TARGET) $(GPU_SRC)

clean:
	rm -f $(TARGET) $(GPU_TARGET) password.txt banned.txt

run: $(TARGET)
	./$(TARGET)

run-gpu: gpu
	./$(GPU_TARGET)

.PHONY: all clean run gpu run-gpu
