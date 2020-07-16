CC       = gcc
object	 = build_header.o

CFLAGS = -Wall -Wstrict-prototypes -g
#CFLAGS =

BUILD_TOOL   = build_header

all: $(BUILD_TOOL)

$(BUILD_TOOL): $(object)
	$(CC) -o $@ $<

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<
clean:
	rm -f *.o 
	rm -f build_header

