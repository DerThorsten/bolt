#include <bolt/bolt.hpp>

// for std::size_t
#include <cstddef>
// for std::malloc
#include <cstdlib>

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
    
    // from r-value reference of ArrayData
    Array::Array(ArrayData && data)
        : m_data(std::make_shared<ArrayData>(std::move(data))),
          p_validity_bitmap(nullptr)
    {
        // for all layouts except union layout, the validity bitmap is stored in the data
        if (m_data->format() != "+ud"  && m_data->format() != "+us")
        {     
            p_validity_bitmap = static_cast<uint8_t *>(m_data->buffers()[0]->data());
        }
    }

    Array::Array(std::shared_ptr<ArrayData> data)
        : m_data(data),
          p_validity_bitmap(nullptr)
    {
        // for all layouts except union layout, the validity bitmap is stored in the data
        if (m_data->format() != "+ud"  && m_data->format() != "+us")
        {
            p_validity_bitmap = static_cast<uint8_t *>(m_data->buffers()[0]->data());
        }
    }


}  // namespace bolt