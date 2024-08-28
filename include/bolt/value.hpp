#pragma once

#include <variant>
#include <vector>
#include <map>
#include <string>

namespace bolt
{   
    class ListOfOptionalValues;
    class MapOfOptionalValues;
    
    using ValueVariantType = std::variant<
        bool,
        uint8_t,
        uint16_t,
        uint32_t,
        uint64_t,   
        int8_t,
        int16_t,
        int32_t,
        int64_t,
        float,
        double,
        std::string, 
        std::string_view,
        ListOfOptionalValues, 
        MapOfOptionalValues, 
        std::monostate
    >;

    using Value = ValueVariantType;

    class ListOfOptionalValues : public std::vector<ValueVariantType>
    {
        public:
        using std::vector<ValueVariantType>::vector;

    };

    class MapOfOptionalValues : public std::map<std::string, ValueVariantType>
    {
        public:
        using std::map<std::string, ValueVariantType>::map;

    };

    inline bool has_value( Value & value)
    {
        return !std::holds_alternative<std::monostate>(value);
    }

    using missing_value_t = std::monostate;
    
};