/*
    RawSpeed - RAW file decoder.

    Copyright (C) 2017 Roman Lebedev

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; withexpected even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
*/

#include "common/Common.h" // for uchar8, int64, int32, short16, uint32
#include "common/Memory.h" // for alignedMallocArray, alignedFree, alignedM...
#include <cstddef>         // for size_t
#include <cstdint>         // for SIZE_MAX, uintptr_t
#include <gtest/gtest.h>   // for Message, TestPartResult, TestPartResult::...
#include <memory>          // for unique_ptr
#include <string>          // for string

using namespace std;
using namespace RawSpeed;

static constexpr const size_t alloc_alignment = 16;

template <typename T> class AlignedMallocTest : public testing::Test {
public:
  static constexpr const size_t alloc_cnt = 8;
  static constexpr const size_t alloc_sizeof = sizeof(T);
  static constexpr const size_t alloc_size = alloc_cnt * alloc_sizeof;

  inline void TheTest(T* ptr) {
    ASSERT_TRUE(((uintptr_t)ptr % alloc_alignment) == 0);
    ptr[0] = 11;
    ptr[1] = 22;
    ptr[2] = 33;
    ptr[3] = 44;
    ptr[4] = 55;
    ptr[5] = 66;
    ptr[6] = 77;
    ptr[7] = 88;
    ASSERT_EQ((int64)ptr[0] + ptr[1] + ptr[2] + ptr[3] + ptr[4] + ptr[5] +
                  ptr[6] + ptr[7],
              396UL);
  }
};

template <typename T>
class AlignedMallocDeathTest : public AlignedMallocTest<T> {};

using Classes = testing::Types<int, unsigned int, short16, ushort16, int32,
                               uint32, int64, uint64, float, double>;

TYPED_TEST_CASE(AlignedMallocTest, Classes);

TYPED_TEST_CASE(AlignedMallocDeathTest, Classes);

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
TYPED_TEST(AlignedMallocTest, BasicTest) {
  ASSERT_NO_THROW({
    TypeParam* ptr =
        (TypeParam*)alignedMalloc(this->alloc_size, alloc_alignment);
    this->TheTest(ptr);
    alignedFree(ptr);
  });
}

TYPED_TEST(AlignedMallocTest, UniquePtrTest) {
  unique_ptr<TypeParam[], decltype(&alignedFree)> ptr(
      (TypeParam*)alignedMalloc(this->alloc_size, alloc_alignment),
      alignedFree);
  this->TheTest(&(ptr[0]));
}

TYPED_TEST(AlignedMallocDeathTest, AlignedMallocAssertions) {
#ifndef NDEBUG
  ASSERT_DEATH(
      {
        TypeParam* ptr = (TypeParam*)alignedMalloc(this->alloc_size, 3);
        this->TheTest(ptr);
        alignedFree(ptr);
      },
      "isPowerOfTwo");
  ASSERT_DEATH(
      {
        TypeParam* ptr =
            (TypeParam*)alignedMalloc(this->alloc_size, sizeof(void*) / 2);
        this->TheTest(ptr);
        alignedFree(ptr);
      },
      "alignment % sizeof");
  ASSERT_DEATH(
      {
        TypeParam* ptr =
            (TypeParam*)alignedMalloc(1 + alloc_alignment, alloc_alignment);
        this->TheTest(ptr);
        alignedFree(ptr);
      },
      "size % alignment");
#endif
}

#pragma GCC diagnostic pop

TYPED_TEST(AlignedMallocTest, TemplateTest) {
  ASSERT_NO_THROW({
    TypeParam* ptr =
        (TypeParam*)alignedMalloc<alloc_alignment>(this->alloc_size);
    this->TheTest(ptr);
    alignedFree(ptr);
  });
}

TYPED_TEST(AlignedMallocTest, TemplatUniquePtrTest) {
  unique_ptr<TypeParam[], decltype(&alignedFree)> ptr(
      (TypeParam*)alignedMalloc<alloc_alignment>(this->alloc_size),
      alignedFree);
  this->TheTest(&(ptr[0]));
}

