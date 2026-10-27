#!/usr/bin/env python3
"""
Script to replace NULL with nullptr in C++ source files
Only replaces in .cpp and .h files, avoiding comments and strings
"""

import re
import os
import sys
from pathlib import Path

def should_process_file(filepath):
    """Check if file should be processed"""
    ext = filepath.suffix.lower()
    return ext in ['.cpp', '.h', '.hpp', '.cxx', '.cc']

def replace_null_with_nullptr(content):
    """
    Replace NULL with nullptr, avoiding:
    - String literals
    - Comments
    - Macro definitions that might need NULL for C compatibility
    """
    
    # Track if we're in a C-style comment
    in_comment = False
    lines = content.split('\n')
    result_lines = []
    
    for line in lines:
        # Skip lines that define NULL itself
        if re.match(r'^\s*#\s*define\s+NULL\s', line):
            result_lines.append(line)
            continue
            
        # Skip lines in extern "C" blocks that might need NULL
        if 'extern "C"' in line or 'EXTERN_C' in line:
            result_lines.append(line)
            continue
        
        # Check for C-style comment start/end
        if '/*' in line:
            in_comment = True
        if '*/' in line:
            in_comment = False
            result_lines.append(line)
            continue
            
        if in_comment:
            result_lines.append(line)
            continue
            
        # Skip C++ comments
        if line.strip().startswith('//'):
            result_lines.append(line)
            continue
        
        # Replace NULL with nullptr, but not in strings
        # This is a simplified approach - for more robust handling,
        # would need a proper C++ parser
        
        # Split by strings to avoid replacing inside them
        parts = re.split(r'("(?:[^"\\]|\\.)*")', line)
        
        for i in range(0, len(parts), 2):  # Process non-string parts
            # Replace NULL as a whole word
            parts[i] = re.sub(r'\bNULL\b', 'nullptr', parts[i])
        
        result_lines.append(''.join(parts))
    
    return '\n'.join(result_lines)

def process_file(filepath):
    """Process a single file"""
    try:
        with open(filepath, 'r', encoding='utf-8', errors='ignore') as f:
            content = f.read()
        
        # Check if file contains NULL
        if 'NULL' not in content:
            return False, "No NULL found"
        
        new_content = replace_null_with_nullptr(content)
        
        if new_content == content:
            return False, "No changes made"
        
        # Write back
        with open(filepath, 'w', encoding='utf-8', newline='') as f:
            f.write(new_content)
        
        # Count replacements
        old_count = content.count('NULL')
        new_count = new_content.count('NULL')
        replaced = old_count - new_count
        
        return True, f"Replaced {replaced} occurrences"
    
    except Exception as e:
        return False, f"Error: {e}"

def main():
    if len(sys.argv) < 2:
        print("Usage: python null_to_nullptr.py <directory>")
        sys.exit(1)
    
    root_dir = Path(sys.argv[1])
    
    if not root_dir.exists():
        print(f"Error: Directory {root_dir} does not exist")
        sys.exit(1)
    
    processed = 0
    modified = 0
    
    print(f"Processing files in {root_dir}...")
    
    for filepath in root_dir.rglob('*'):
        if not filepath.is_file():
            continue
            
        if not should_process_file(filepath):
            continue
        
        processed += 1
        success, message = process_file(filepath)
        
        if success:
            modified += 1
            print(f"OK {filepath.relative_to(root_dir)}: {message}")
        elif 'NULL' in str(message):
            pass  # Silent for files without NULL
        else:
            print(f"-- {filepath.relative_to(root_dir)}: {message}")

    print(f"\nSummary:")
    print(f"  Processed: {processed} files")
    print(f"  Modified:  {modified} files")

if __name__ == '__main__':
    main()
