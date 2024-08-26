//doctest
#include <doctest/doctest.h>
//bolt
#include <bolt/bolt.hpp>

namespace bolt
{
    TEST_CASE("Buffer")
    {
        SUBCASE("Buffer(std::size_t)")
        {
            Buffer buffer(10);
            CHECK(buffer.size() == 10);
            CHECK(buffer.data() != nullptr);
        }

        SUBCASE("Buffer(void *, std::size_t)")
        {
            void * data = std::malloc(10);
            Buffer buffer(data, 10);
            CHECK(buffer.size() == 10);
            CHECK(buffer.data() == data);
        }

        // move constructor
        SUBCASE("Buffer(Buffer &&)")
        {
            Buffer buffer(10);
            void * data = buffer.data();
            Buffer buffer2(std::move(buffer));
            CHECK(buffer.data() == nullptr);
            CHECK(buffer.size() == 0);
            CHECK(buffer2.data() == data);
            CHECK(buffer2.size() == 10);
        }

        SUBCASE("~Buffer()")
        {
            void * data = std::malloc(10); 
            {
                Buffer buffer(data, 10);
                CHECK(buffer.data() == data);
            }
            std::free(data);
        }
    }
    TEST_CASE("NumericArray")
    {
        std::vector<int> vec = {1, 2, 3, 4, 5};
        std::vector<uint8_t> validity = {1,1,1,0,1};
        NumericArray<int> array(vec, validity);
        CHECK(array.size() == 5);
        CHECK(array[0] == 1);
        CHECK(array[1] == 2);
        CHECK(array[2] == 3);
        CHECK(array[3] == 4);
        CHECK(array[4] == 5);
        
        CHECK(array.is_valid(0));
        CHECK(array.is_valid(1));
        CHECK(array.is_valid(2));
        CHECK(!array.is_valid(3));
        CHECK(array.is_valid(4));
    }

    TEST_CASE("StringArray")
    {
        std::vector<std::string> vec = {"hello" , "world", "bolt", "is","", "awesome"};
        std::vector<uint8_t> validity = {1,1,1,1,0,1};
        BigStringArray array(vec, validity);
        CHECK(array.size() == 6);
        CHECK(array[0] == "hello");
        CHECK(array[1] == "world");
        CHECK(array[2] == "bolt");
        CHECK(array[3] == "is");
        CHECK(array[4] == "");
        CHECK(array[5] == "awesome");

        // check the validity
        CHECK(array.is_valid(0));
        CHECK(array.is_valid(1));
        CHECK(array.is_valid(2));
        CHECK(array.is_valid(3));
        CHECK(!array.is_valid(4));
        CHECK(array.is_valid(5));

        
    }
}  // namespace bolt
