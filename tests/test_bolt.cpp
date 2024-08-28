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


    TEST_CASE("BigListArray[int]")
    {
        // the inner array
        std::vector<int> flat_data = {1, 2, 3, 4, 5};
        std::vector<uint8_t> flat_validity = {1,1,1,0,1};
        auto flat_values = std::make_shared<NumericArray<int>>(flat_data, flat_validity);

        // the list array
        std::vector<int> sizes = {2, 1, 2};
        std::vector<uint8_t> validity = {1,1,1};
        BigListArray list_array(flat_values, sizes, validity);


        // get child as std::shared_ptr<Array>
        auto child_array = list_array.values();
        CHECK(child_array->size() == 5);

        // use visitor when child type is unknown
        bool visited = false;
        child_array->visit([&visited](auto & array)
        {
            CHECK(array.size() == 5);
            CHECK(array.is_valid(0));
            CHECK(array.is_valid(1));
            CHECK(array.is_valid(2));
            CHECK(!array.is_valid(3));   
            CHECK(array.is_valid(4));
            visited = true;
        });
        CHECK(visited);


        // cast into known type
        auto child_typed = std::static_pointer_cast<NumericArray<int>>(child_array);
        CHECK(child_typed->size() == 5);
        CHECK(child_typed->is_valid(0));
        CHECK(child_typed->is_valid(1));
        CHECK(child_typed->is_valid(2));
        CHECK(!child_typed->is_valid(3));   
        CHECK(child_typed->is_valid(4));

        for(std::size_t i = 0; i < list_array.size(); i++)
        {
            CHECK(list_array.list_size(i) == sizes[i]);
        }
    }


    class TestVisitor{
        public:
        void operator()(const NumericArray<int> & array)
        {
            CHECK(array.size() == 5);
            CHECK(array.is_valid(0));
            CHECK(array.is_valid(1));
            CHECK(array.is_valid(2));
            CHECK(!array.is_valid(3));   
            CHECK(array.is_valid(4));
            visited = true;
        }

        void operator()(const Array & array)
        {
            CHECK(false);
        }
        bool visited = false;
    };

    TEST_CASE("Visitor")
    {
        std::shared_ptr<Array> detyped_array = std::make_shared<NumericArray<int>>(std::vector<int>{1, 2, 3, 4, 5}, std::vector<uint8_t>{1,1,1,0,1});
        bool visited = false;
        TestVisitor visitor;
        detyped_array->visit(visitor);
        CHECK(visitor.visited);
    }
    TEST_CASE("inline-labmda-visitor")
    {
        std::shared_ptr<Array> detyped_array = std::make_shared<NumericArray<uint8_t>>(std::vector<uint8_t>{1, 2, 3, 4, 5}, std::vector<uint8_t>{1,1,1,0,1});
        
        detyped_array->visit([](const auto & array)
        {
            using T = std::decay_t<decltype(array)>;
            if constexpr (std::is_same_v<T, NumericArray<uint8_t>>)
            {
                CHECK(array.size() == 5);
                CHECK(array.is_valid(0));
                CHECK(array.is_valid(1));
                CHECK(array.is_valid(2));
                CHECK(!array.is_valid(3));   
                CHECK(array.is_valid(4));

                CHECK(array[0] == 1);
                CHECK(array[1] == 2);
                //CHECK(array[2] == 3);
                CHECK(array[3] == 4);
                CHECK(array[4] == 5);
            }
            else
            {
                CHECK(false);
            }
        });

    }


    TEST_CASE("DetypedVapueTypeBigListArray[int]")
    {
        // the inner array
        std::vector<int> flat_data = {1, 2, 3, 4, 5};
        std::vector<uint8_t> flat_validity = {1,1,1,0,1};
        auto flat_values = std::make_shared<NumericArray<int>>(flat_data, flat_validity);

        // the list array
        std::vector<int> sizes = {2, 1, 2};
        std::vector<uint8_t> validity = {1,1,1};
        std::shared_ptr<Array> list_array = std::make_shared<BigListArray>(flat_values, sizes, validity);

        auto value = list_array->operator[](0);

        bool visited = false;
        int flat_index = 0;


        // lets visit it!
        std::visit([&](auto && actual_typed_value)
        {
            using T = std::decay_t<decltype(actual_typed_value)>;
            if constexpr (std::is_same_v<T, ListOfOptionalValues>)
            {
                visited = true;
                CHECK(actual_typed_value.size() == 2);
                for(std::size_t i = 0; i < actual_typed_value.size(); i++)
                {
                    // direct access
                    CHECK(std::get<int>(actual_typed_value[i]) == flat_data[flat_index]);

                    // inner visitor in case we do not know inner type
                    bool visited_inner = false;
                    std::visit([&](auto && inner_typed_value){
                        using InnerT = std::decay_t<decltype(inner_typed_value)>;
                        if constexpr (std::is_same_v<InnerT, int>)
                        {
                            CHECK(inner_typed_value == flat_data[flat_index]);
                            visited_inner = true;
                        }
                        else
                        {
                            CHECK(false);
                        }
                    }, actual_typed_value[i]);
                    CHECK(visited_inner);
                    ++flat_index;
                }
            }
            else{
                CHECK(false);
            }
        }, value);
        CHECK(visited);
     
    }

    TEST_CASE("TestRange")
    {   
        std::vector<int> flat_data = {1, 2, 3, 4, 5};
        std::vector<uint8_t> flat_validity = {1,1,1,0,1};
        auto flat_values = std::make_shared<NumericArray<int>>(flat_data, flat_validity);

        // the list array
        std::vector<int> sizes = {2, 1, 2};
        std::vector<uint8_t> validity = {1,1,1};
        std::shared_ptr<Array> list_array = std::make_shared<BigListArray>(flat_values, sizes, validity);

        int i =0;
        int flat_i = 0;
        for(auto value : list_array->value_range())
        {
            // here we assue me know that the value is ListOfOptionalValues
            auto & actual_typed_value = std::get<ListOfOptionalValues>(value);
            CHECK(actual_typed_value.size() == sizes[i]);
            for(std::size_t j = 0; j < actual_typed_value.size(); j++)
            {
                // direct access
                CHECK(std::get<int>(actual_typed_value[j]) == flat_data[flat_i]);
                ++flat_i;
            }
            
            ++i;

        }   

    }

}  // namespace bolt
