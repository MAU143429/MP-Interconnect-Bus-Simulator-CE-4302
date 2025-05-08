#ifndef PARSER_H
#define PARSER_H

#include "SMS.h"
#include <vector>
#include <string>

std::vector<SMS> parseInstructionsFromFile(const std::string& filename);

#endif
