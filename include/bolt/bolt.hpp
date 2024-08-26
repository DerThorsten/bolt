#pragma once

#include <cstddef>
#include <vector>
#include <memory>
#include <string>
#include <ranges>
#include <iostream>

#include <bolt/format.hpp>
#include <bolt/buffer.hpp>
#include <bolt/array_data.hpp>

namespace bolt
{ 


    class Array
    {
        public:

        Array(ArrayData && data);
        Array(std::shared_ptr<ArrayData> data);
        ~Array() = default;
        
        bool is_valid(std::size_t index) const
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

        std::size_t size() const
        {
            return m_data->m_length  - m_data->m_offset;
        }
        std::shared_ptr<ArrayData> array_data() const
        {
            return m_data;
        }
        protected:

        std::shared_ptr<ArrayData> m_data;
        uint8_t * p_validity_bitmap = nullptr;
    };
    
    template< bool BIG> // either int32_t or int64_t offsets
    class ListArrayImpl : public  Array
    {   
        public:
        using offset_type = std::conditional_t<BIG, std::int64_t, std::int32_t>;

        template<std::ranges::range T, std::ranges::range U>
        static std::shared_ptr<ArrayData>  make_list_array_data(std::shared_ptr<Array> values, T && sizes, U && validity_bitmap)
        {
            std::shared_ptr<ArrayData> list_array_data = std::make_shared<ArrayData>();
            list_array_data->set_format(BIG ? std::string("+L") : std::string("+l"));
            list_array_data->set_length(std::ranges::size(sizes));
            list_array_data->set_offset(0);
            list_array_data->add_child(values->array_data());

            // buffer for validity
            auto [validity, nullcount] = ArrayData::make_validity_bitmap(list_array_data->length(), std::forward<U>(validity_bitmap));
            list_array_data->add_buffer(validity);
            list_array_data->set_null_count(nullcount);

            // buffer for offsets
            auto offset_buffer = std::make_shared<Buffer>(sizeof(offset_type) * (list_array_data->length() + 1));
            list_array_data->add_buffer(offset_buffer);
            auto p_offsets = static_cast<offset_type *>(offset_buffer->data());

            p_offsets[0] = 0;
            for(std::size_t i = 0; i < list_array_data->length(); i++)
            {
                p_offsets[i+1] = p_offsets[i] + sizes[i];
            }

            return list_array_data;
        }

        template<std::ranges::range T, std::ranges::range U>
        ListArrayImpl(std::shared_ptr<Array> flat_values, T && sizes, U && validity_bitmap)
            : Array(make_list_array_data(flat_values, std::forward<T>(sizes), std::forward<U>(validity_bitmap))),
            p_offsets(static_cast<offset_type *>(m_data->buffers()[1]->data()) + m_data->offset()),
            m_flat_values(flat_values)
        {
        }

        offset_type list_size(const std::size_t index) const
        {
            return p_offsets[index + 1] - p_offsets[index];
        }
        // TODO fix for offset!
        std::shared_ptr<Array> values() const
        {
            return m_flat_values;
        }

        private:
        
        std::shared_ptr<Array> m_flat_values;
        offset_type * p_offsets;
    };

    using ListArray = ListArrayImpl<false>;
    using BigListArray = ListArrayImpl<true>;
    
    template<class T>
    class NumericArray : public Array
    {
        public:

        template<std::ranges::range U, std::ranges::range V>
        NumericArray(U && values, V && validity_bitmap)
            : Array(ArrayData::make_owned_fixed_size_primite<T>(std::forward<U>(values), std::forward<V>(validity_bitmap))),
            p_values(static_cast<T *>(m_data->buffers()[1]->data())+ m_data->offset())
        {
        }
        const T & operator[](std::size_t index) const
        {
            return p_values[index];
        }
        private:
        T * p_values;
    };


    // variable sized binary layout
    template< bool BIG> // either int32_t or int64_t offsets
    class StringArrayImpl : public Array
    {
        public:
        using offset_type = std::conditional_t<BIG, std::int64_t, std::int32_t>;

        template<std::ranges::range U, std::ranges::range V>
        StringArrayImpl(U && values, V && validity_bitmap)
            : Array(ArrayData::make_owned_string_array<offset_type>(std::forward<U>(values), std::forward<V>(validity_bitmap))),
            p_offsets(static_cast<offset_type *>(m_data->buffers()[1]->data()) + m_data->offset()),
            p_values(static_cast<char *>(m_data->buffers()[2]->data()) + m_data->offset())
        {
        }
        inline std::string_view operator[](std::size_t index) const
        {
            const auto size = p_offsets[index + 1] - p_offsets[index];  
            const auto begin = p_values + p_offsets[index];
            return std::string_view(begin, size);
        }
        private:

        offset_type * p_offsets;
        char * p_values;

    };

    using StringArray = StringArrayImpl<false>;
    using BigStringArray = StringArrayImpl<true>;



}