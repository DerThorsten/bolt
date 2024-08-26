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


}  // namespace bolt