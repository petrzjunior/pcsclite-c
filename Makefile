TARGET := pcsclite.out
OBJ := pcsclite.o
CPPFLAGS=-I/usr/include/PCSC/
LDFLAGS=-lpcsclite -L/usr/lib/


.PHONY: all clen

all: $(TARGET)

clean:
	rm -r $(TARGET) $(OBJ)

$(OBJ): %.o: %.c
	$(CC) -c -o $@ $< $(CPPFLAGS)

$(TARGET): %.out: %.o
	$(CC) -o $@ $< $(LDFLAGS)
