#ifndef SOURCE_MANAGER_H
#define SOURCE_MANAGER_H

#include "sources/source.h"

#include <vector>
#include <memory>
#include <string>
#include <mutex>
#include <unordered_map>

class SourceManager
{
public:
    void add(SourceId id, std::shared_ptr<Source> src);

    template<typename T>
    T* get(SourceId id)
    {
        std::lock_guard<std::mutex> lock(m_mutex);

        auto it = m_sources.find(id);
        if (it == m_sources.end()) {
            return nullptr;
        }

        return dynamic_cast<T*>(it->second.get());
    }

    void updateAll();

private:
    std::mutex m_mutex;
    std::unordered_map<SourceId, std::shared_ptr<Source>> m_sources;
};

#endif // SOURCE_MANAGER_H^