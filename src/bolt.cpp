#include <bolt/bolt.hpp>

// for std::size_t
#include <cstddef>
// for std::malloc
#include <cstdlib>

namespace bolt
{


    ListValue::ListValue(
        std::shared_ptr<Array> flat_values,
        std::uint64_t flat_index_begin,
        std::uint64_t flat_index_end
    )
     :  m_flat_values(flat_values),
        m_flat_index_begin(flat_index_begin),
        m_flat_index_end(flat_index_end)
    {
    }

    StructValue::StructValue(std::shared_ptr<const StructArray> array, std::size_t index)
        : m_stuct_array(array),
          m_index(index)
    {
    }

    std::size_t StructValue::size()const
    {
        return m_stuct_array->field_values().size();
    }
    const std::vector<std::string> & StructValue::field_names()const
    {
        return m_stuct_array->field_names();
    }
    OptionalValue StructValue::operator[](const std::size_t index) const
    {
        return m_stuct_array->field_values()[index]->optional_value(m_index);
    }

    

    OptionalValue ListValue::operator[](const std::size_t index) const
    {
        return m_flat_values->optional_value(m_flat_index_begin + index);
    }

    uint64_t ListValue::size() const
    {
        return m_flat_index_end - m_flat_index_begin;
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

    void Array::assign_data(std::shared_ptr<ArrayData> data)
    {
        m_data = data;
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