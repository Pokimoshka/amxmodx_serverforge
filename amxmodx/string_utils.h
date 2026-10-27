// vim: set ts=4 sw=4 tw=99 noet:
//
// AMX Mod X, based on AMX Mod by Aleksander Naszko ("OLO").
// Copyright (C) The AMX Mod X Development Team.
//
// This software is licensed under the GNU General Public License, version 3 or higher.
// Additional exceptions apply. For full license details, see LICENSE.txt or visit:
//     https://alliedmods.net/amxmodx-license

#ifndef _INCLUDE_STRING_UTILS_H_
#define _INCLUDE_STRING_UTILS_H_

#include <cstring>
#include <cstdio>
#include <cstdarg>
#include <cassert>

namespace amx {

/**
 * Safe string copy that guarantees null-termination
 * 
 * @param dest      Destination buffer
 * @param destSize  Size of destination buffer in bytes
 * @param src       Source string (must be null-terminated)
 * @return          Number of characters copied (excluding null terminator)
 * 
 * Unlike strncpy, this function:
 * - Always null-terminates the destination
 * - Returns the number of characters actually copied
 * - Will not write past destSize
 */
inline size_t safe_strcopy(char* dest, size_t destSize, const char* src)
{
    assert(dest != nullptr && "Destination cannot be null");
    assert(src != nullptr && "Source cannot be null");
    assert(destSize > 0 && "Destination size must be > 0");
    
    if (destSize == 0) {
        return 0;
    }
    
    size_t i = 0;
    while (i < destSize - 1 && src[i] != '\0') {
        dest[i] = src[i];
        ++i;
    }
    
    dest[i] = '\0';
    return i;
}

/**
 * Safe string concatenation that guarantees null-termination
 * 
 * @param dest      Destination buffer
 * @param destSize  Size of destination buffer in bytes
 * @param src       Source string to append (must be null-terminated)
 * @return          Number of characters in the resulting string (excluding null terminator)
 * 
 * Unlike strncat, this function:
 * - Takes the total buffer size, not remaining size
 * - Always null-terminates the destination
 * - Will not write past destSize
 */
inline size_t safe_strcat(char* dest, size_t destSize, const char* src)
{
    assert(dest != nullptr && "Destination cannot be null");
    assert(src != nullptr && "Source cannot be null");
    assert(destSize > 0 && "Destination size must be > 0");
    
    if (destSize == 0) {
        return 0;
    }
    
    // Find current end of dest
    size_t dest_len = 0;
    while (dest_len < destSize && dest[dest_len] != '\0') {
        ++dest_len;
    }
    
    if (dest_len >= destSize) {
        // Already at capacity or not null-terminated
        if (destSize > 0) {
            dest[destSize - 1] = '\0';
        }
        return destSize - 1;
    }
    
    // Append from src
    size_t i = 0;
    while (dest_len + i < destSize - 1 && src[i] != '\0') {
        dest[dest_len + i] = src[i];
        ++i;
    }
    
    dest[dest_len + i] = '\0';
    return dest_len + i;
}

/**
 * Safe formatted string print that guarantees null-termination
 * 
 * @param dest      Destination buffer
 * @param destSize  Size of destination buffer in bytes
 * @param format    Printf-style format string
 * @param ...       Format arguments
 * @return          Number of characters written (excluding null terminator),
 *                  or -1 if there was an encoding error
 * 
 * Unlike snprintf, this function:
 * - Always null-terminates even if the output was truncated
 * - Returns -1 on encoding error instead of a large positive number
 */
inline int safe_sprintf(char* dest, size_t destSize, const char* format, ...)
{
    assert(dest != nullptr && "Destination cannot be null");
    assert(format != nullptr && "Format cannot be null");
    assert(destSize > 0 && "Destination size must be > 0");
    
    if (destSize == 0) {
        return 0;
    }
    
    va_list args;
    va_start(args, format);
    
    int result = vsnprintf(dest, destSize, format, args);
    
    va_end(args);
    
    // Ensure null-termination even if truncated
    dest[destSize - 1] = '\0';
    
    // Handle different vsnprintf return value behaviors
    if (result < 0) {
        // Encoding error
        dest[0] = '\0';
        return -1;
    }
    
    if (static_cast<size_t>(result) >= destSize) {
        // Output was truncated
        return static_cast<int>(destSize - 1);
    }
    
    return result;
}

/**
 * Safe formatted string print with va_list that guarantees null-termination
 * 
 * @param dest      Destination buffer
 * @param destSize  Size of destination buffer in bytes
 * @param format    Printf-style format string
 * @param args      va_list of format arguments
 * @return          Number of characters written (excluding null terminator),
 *                  or -1 if there was an encoding error
 */
inline int safe_vsprintf(char* dest, size_t destSize, const char* format, va_list args)
{
    assert(dest != nullptr && "Destination cannot be null");
    assert(format != nullptr && "Format cannot be null");
    assert(destSize > 0 && "Destination size must be > 0");
    
    if (destSize == 0) {
        return 0;
    }
    
    int result = vsnprintf(dest, destSize, format, args);
    
    // Ensure null-termination even if truncated
    dest[destSize - 1] = '\0';
    
    // Handle different vsnprintf return value behaviors
    if (result < 0) {
        // Encoding error
        dest[0] = '\0';
        return -1;
    }
    
    if (static_cast<size_t>(result) >= destSize) {
        // Output was truncated
        return static_cast<int>(destSize - 1);
    }
    
    return result;
}

/**
 * Calculate length of a string with a maximum limit
 * 
 * @param str       String to measure (must be null-terminated within maxlen)
 * @param maxlen    Maximum number of characters to examine
 * @return          Length of string, or maxlen if no null terminator found
 * 
 * This is similar to strnlen_s but works on all platforms
 */
inline size_t safe_strlen(const char* str, size_t maxlen)
{
    assert(str != nullptr && "String cannot be null");
    
    size_t len = 0;
    while (len < maxlen && str[len] != '\0') {
        ++len;
    }
    
    return len;
}

/**
 * Check if a string is null-terminated within a given buffer size
 * 
 * @param str       String to check
 * @param bufsize   Size of the buffer containing the string
 * @return          true if string is properly null-terminated, false otherwise
 */
inline bool is_null_terminated(const char* str, size_t bufsize)
{
    if (str == nullptr || bufsize == 0) {
        return false;
    }
    
    for (size_t i = 0; i < bufsize; ++i) {
        if (str[i] == '\0') {
            return true;
        }
    }
    
    return false;
}

/**
 * RAII wrapper for dynamically allocated strings
 * Automatically frees memory on destruction
 */
class String
{
public:
    String() : m_data(nullptr), m_length(0), m_capacity(0) {}
    
