#ifndef PARSER_H
#define PARSER_H

#include "SMS.h"
#include <vector>
#include <string>

// Función para convertir las instrucciones de un archivo a objetos SMS
std::vector<SMS> parseInstructionsFromFile(const std::string& filename, int QoS);

#endif
