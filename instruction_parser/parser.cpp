#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <unordered_map>
#include <cstdint>

struct Instruction {
    int opcode;
    std::vector<uint32_t> args;
};

std::unordered_map<std::string, int> opcodeMap = {
    {"WRITE_MEM", 1},
    {"READ_MEM", 2},
    {"BROADCAST_INVALIDATE", 3},
    {"INV_ACK", 4},
    {"INV_COMPLETE", 5},
    {"READ_RESP", 6},
    {"WRITE_RESP", 7}
};

uint32_t hexToDecimal(const std::string& hexStr) {
    return std::stoul(hexStr, nullptr, 16);
}

Instruction parseInstruction(const std::string& line) {
    std::istringstream iss(line);
    std::string opcodeStr;
    iss >> opcodeStr;

    Instruction instr;
    if (opcodeMap.find(opcodeStr) == opcodeMap.end()) {
        std::cerr << "Instrucción desconocida: " << opcodeStr << std::endl;
        exit(1);
    }

    instr.opcode = opcodeMap[opcodeStr];

    std::string arg;
    while (iss >> arg) {
        if (!arg.empty() && arg.back() == ',') {
            arg.pop_back();
        }

        if (arg.find("0x") == 0 || arg.find("0X") == 0) {
            instr.args.push_back(hexToDecimal(arg));
        } else {
            instr.args.push_back(std::stoul(arg));
        }
    }

    return instr;
}

void saveEncodedInstructions(const std::vector<Instruction>& program, const std::string& outFile) {
    std::ofstream out(outFile);
    if (!out.is_open()) {
        std::cerr << "No se pudo abrir el archivo de salida." << std::endl;
        return;
    }

    for (const auto& instr : program) {
        out << instr.opcode;
        for (const auto& arg : instr.args) {
            out << "," << arg;
        }
        out << "\n";
    }

    out.close();
    std::cout << "Instrucciones codificadas guardadas en: " << outFile << std::endl;
}

int main() {
    std::ifstream file("inst.txt"); // Cambia aquí el archivo de entrada si lo necesitas
    if (!file.is_open()) {
        std::cerr << "No se pudo abrir el archivo de entrada." << std::endl;
        return 1;
    }

    std::string line;
    std::vector<Instruction> program;

    while (std::getline(file, line)) {
        if (line.empty() || line.rfind("PE", 0) == 0) continue;
        program.push_back(parseInstruction(line));
    }

    file.close();

    saveEncodedInstructions(program, "inst_encoded.txt"); // Archivo de salida

    return 0;
}