    explicit String(size_t capacity) : m_data(nullptr), m_length(0), m_capacity(0)
    {
        reserve(capacity);
    }
    
    explicit String(const char* str) : m_data(nullptr), m_length(0), m_capacity(0)
    {
        if (str != nullptr) {
            assign(str);
        }
    }
    
    ~String()
    {
        free(m_data);
    }
    
    // Move semantics
    String(String&& other) noexcept
        : m_data(other.m_data)
        , m_length(other.m_length)
        , m_capacity(other.m_capacity)
    {
        other.m_data = nullptr;
        other.m_length = 0;
        other.m_capacity = 0;
    }
    
    String& operator=(String&& other) noexcept
    {
        if (this != &other) {
            free(m_data);
            m_data = other.m_data;
            m_length = other.m_length;
            m_capacity = other.m_capacity;
            other.m_data = nullptr;
            other.m_length = 0;
            other.m_capacity = 0;
        }
        return *this;
    }
    
    // Delete copy operations (can be added later if needed)
    String(const String&) = delete;
    String& operator=(const String&) = delete;
    
    void assign(const char* str)
    {
        if (str == nullptr) {
            clear();
            return;
        }
        
        size_t len = strlen(str);
        reserve(len + 1);
        strcpy(m_data, str);
        m_length = len;
    }
    
    void append(const char* str)
    {
        if (str == nullptr) {
            return;
        }
        
        size_t str_len = strlen(str);
        size_t new_length = m_length + str_len;
        
        reserve(new_length + 1);
        strcpy(m_data + m_length, str);
        m_length = new_length;
    }
    
    void clear()
    {
        if (m_data != nullptr) {
            m_data[0] = '\0';
        }
        m_length = 0;
    }
    
    void reserve(size_t new_capacity)
    {
        if (new_capacity <= m_capacity) {
            return;
        }
        
        char* new_data = static_cast<char*>(realloc(m_data, new_capacity));
        if (new_data == nullptr) {
            // Allocation failed - keep old data
            return;
        }
        
        m_data = new_data;
        m_capacity = new_capacity;
        
        if (m_length == 0 && m_capacity > 0) {
            m_data[0] = '\0';
        }
    }
    
    const char* c_str() const { return m_data ? m_data : ""; }
    char* data() { return m_data; }
    size_t length() const { return m_length; }
    size_t capacity() const { return m_capacity; }
    bool empty() const { return m_length == 0; }
    
private:
    char* m_data;
    size_t m_length;
    size_t m_capacity;
};

} // namespace amx

// Convenience macros for common buffer sizes
#define SAFE_STRCOPY(dest, src) amx::safe_strcopy(dest, sizeof(dest), src)
#define SAFE_STRCAT(dest, src) amx::safe_strcat(dest, sizeof(dest), src)
#define SAFE_SPRINTF(dest, ...) amx::safe_sprintf(dest, sizeof(dest), __VA_ARGS__)

#endif // _INCLUDE_STRING_UTILS_H_
