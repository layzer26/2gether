
#include "core/EventStore.h"
#include <iostream>

int main (){
    std::cout << "\nEventStore Version: " 
    << together::EventStore::version() 
    << std::endl;
    return 0;
}