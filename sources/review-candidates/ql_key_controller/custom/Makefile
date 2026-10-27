SOURCE = $(wildcard *.c) \

TARGET := libql_custom_key_handler.so

all : $(TARGET)

$(TARGET) :
	$(CC) $(CFLAGS) -shared -fPIC -o $@ $(SOURCE)
