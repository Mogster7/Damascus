
#pragma once
#include "Sorting/RenderSortKey.h"

namespace dm
{
    template<class T>
    using MeshSortKey = RenderSortKeyElement<T, 0, 8>;

    template<class T>
    using DepthSortKey = RenderSortKeyElement<T, 8, 8 + 10>;

    // Adapted from: https://aras-p.info/blog/2014/01/16/rough-sorting-by-depth/

    /// \brief Flips the bits of unsigned int (float val) and makes them sortable
    /// \param f Float interpreted as uint32_t
    /// \return Sortable uint32_t representation of interpreted float
    inline std::uint32_t FloatFlip(std::uint32_t f)
    {
        std::uint32_t mask = -int(f >> 31) | 0x80000000;
        return f ^ mask;
    }

    /// \brief Takes the highest 10 bits for rough sort of floats. (Smallest size is 16 bits for the key)
    /// \param depth Depth value
    /// \return Sortable key for depth value
    inline std::uint16_t DepthToBits(float depth)
    {
        union {float f; std::uint32_t i;} f2i {};
        f2i.f = depth;
        f2i.i = FloatFlip(f2i.i);       // Flips bits to be sortable
        std::uint16_t b = f2i.i >> 22;  // Takes the highest 10 bits
        return b;
    }
}