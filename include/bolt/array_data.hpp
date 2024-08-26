#pragma once

#include <cstddef>
#include <vector>
#include <memory>
#include <string>
#include <ranges>
#include <iostream>

#include <bolt/format.hpp>
#include <bolt/buffer.hpp>

namespace bolt
{ 

    class Buffer;
    class ArrayData;
    class Array;


    class ArrayData
    {   
        public: 

        friend class Array;
        
        
        template<class OFFSET_TYPE, std::ranges::range T,  std::ranges::range U>
        static std::shared_ptr<ArrayData> make_owned_string_array(
            T && values,
            U && validity_bitmap
        )
        {
            std::shared_ptr<ArrayData> data = std::make_shared<ArrayData>();

            data->m_format = ""; // todo
            data->m_length = std::ranges::size(values);             

            // validity buffer
            auto [validity_buffer, null_count] = data->make_validity_bitmap(data->m_length, std::forward<U>(validity_bitmap));
            data->m_buffers.push_back(std::move(validity_buffer));
            data->m_null_count = null_count;
            
            // offset buffer
            auto offset_buffer = std::make_shared<Buffer>(sizeof(OFFSET_TYPE) * (data->m_length + 1));
            data->m_buffers.push_back(std::move(offset_buffer));
            OFFSET_TYPE * offset_ptr = static_cast<OFFSET_TYPE *>(data->m_buffers[1]->data());\
            fill_offset_ptr(values, validity_bitmap, offset_ptr);

            // value buffer
            auto begin_values = std::ranges::begin(values);
            auto begin_validity = std::ranges::begin(validity_bitmap);
            int total_size = 0;
            while(begin_values != std::ranges::end(values))
            {
                if(*begin_validity)
                {
                    total_size += begin_values->size();
                }
                begin_values++;
                begin_validity++;
            }

            auto value_buffer = std::make_shared<Buffer>(total_size);
            data->m_buffers.push_back(std::move(value_buffer));
            char * value_ptr = static_cast<char *>(data->m_buffers[2]->data()); 
            for(std::size_t i = 0; i < data->m_length; i++)
            {
                if(validity_bitmap[i])
                {
                    const auto size = values[i].size();
                    std::copy(values[i].begin(), values[i].end(), value_ptr);
                    value_ptr += size;
                }
            }
            
            return data;
        }

        template<class VALUE_TYPE, std::ranges::range T,  std::ranges::range U>
        static std::shared_ptr<ArrayData> make_owned_fixed_size_primite(
            T && values,
            U && validity_bitmap
        )
        {
            using value_type = VALUE_TYPE;
            constexpr std::size_t dsize = sizeof(VALUE_TYPE);

            std::shared_ptr<ArrayData> data = std::make_shared<ArrayData>();
            data->m_format = primitive_to_format<value_type>();
            data->m_length = std::ranges::size(values);
            data->m_null_count = 0;
            data->m_offset = 0;

            
            auto [validity_buffer, null_count] = data->make_validity_bitmap(data->m_length, std::forward<U>(validity_bitmap));
            data->m_buffers.push_back(std::move(validity_buffer));
            data->m_null_count = null_count;

            auto buffer = std::make_shared<Buffer>(dsize * data->m_length);
            data->m_buffers.push_back(std::move(buffer));   

            auto ptr = static_cast<VALUE_TYPE *>(data->m_buffers[1]->data());
            for(std::size_t i = 0; i < data->m_length; i++)
            {   
                ptr[i] = static_cast<VALUE_TYPE>(values[i]);
            }
            return data;
        }
        

        // default constructors 
        ArrayData() = default;
        // copy constructor
        ArrayData(const ArrayData & other) = default;
        // move constructor
        ArrayData(ArrayData && other) = default;
        // copy assignment
        ArrayData & operator=(const ArrayData & other) = default;
        // move assignment
        ArrayData & operator=(ArrayData && other) = default;

        inline std::size_t size() const
        {
            return m_length - m_offset;
        }
        inline std::size_t length() const
        {
            return m_length;
        }
        inline std::size_t offset() const
        {
            return m_offset;
        }
        inline std::vector<std::shared_ptr<Buffer>> buffers() const
        {
            return m_buffers;
        }
        inline std::string format() const
        {
            return m_format;
        }

        // setterrs for all the fields
        inline void set_length(std::size_t length)
        {
            m_length = length;
        }
        inline void set_null_count(std::size_t null_count)
        {
            m_null_count = null_count;
        }
        inline void set_offset(std::size_t offset)
        {
            m_offset = offset;
        }
        inline void set_format(std::string format)
        {
            m_format = format;
        }

        // add child to the array
        inline void add_child(std::shared_ptr<ArrayData> child)
        {
            m_children.push_back(child);
        }

        // add buffer to the array
        inline void add_buffer(std::shared_ptr<Buffer> buffer)
        {
            m_buffers.push_back(buffer);
        }


        template<std::ranges::range U>
        static std::pair<std::shared_ptr<Buffer>, std::int64_t> make_validity_bitmap(
            std::int64_t length,
            U && validity_bitmap)
        {
            std::int64_t null_count = 0;
            const auto validity_buffer_length = (length + 7) / 8;
            auto validity_buffer = std::make_shared<Buffer>(validity_buffer_length);
            uint8_t * validity_buffer_ptr = static_cast<uint8_t *>(validity_buffer->data());
            std::fill(validity_buffer_ptr, validity_buffer_ptr + validity_buffer_length, 0);

            // copy the value "all bits one" to the validity buffer
            for(std::size_t i = 0; i < length; i++)
            {
                if(validity_bitmap[i])
                {
                    validity_buffer_ptr[i / 8] |= (1 << (i % 8));
                }
                else
                {
                    null_count++;
                }
            }
            return std::make_pair(validity_buffer, null_count);
        }

        template<std::ranges::range T, std::ranges::range U, class OFFSET>
        static void fill_offset_ptr(T && values, U && is_valid, OFFSET * offset_ptr)
        {
            std::size_t i = 0;
            offset_ptr[0] = 0;
            
            auto iter_values = std::ranges::begin(values);
            auto iter_is_valid = std::ranges::begin(is_valid);

            while(iter_values != std::ranges::end(values))
            {
                if(*iter_is_valid)
                {   
                    const auto size  = iter_values->size();
                    offset_ptr[i + 1] = offset_ptr[i] + size;

                }
                else{
                    offset_ptr[i + 1] = offset_ptr[i];
                }
                iter_values++;
                iter_is_valid++;
                ++i;
            }
        }

        std::string m_format;
        std::int64_t m_length;
        std::int64_t m_null_count;
        std::int64_t m_offset;

        std::vector<std::shared_ptr<Buffer>>    m_buffers;
        std::vector<std::shared_ptr<ArrayData>> m_children;
        std::shared_ptr<ArrayData>              m_dictionary;
    };







}