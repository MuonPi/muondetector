#include "source.h"

Source::Source(SourceId id) : m_id{id} {}

auto Source::id() -> SourceId
{
    return m_id;
}