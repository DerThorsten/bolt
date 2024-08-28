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
        virtual ~Array() = default;

        // delete copy and move semantics
        Array(const Array &) = delete;
        Array & operator=(const Array &) = delete;
        Array(Array &&) = delete;
        
        bool is_valid(std::size_t index) const;

        std::size_t size() const;
        std::shared_ptr<ArrayData> array_data() const;

        template<class VISITOR>
        void visit(VISITOR && visitor) const;

        protected:

        std::shared_ptr<ArrayData> m_data;
        uint8_t * p_validity_bitmap = nullptr;
    };
    
    template< bool BIG> // either int32_t or int64_t offsets
    class ListArrayImpl : public  Array
    {   
        private:
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
            auto validity_buffer = std::make_shared<Buffer>(validity_bitmap, compact_bool_flag{});
            list_array_data->add_buffer(validity_buffer);
            

            auto offset_buffer = offset_buffer_from_sizes<BIG>(std::forward<T>(sizes), validity_bitmap);
            list_array_data->add_buffer(offset_buffer);
            

            return list_array_data;
        }
        public:
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
        private:

        template<std::ranges::range U,  std::ranges::range V>
        static std::shared_ptr<ArrayData> make_owned_fixed_size_primite(
            U && values,
            V  && validity_bitmap
        )
        {
            constexpr std::size_t dsize = sizeof(T);
            std::shared_ptr<ArrayData> data = std::make_shared<ArrayData>();
            data->set_format(primitive_to_format<T>());
            data->set_length(std::ranges::size(values));
            data->m_offset = 0; 

            auto validity_buffer = std::make_shared<Buffer>(validity_bitmap, compact_bool_flag{});
            data->add_buffer(validity_buffer);
            data->m_null_count = 0; // TODO

            auto buffer = std::make_shared<Buffer>(dsize * data->m_length);
            data->add_buffer(std::move(buffer));
            auto ptr = static_cast<T *>(data->m_buffers[1]->data());
            for(std::size_t i = 0; i < data->m_length; i++)
            {   
                ptr[i] = static_cast<T>(values[i]);
            }
            return data;
        }

        public:

        template<std::ranges::range U, std::ranges::range V>
        NumericArray(U && values, V && validity_bitmap)
            : Array(make_owned_fixed_size_primite(std::forward<U>(values), std::forward<V>(validity_bitmap))),
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
        private:
        template<class OFFSET_TYPE, std::ranges::range T,  std::ranges::range U>
        static std::shared_ptr<ArrayData> make_owned_string_array(
            T && values,
            U && validity_bitmap
        )
        {
            std::shared_ptr<ArrayData> data = std::make_shared<ArrayData>();

            data->set_format(BIG ? std::string("U") : std::string("u"));
            data->set_length(std::ranges::size(values));

            // validity buffer
            auto validity_buffer = std::make_shared<Buffer>(validity_bitmap, compact_bool_flag{});
            data->add_buffer(validity_buffer);
            auto validity_range = packed_bit_range(validity_buffer->template data_as<uint8_t>(), data->length());

            // offset buffer
            data->add_buffer(offset_buffer_from_sizes<BIG>(
                values | std::views::transform([](auto && s) { return s.size(); }),
                validity_range
            ));

            // value buffer
            auto begin_values = std::ranges::begin(values);
            auto begin_validity = std::ranges::begin(validity_range);
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
            data->add_buffer(value_buffer);
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


        public:
        
        template<std::ranges::range U, std::ranges::range V>
        StringArrayImpl(U && values, V && validity_bitmap)
            : Array(make_owned_string_array<offset_type>(std::forward<U>(values), std::forward<V>(validity_bitmap))),
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



    template<class VISITOR>
    void Array::visit(VISITOR && visitor) const
    {
        const auto & format = m_data->format();
        const auto format_size = format.size();
        if(format_size == 1)
        {
            const auto format_char = format[0];
            switch(format_char)
            {   

                #define VISIT_NUMERIC(CHAR, TYPE) \
                case(CHAR): \
                { \
                    const auto & casted = static_cast<const NumericArray<TYPE> * >(this); \
                    visitor(*casted); \
                    break; \
                }
                VISIT_NUMERIC('b', bool)
                VISIT_NUMERIC('c', char)
                VISIT_NUMERIC('C', unsigned char)
                VISIT_NUMERIC('s', std::int16_t)
                VISIT_NUMERIC('S', std::uint16_t)
                VISIT_NUMERIC('i', std::int32_t)
                VISIT_NUMERIC('I', std::uint32_t)
                VISIT_NUMERIC('l', std::int64_t)
                VISIT_NUMERIC('L', std::uint64_t)
                VISIT_NUMERIC('f', float)
                VISIT_NUMERIC('d', double)
                #undef VISIT_NUMERIC
            }
        }
        else if( format == "+l"){
            const auto & casted = static_cast<const ListArray * >(this);
            visitor(*casted);
        }
        else if( format == "+L"){
            const auto & casted = static_cast<const BigListArray * >(this);
            visitor(*casted);
        }
        else if( format == "u"){
            const auto & casted = static_cast<const StringArray * >(this);
            visitor(*casted);
        }
        else if( format == "U"){
            const auto & casted = static_cast<const BigStringArray * >(this);
            visitor(*casted);
        }
        else
        {
            throw std::runtime_error("Unknown format");
        }
    }

}