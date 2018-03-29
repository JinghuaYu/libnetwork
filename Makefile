CC = gcc
CFLAGS = -Wall -std=gnu99

TARGET = nettest
# C文件
SRCS = nettest.c
INC = -I./
DLIBS = -lnetwork
LDFLAGS = -L./build
RPATH = -Wl,-rpath=./build

OBJS = $(SRCS:.c=.o)

# 链接为可执行文件
$(TARGET):$(OBJS)
	$(CC) -o $@ $^ $(LDFLAGS) $(DLIBS) $(RPATH)

clean:
	rm -rf $(TARGET) $(OBJS)


# 编译规则 $@代表目标文件 $< 代表第一个依赖文件
%.o:%.c
	$(CC) $(CFLAGS) $(INC) -o $@ -c $<
