#pragma once

#include <cstddef>
#include <vector>
#include <memory>
#include <string>
#include <ranges>
#include <iostream>
#include <map>
#include <variant>
#include <string>
#include <string_view>
#include <ranges>
#include <algorithm>
#include <type_traits>
#include <iostream>
#include <stdexcept>
#include <optional>

#include <numeric>
#include <bolt/format.hpp>
#include <bolt/buffer.hpp>
#include <bolt/array_data.hpp>
#include <bolt/meta.hpp>
#include <bolt/array_crtp_base.hpp>
#include <bolt/array_traits.hpp>

namespace bolt
{ 

    // all sclalar types
    using all_scalar_type_list = type_list<
        bool,
        char,
        uint8_t,
        uint16_t,
        uint32_t,
        uint64_t,   
        int8_t,
        int16_t,
        int32_t,
        int64_t,
        float,
        double
    >;



    // forward declarations
    template<class ARRAY>
    struct ArrayTraits;
    class Array;
    class ArrayData;
    class Buffer;

    template<bool BIG> // either int32_t or int64_t offsets
    class ListArrayImpl;

    using ListArray = ListArrayImpl<false>;
    using BigListArray = ListArrayImpl<true>;


    class StructArray;

    template<class T>
    class NumericArray;

    template<bool BIG> // either int32_t or int64_t offsets
    class StringArrayImpl;

    using StringArray = StringArrayImpl<false>;
    using BigStringArray = StringArrayImpl<true>;



    class ListValue;
    class StructValue;


    template<class ... T>
    class ValueVariant : public std::variant<T...>
    { 
        public:
        using std::variant<T...>::variant;
        // = operator
        using std::variant<T...>::operator=;

        bool has_value() const
        {
            return !std::holds_alternative<std::monostate>(*this);
        }
    };
    
    template<class ... T>
    class OptionalValueVariant : public std::variant<T...>
    {
        public:
        using std::variant<T...>::variant;
        // = operator
        using std::variant<T...>::operator=;
        bool has_value() const
        {
            // visit the variant
            return std::visit([](auto && value) -> bool
            {
                return value.has_value();
            }, *this);
        }
    };


    using value_variant_type_list = merge_variadic_t<all_scalar_type_list, type_list<ListValue,StructValue, std::string, std::string_view, std::monostate>>;
    using ValueVariantType = use_variadic_args_from_t<ValueVariant, value_variant_type_list>;
    using Value = ValueVariantType;

    using optional_value_variant_type_list = apply_to_variadic_t<std::optional,
        merge_variadic_t<
            all_scalar_type_list, 
            type_list<std::string, ListValue, StructValue, std::string_view>
        >>;
    using OptionalValueVariantType = use_variadic_args_from_t<OptionalValueVariant, optional_value_variant_type_list>;
    using OptionalValue = OptionalValueVariantType;





    class ListValue
    {
        public:
        ListValue(
            std::shared_ptr<Array> flat_values,
            std::uint64_t flat_index_begin,
            std::uint64_t flat_index_end
        );

        OptionalValue operator[](const std::size_t index) const;
        uint64_t size() const;
        private:
        std::shared_ptr<Array> m_flat_values;
        std::uint64_t m_flat_index_begin;
        std::uint64_t m_flat_index_end;
    };

    class StructValue
    {
        public:
        StructValue(std::shared_ptr<const StructArray> array, std::size_t index);
        OptionalValue operator[](const std::size_t index) const;
        std::size_t size() const;
        const std::vector<std::string> & field_names() const;
        private:
        std::shared_ptr<const StructArray> m_stuct_array;
        std::size_t m_index;
    };



    // class ListOfOptionalValues;


     


    inline bool has_value(const Value & value)
    {
        return !std::holds_alternative<std::monostate>(value);
    }

    inline bool has_value(const OptionalValue & variant_value)
    {
       // visit the variant
        return std::visit([](auto && value) -> bool
        {
            return value.has_value();
        }, variant_value);
    }

    using missing_value_t = std::monostate;




    class Array
    {
        public:

