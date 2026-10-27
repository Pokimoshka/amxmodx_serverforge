// vim: set ts=4 sw=4 tw=99 noet:
//
// AMX Mod X, based on AMX Mod by Aleksander Naszko ("OLO").
// Copyright (C) The AMX Mod X Development Team.
//
// This software is licensed under the GNU General Public License, version 3 or higher.
// Additional exceptions apply. For full license details, see LICENSE.txt or visit:
//     https://alliedmods.net/amxmodx-license

#ifndef RAII_WRAPPERS_H_INCLUDED
#define RAII_WRAPPERS_H_INCLUDED

#include <cstdio>
#include <cstdlib>
#include "amx.h"

namespace amx {

// RAII wrapper for malloc/free memory
// Ensures automatic deallocation when going out of scope
template<typename T>
class ScopedMalloc
{
public:
    explicit ScopedMalloc(size_t count = 0) noexcept
        : ptr_(nullptr)
        , size_(0)
    {
        if (count > 0) {
            allocate(count);
        }
    }
    
    ~ScopedMalloc() noexcept
    {
        free();
    }
    
    // No copying
    ScopedMalloc(const ScopedMalloc&) = delete;
    ScopedMalloc& operator=(const ScopedMalloc&) = delete;
    
    // Move support
    ScopedMalloc(ScopedMalloc&& other) noexcept
        : ptr_(other.ptr_)
        , size_(other.size_)
    {
        other.ptr_ = nullptr;
        other.size_ = 0;
    }
    
    ScopedMalloc& operator=(ScopedMalloc&& other) noexcept
    {
        if (this != &other) {
            free();
            ptr_ = other.ptr_;
            size_ = other.size_;
            other.ptr_ = nullptr;
            other.size_ = 0;
        }
        return *this;
    }
    
    // Allocate memory
    bool allocate(size_t count) noexcept
    {
        free();
        if (count == 0) {
            return true;
        }
        
        ptr_ = static_cast<T*>(::malloc(count * sizeof(T)));
        if (ptr_ != nullptr) {
            size_ = count;
            return true;
        }
        return false;
    }
    
    // Reallocate memory
    bool reallocate(size_t new_count) noexcept
    {
        if (new_count == 0) {
            free();
            return true;
        }
        
        T* new_ptr = static_cast<T*>(::realloc(ptr_, new_count * sizeof(T)));
        if (new_ptr != nullptr) {
            ptr_ = new_ptr;
            size_ = new_count;
            return true;
        }
        // On failure, original pointer is still valid
        return false;
    }
    
    // Free memory
    void free() noexcept
    {
        if (ptr_ != nullptr) {
            ::free(ptr_);
            ptr_ = nullptr;
            size_ = 0;
        }
    }
    
    // Release ownership
    T* release() noexcept
    {
        T* tmp = ptr_;
        ptr_ = nullptr;
        size_ = 0;
        return tmp;
    }
    
    // Get pointer
    T* get() noexcept { return ptr_; }
    const T* get() const noexcept { return ptr_; }
    
    // Get size
    size_t size() const noexcept { return size_; }
    
    // Operators
    T& operator[](size_t index) noexcept { return ptr_[index]; }
    const T& operator[](size_t index) const noexcept { return ptr_[index]; }
    
    explicit operator bool() const noexcept { return ptr_ != nullptr; }
    
    T* operator->() noexcept { return ptr_; }
    const T* operator->() const noexcept { return ptr_; }
    
private:
    T* ptr_;
    size_t size_;
};

// RAII wrapper for FILE* from fopen
class ScopedFile
{
public:
    ScopedFile() noexcept : fp_(nullptr) {}
    
    explicit ScopedFile(const char* filename, const char* mode) noexcept
        : fp_(nullptr)
    {
        open(filename, mode);
    }
    
    ~ScopedFile() noexcept
    {
        close();
    }
    
    // No copying
    ScopedFile(const ScopedFile&) = delete;
    ScopedFile& operator=(const ScopedFile&) = delete;
    
    // Move support
    ScopedFile(ScopedFile&& other) noexcept
        : fp_(other.fp_)
    {
        other.fp_ = nullptr;
    }
    
    ScopedFile& operator=(ScopedFile&& other) noexcept
    {
        if (this != &other) {
            close();
            fp_ = other.fp_;
            other.fp_ = nullptr;
        }
        return *this;
    }
    
    bool open(const char* filename, const char* mode) noexcept
    {
        close();
        fp_ = fopen(filename, mode);
        return fp_ != nullptr;
    }
    
    void close() noexcept
    {
        if (fp_ != nullptr) {
            fclose(fp_);
            fp_ = nullptr;
        }
    }
    
    FILE* get() noexcept { return fp_; }
    const FILE* get() const noexcept { return fp_; }
    
    FILE* release() noexcept
    {
        FILE* tmp = fp_;
        fp_ = nullptr;
        return tmp;
    }
    
