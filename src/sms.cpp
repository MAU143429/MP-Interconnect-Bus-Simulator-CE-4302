#include "../include/SMS.h"
#include <iostream>

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
