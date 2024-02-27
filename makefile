CC = gcc
CFLAGS = -Wall -pthread
LIBS = 
TARGET = webserver

all: $(TARGET)

$(TARGET):
	$(CC) -o $(TARGET) $(CFLAGS) $(LIBS) $(TARGET).c

clear:
	rm $(TARGET)

release:
	$(CC) -o $(TARGET) $(CFLAGS) $(LIBS) $(TARGET).c -O3

move:
	mv examples/* .

back:
	mv index.html examples/ && mv newPage.html examples/ && mv page.css examples/
	mv favicon.ico examples/
