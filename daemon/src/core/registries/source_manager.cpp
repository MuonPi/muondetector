#include "source_manager.h"

#include <vector>
#include <memory>
#include <string>
#include <mutex>
#include <unordered_map>


void SourceManager::add(SourceId id, std::shared_ptr<Source> src)
{
    m_sources[id] = std::move(src);
}

void SourceManager::updateAll()
{
    for (auto &s : m_sources)
    {
        s.second->update();
    }
}