// MIT License
//
// Copyright (c) 2018 Advanced Micro Devices, Inc. All rights reserved.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

// Google Test
#include <gtest/gtest.h>
#include "test_utils.hpp"

// Thrust
#include <thrust/device_vector.h>
#include <thrust/replace.h>
#include <thrust/iterator/discard_iterator.h>
#include <thrust/iterator/retag.h>

// HIP API
#if THRUST_DEVICE_COMPILER == THRUST_DEVICE_COMPILER_HCC
#include <hip/hip_runtime_api.h>
#include <hip/hip_runtime.h>

#define HIP_CHECK(condition) ASSERT_EQ(condition, hipSuccess)
#endif // THRUST_DEVICE_COMPILER == THRUST_DEVICE_COMPILER_HCC

template< class InputType >
struct Params
{
    using input_type = InputType;
};

template<class Params>
class ReplaceTests : public ::testing::Test
{
public:
    using input_type = typename Params::input_type;
};

template<class Params>
class PrimitiveReplaceTests : public ::testing::Test
{
public:
    using input_type = typename Params::input_type;
};

typedef ::testing::Types<
    Params<thrust::host_vector<short>>,
    Params<thrust::host_vector<int>>,
    Params<thrust::host_vector<long long>>,
    Params<thrust::host_vector<unsigned short>>,
    Params<thrust::host_vector<unsigned int>>,
    Params<thrust::host_vector<unsigned long long>>,
    Params<thrust::host_vector<float>>,
    Params<thrust::host_vector<double>>,
    Params<thrust::device_vector<short>>,
    Params<thrust::device_vector<int>>,
    Params<thrust::device_vector<long long>>,
    Params<thrust::device_vector<unsigned short>>,
    Params<thrust::device_vector<unsigned int>>,
    Params<thrust::device_vector<unsigned long long>>,
    Params<thrust::device_vector<float>>,
    Params<thrust::device_vector<double>>
> ReplaceTestsParams;

typedef ::testing::Types<
    Params<short>,
    Params<int>,
    Params<long long>,
    Params<unsigned short>,
    Params<unsigned int>,
    Params<unsigned long long>,
    Params<float>,
    Params<double>
> ReplaceTestsPrimitiveParams;

TYPED_TEST_CASE(ReplaceTests, ReplaceTestsParams);
TYPED_TEST_CASE(PrimitiveReplaceTests, ReplaceTestsPrimitiveParams);

#if THRUST_DEVICE_COMPILER == THRUST_DEVICE_COMPILER_HCC

TEST(ReplaceTests, UsingHip)
{
  ASSERT_EQ(THRUST_DEVICE_SYSTEM, THRUST_DEVICE_SYSTEM_HIP);
}

TYPED_TEST(ReplaceTests, SimpleReplace)
{
    using Vector = typename TestFixture::input_type;
    using T = typename Vector::value_type;
    const size_t size = 5;

    Vector data(5);
    data[0] =  1; 
    data[1] =  2; 
    data[2] =  1;
    data[3] =  3; 
    data[4] =  2;

    thrust::replace(data.begin(), data.end(), (T) 1, (T) 4);
    thrust::replace(data.begin(), data.end(), (T) 2, (T) 5);

    Vector result(5);
    result[0] =  4; 
    result[1] =  5; 
    result[2] =  4;
    result[3] =  3; 
    result[4] =  5; 
        
    for (size_t i = 0; i < size; i++)
        ASSERT_EQ(data[i], result[i]);
}

template<typename ForwardIterator, typename T>
void replace(my_system &system,
             ForwardIterator, ForwardIterator, const T &,
             const T &)
{
    system.validate_dispatch();
}

TEST(ReplaceTests, ValidateDispatch){
    thrust::device_vector<int> vec(1);

    my_system sys(0);
    thrust::replace(sys,
                    vec.begin(),
                    vec.begin(),
                    0,
                    0);

    ASSERT_EQ(sys.is_valid(), true);
}

