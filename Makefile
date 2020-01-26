TARGET := pcsclite.out
OBJ := pcsclite.o

.PHONY: all clen

all: $(TARGET)

clean:
	rm -r $(TARGET) $(OBJ)

$(OBJ): %.o: %.c
	$(CC) -c -o $@ $<

$(TARGET): %.out: %.o
	$(CC) -o $@ $<
