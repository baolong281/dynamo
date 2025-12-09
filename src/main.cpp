#include "storage/disk_engine.h"
#include "server/server.h"

int main() {
    DiskEngine db{};
    Server service{std::move(db)};

    service.start("0.0.0.0", 8080);
}