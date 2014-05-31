OBJDIR=objs

CC=clang
CXX=clang++
CFLAGS+=-c -Wall -Wextra -Werror -pedantic -g -std=c99
CXXFLAGS+=-c -Wall -Wextra -Werror -pedantic -g -std=c++11
LDFLAGS+=-lavcodec -lavformat -lavutil -lswresample -lportaudiocpp -lportaudio -lasound -lm -lpthread
SOURCES=$(wildcard *.cpp)
CSOURCES=contrib/pa_ringbuffer.c

OBJECTS=$(addprefix $(OBJDIR)/,$(SOURCES:.cpp=.o))
COBJECTS=$(addprefix $(OBJDIR)/,$(CSOURCES:.c=.o))
TARGET=playslave++

all: $(TARGET)

$(TARGET): mkdir $(COBJECTS) $(OBJECTS)
	$(CXX) $(COBJECTS) $(OBJECTS) $(LDFLAGS) -o $@

$(OBJDIR)/%.o: %.c
	$(CC) $(CFLAGS) $< -o $@

$(OBJDIR)/%.o: %.cpp
	$(CXX) $(CXXFLAGS) $< -o $@

clean:
	rm -f $(OBJECTS) $(COBJECTS) $(TARGET)

mkdir:
	mkdir -p $(OBJDIR)
	mkdir -p $(OBJDIR)/contrib

run: $(TARGET)
	./$(TARGET)

gdbrun: $(TARGET)
	gdb $(TARGET)
