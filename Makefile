TARGET := pcsclite.out
OBJ := pcsclite.o
CPPFLAGS=-I/usr/include/PCSC/ -g
LDFLAGS=-lpcsclite -L/usr/lib/ -g


.PHONY: all clen

all: $(TARGET)

clean:
	rm -r $(TARGET) $(OBJ)

$(OBJ): %.o: %.c
	$(CC) -c -o $@ $< $(CPPFLAGS)

$(TARGET): %.out: %.o
	$(CC) -o $@ $< $(LDFLAGS)