template<typename ForwardIterator, typename T>
void replace(my_tag,
             ForwardIterator first, ForwardIterator, const T &,
             const T &)
{
    *first = 13;
}

TEST(ReplaceTests, ValidateDispatchImplicit)
{
    thrust::device_vector<int> vec(1);

    thrust::replace(thrust::retag<my_tag>(vec.begin()),
                    thrust::retag<my_tag>(vec.begin()),
                    0,
                    0);

    ASSERT_EQ(13, vec.front());
}

TYPED_TEST(PrimitiveReplaceTests, ReplaceWithRandomDataAndDifferentSizes)
{
    using T = typename TestFixture::input_type;

    const std::vector<size_t> sizes = get_sizes_smaller();
    for(auto size : sizes)
    {
        thrust::host_vector<T>   h_data = get_random_data<T>(size, 0, 10);
        thrust::device_vector<T> d_data = h_data;

        T new_value = (T) 0;
        T old_value = (T) 1;

        thrust::replace(h_data.begin(), h_data.end(), old_value, new_value);
        thrust::replace(d_data.begin(), d_data.end(), old_value, new_value);
        
        ASSERT_EQ(h_data.size(), size);
        ASSERT_EQ(d_data.size(), size);

        for (size_t i = 0; i < size; i++)
            ASSERT_NEAR(h_data[i], d_data[i], 0.1);
    }
       
}

TYPED_TEST(ReplaceTests, SimpleCopyReplace)
{
    using Vector = typename TestFixture::input_type;
    using T = typename Vector::value_type;

    Vector data(5);
    data[0] = 1; 
    data[1] = 2; 
    data[2] = 1;
    data[3] = 3; 
    data[4] = 2; 

    Vector dest(5);

    thrust::replace_copy(data.begin(), data.end(), dest.begin(), (T) 1, (T) 4);
    thrust::replace_copy(dest.begin(), dest.end(), dest.begin(), (T) 2, (T) 5);

    Vector result(5);
    result[0] = 4; 
    result[1] = 5; 
    result[2] = 4;
    result[3] = 3; 
    result[4] = 5; 

    for (size_t i = 0; i < 5; i++)
        ASSERT_NEAR(dest[i], result[i], 0.1);
}

template<typename InputIterator, typename OutputIterator, typename T>
OutputIterator replace_copy(my_system &system,
                            InputIterator, InputIterator,
                            OutputIterator result,
                            const T &,
                            const T &)
{
    system.validate_dispatch();
    return result;
}

TEST(ReplaceTests, ReplaceCopyValidateDispatch)
{
    thrust::device_vector<int> vec(1);

    my_system sys(0);
    thrust::replace_copy(sys,
                         vec.begin(),
                         vec.begin(),
                         vec.begin(),
                         0,
                         0);

    ASSERT_EQ(true, sys.is_valid());
}

template<typename InputIterator, typename OutputIterator, typename T>
OutputIterator replace_copy(my_tag,
                            InputIterator, InputIterator,
                            OutputIterator result,
                            const T &,
                            const T &)
{
    *result = 13;
    return result;
}

TEST(ReplaceTests, ReplaceCopyValidateDispatchImplicit)
{
    thrust::device_vector<int> vec(1);

    thrust::replace_copy(thrust::retag<my_tag>(vec.begin()),
                         thrust::retag<my_tag>(vec.begin()),
                         thrust::retag<my_tag>(vec.begin()),
                         0,
                         0);

    ASSERT_EQ(13, vec.front());
}