        Array(ArrayData && data);
        Array(std::shared_ptr<ArrayData> data);
        
        protected:
        Array() = default;
        public:
        

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

        inline Value raw_value(std::size_t index) const{
            Value value;
            this->visit([index,&value](const auto & array)
            {
                if(array.is_valid(index))
                {
                    value = array.raw_value(index);
                }
                else{
                    value = missing_value_t{};
                }
            });
            return value;
        }
        inline OptionalValue optional_value(std::size_t index) const
        {
            OptionalValue value;
            this->visit([index,&value](const auto & array)
            {
                value = array.optional_value(index);
            });
            return value;
        }

        auto inline value_range() const
        {
            return std::views::iota(std::size_t(0), this->size()) | std::views::transform([this](std::size_t i) -> Value
            {
                return this->raw_value(i);
            });
        }
        protected:
        void assign_data(std::shared_ptr<ArrayData> data);

        std::shared_ptr<ArrayData> m_data;
        uint8_t * p_validity_bitmap = nullptr;
    };
    
    template<bool BIG>
    struct ArrayTraits<ListArrayImpl<BIG>> : public ArrayTraitsBase<ListArrayImpl<BIG>>
    {
        using value_type = ListValue;
    };

    template< bool BIG> // either int32_t or int64_t offsets
    class ListArrayImpl : public  ArrayCrtpBase<ListArrayImpl<BIG>, Array>
    {   
        using crtp_base = ArrayCrtpBase<ListArrayImpl<BIG>, Array>;
        using offset_type = std::conditional_t<BIG, std::int64_t, std::int32_t>;

        public:
        template<std::ranges::range T, std::ranges::range U>
        ListArrayImpl(std::shared_ptr<Array> flat_values, T && sizes, U && validity_bitmap)
            : crtp_base(),
              p_offsets(nullptr),
              m_flat_values(flat_values)
        {
            std::shared_ptr<ArrayData> list_array_data = std::make_shared<ArrayData>(
                BIG ? std::string("+L") : std::string("+l"),
                std::ranges::size(sizes)
            );
            list_array_data->add_child(flat_values->array_data());

            // buffer for validity
            auto validity_buffer = std::make_shared<Buffer>(validity_bitmap, compact_bool_flag{});
            list_array_data->add_buffer(validity_buffer);


            auto offset_buffer = offset_buffer_from_sizes<BIG>(std::forward<T>(sizes), validity_bitmap);
            list_array_data->add_buffer(offset_buffer);
            
            this->assign_data(list_array_data);

            p_offsets = static_cast<offset_type *>(this->m_data->buffers()[1]->data()) + this->m_data->offset();
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

        ListValue  raw_value(std::size_t index) const
        {   
            const auto flat_index_begin = p_offsets[index] + this->m_data->offset();
            const auto flat_index_end = p_offsets[index + 1] + this->m_data->offset();
            return ListValue(m_flat_values, flat_index_begin, flat_index_end);
        }

        private:
        
        std::shared_ptr<Array> m_flat_values;
        offset_type * p_offsets;
    };

    using ListArray = ListArrayImpl<false>;
    using BigListArray = ListArrayImpl<true>;
    




    template<>
    struct ArrayTraits<StructArray> : public ArrayTraitsBase<StructArray>
    {
        using value_type = StructValue;
    };


    class StructArray : public std::enable_shared_from_this<StructArray>, 
                        public ArrayCrtpBase<StructArray, Array>
    {   
        using crtp_base = ArrayCrtpBase<StructArray, Array>;

        public:
        template<std::ranges::range U>
        StructArray(
            const std::vector<std::shared_ptr<Array>> & field_values, 
            const std::vector<std::string> & field_names,
            U && validity_bitmap
        )
            : crtp_base(),
              m_field_values(field_values),
              m_field_names(field_names)
        {
            std::shared_ptr<ArrayData> array_data = std::make_shared<ArrayData>(std::string("+s"),field_values[0]->size());
            for(auto & field : field_values)
            {
                array_data->add_child(field->array_data());
            }   

            // buffer for validity
            auto validity_buffer = std::make_shared<Buffer>(validity_bitmap, compact_bool_flag{});
            array_data->add_buffer(validity_buffer);


            this->assign_data(array_data);

        }


        StructValue raw_value(std::size_t index) const
        {   
            const auto field_index = this->m_data->offset() + index;
            return StructValue(this->shared_from_this(), std::size_t(field_index));
        }
        
        const std::vector<std::shared_ptr<Array>> &  field_values() const
        {
            return m_field_values;
        }
        const std::vector<std::string> & field_names() const
        {
            return m_field_names;
        }

        private:

        std::vector<std::shared_ptr<Array>> m_field_values;
        std::vector<std::string> m_field_names;

    };







    