TYPED_TEST(AlignedMallocTest, TemplateArrayTest) {
  ASSERT_NO_THROW({
    TypeParam* ptr = (TypeParam*)alignedMallocArray<alloc_alignment>(
        this->alloc_cnt, this->alloc_sizeof);
    this->TheTest(ptr);
    alignedFree(ptr);
  });
}

TYPED_TEST(AlignedMallocTest, TemplateArrayHandlesOverflowTest) {
  ASSERT_NO_THROW({
    static const size_t nmemb = 1 + (SIZE_MAX / this->alloc_sizeof);
    TypeParam* ptr = (TypeParam*)alignedMallocArray<alloc_alignment>(
        nmemb, this->alloc_sizeof);
    ASSERT_EQ(ptr, nullptr);
  });
}

TYPED_TEST(AlignedMallocTest, TemplateUniquePtrArrayTest) {
  unique_ptr<TypeParam[], decltype(&alignedFree)> ptr(
      (TypeParam*)alignedMallocArray<alloc_alignment>(this->alloc_cnt,
                                                      this->alloc_sizeof),
      alignedFree);
  this->TheTest(&(ptr[0]));
}

TYPED_TEST(AlignedMallocDeathTest, TemplateArrayAssertions) {
#ifndef NDEBUG
  // unlike TemplateArrayRoundUp, should fail
  ASSERT_DEATH(
      {
        TypeParam* ptr = (TypeParam*)alignedMallocArray<alloc_alignment>(
            1, 1 + sizeof(TypeParam));
        alignedFree(ptr);
      },
      "size % alignment");
#endif
}

TYPED_TEST(AlignedMallocTest, TemplateArrayRoundUp) {
  // unlike TemplateArrayAssertions, should NOT fail
  ASSERT_NO_THROW({
    TypeParam* ptr = (TypeParam*)(alignedMallocArray<alloc_alignment, true>(
        1, 1 + sizeof(TypeParam)));
    alignedFree(ptr);
  });
}

TYPED_TEST(AlignedMallocTest, TemplateArraySizeTest) {
  ASSERT_NO_THROW({
    TypeParam* ptr =
        (TypeParam*)(alignedMallocArray<alloc_alignment, TypeParam>(
            this->alloc_cnt));
    this->TheTest(ptr);
    alignedFree(ptr);
  });
}

TYPED_TEST(AlignedMallocTest, TemplateUniquePtrArraySizeTest) {
  unique_ptr<TypeParam[], decltype(&alignedFree)> ptr(
      (TypeParam*)alignedMallocArray<alloc_alignment, TypeParam>(
          this->alloc_cnt),
      alignedFree);
  this->TheTest(&(ptr[0]));
}

TEST(AlignedMallocDeathTest, TemplateArraySizeAssertions) {
#ifndef NDEBUG
  // unlike TemplateArraySizeRoundUp, should fail
  ASSERT_DEATH(
      {
        uchar8* ptr = (uchar8*)(alignedMallocArray<alloc_alignment, uchar8>(1));
        alignedFree(ptr);
      },
      "size % alignment");
#endif
}

TEST(AlignedMallocTest, TemplateArraySizeRoundUp) {
  // unlike TemplateArraySizeAssertions, should NOT fail
  ASSERT_NO_THROW({
    uchar8* ptr =
        (uchar8*)(alignedMallocArray<alloc_alignment, uchar8, true>(1));
    alignedFree(ptr);
  });
}

TYPED_TEST(AlignedMallocTest, TemplateArraySizeRoundUpTest) {
  // unlike TemplateArraySizeAssertions, should NOT fail
  ASSERT_NO_THROW({
    TypeParam* ptr =
        (TypeParam*)(alignedMallocArray<alloc_alignment, TypeParam, true>(1));
    alignedFree(ptr);
  });
}
