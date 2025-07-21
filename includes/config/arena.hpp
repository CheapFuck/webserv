#pragma once

#include <vector>
#include <new>

class Arena {
    struct Destructible {
        void* ptr;
        void (*destroy)(void*);
    };

    std::vector<void*> memoryBlocks;
    std::vector<Destructible> destructibles;

public:
    Arena() = default;
    Arena(const Arena&) = delete;
    Arena& operator=(const Arena&) = delete;
    ~Arena();

    template <typename T, typename... Args>
    T* alloc(Args&&... args) {
        void* mem = ::operator new(sizeof(T));
        memoryBlocks.push_back(mem);

        T* obj = new (mem) T(std::forward<Args>(args)...);

        if constexpr (!std::is_trivially_destructible_v<T>) {
            destructibles.push_back({
                obj,
                [](void* p) {
                    static_cast<T*>(p)->~T();
                }
            });
        }

        return obj;
    }
};
