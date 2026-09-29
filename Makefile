CXX = g++
CXXFLAGS = -std=c++14 -O3 -Wall -Wextra -Iinclude

TARGET = vertex_processor.exe
SRC = src/main.cpp

all: $(TARGET)

$(TARGET): $(SRC) include/*.hpp
	$(CXX) $(CXXFLAGS) $(SRC) -o $(TARGET)

clean:
	del /Q $(TARGET) *.bmp 2>NUL || rm -f $(TARGET) *.bmp

run: $(TARGET)
	./$(TARGET)
