CC := gcc
RM := rm -rf

CFLAGS += -Wall -O2

APP := adler32

SRC := adler32.c

all: $(APP)

$(APP): $(SRC)
	$(CC) $(CFLAGS) -o $@ $^

clean:
	$(RM) $(APP)