TYPED_TEST(PrimitiveReplaceTests, ReplaceCopyWithRandomData)
{
    using T = typename TestFixture::input_type;

    const std::vector<size_t> sizes = get_sizes_smaller();
    for(auto size : sizes)
    {

        thrust::host_vector<T>   h_data = get_random_data<T>(size, 0, 10);
        thrust::device_vector<T> d_data = h_data;
        
        T old_value = (T) 0;
        T new_value = (T) 1;
        
        thrust::host_vector<T>   h_dest(size);
        thrust::device_vector<T> d_dest(size);

        thrust::replace_copy(h_data.begin(), h_data.end(), h_dest.begin(), old_value, new_value);
        thrust::replace_copy(d_data.begin(), d_data.end(), d_dest.begin(), old_value, new_value);


        for (size_t i = 0; i < size; i++)
            ASSERT_NEAR(h_data[i], d_data[i], 0.1);

        for (size_t i = 0; i < size; i++)
            ASSERT_NEAR(h_dest[i], d_dest[i], 0.1);
    }
}

TYPED_TEST(PrimitiveReplaceTests, ReplaceCopyToDiscardIterator)
{
    using T = typename TestFixture::input_type;

    const std::vector<size_t> sizes = get_sizes_smaller();
    for(auto size : sizes)
    {
        thrust::host_vector<T>   h_data = get_random_data<T>(size, 0, 10);
        thrust::device_vector<T> d_data = h_data;
        
        T old_value = 0;
        T new_value = 1;

        thrust::discard_iterator<> h_result =
        thrust::replace_copy(h_data.begin(), h_data.end(), thrust::make_discard_iterator(), old_value, new_value);

        thrust::discard_iterator<> d_result =
        thrust::replace_copy(d_data.begin(), d_data.end(), thrust::make_discard_iterator(), old_value, new_value);

        thrust::discard_iterator<> reference(size);

        ASSERT_EQ(reference, d_result);
        ASSERT_EQ(reference, h_result);
    }
}


template <typename T>
struct less_than_five
{
  __host__ __device__ bool operator()(const T &val) const {return val < 5;}
};

TYPED_TEST(ReplaceTests, ReplaceIfSimple)
{
    using Vector = typename TestFixture::input_type;
    using T = typename Vector::value_type;
    size_t size = 5;

    Vector data(size);
    data[0] =  1; 
    data[1] =  3; 
    data[2] =  4;
    data[3] =  6; 
    data[4] =  5; 

    thrust::replace_if(data.begin(), data.end(), less_than_five<T>(), (T) 0);

    Vector result(size);
    result[0] =  0; 
    result[1] =  0; 
    result[2] =  0;
    result[3] =  6; 
    result[4] =  5; 

    for (size_t i = 0; i < size; i++)
        ASSERT_NEAR(data[i], result[i], 0.1);
}


template<typename ForwardIterator, typename Predicate, typename T>
void replace_if(my_system &system,
                ForwardIterator, ForwardIterator,
                Predicate,
                const T &)
{
    system.validate_dispatch();
}

TEST(ReplaceTests, ValidateDispatchReplaceIf)
{
    thrust::device_vector<int> vec(1);

    my_system sys(0);
    thrust::replace_if(sys,
                       vec.begin(),
                       vec.begin(),
                       less_than_five<int>(),
                       0);

    ASSERT_EQ(sys.is_valid(), true);
}

template<typename ForwardIterator, typename Predicate, typename T>
void replace_if(my_tag,
                ForwardIterator first, ForwardIterator,
                Predicate,
                const T &)
{
    *first = 13;
}

template<class T>
struct always_true 
{
  __host__ __device__ bool operator()(const T&) const {return true;}
};

TEST(ReplaceTests, ValidateDispatchImplicitReplaceIf)
{
    thrust::device_vector<int> vec(1);

    thrust::replace_if(thrust::retag<my_tag>(vec.begin()),
                       thrust::retag<my_tag>(vec.begin()),
                       always_true<int>(),
                       0);

    ASSERT_EQ(13, vec.front());
}

