#include "storage/storage_engine.h"
#include "storage/memory_engine.h"
#include <iostream>

int main() {
    MemoryEngine db{};

    std::vector<std::byte> raw_data = {std::byte{0x01}, std::byte{0x02}, std::byte{0x03}};
    Value v{ raw_data }; 

    db.put("base", v);
}