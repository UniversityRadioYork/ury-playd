OBJDIR=objs

CC=clang
CXX=clang++
CFLAGS+=-c -Wall -Wextra -Werror -pedantic -g -std=c99
CXXFLAGS+=-c -Wall -Wextra -Werror -pedantic -g -std=c++11
LDFLAGS+=-lavcodec -lavformat -lavutil -lswresample -lportaudiocpp -lportaudio -lasound -lm -lpthread -lboost_system
SOURCES=$(wildcard *.cpp)
SOURCES+=$(wildcard audio/*.cpp)
SOURCES+=$(wildcard io/*.cpp)
SOURCES+=$(wildcard player/*.cpp)
SOURCES+=$(wildcard ringbuffer/*.cpp)
CSOURCES=contrib/pa_ringbuffer.c

OBJECTS=$(addprefix $(OBJDIR)/,$(SOURCES:.cpp=.o))
COBJECTS=$(addprefix $(OBJDIR)/,$(CSOURCES:.c=.o))
TARGET=playslave++

all: mkdir $(TARGET)

$(TARGET): $(COBJECTS) $(OBJECTS)
	$(CXX) $(COBJECTS) $(OBJECTS) $(LDFLAGS) -o $@

$(OBJDIR)/%.o: %.c
	$(CC) $(CFLAGS) $< -o $@

$(OBJDIR)/%.o: %.cpp
	$(CXX) $(CXXFLAGS) $< -o $@

clean:
	rm -f $(OBJECTS) $(COBJECTS) $(TARGET)

mkdir:
	mkdir -p $(OBJDIR)
	mkdir -p $(OBJDIR)/audio
	mkdir -p $(OBJDIR)/contrib
	mkdir -p $(OBJDIR)/io
	mkdir -p $(OBJDIR)/player
	mkdir -p $(OBJDIR)/ringbuffer

run: $(TARGET)
	./$(TARGET)

gdbrun: $(TARGET)
	gdb $(TARGET)
