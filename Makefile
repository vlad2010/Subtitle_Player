# Run make with  DEBUG=-DDEBUG to turn on debug output

TARGET=$(shell basename `pwd`)
SOURCES=$(wildcard *.cpp)
OBJECTS=$(SOURCES:%.cpp=%.o)

CFLAGS="-std=c++11"
LDLIBS=-l ncurses -l rt 


all: $(TARGET)
	@echo  "All done"

$(OBJECTS): $(SOURCES)
	$(CXX) -g -c $(SOURCES) $(CFLAGS) $(DEBUG)  -Wno-write-strings -v

$(TARGET): $(OBJECTS)
	$(CXX) -o $(TARGET) $(LDFLAGS) $(OBJECTS) $(LOADLIBES) $(LDLIBS) -v

clean:
	$(RM) $(TARGET) *.o *.txt

.PHONY: all clean


