#include "config/arena.hpp"

Arena::~Arena() {
    for (auto it = destructibles.rbegin(); it != destructibles.rend(); ++it)
        it->destroy(it->ptr);
    
    for (void *block : memoryBlocks)
        ::operator delete(block);
}