    explicit operator bool() const noexcept { return fp_ != nullptr; }
    
private:
    FILE* fp_;
};

// RAII wrapper for AMX allocated memory (via amx_Allot)
class ScopedAmxAllot
{
public:
    ScopedAmxAllot(AMX* amx, int cells) noexcept
        : amx_(amx)
        , phys_addr_(nullptr)
        , amx_addr_(0)
        , cells_(cells)
        , allocated_(false)
    {
        if (amx != nullptr && cells > 0) {
            cell* addr = nullptr;
            if (amx_Allot(amx, cells, &amx_addr_, &addr) == AMX_ERR_NONE) {
                phys_addr_ = addr;
                allocated_ = true;
            }
        }
    }
    
    ~ScopedAmxAllot() noexcept
    {
        // Note: AMX doesn't have explicit free for Allot
        // Memory is freed when the AMX heap is reset
        // This wrapper primarily serves as documentation
        // and to hold the addresses together
    }
    
    // No copying
    ScopedAmxAllot(const ScopedAmxAllot&) = delete;
    ScopedAmxAllot& operator=(const ScopedAmxAllot&) = delete;
    
    // Move support
    ScopedAmxAllot(ScopedAmxAllot&& other) noexcept
        : amx_(other.amx_)
        , phys_addr_(other.phys_addr_)
        , amx_addr_(other.amx_addr_)
        , cells_(other.cells_)
        , allocated_(other.allocated_)
    {
        other.allocated_ = false;
    }
    
    cell* get_physical() noexcept { return phys_addr_; }
    const cell* get_physical() const noexcept { return phys_addr_; }
    
    cell get_amx_address() const noexcept { return amx_addr_; }
    int get_cells() const noexcept { return cells_; }
    
    bool is_allocated() const noexcept { return allocated_; }
    explicit operator bool() const noexcept { return allocated_; }
    
    cell& operator[](int index) noexcept { return phys_addr_[index]; }
    const cell& operator[](int index) const noexcept { return phys_addr_[index]; }
    
private:
    AMX* amx_;
    cell* phys_addr_;
    cell amx_addr_;
    int cells_;
    bool allocated_;
};

// RAII wrapper for dynamic library handles
#if defined(_WIN32)
#include <windows.h>
class ScopedLibrary
{
public:
    ScopedLibrary() noexcept : handle_(nullptr) {}
    
    explicit ScopedLibrary(const char* filename) noexcept
        : handle_(nullptr)
    {
        load(filename);
    }
    
    ~ScopedLibrary() noexcept
    {
        unload();
    }
    
    // No copying
    ScopedLibrary(const ScopedLibrary&) = delete;
    ScopedLibrary& operator=(const ScopedLibrary&) = delete;
    
    // Move support
    ScopedLibrary(ScopedLibrary&& other) noexcept
        : handle_(other.handle_)
    {
        other.handle_ = nullptr;
    }
    
    ScopedLibrary& operator=(ScopedLibrary&& other) noexcept
    {
        if (this != &other) {
            unload();
            handle_ = other.handle_;
            other.handle_ = nullptr;
        }
        return *this;
    }
    
    bool load(const char* filename) noexcept
    {
        unload();
        handle_ = LoadLibraryA(filename);
        return handle_ != nullptr;
    }
    
    void unload() noexcept
    {
        if (handle_ != nullptr) {
            FreeLibrary(handle_);
            handle_ = nullptr;
        }
    }
    
    template<typename FuncType>
    FuncType get_symbol(const char* name) noexcept
    {
        if (handle_ == nullptr) return nullptr;
        return reinterpret_cast<FuncType>(GetProcAddress(handle_, name));
    }
    
    HMODULE get() noexcept { return handle_; }
    explicit operator bool() const noexcept { return handle_ != nullptr; }
    
private:
    HMODULE handle_;
};
#else
#include <dlfcn.h>
class ScopedLibrary
{
public:
    ScopedLibrary() noexcept : handle_(nullptr) {}
    
    explicit ScopedLibrary(const char* filename) noexcept
        : handle_(nullptr)
    {
        load(filename);
    }
    
    ~ScopedLibrary() noexcept
    {
        unload();
    }
    
    // No copying
    ScopedLibrary(const ScopedLibrary&) = delete;
    ScopedLibrary& operator=(const ScopedLibrary&) = delete;
    
    // Move support
    ScopedLibrary(ScopedLibrary&& other) noexcept
        : handle_(other.handle_)
    {
        other.handle_ = nullptr;
    }
    
    ScopedLibrary& operator=(ScopedLibrary&& other) noexcept
    {
        if (this != &other) {
            unload();
            handle_ = other.handle_;
            other.handle_ = nullptr;
        }
        return *this;
    }
    
    bool load(const char* filename) noexcept
    {
        unload();
        handle_ = dlopen(filename, RTLD_NOW);
        return handle_ != nullptr;
    }
    
    void unload() noexcept
    {
        if (handle_ != nullptr) {
            dlclose(handle_);
            handle_ = nullptr;
        }
    }
    
    template<typename FuncType>
    FuncType get_symbol(const char* name) noexcept
    {
        if (handle_ == nullptr) return nullptr;
        return reinterpret_cast<FuncType>(dlsym(handle_, name));
    }
    
    void* get() noexcept { return handle_; }
    explicit operator bool() const noexcept { return handle_ != nullptr; }
    
private:
    void* handle_;
};
#endif

} // namespace amx

#endif // RAII_WRAPPERS_H_INCLUDED
