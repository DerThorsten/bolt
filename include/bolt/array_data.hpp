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