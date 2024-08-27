#pragma once

#include <cstddef>
#include <memory>
#include <ranges>
#include <algorithm>
#include <type_traits>

namespace bolt
{ 
    struct compact_bool_flag{};






    // this may or may not own the memory
    class Buffer
    {
        public:

        template<class T> // require pod
        requires (std::is_pod_v<T> && !std::is_same_v<T, bool>)
        static std::shared_ptr<Buffer> make_shared(std::size_t size)
        {
            const auto needed_size = size * sizeof(T);
            return std::make_shared<Buffer>(needed_size);
        }

        template<class T>
        auto data_as() const
        {
            return static_cast<T *>(m_data);
        }



        Buffer(std::size_t size);
        Buffer(void * data, std::size_t size);


        template<std::ranges::range T>
        requires (std::is_pod_v<std::ranges::range_value_t<T>> &&  !std::is_same_v<std::ranges::range_value_t<T>, bool>)
        Buffer(T && data)
        :   m_data(std::malloc(std::ranges::size(data) * sizeof(std::ranges::range_value_t<T>))), 
            m_size(std::ranges::size(data) * sizeof(std::ranges::range_value_t<T>)), 
            m_owned(true)
        {
            std::ranges::copy(data, static_cast<std::ranges::range_value_t<T> *>(m_data));
        }

        template<std::ranges::range T>
        // convertible to bool
        requires (std::is_convertible_v<std::ranges::range_value_t<T>, bool>)
        Buffer(T && data , compact_bool_flag)
        {
            const auto size = std::ranges::size(data);

            const auto compact_size = (size + 7) / 8;
            m_data = std::malloc(compact_size);
            std::fill_n(static_cast<uint8_t *>(m_data), compact_size, 0);
            m_size = compact_size;
            m_owned = true;
            for(std::size_t i = 0; i < size; i++)
            {
                if(data[i])
                {
                    static_cast<uint8_t *>(m_data)[i / 8] |= 1 << (i % 8);
                }
            }
        }

        ~Buffer();
        // move constructor
        Buffer(Buffer && other);

        // delete copy constructor
        Buffer(const Buffer & other) = delete;
        // delete copy assignment
        Buffer & operator=(const Buffer & other) = delete;
        // delete move assignment
        Buffer & operator=(Buffer && other) = delete;

        void * data() const;
        std::size_t size() const;


        private:

        void * m_data;
        std::size_t m_size;
        bool m_owned;

        std::shared_ptr<Buffer> m_parent = nullptr;
    };

}