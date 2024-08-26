#include <bolt/buffer.hpp>

namespace bolt
{
    Buffer::Buffer(std::size_t size)
        : m_data(std::malloc(size)), m_size(size), m_owned(true)
    {
    }
    Buffer::Buffer(void * data, std::size_t size)
        : m_data(data), m_size(size), m_owned(false)
    {
    }

    Buffer::~Buffer()
    {
        if (m_owned)
        {
            std::free(m_data);
        }
    }

    Buffer::Buffer(Buffer && other)
        : m_data(other.m_data), m_size(other.m_size), m_owned(other.m_owned)
    {
        other.m_data = nullptr;
        other.m_size = 0;
        other.m_owned = false;
    }

    void * Buffer::data() const
    {
        return m_data;
    }
    

    std::size_t Buffer::size() const
    {
        return m_size;
    }
    
}  // namespace bolt