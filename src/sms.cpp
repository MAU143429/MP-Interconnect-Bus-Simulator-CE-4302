#include "../include/sms.h"
#include <iostream>

SMS::SMS(MessageType t)
    : type(t),
      src(0),
      addr(0),
      size(0),
      qos(0),
      dest(0),
      cache_line(0),
      status(0) {}

void SMS::printInfo() const {
    
    
    std::cout << "Message Info:" << std::endl;
    std::cout << "Source: " << src << std::endl;
    std::cout << "Address: " << addr << std::endl;
    std::cout << "Size: " << size << std::endl;
    std::cout << "QoS: " << qos << std::endl;
    std::cout << "Destination: " << dest << std::endl;
    std::cout << "Cache Line: " << cache_line << std::endl;
    std::cout << "Status: " << status << std::endl;
}
