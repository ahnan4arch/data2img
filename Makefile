
CFLAGS= -std=c99
LDFLAGS= -lSDL2main -lSDL2

all: data2img

data2img: data2img.o
	gcc data2img.o ${LDFLAGS} -o data2img

data2img.o: data2img.c
	gcc ${CFLAGS} -c data2img.c


clean:
	rm -rf *.o *.exe data2img
