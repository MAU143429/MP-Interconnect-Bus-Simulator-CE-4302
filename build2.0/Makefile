# Nombre del ejecutable
TARGET = simulator

# Directorios
SRC_DIR = src
INC_DIR = include
OBJ_DIR = build

# Archivos fuente sin ruta
SRC_FILES = Simulator.cpp PE.cpp SMS.cpp Memory.cpp Parser.cpp Interconnect.cpp

# Generar rutas absolutas
SRCS = $(addprefix $(SRC_DIR)/, $(SRC_FILES))
OBJS = $(patsubst %.cpp, $(OBJ_DIR)/%.o, $(SRC_FILES))

# Compilador y flags
CXX = g++
#CXXFLAGS = -std=c++20 -Wall -Wextra -O2 -I$(INC_DIR)
CXXFLAGS = -std=c++20 -w -O2 -I$(INC_DIR)


# Regla principal
all: $(TARGET)

# Ejecutable: depende de objetos en build/
$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^

# Regla para compilar .cpp a .o en build/
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp | $(OBJ_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Crear directorio build/ si no existe
$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

# Limpiar todo
clean:
	rm -rf $(OBJ_DIR) $(TARGET)

# Ejecutar
run: $(TARGET)
	./$(TARGET)
