# Migration Guide - Replacing Unsafe Code Patterns

## Overview
This guide shows how to replace unsafe C-style code with modern type-safe C++ alternatives using the new utilities in `type_utils.h` and `raii_wrappers.h`.

---

## 1. Float/Cell Conversions

### ❌ Old (Undefined Behavior):
```cpp
float fval = 3.14f;
cell cval = amx_ftoc(fval);  // Type punning through pointer cast
float fval2 = amx_ctof(cval);
```

### ✅ New (Type-Safe):
```cpp
float fval = 3.14f;
cell cval = amx_ftoc(fval);  // Now uses safe bit_cast internally
float fval2 = amx_ctof(cval);

// Or explicitly use the safe version:
cell cval = amx_ftoc_safe(fval);
```

**Migration**: No code changes needed! The macros now use safe implementation internally.

---

## 2. NULL → nullptr

### ❌ Old:
```cpp
void* ptr = NULL;
if (ptr == NULL) { ... }

char* str = NULL;
MyClass* obj = NULL;
```

### ✅ New:
```cpp
void* ptr = nullptr;
if (ptr == nullptr) { ... }

char* str = nullptr;
MyClass* obj = nullptr;
```

**Migration**: Simple find-and-replace of `NULL` with `nullptr` in C++ files.

