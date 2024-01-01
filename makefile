CC = gcc
CFLAGS = -Wall
LIBS = 
TARGET = webserver

all: $(TARGET)

$(TARGET):
	$(CC) -o $(TARGET) $(CFLAGS) $(LIBS) $(TARGET).c

clear:
	rm $(TARGET)

release:
	$(CC) -o $(TARGET) $(CFLAGS) $(LIBS) $(TARGET).c -O3
