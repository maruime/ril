TARGET  = ../lib/libril.a
OBJS     = ril_api.o ril_compiler.o ril_utils.o ril_var.o ril_vm.o ril_tag.o ril_state.o ril_buffer.o ril_calc.o
OBJS    += list.o crc.o md5.o hashmap.o variant.o
CC       = gcc
CFLAGS   = -O2 -Wall #-D_DEBUG -g
INCLUDES = -I../include

all: $(OBJS) $(TARGET)

$(TARGET): $(OBJS)
				ar rcu $@ $(OBJS)

.c.o:
				$(CC) $(CFLAGS) $(INCLUDES) -c $<

clean:
				-rm $(OBJS) $(TARGET)
