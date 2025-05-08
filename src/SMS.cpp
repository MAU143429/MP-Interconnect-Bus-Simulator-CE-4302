#include "../include/SMS.h"
#include <iostream>
#include <string>

SMS::SMS()
    : type(MessageType::READ_MEM),  // Valor por defecto, puede ser cualquiera válido
      src(0), addr(0), size(0), qos(0), dest(0),
      inv_cache_line(0), num_of_cache_lines(0),
      start_cache_line(0), status(0), data{} {}

SMS::SMS(MessageType t)
    : type(t),
      src(0),
      addr(0),
      size(0),
      qos(0),
      dest(0),
      inv_cache_line(0),
      num_of_cache_lines(0),
      start_cache_line(0),
      status(0),
      data{} {}

void SMS::printInfo() const {
    
    std::cout << "Message Info:" << std::endl;
    std::cout << "Type: " << static_cast<int>(type) << std::endl;
    std::cout << "Source: " << src << std::endl;
    std::cout << "Address: " << addr << std::endl;
    std::cout << "Size: " << size << std::endl;
    std::cout << "QoS: " << qos << std::endl;
    std::cout << "Destination: " << dest << std::endl;
    std::cout << "Invalidated Cache Line: " << inv_cache_line << std::endl;
    std::cout << "Number of Cache Lines: " << num_of_cache_lines << std::endl;
    std::cout << "Start Cache Line: " << start_cache_line << std::endl;
    std::cout << "Status: " << status << std::endl;
    std::cout << "Data: ";
    for (const auto& d : data) {
        std::cout << d << " ";
    }
    std::cout << std::endl;
    
}


size_t SMS::calculateSize() const {
    size_t size_sms = sizeof(type) + sizeof(src) + sizeof(dest) + sizeof(qos);
    
    switch(type) {
        case MessageType::WRITE_MEM:
            size_sms += sizeof(addr) + sizeof(num_of_cache_lines) + sizeof(start_cache_line);
            size_sms += num_of_cache_lines * 16; // 16 bytes per cache line
            break;
        case MessageType::READ_MEM:
            size_sms += size;
            break;
        case MessageType::BROADCAST_INVALIDATE:
            size_sms += sizeof(inv_cache_line);
            break;
        case MessageType::READ_RESP:
            size_sms += data.size() * sizeof(int);
            break;
        case MessageType::WRITE_RESP:
            size_sms += sizeof(status);
            break;
        default:
            break;
    }
    
    return size_sms;
}
