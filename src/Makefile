# Nombre del ejecutable
TARGET = simulator

# Archivos fuente
SRCS = simulator.cpp PE.cpp sms.cpp

# Archivos objeto (generados automáticamente)
OBJS = $(SRCS:.cpp=.o)

# Flags del compilador
CXX = g++
CXXFLAGS = -std=c++20 -Wall -Wextra -O2

# Regla por defecto
all: $(TARGET)

# Compilar el ejecutable
$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^

# Regla para archivos .o
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Limpiar archivos temporales
clean:
	rm -f $(OBJS) $(TARGET)

# Ejecutar el simulador
run: $(TARGET)
	./$(TARGET)