    template<class T>
    struct ArrayTraits<NumericArray<T>> : public ArrayTraitsBase<NumericArray<T>>
    {
        using value_type = T;
        using const_value_iterator = const T *;
    };
    
    template<class T>
    class NumericArray : public ArrayCrtpBase<NumericArray<T>, Array>
    {
        private:
        using crtp_base = ArrayCrtpBase<NumericArray<T>, Array>;
        using const_value_iterator = const T *;

        public:

        template<std::ranges::range U, std::ranges::range V>
        NumericArray(U && values, V && validity_bitmap)
        :   crtp_base(),
            p_values(nullptr)
        {
            constexpr std::size_t dsize = sizeof(T);
            std::shared_ptr<ArrayData> data = std::make_shared<ArrayData>(primitive_to_format<T>(),std::ranges::size(values));
       

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
            this->assign_data(data);
            p_values = static_cast<T *>(this->m_data->buffers()[1]->data()) + this->m_data->offset();
        }
        T raw_value(std::size_t index) const
        {
            return p_values[index];
        }
        const_value_iterator values_begin() const
        {
            return p_values;
        }
        const_value_iterator values_end() const
        {
            return p_values + this->m_data->size();
        }
        private:
        T * p_values;
    };


    template<bool BIG>
    struct ArrayTraits<StringArrayImpl<BIG>> : public ArrayTraitsBase<StringArrayImpl<BIG>>
    {
        using value_type = std::string;
    };

    // variable sized binary layout
    template< bool BIG> // either int32_t or int64_t offsets
    class StringArrayImpl : public ArrayCrtpBase<StringArrayImpl<BIG>, Array>
    {   
        public:
        using offset_type = std::conditional_t<BIG, std::int64_t, std::int32_t>;
        using crtp_base = ArrayCrtpBase<StringArrayImpl<BIG>, Array>;


        public:
        
        template<std::ranges::range U, std::ranges::range V>
        StringArrayImpl(U && values, V && validity_bitmap)
        :   crtp_base(),
            p_offsets(nullptr),
            p_values(nullptr)
        {
            std::shared_ptr<ArrayData> data = std::make_shared<ArrayData>(
                BIG ? std::string("U") : std::string("u"),
                std::ranges::size(values)
            );

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
            
            this->assign_data(data);
            p_offsets = static_cast<offset_type *>(this->m_data->buffers()[1]->data()) + this->m_data->offset();
            p_values = static_cast<char *>(this->m_data->buffers()[2]->data()) + this->m_data->offset();
        }

        inline std::string_view raw_value(std::size_t index) const
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
                case('u'):
                {
                    const auto & casted = static_cast<const StringArray * >(this);
                    visitor(*casted);
                    break;
                }
                case('U'):
                {
                    const auto & casted = static_cast<const BigStringArray * >(this);
                    visitor(*casted);
                    break;
                }
                default:
                {
                    throw std::runtime_error("Unknown format");
                }
            }
        }
        else if( format == "+s"){
            const auto & casted = static_cast<const StructArray * >(this);
            visitor(*casted);
        }
        else if( format == "+l"){
            const auto & casted = static_cast<const ListArray * >(this);
            visitor(*casted);
        }
        else if( format == "+L"){
            const auto & casted = static_cast<const BigListArray * >(this);
            visitor(*casted);
        } 
        else
        {
            throw std::runtime_error("Unknown format");
        }
    }

}