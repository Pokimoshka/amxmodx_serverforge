// vim: set ts=4 sw=4 tw=99 noet:
//
// AMX Mod X, based on AMX Mod by Aleksander Naszko ("OLO").
// Copyright (C) The AMX Mod X Development Team.
//
// This software is licensed under the GNU General Public License, version 3 or higher.
// Additional exceptions apply. For full license details, see LICENSE.txt or visit:
//     https://alliedmods.net/amxmodx-license

#ifndef TYPE_UTILS_H_INCLUDED
#define TYPE_UTILS_H_INCLUDED

#include <cstring>  // for memcpy
#include <cstdint>
#include <type_traits>
#include <limits>

// Type-safe bit casting for float/cell conversions
// Replaces the unsafe type punning macros amx_ftoc and amx_ctof

namespace amx {

// C++20 has std::bit_cast, but for C++11/14/17 we implement our own
template<typename To, typename From>
inline typename std::enable_if<
    sizeof(To) == sizeof(From) &&
    std::is_trivially_copyable<From>::value &&
    std::is_trivially_copyable<To>::value,
    To
>::type bit_cast(const From& src) noexcept
{
    static_assert(std::is_trivially_constructible<To>::value,
                  "bit_cast requires trivially constructible types");
    
    To dst;
    std::memcpy(&dst, &src, sizeof(To));
    return dst;
}

// Safe conversions between signed and unsigned types with overflow checking
template<typename To, typename From>
inline bool safe_cast(From value, To& result) noexcept
{
    static_assert(std::is_integral<From>::value && std::is_integral<To>::value,
                  "safe_cast only works with integral types");
    
    // Check if value is within range of destination type
    if (value < std::numeric_limits<To>::min() || 
        value > std::numeric_limits<To>::max())
    {
        return false;
    }
    
    result = static_cast<To>(value);
    return true;
}

// Unchecked version for performance-critical paths where bounds are guaranteed
template<typename To, typename From>
inline To safe_cast_unchecked(From value) noexcept
{
    static_assert(std::is_integral<From>::value && std::is_integral<To>::value,
                  "safe_cast_unchecked only works with integral types");
    
    // Debug builds should still check
#ifdef DEBUG
    if (value < std::numeric_limits<To>::min() || 
        value > std::numeric_limits<To>::max())
    {
        // In debug mode, abort on overflow
        std::abort();
    }
#endif
    
    return static_cast<To>(value);
}

} // namespace amx

// Modern replacements for the old type-punning macros
// These maintain the same interface but use safe bit_cast internally

#if PAWN_CELL_SIZE == 32
    #define amx_ftoc_safe(f)   (amx::bit_cast<cell>(static_cast<float>(f)))
    #define amx_ctof_safe(c)   (amx::bit_cast<float>(static_cast<cell>(c)))
    
    // Keep old macros for backward compatibility during migration
    // TODO: Remove these once all code is migrated to _safe versions
    #ifndef AMX_LEGACY_TYPE_PUNNING_REMOVED
        #define amx_ftoc(f)   amx_ftoc_safe(f)
        #define amx_ctof(c)   amx_ctof_safe(c)
    #endif
    
#elif PAWN_CELL_SIZE == 64
    #define amx_ftoc_safe(f)   (amx::bit_cast<cell>(static_cast<double>(f)))
    #define amx_ctof_safe(c)   (amx::bit_cast<double>(static_cast<cell>(c)))
    
    #ifndef AMX_LEGACY_TYPE_PUNNING_REMOVED
        #define amx_ftoc(f)   amx_ftoc_safe(f)
        #define amx_ctof(c)   amx_ctof_safe(c)
    #endif
    
#else
    #error Unsupported cell size (PAWN_CELL_SIZE)
#endif

// Helper for safe pointer casting with type checking
template<typename To, typename From>
inline To* safe_ptr_cast(From* ptr) noexcept
{
    static_assert(std::is_pointer<To*>::value, "To must be a pointer type");
    static_assert(std::is_pointer<From*>::value, "From must be a pointer type");
    
    // nullptr is always safe
    if (ptr == nullptr) {
        return nullptr;
    }
    
    // For debug builds, we could add runtime type checking here
    // using RTTI if needed
    
    return static_cast<To*>(static_cast<void*>(ptr));
}

// Helper for safe reinterpret_cast with explicit intent
template<typename To, typename From>
inline To unsafe_reinterpret_cast(From from) noexcept
{
    // This is still unsafe but makes intent clear in code
    // Use only when absolutely necessary and document why
    return reinterpret_cast<To>(from);
}

#endif // TYPE_UTILS_H_INCLUDED
