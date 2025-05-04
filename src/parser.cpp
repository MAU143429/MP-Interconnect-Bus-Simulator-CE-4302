#include "../include/Parser.h"
#include <fstream>
#include <sstream>
#include <unordered_map>
#include <iostream>
#include <cstdint>

static std::unordered_map<std::string, MessageType> opcodeMap = {
    {"WRITE_MEM", MessageType::WRITE_MEM},
    {"READ_MEM", MessageType::READ_MEM},
    {"BROADCAST_INVALIDATE", MessageType::BROADCAST_INVALIDATE},
    {"INV_ACK", MessageType::INV_ACK},
    {"INV_COMPLETE", MessageType::INV_COMPLETE},
    {"READ_RESP", MessageType::READ_RESP},
    {"WRITE_RESP", MessageType::WRITE_RESP}
};

static int hexToDecimal(const std::string& hexStr) {
    return std::stoul(hexStr, nullptr, 16);
}

static SMS parseInstructionToSMS(const std::string& line) {
    std::istringstream iss(line);
    std::string opcodeStr;
    iss >> opcodeStr;

    if (opcodeMap.find(opcodeStr) == opcodeMap.end()) {
        std::cerr << "Instrucción desconocida: " << opcodeStr << std::endl;
        exit(1);
    }

    MessageType type = opcodeMap[opcodeStr];
    SMS sms(type);

    std::string arg;
    std::vector<int> args;

    while (iss >> arg) {
        if (!arg.empty() && arg.back() == ',') {
            arg.pop_back();
        }
        try {
            if (arg.find("0x") == 0 || arg.find("0X") == 0) {
                args.push_back(hexToDecimal(arg));
            } else {
                args.push_back(std::stoi(arg));
            }
        } catch (const std::invalid_argument& e) {
            std::cerr << "Error: Argumento inválido '" << arg << "' en la instrucción." << std::endl;
            exit(1);
        } catch (const std::out_of_range& e) {
            std::cerr << "Error: Argumento fuera de rango '" << arg << "' en la instrucción." << std::endl;
            exit(1);
        }
    }

    switch (type) {
        case MessageType::WRITE_MEM:
            if (args.size() >= 5) {
                sms.src = args[0];
                sms.addr = args[1];
                sms.num_of_cache_lines = args[2];
                sms.start_cache_line = args[3];
                sms.qos = args[4];
            }
            break;
        case MessageType::READ_MEM:
            if (args.size() >= 4) {
                sms.src = args[0];
                sms.addr = args[1];
                sms.size = args[2];
                sms.qos = args[3];
            }
            break;
        case MessageType::BROADCAST_INVALIDATE:
            if (args.size() >= 3) {
                sms.src = args[0];
                sms.inv_cache_line = args[1];
                sms.qos = args[2];
            }
            break;
        case MessageType::INV_ACK:
            if (args.size() >= 2) {
                sms.src = args[0];
                sms.qos = args[1];
            }
            break;
        case MessageType::INV_COMPLETE:
            if (args.size() >= 2) {
                sms.dest = args[0];
                sms.qos = args[1];
            }
            break;
        case MessageType::READ_RESP:
            if (args.size() >= 3) {
                sms.dest = args[0];
                sms.data.push_back(args[1]);
                sms.qos = args[2];
            }
            break;
        case MessageType::WRITE_RESP:
            if (args.size() >= 3) {
                sms.dest = args[0];
                sms.status = args[1];
                sms.qos = args[2];
            }
            break;
    }

    return sms;
}

std::vector<SMS> parseInstructionsFromFile(const std::string& filename) {
    std::ifstream file(filename);
    std::vector<SMS> messages;

    if (!file.is_open()) {
        std::cerr << "No se pudo abrir el archivo: " << filename << std::endl;
        return messages;
    }

    std::string line;
    
    while (std::getline(file, line)) {
        if (line.empty() || line.rfind("PE", 0) == 0) continue;
        messages.push_back(parseInstructionToSMS(line));
    }

    file.close();
    return messages;
}
