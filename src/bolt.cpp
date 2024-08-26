#include <bolt/bolt.hpp>

// for std::size_t
#include <cstddef>
// for std::malloc
#include <cstdlib>

namespace bolt
{
    
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

    bool Array::is_valid(std::size_t index) const
    {
        if(p_validity_bitmap == nullptr)
        {
            throw std::runtime_error("validity for union is not yet implemented");
        }
        else
        {
            return (p_validity_bitmap[index / 8] & (1 << (index % 8))) != 0;
        }
    }

    std::size_t Array::size() const
    {
        return m_data->m_length  - m_data->m_offset;
    }
    std::shared_ptr<ArrayData> Array::array_data() const
    {
        return m_data;
    }

    
    


}  // namespace bolt