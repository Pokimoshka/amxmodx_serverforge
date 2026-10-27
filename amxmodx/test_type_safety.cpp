// vim: set ts=4 sw=4 tw=99 noet:
//
// AMX Mod X, based on AMX Mod by Aleksander Naszko ("OLO").
// Copyright (C) The AMX Mod X Development Team.
//
// This software is licensed under the GNU General Public License, version 3 or higher.
// Additional exceptions apply. For full license details, see LICENSE.txt or visit:
//     https://alliedmods.net/amxmodx-license

/**
 * Test file to verify that new type safety utilities compile correctly
 * This is not part of the production build - it's for testing only
 */

#include "type_utils.h"
#include "raii_wrappers.h"
#include "amx.h"

#include <cassert>
#include <cstdio>

namespace amx {
namespace test {

// Test bit_cast functionality
void test_bit_cast()
{
    // Test float to cell conversion
    float f = 3.14159f;
    cell c = bit_cast<cell>(f);
    float f2 = bit_cast<float>(c);
    
    // Round-trip should preserve bit pattern
    assert(bit_cast<cell>(f2) == c);
    
    printf("✓ bit_cast test passed\n");
}

// Test safe_cast functionality
void test_safe_cast()
{
    // Test successful cast
    int x = 42;
    unsigned int y;
    assert(safe_cast(x, y));
    assert(y == 42);
    
    // Test overflow detection
    int negative = -1;
    unsigned int u;
    assert(!safe_cast(negative, u));  // Should fail
    
    // Test size_t to int
    size_t large = 1000;
    int i;
    assert(safe_cast(large, i));
    assert(i == 1000);
    
    printf("✓ safe_cast test passed\n");
}

// Test ScopedMalloc
void test_scoped_malloc()
{
    {
        ScopedMalloc<int> buffer(10);
        assert(buffer);
        assert(buffer.size() == 10);
        
        // Write some data
        for (size_t i = 0; i < 10; ++i) {
            buffer[i] = static_cast<int>(i * 2);
        }
        
        // Verify
        for (size_t i = 0; i < 10; ++i) {
            assert(buffer[i] == static_cast<int>(i * 2));
        }
        
        // Test reallocation
        assert(buffer.reallocate(20));
        assert(buffer.size() == 20);
        
        // Old data should still be valid
        for (size_t i = 0; i < 10; ++i) {
            assert(buffer[i] == static_cast<int>(i * 2));
        }
        
        // buffer automatically freed here
    }
    
    printf("✓ ScopedMalloc test passed\n");
}

// Test ScopedFile
void test_scoped_file()
{
    const char* test_filename = "test_raii_file.txt";
    
    // Write test
    {
        ScopedFile file(test_filename, "w");
        assert(file);
        
        fprintf(file.get(), "Hello, RAII!\n");
        // File automatically closed here
    }
    
    // Read test
    {
        ScopedFile file(test_filename, "r");
        assert(file);
        
        char buffer[100];
        if (fgets(buffer, sizeof(buffer), file.get())) {
            assert(strncmp(buffer, "Hello, RAII!", 12) == 0);
        }
        // File automatically closed here
    }
    
    // Cleanup
    remove(test_filename);
    
    printf("✓ ScopedFile test passed\n");
}

// Test move semantics
void test_move_semantics()
{
    ScopedMalloc<int> buffer1(10);
    assert(buffer1);
    
    // Move construction
    ScopedMalloc<int> buffer2(std::move(buffer1));
    assert(buffer2);
    assert(!buffer1);  // buffer1 should be empty after move
    
    // Move assignment
    ScopedMalloc<int> buffer3;
    buffer3 = std::move(buffer2);
    assert(buffer3);
    assert(!buffer2);  // buffer2 should be empty after move
    
    printf("✓ Move semantics test passed\n");
}

// Test safe_ptr_cast
void test_safe_ptr_cast()
{
    char buffer[100];
    void* void_ptr = buffer;
    
    // Document a safe pointer cast
    char* char_ptr = safe_ptr_cast<char*>(void_ptr, "Test buffer cast");
    assert(char_ptr == buffer);
    
    // Test with const
    const void* const_void_ptr = buffer;
    const char* const_char_ptr = safe_ptr_cast<const char*>(const_void_ptr, "Const test");
    assert(const_char_ptr == buffer);
    
    printf("✓ safe_ptr_cast test passed\n");
}

// Test macros still work
void test_legacy_macros()
{
    float f = 2.71828f;
    cell c = amx_ftoc(f);  // Should use safe version internally
    float f2 = amx_ctof(c);
    
    // Values should be very close (floating point comparison)
    assert(f == f2 || (f - f2) < 0.0001f);
    
    printf("✓ Legacy macro test passed\n");
}

} // namespace test
} // namespace amx

// Main test runner
int main(int argc, char** argv)
{
    printf("Running AMX Mod X type safety tests...\n\n");
    
    try {
        amx::test::test_bit_cast();
        amx::test::test_safe_cast();
        amx::test::test_scoped_malloc();
        amx::test::test_scoped_file();
        amx::test::test_move_semantics();
        amx::test::test_safe_ptr_cast();
        amx::test::test_legacy_macros();
        
        printf("\n✓✓✓ All tests passed! ✓✓✓\n");
        return 0;
    }
    catch (const std::exception& e) {
        printf("\n✗✗✗ Test failed: %s ✗✗✗\n", e.what());
        return 1;
    }
    catch (...) {
        printf("\n✗✗✗ Unknown test failure ✗✗✗\n");
        return 1;
    }
}
