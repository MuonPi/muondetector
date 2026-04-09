#pragma once
#include <vector>
#include <memory>

#include "sources/source.h"

class SourceManager
{
public:
    void add(std::unique_ptr<Source> src);

    void updateAll();

private:
    std::vector<std::unique_ptr<Source>> m_sources;
};