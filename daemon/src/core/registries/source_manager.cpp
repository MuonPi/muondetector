#include "source_manager.h"

void SourceManager::add(std::unique_ptr<Source> src)
{
    m_sources.push_back(std::move(src));
}

void SourceManager::updateAll()
{
    for (auto& s : m_sources)
        s->update();
}