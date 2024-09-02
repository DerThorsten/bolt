#pragma once

#include <bolt/meta.hpp>

namespace bolt
{   

    template<class ARRAY>
    struct ArrayTraitsBase
    {
       using const_value_iterator = null_type;
    };

    template<class ARRAY>
    struct ArrayTraits;
}//;