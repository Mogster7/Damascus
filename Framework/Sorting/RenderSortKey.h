//------------------------------------------------------------------------------
//
// File Name:	RenderSortKey.h
// Author(s):	Roland Munguia (roland.m)
// Date:		09/29/2021
//
//------------------------------------------------------------------------------

#pragma once

#include <cstddef>

namespace dm
{

/// \brief Enable all bits in the given range of a 64-bit unsigned integer.
/// \param from Start of range
/// \param to End of range
/// \return Bitmask with bits turned off in the given range
constexpr std::uint64_t BitRange(std::uint64_t from, std::uint64_t to)
{
    return ((std::uint64_t)(-1) >> from << from << (sizeof(std::uint64_t) * 8 - to) >> (sizeof(std::uint64_t) * 8 - to));
}

// Adapted from https://peter.bloomfield.online/using-cpp-templates-for-size-based-type-selection/.
// Note: Can be removed an specify RenderSortKeyElement as a template parameter.
template<std::uint8_t NumBytes>
using UIntSelector = typename std::conditional<NumBytes == 1, std::uint8_t, typename std::conditional<NumBytes == 2, std::uint16_t, typename std::conditional<NumBytes == 3 || NumBytes == 4, std::uint32_t, std::uint64_t>::type>::type>::type;

/// \brief Specify a range
/// \tparam T Type that this render sort key range belongs to.
/// \tparam BitStart Start of the range in the render sort key
/// \tparam BitEnd End of the range in the render sort key
template<typename T, std::uint8_t BitStart, std::uint8_t BitEnd>
struct RenderSortKeyElement
{
    using keyType = UIntSelector<(BitEnd - BitStart) / 8>; //< Type of our key based on the num of bits required.

    static constexpr std::uint8_t offset = { BitStart };                  //< Offset in bits from the least significant bit in RenderSortKey.
    static constexpr std::uint64_t mask = { BitRange(BitStart, BitEnd) }; //< RenderSortKey Visibility bitmask for this RenderSortKeyElement.

    /// \brief Generate a unique key for this element.
    /// \return Unique key.
    static RenderSortKeyElement GetUnique()
    {
        static RenderSortKeyElement<T, BitStart, BitEnd> uniqueKey;

        // Copy unique and generate new one for next request.
        auto result = uniqueKey;
        ++uniqueKey.key;

        return result;
    }

    keyType key{ 0u }; //< Key for this element
};

// using MatRenderSortKey = RenderSortKeyElement<int, 0, 8>;

/* 2nd Option where the type is specified as template parameter and not use std::optional.
template<typename T, typename KeyType, std::uint8_t BitStart, std::uint8_t BitEnd>
struct RenderSortKeyElementV2
{
    static constexpr std::uint8_t offset = { BitStart };                  //< Offset in bits from the least significant bit in RenderSortKey.
    static constexpr std::uint64_t mask = { BitRange(BitStart, BitEnd) }; //< RenderSortKey Visibility bitmask for this RenderSortKeyElement.

    /// \brief Generate a unique key for this element.
    /// \return Unique key.
    static RenderSortKeyElementV2 GetUnique()
    {
        static RenderSortKeyElementV2<T, KeyType, BitStart, BitEnd> uniqueKey;

        // Copy unique and generate new one for next request.
        auto result = uniqueKey;
        ++uniqueKey.key;

        return result;
    }

    KeyType key{ 0u }; //< Key for this element
};

 using MatRenderSortKeyV2 = RenderSortKeyElementV2<int, std::uint8_t, 8, 16>;
*/

/// \brief Render sorting key for bucket sorting
struct RenderSortKey
{
    /// \brief Update a given element in the render sort key.
    /// \tparam T Render Sort Key Element Type
    /// \param element Element Key to update in the render sort key
    template<typename T>
    void Set(T element)
    {
        std::uint64_t elementKey = (std::uint64_t) element.key;
        key = (key & (~T::mask)) | (elementKey << T::offset);
    }

    operator std::uint64_t () const
    {
        return key;
    }

    std::uint64_t key = { 0 };  //< Sorting key for bucketing rendering
};


} // namespace dm