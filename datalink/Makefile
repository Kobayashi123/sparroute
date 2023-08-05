OBJS = ltest.o
SRCS = $(OBJS:%.o=%.c)
CFLAGS = -g -Wall # -g: debug, -Wall: show all warnings
LDLIBS =
TARGET = ltest
$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $ $(TARGET) $(OBJS) $(LDLIBS)

clean:
	-rm $(OBJS) $(TARGET)