**Benefits**:
- Type-safe (won't accidentally match with integer 0)
- Better overload resolution
- More explicit intent

---

## 3. C-Style Casts

### Pointer Casts

#### ❌ Old:
```cpp
AMX_HEADER* hdr = (AMX_HEADER*)amx->base;
void* data = (void*)some_ptr;
```

#### ✅ New:
```cpp
// For pointer type conversions where you know the types are related:
AMX_HEADER* hdr = reinterpret_cast<AMX_HEADER*>(amx->base);

// For casting away const (avoid if possible):
void* data = const_cast<void*>(some_ptr);

// For downcasting in inheritance hierarchies:
DerivedClass* derived = static_cast<DerivedClass*>(base_ptr);
```

### Numeric Casts

#### ❌ Old:
```cpp
unsigned int uval = (unsigned int)params[2];  // Can overflow!
int ival = (int)some_unsigned;
```

#### ✅ New:
```cpp
// With overflow checking:
unsigned int uval;
if (amx::safe_cast(params[2], uval)) {
    // Use uval safely
} else {
    // Handle overflow error
}

// Without checking (when you're certain it's safe):
unsigned int uval = amx::safe_cast_unchecked<unsigned int>(params[2]);
// Note: Debug builds still check and abort on overflow
```

### Size/Offset Casts

#### ❌ Old:
```cpp
size_t size = (size_t)params[1];
cell value = (cell)some_size;
```

#### ✅ New:
```cpp
// For safe conversions:
size_t size;
if (amx::safe_cast(params[1], size)) {
    // Use size
}

// For explicit conversions where you know the range:
size_t size = static_cast<size_t>(params[1]);
```

---

## 4. Memory Management

### Manual malloc/free

#### ❌ Old:
```cpp
void process_data() {
    char* buffer = (char*)malloc(1024);
    if (buffer == NULL) return;
    
    // Use buffer...
    
    if (some_error) {
        return;  // LEAK! Forgot to free()
    }
    
    free(buffer);
}
```

#### ✅ New:
```cpp
void process_data() {
    amx::ScopedMalloc<char> buffer(1024);
    if (!buffer) return;
    
    // Use buffer.get()...
    
    if (some_error) {
        return;  // No leak! Automatically freed
    }
    
    // Automatically freed here too
}
```

### Unsafe realloc

#### ❌ Old:
```cpp
ptr = (char*)realloc(ptr, new_size);  // If this fails, ptr is lost!
```

#### ✅ New:
```cpp
// Method 1: Using ScopedMalloc
amx::ScopedMalloc<char> buffer;
buffer.allocate(initial_size);
// ...
if (!buffer.reallocate(new_size)) {
    // Handle error, original data still valid
}

// Method 2: Manual with temporary
char* new_ptr = static_cast<char*>(realloc(ptr, new_size));
if (new_ptr == nullptr) {
    // ptr is still valid, handle error
} else {
    ptr = new_ptr;
}
```

---

## 5. File Operations

### Manual fopen/fclose

#### ❌ Old:
```cpp
FILE* fp = fopen("file.txt", "r");
if (fp == NULL) return;

// Process file...

if (error) {
    return;  // LEAK! File not closed
}

fclose(fp);
```

#### ✅ New:
```cpp
amx::ScopedFile file("file.txt", "r");
if (!file) return;

// Process file using file.get()...

if (error) {
    return;  // No leak! File automatically closed
}

// File automatically closed here
```

---

## 6. AMX Memory Allocation

### amx_Allot with manual cleanup

#### ❌ Old:
```cpp
cell* phys_addr = new cell[size];  // Memory allocation
cell amx_addr;
amx_Allot(amx, size, &amx_addr, &phys_addr);

// Use phys_addr...

if (error) {
    return;  // LEAK if phys_addr was heap-allocated
}

delete[] phys_addr;
```

#### ✅ New:
```cpp
amx::ScopedAmxAllot allot(amx, size);
if (!allot) return;  // Allocation failed

// Use allot.get_physical() or allot[index]
cell value = allot[0];

// Use allot.get_amx_address() for the AMX address

// Automatically cleaned up
```

---

## 7. Dynamic Libraries

### Manual LoadLibrary/dlopen

#### ❌ Old (Windows):
```cpp
HMODULE lib = LoadLibraryA("module.dll");
if (lib == NULL) return;

FuncType func = (FuncType)GetProcAddress(lib, "FunctionName");

if (error) {
    return;  // LEAK! Library not freed
}

FreeLibrary(lib);
```

#### ❌ Old (Linux):
```cpp
void* lib = dlopen("module.so", RTLD_NOW);
if (lib == NULL) return;

FuncType func = (FuncType)dlsym(lib, "FunctionName");

if (error) {
    return;  // LEAK! Library not freed
}

dlclose(lib);
```

#### ✅ New (Cross-platform):
```cpp
amx::ScopedLibrary lib("module");  // Extension added automatically
if (!lib) return;

FuncType func = lib.get_symbol<FuncType>("FunctionName");
if (!func) return;

if (error) {
    return;  // No leak! Library automatically unloaded
}

// Library automatically unloaded here
```

---

## 8. Pointer Casting in Structures

### Casting structure pointers

#### ❌ Old:
```cpp
AMX_FUNCSTUB* func = (AMX_FUNCSTUB*)((unsigned char*)base + offset);
```

#### ✅ New:
```cpp
// Method 1: Explicit steps
unsigned char* base_ptr = reinterpret_cast<unsigned char*>(base);
unsigned char* func_ptr = base_ptr + offset;
AMX_FUNCSTUB* func = reinterpret_cast<AMX_FUNCSTUB*>(func_ptr);

// Method 2: Combined (still better than C-style)
AMX_FUNCSTUB* func = reinterpret_cast<AMX_FUNCSTUB*>(
    reinterpret_cast<unsigned char*>(base) + offset
);
```

---

## 9. Void Pointer Storage

### Storing non-pointer values in void*

#### ❌ Old:
```cpp
void* tag = (void*)index;  // Storing integer in pointer
int index2 = (int)tag;      // Getting it back
```

#### ✅ New:
```cpp
// Store:
void* tag = reinterpret_cast<void*>(static_cast<uintptr_t>(index));

// Retrieve:
int index2 = static_cast<int>(reinterpret_cast<uintptr_t>(tag));
```

**Why**: `uintptr_t` is guaranteed to safely hold pointer values, making the intent clear.

---

## 10. Macro Definitions

### Unsafe casting macros

#### ❌ Old:
```cpp
#define TO_PTR(x) ((void*)(x))
#define FROM_PTR(x) ((int)(x))
```

#### ✅ New:
```cpp
// Use inline functions for type safety:
template<typename T>
inline void* to_ptr(T* ptr) {
    return static_cast<void*>(ptr);
}

template<typename T>
inline T* from_ptr(void* ptr) {
    return static_cast<T*>(ptr);
}

// Or for non-pointer values:
template<typename T>
inline void* int_to_ptr(T value) {
    return reinterpret_cast<void*>(static_cast<uintptr_t>(value));
}

template<typename T>
inline T ptr_to_int(void* ptr) {
    return static_cast<T>(reinterpret_cast<uintptr_t>(ptr));
}
```

---

## Migration Checklist

When migrating code, follow this order:

1. ✅ Replace `NULL` with `nullptr`
2. ✅ Replace manual `malloc/free` with `ScopedMalloc`
3. ✅ Replace manual `fopen/fclose` with `ScopedFile`
4. ✅ Replace C-style pointer casts with `reinterpret_cast` or `static_cast`
5. ✅ Replace C-style numeric casts with `static_cast` or `safe_cast`
6. ✅ Add `const` qualifiers where appropriate
7. ✅ Review and document any remaining `reinterpret_cast` usage
8. ✅ Test thoroughly after each change

---

## Static Analysis

After migration, run static analysis tools:

```bash
# clang-tidy
clang-tidy amxmodx/*.cpp -checks=modernize-*,cppcoreguidelines-*

# cppcheck
cppcheck --enable=all --std=c++11 amxmodx/
```

---

## Performance Notes

- **bit_cast**: Compiles to identical assembly as type punning with optimization
- **ScopedMalloc**: Zero overhead - just adds destructor call
- **static_cast**: Zero runtime cost - purely compile-time
- **safe_cast with checking**: Small overhead for bounds check, use unchecked version in hot paths
- **Debug assertions**: Only active in debug builds

---

## Common Pitfalls

### ⚠️ Don't mix memory management styles:
```cpp
// BAD:
char* ptr = (char*)malloc(100);
delete[] ptr;  // WRONG! Use free()

// BAD:
char* ptr = new char[100];
free(ptr);  // WRONG! Use delete[]
```

### ⚠️ Don't cast away const without good reason:
```cpp
// BAD:
void modify(const char* str) {
    char* mutable_str = const_cast<char*>(str);
    mutable_str[0] = 'X';  // Undefined behavior!
}

// GOOD:
void modify(char* str) {  // Take non-const if you need to modify
    str[0] = 'X';
}
```

### ⚠️ Be careful with reference lifetime:
```cpp
// BAD:
const char* get_string() {
    std::string str = "hello";
    return str.c_str();  // Dangling pointer!
}

// GOOD:
std::string get_string() {
    return std::string("hello");  // Return by value
}
```

---

## Questions?

See `type_utils.h` and `raii_wrappers.h` for full API documentation and implementation details.
