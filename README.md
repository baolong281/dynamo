# dynamo

**goal:** a highly available, eventually consistent, low latency kv store. 

## Web UI

To use the development web UI (avoids CORS errors):
```bash
cd webui && ./serve.sh
```
Then open http://localhost:3000 in your browser.

TODO: 
- [x] Basic in memory storage layer and http connections
    - [] investigate leveldb stalling
- [x] Consistent hashing and partitioning across virtual nodes
- [x] Persistent storage and crash recovery
- [x] Replication
- [x] R/W quorom
- [ ] data versioning and vector clocks
- [ ] failure handling (?)