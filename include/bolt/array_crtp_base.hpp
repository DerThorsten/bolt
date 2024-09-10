#pragma once

#include <bolt/array_traits.hpp>
#include <bolt/iterator.hpp>

namespace bolt
{
    
    template<class CRTP_BASE>
    class ValueIteratorFunctor
    {
        public:
        using crtp_base_type = CRTP_BASE;
        using derived_type = typename crtp_base_type::derived_type;
        using value_type = typename ArrayTraits<derived_type>::value_type;
        
        ValueIteratorFunctor(const crtp_base_type * crtp_base) : m_crtp_base(crtp_base) {}

        value_type operator()(std::size_t index) 
        {
            return m_crtp_base->raw_value(index);
        }
        private:
        const crtp_base_type * m_crtp_base;
    };

    template<class CRTP_BASE>
    class OptionalValueIteratorFunctor
    {
        public:
        using crtp_base_type = CRTP_BASE;
        using derived_type = typename crtp_base_type::derived_type;
        using value_type = typename ArrayTraits<derived_type>::value_type;
        
        OptionalValueIteratorFunctor(const crtp_base_type * crtp_base) : m_crtp_base(crtp_base) {}

        std::optional<value_type> operator()(std::size_t index) 
        {
            return m_crtp_base->optional_value(index);
        }
        private:
        const crtp_base_type * m_crtp_base;
    };  


    template<class DERIVED_TYPE, class BASE_CLASS>
    class ArrayCrtpBase : public BASE_CLASS
    {
        private:
        using self_type = ArrayCrtpBase<DERIVED_TYPE, BASE_CLASS>;
        using value_iterator_functor = ValueIteratorFunctor<self_type>;
        using iterator_functor = OptionalValueIteratorFunctor<self_type>;
        friend class ValueIteratorFunctor<self_type>;

        using derived_type = DERIVED_TYPE;
        using derived_type_traits = ArrayTraits<derived_type>;


        // if the derived class does not provide a const_value_iterator, we will synthesize one
        constexpr static bool syntesize_value_iterator = std::is_same_v<typename derived_type_traits::const_value_iterator, null_type>;
        public:

        // select the correct value iterator (either the synthesized one or the one provided by the derived class)
        using const_value_iterator = typename std::conditional_t<
            syntesize_value_iterator, 
            functor_index_iterator<value_iterator_functor>,
            typename derived_type_traits::const_value_iterator
        >;

        using const_iterator = functor_index_iterator<iterator_functor>;
        
        using BASE_CLASS::BASE_CLASS;
        using value_type = typename derived_type_traits::value_type;

        

        const_iterator begin() const
        {
            return const_iterator(iterator_functor(this), 0);
        }
        const_iterator end() const
        {
            return const_iterator(iterator_functor(this), this->derived_cast().size());
        }

        const_value_iterator values_begin() const requires syntesize_value_iterator
        {
            return const_value_iterator(value_iterator_functor(this), 0);
        }
        
        const_value_iterator values_end() const requires syntesize_value_iterator
        {
            return const_value_iterator(value_iterator_functor(this), this->derived_cast().size());
        }


        
        auto value_range() const
        {
            return std::views::iota(std::size_t(0), this->derived_cast().size()) | std::views::transform([this](std::size_t i) 
            {
                return this->derived_cast().raw_value(i);
            });
        }
        auto optional_value_range() const
        {
            return std::views::iota(std::size_t(0), this->derived_cast().size()) | std::views::transform([this](std::size_t i) 
            {
                return this->derived_cast().optional_value(i);
            });
        }

        auto optional_value(std::size_t index) const
        {
            // type of derived_type::raw_value(index)
            using value_type = decltype(this->derived_cast().raw_value(index));
            using optional_value_type = std::optional<value_type>;

            // is the index valid
            if(this->derived_cast().is_valid(index))
            {
                return optional_value_type{this->derived_cast().raw_value(index)};
            }
            else
            {
                return optional_value_type{};
            }
        }
        private:

        derived_type & derived_cast()
        {
            return static_cast<derived_type &>(*this);
        }
        const derived_type & derived_cast() const
        {
            return static_cast<const derived_type &>(*this);
        }
        const derived_type & derived_const_cast() const
        {
            return static_cast<const derived_type &>(*this);
        }
        const derived_type & derived_const_cast()
        {
            return static_cast<const derived_type &>(*this);
        }

    };



} // namespace bolt
