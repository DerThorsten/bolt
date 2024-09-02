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


    // packed bit range from ptr
    inline auto packed_bit_range(uint8_t * ptr, std::size_t length)
    {
        // iota iterator
        return std::views::iota(std::size_t(0), length) | std::views::transform([ptr](std::size_t i) -> bool
        {
            const auto byte = i / 8;
            const auto bit = i % 8;
            return ptr[byte] & (1 << bit);
        });
    }


    template<bool BIG, std::ranges::range SIZES, std::ranges::range VALIDITY>
    std::shared_ptr<Buffer> offset_buffer_from_sizes(SIZES && sizes, VALIDITY && validity)
    {
        using offset_type = std::conditional_t<BIG, std::int64_t, std::int32_t>;
        const auto length = std::ranges::size(sizes);
        auto offset_buffer = Buffer::make_shared<offset_type>(length + 1);
        auto p_offsets = offset_buffer-> template data_as<offset_type>();
        
        auto sizes_iter_begin = std::ranges::begin(sizes);
        auto validity_iter_begin = std::ranges::begin(validity);

        p_offsets[0] = 0;
        for(std::size_t i=0; i < length; ++i)
        {
            if(*validity_iter_begin)
            {
                p_offsets[i + 1] = p_offsets[i] + *sizes_iter_begin;
            }
            else
            {
                p_offsets[i + 1] = p_offsets[i];
            }
            sizes_iter_begin++;
            validity_iter_begin++;
        }
        return offset_buffer;    
    }




    class ArrayData
    {   
        public: 
     
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


        inline ArrayData(const char format, std::int64_t length, std::int64_t offset=0)
        : ArrayData(std::string(1, format), length, offset)
        {
        }
        inline ArrayData(const std::string & format, std::int64_t length, std::int64_t offset=0)
        : m_format(format), m_length(length), m_offset(offset)
        {
        }
        
            

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
        inline void set_format(char format)
        {
            m_format = std::string(1, format);
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