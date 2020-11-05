TARGET  = ril
LDFLAGS = 
CFLAGS  = -I../include/ -O2
OBJS    = ril.o
LIBS    = ../lib/libril.a

all: $(OBJS) $(TARGET)

$(TARGET): $(OBJS)
				$(CC) $(LDFLAGS) -o $@ $(OBJS) $(LIBS)

.c.o:
				$(CC) $(CFLAGS) $(INCLUDES) -c $<

clean:
			-rm $(TARGET) $(OBJS)
