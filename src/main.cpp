#include "storage/memory_engine.h"
#include "server/server.h"

int main() {
    MemoryEngine db{};
    Server service{db};

    service.start("0.0.0.0", 8080);
}