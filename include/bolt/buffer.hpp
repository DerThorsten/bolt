#pragma once

#include <cstddef>
#include <memory>

namespace bolt
{ 

    // this may or may not own the memory
    class Buffer
    {
        public:

        Buffer(std::size_t size);
        Buffer(void * data, std::size_t size);
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