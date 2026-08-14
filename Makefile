CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -g

TARGET = chess

SOURCES = main.cpp movegen.cpp game.cpp
HEADERS = types.h movegen.h game.h

$(TARGET): $(SOURCES) $(HEADERS)
	$(CXX) $(CXXFLAGS) $(SOURCES) -o $(TARGET)

clean:
	rm -f $(TARGET)

run: $(TARGET)
	./$(TARGET)
