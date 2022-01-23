#include "ShowtimeURI.h"

void UShowtimeURI::Init(const ZstURI& URI_to_wrap)
{
    m_wrapped_URI = URI_to_wrap;
}

int32 UShowtimeURI::Size() const
{
    return m_wrapped_URI.size();
}

int32 UShowtimeURI::FullSize() const
{
    return m_wrapped_URI.full_size();
}

FName UShowtimeURI::Path() const
{
    return UTF8_TO_TCHAR(m_wrapped_URI.path());
}

const ZstURI& UShowtimeURI::WrappedURI() const
{
    return m_wrapped_URI;
}
