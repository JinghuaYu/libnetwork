CC = gcc
CFLAGS = -Wall -std=gnu99

TARGET = nettest
# C�ļ�
SRCS = nettest.c
INC = -I./
DLIBS = -lnetwork
LDFLAGS = -L./build
RPATH = -Wl,-rpath=./build

OBJS = $(SRCS:.c=.o)

# ����Ϊ��ִ���ļ�
$(TARGET):$(OBJS)
	$(CC) -o $@ $^ $(LDFLAGS) $(DLIBS) $(RPATH)

clean:
	rm -rf $(TARGET) $(OBJS)


# ������� $@����Ŀ���ļ� $< �����һ�������ļ�
%.o:%.c
	$(CC) $(CFLAGS) $(INC) -o $@ -c $<
