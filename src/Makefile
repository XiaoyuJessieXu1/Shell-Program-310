CC = gcc

framesize ?= 0
varmemsize ?= 0

FRAME_SIZE = $(framesize)
VAR_MEM_SIZE = $(varmemsize)

CFLAGS = -D FRAME_SIZE=$(FRAME_SIZE) -D VAR_MEM_SIZE=$(VAR_MEM_SIZE)

OBJS = shell.o interpreter.o shellmemory.o

mysh: .PHONY
	$(CC) $(CFLAGS) -o mysh $(OBJS) -lpthread

.PHONY: $(OBJS)
$(OBJS): shell.c interpreter.c shellmemory.c 
	$(CC) $(CFLAGS) -c shell.c interpreter.c shellmemory.c 

clean:
	rm mysh; rm *.o