TYPED_TEST(ReplaceTests, ReplaceIfStencilSimple)
{
    using Vector = typename TestFixture::input_type;
    using T = typename Vector::value_type;
    size_t size = 5;

    Vector data(5);
    data[0] =  1; 
    data[1] =  3; 
    data[2] =  4;
    data[3] =  6; 
    data[4] =  5; 

    Vector stencil(5);
    stencil[0] = 5;
    stencil[1] = 4;
    stencil[2] = 6;
    stencil[3] = 3;
    stencil[4] = 7;

    thrust::replace_if(data.begin(), data.end(), stencil.begin(), less_than_five<T>(), (T) 0);

    Vector result(5);
    result[0] =  1; 
    result[1] =  0; 
    result[2] =  4;
    result[3] =  0; 
    result[4] =  5; 

    for (size_t i = 0; i < size; i++)
        ASSERT_NEAR(data[i], result[i], 0.1);
}


template<typename ForwardIterator, typename InputIterator, typename Predicate, typename T>
void replace_if(my_system &system,
                ForwardIterator, ForwardIterator,
                InputIterator,
                Predicate,
                const T &)
{
    system.validate_dispatch();
}

TEST(ReplaceTests, ReplaceIfStencilDispatchExplicit)
{
    thrust::device_vector<int> vec(1);

    my_system sys(0);
    thrust::replace_if(sys,
                       vec.begin(),
                       vec.begin(),
                       vec.begin(),
                       less_than_five<int>(),
                       0);

    ASSERT_EQ(true, sys.is_valid());
}

template<typename ForwardIterator, typename InputIterator, typename Predicate, typename T>
void replace_if(my_tag,
                ForwardIterator first, ForwardIterator,
                InputIterator,
                Predicate,
                const T &)
{
    *first = 13;
}

TEST(ReplaceTests, ReplaceIfStencilDispatchImplicit)
{
    thrust::device_vector<int> vec(1);

    thrust::replace_if(thrust::retag<my_tag>(vec.begin()),
                       thrust::retag<my_tag>(vec.begin()),
                       thrust::retag<my_tag>(vec.begin()),
                       always_true<int>(),
                       0);

    ASSERT_EQ(13, vec.front());
}


TYPED_TEST(PrimitiveReplaceTests, ReplaceIfWithRandomData)
{
    using T = typename TestFixture::input_type;

    const std::vector<size_t> sizes = get_sizes_smaller();
    for(auto size : sizes)
    {
        thrust::host_vector<T>   h_data = get_random_data<T>(size, 0, 10);
        thrust::device_vector<T> d_data = h_data;

        thrust::replace_if(h_data.begin(), h_data.end(), less_than_five<T>(), (T) 0);
        thrust::replace_if(d_data.begin(), d_data.end(), less_than_five<T>(), (T) 0);

        for (size_t i = 0; i < size; i++)
            ASSERT_NEAR(h_data[i], d_data[i], 0.1);
    }
}

TYPED_TEST(ReplaceTests, ReplaceCopyIfSimple)
{
    using Vector = typename TestFixture::input_type;
    using T = typename Vector::value_type;
    size_t size = 5;
    
    Vector data(5);
    data[0] =  1; 
    data[1] =  3; 
    data[2] =  4;
    data[3] =  6; 
    data[4] =  5; 

    Vector dest(5);
    thrust::replace_copy_if(data.begin(), data.end(), dest.begin(), less_than_five<T>(), (T) 0);

    Vector result(5);
    result[0] =  0; 
    result[1] =  0; 
    result[2] =  0;
    result[3] =  6; 
    result[4] =  5; 

    for (size_t i = 0; i < size; i++)
        ASSERT_NEAR(dest[i], result[i], 0.1);
}


template<typename InputIterator, typename OutputIterator, typename Predicate, typename T>
OutputIterator replace_copy_if(my_system &system,
                               InputIterator, InputIterator,
                               OutputIterator result,
                               Predicate,
                               const T &)
{
    system.validate_dispatch();
    return result;
}

TEST(ReplaceTests, ReplaceCopyIfDispatchExplicit)
{
    thrust::device_vector<int> vec(1);

    my_system sys(0);
    thrust::replace_copy_if(sys,
                            vec.begin(),
                            vec.begin(),
                            vec.begin(),
                            always_true<int>(),
                            0);

    ASSERT_EQ(true, sys.is_valid());
}


