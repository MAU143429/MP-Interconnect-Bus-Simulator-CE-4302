# Nombre del ejecutable
TARGET = simulator

# Directorios (relativos a build2.0/)
SRC_DIR = ../src
INC_DIR = ../include
OBJ_DIR = ./build

# Archivos fuente
SRC_FILES = Simulator.cpp PE.cpp SMS.cpp Memory.cpp Parser.cpp Interconnect.cpp
SRCS = $(addprefix $(SRC_DIR)/, $(SRC_FILES))
OBJS = $(addprefix $(OBJ_DIR)/, $(patsubst %.cpp, %.o, $(SRC_FILES)))

# Compilador y flags
CXX = g++
#CXXFLAGS = -std=c++20 -Wall -Wextra -O2 -I$(INC_DIR)
CXXFLAGS = -std=c++20 -w -O2 -I$(INC_DIR)


# Regla principal
all: $(TARGET)

# Enlazado del ejecutable
$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^

# Compilación de objetos
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp | $(OBJ_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Crear carpeta build/ si no existe
$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)


run: $(TARGET)
	./$(TARGET)

# Limpiar
clean:
	rm -rf $(OBJ_DIR)/*.o $(TARGET)

