#pragma once

#include <cstddef>
#include <vector>
#include <memory>
#include <string>
#include <ranges>
#include <iostream>



namespace bolt
{ 

    template <class... T>
    struct dependent_false : std::false_type
    {
    };


    template<class T>
    constexpr char primitive_to_format()
    {
        if constexpr (std::same_as<T, bool>)
        {
            return 'b';
        }
        else if constexpr (std::same_as<T, std::int8_t>)
        {
            return 'c';
        }
        else if constexpr (std::same_as<T, std::uint8_t>)
        {
            return 'C';
        }
        else if constexpr (std::same_as<T, std::int16_t>)
        {
            return 's';
        }
        else if constexpr (std::same_as<T, std::uint16_t>)
        {
            return 'S';
        }
        else if constexpr (std::same_as<T, std::int32_t>)
        {
            return 'i';
        }
        else if constexpr (std::same_as<T, std::uint32_t>)
        {
            return 'I';
        }
        else if constexpr (std::same_as<T, std::int64_t>)
        {
            return 'l';
        }
        else if constexpr (std::same_as<T, std::uint64_t>)
        {
            return 'L';
        }
        else if constexpr (std::same_as<T, float>)
        {
            return 'f';
        }
        else if constexpr (std::same_as<T, double>)
        {
            return 'g';
        }
        else 
        {
            static_assert(dependent_false<T>::value, "unsupported type");
        }
    }

} // namespace bolt