template<typename InputIterator, typename OutputIterator, typename Predicate, typename T>
OutputIterator replace_copy_if(my_tag,
                               InputIterator, InputIterator,
                               OutputIterator result,
                               Predicate,
                               const T &)
{
    *result = 13;
    return result;
}

TEST(ReplaceTests, ReplaceCopyIfDispatchImplicit)
{
    thrust::device_vector<int> vec(1);

    thrust::replace_copy_if(thrust::retag<my_tag>(vec.begin()),
                            thrust::retag<my_tag>(vec.begin()),
                            thrust::retag<my_tag>(vec.begin()),
                            always_true<int>(),
                            0);

    ASSERT_EQ(13, vec.front());
}


TYPED_TEST(ReplaceTests, ReplaceCopyIfStencilSimple)
{
    using Vector = typename TestFixture::input_type;
    using T = typename Vector::value_type;
    size_t size = 5;
    
    Vector data(5);
    data[0] =  1; 
    data[1] =  3; 
    data[2] =  4;
    data[3] =  6; 
    data[4] =  5; 

    Vector stencil(5);
    stencil[0] = 1;
    stencil[1] = 5;
    stencil[2] = 4;
    stencil[3] = 7;
    stencil[4] = 8;

    Vector dest(5);
    thrust::replace_copy_if(data.begin(), data.end(), stencil.begin(), dest.begin(), less_than_five<T>(), (T) 0);

    Vector result(5);
    result[0] =  0; 
    result[1] =  3; 
    result[2] =  0;
    result[3] =  6; 
    result[4] =  5; 

    for (size_t i = 0; i < size; i++)
        ASSERT_NEAR(dest[i], result[i], 0.1);
}


template<typename InputIterator1, typename InputIterator2, typename OutputIterator, typename Predicate, typename T>
OutputIterator replace_copy_if(my_system &system,
                               InputIterator1, InputIterator1,
                               InputIterator2,
                               OutputIterator result,
                               Predicate,
                               const T &)
{
    system.validate_dispatch();
    return result;
}


TEST(ReplaceTests, TestReplaceCopyIfStencilDispatchExplicit)
{
    thrust::device_vector<int> vec(1);

    my_system sys(0);
    thrust::replace_copy_if(sys,
                            vec.begin(),
                            vec.begin(),
                            vec.begin(),
                            vec.begin(),
                            always_true<int>(),
                            0);

    ASSERT_EQ(true, sys.is_valid());
}


template<typename InputIterator1, typename InputIterator2, typename OutputIterator, typename Predicate, typename T>
OutputIterator replace_copy_if(my_tag,
                               InputIterator1, InputIterator1,
                               InputIterator2,
                               OutputIterator result,
                               Predicate,
                               const T &)
{
    *result = 13;
    return result;
}


TEST(ReplaceTests, ReplaceCopyIfStencilDispatchImplicit)
{
    thrust::device_vector<int> vec(1);

    thrust::replace_copy_if(thrust::retag<my_tag>(vec.begin()),
                            thrust::retag<my_tag>(vec.begin()),
                            thrust::retag<my_tag>(vec.begin()),
                            thrust::retag<my_tag>(vec.begin()),
                            always_true<int>(),
                            0);

    ASSERT_EQ(13, vec.front());
}


