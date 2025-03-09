
# Makefile for v4l2_alsa_ff project

# Compiler
CC = arm-buildroot-linux-gnueabihf-gcc

# Compiler flags
CFLAGS = -Wall -g

# Linker flags
LDFLAGS = -lavcodec -lavfilter -lavutil -lswresample -lavdevice -lavformat -lswscale -lasound -lpthread

# Source files
SRCS = main.c audio.c ffmpeg_process.c list.c thread_task.c v4l2.c

# Header files
HDRS = audio.h ffmpeg_process.h list.h thread_task.h v4l2.h common.h

# Output executable
TARGET = my_program

# Default target
all: $(TARGET)

# Rule to create the final executable
$(TARGET): $(SRCS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

# Clean target to remove the executable
clean:
	rm -f $(TARGET)

# Phony targets
.PHONY: all clean