TYPED_TEST(PrimitiveReplaceTests, ReplaceCopyIfWithRandomData)
{
    using T = typename TestFixture::input_type;
    
    const std::vector<size_t> sizes = get_sizes_smaller();
    for(auto size : sizes)
    {

        thrust::host_vector<T>   h_data = get_random_data<T>(size, 0, 10);
        thrust::device_vector<T> d_data = h_data;

        thrust::host_vector<T>   h_dest(size);
        thrust::device_vector<T> d_dest(size);

        thrust::replace_copy_if(h_data.begin(), h_data.end(), h_dest.begin(), less_than_five<T>(), 0);
        thrust::replace_copy_if(d_data.begin(), d_data.end(), d_dest.begin(), less_than_five<T>(), 0);

        for (size_t i = 0; i < size; i++)
            ASSERT_NEAR(h_data[i], d_data[i], 0.1);

        for (size_t i = 0; i < size; i++)
            ASSERT_NEAR(h_dest[i], d_dest[i], 0.1);
    }
}

TYPED_TEST(PrimitiveReplaceTests, ReplaceCopyIfToDiscardIteratorRandomData)
{
    using T = typename TestFixture::input_type;

    const std::vector<size_t> sizes = get_sizes_smaller();
    for(auto size : sizes)
    {
        thrust::host_vector<T>   h_data = get_random_data<T>(size, 0, 10);
        thrust::device_vector<T> d_data = h_data;

        thrust::discard_iterator<> h_result =
        thrust::replace_copy_if(h_data.begin(), h_data.end(), thrust::make_discard_iterator(), less_than_five<T>(), 0);

        thrust::discard_iterator<> d_result =
        thrust::replace_copy_if(d_data.begin(), d_data.end(), thrust::make_discard_iterator(), less_than_five<T>(), 0);

        thrust::discard_iterator<> reference(size);

        ASSERT_EQ(reference, h_result);
        ASSERT_EQ(reference, d_result);
    }
}


TYPED_TEST(PrimitiveReplaceTests, ReplaceCopyIfStencil)
{
    using T = typename TestFixture::input_type;

    const std::vector<size_t> sizes = get_sizes_smaller();
    for(auto size : sizes)
    {
        thrust::host_vector<T>   h_data = get_random_data<T>(size, 0, 10);
        thrust::device_vector<T> d_data = h_data;

        thrust::host_vector<T>   h_stencil = get_random_data<T>(size, 0, 10);
        thrust::device_vector<T> d_stencil = h_stencil;

        thrust::host_vector<T>   h_dest(size);
        thrust::device_vector<T> d_dest(size);

        thrust::replace_copy_if(h_data.begin(), h_data.end(), h_stencil.begin(), h_dest.begin(), less_than_five<T>(), 0);
        thrust::replace_copy_if(d_data.begin(), d_data.end(), d_stencil.begin(), d_dest.begin(), less_than_five<T>(), 0);

        for (size_t i = 0; i < size; i++)
            ASSERT_NEAR(h_data[i], d_data[i], 0.1);

        for (size_t i = 0; i < size; i++)
            ASSERT_NEAR(h_dest[i], d_dest[i], 0.1);
    }
}


TYPED_TEST(PrimitiveReplaceTests, ReplaceCopyIfStencilToDiscardIteratorRandomData)
{
    using T = typename TestFixture::input_type;

    const std::vector<size_t> sizes = get_sizes_smaller();
    for(auto size : sizes)
    {

        thrust::host_vector<T>   h_data = get_random_data<T>(size, 0, 10);
        thrust::device_vector<T> d_data = h_data;

        thrust::host_vector<T>   h_stencil = get_random_data<T>(size, 0, 10);
        thrust::device_vector<T> d_stencil = h_stencil;

        thrust::discard_iterator<> h_result =
        thrust::replace_copy_if(h_data.begin(), h_data.end(), h_stencil.begin(), thrust::make_discard_iterator(), less_than_five<T>(), 0);

        thrust::discard_iterator<> d_result =
        thrust::replace_copy_if(d_data.begin(), d_data.end(), d_stencil.begin(), thrust::make_discard_iterator(), less_than_five<T>(), 0);

        thrust::discard_iterator<> reference(size);

        ASSERT_EQ(reference, h_result);
        ASSERT_EQ(reference, d_result);
    }
}

#endif // THRUST_DEVICE_COMPILER == THRUST_DEVICE_COMPILER_HCC