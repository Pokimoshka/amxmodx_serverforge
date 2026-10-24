#!/usr/bin/env bash
set -euo pipefail

root=$(cd "$(dirname "$0")/.." && pwd)
compiler="$root/compiler/libpc300"
tmp_dir=$(mktemp -d)
trap 'rm -rf "$tmp_dir"' EXIT

cd "$compiler"
${CC:-gcc} -std=gnu99 -O0 -DHAVE_STDINT_H -DHAVE_UNISTD_H -DLINUX \
  -D_GNU_SOURCE -DENABLE_BINRELOC -DNO_MAIN -I. \
  sc1.c sc2.c sc3.c sc4.c sc5.c sc6.c sc7.c sci18n.c sclist.c \
  scmemfil.c scstate.c scvars.c libpawnc.c prefix.c memfile.c \
  sp_symhash.c pawncc.c -lm -ldl -lpthread -o "$tmp_dir/pawncc"

cat > "$tmp_dir/typed.sma" <<'EOF'
#include <amxmodx>

native bool:native_result();
native native_consume(bool:value);
native native_ref(bool:prefix, &bool:value);
native native_array(bool:values[]);

stock pawn_ref(&bool:value)
{
    value = true;
}

stock pawn_array(bool:values[])
{
    values[0] = true;
}

stock bool:returns_invalid()
{
    return 2;
}

stock set_invalid(&bool:value)
{
    value = 2;
}

public plugin_init()
{
    native_consume(bool:2);
    new bool:reference_value = bool:2;
    set_invalid(reference_value);
    native_ref(true, reference_value);
    new bool:values[2][3];
    values[0][0] = bool:2;
    native_array(values[0]);
    pawn_ref(reference_value);
    pawn_array(values[0]);
    if (values[0][0])
        return;
    if (native_result())
        return;
    if (returns_invalid())
        return;
}
EOF

"$tmp_dir/pawncc" "$tmp_dir/typed.sma" \
  "-i$root/plugins/include" -w213- -o"$tmp_dir/typed.amx"

python3 - "$tmp_dir/typed.amx" <<'PY'
import struct
import sys

with open(sys.argv[1], "rb") as amx_file:
    data = amx_file.read()

header = struct.unpack_from("<I H B B h h 11i", data)
code_start, data_start = header[6:8]
code = struct.unpack("<%di" % ((data_start - code_start) // 4),
                     data[code_start:data_start])
assert header[2] >= 11, "typed bytecode must use format version 11"
assert header[3] >= 11, "typed bytecode must require AMX version 11"
assert header[4] & 0x40, "bool bytecode must set AMX_FLAG_TYPED"
assert code.count(145) >= 8, "bool storage, reads, returns, and native boundaries need checks"
assert code.count(146) == 6, f"bool reference check count is {code.count(146)}, expected pre/post checks for Pawn and native calls"
assert code.count(147) == 4, f"bool array check count is {code.count(147)}, expected pre/post checks for Pawn and native calls"
assert any(code[index:index + 2] == (146, 1)
           for index in range(len(code) - 1)), "boolrefchk must target the correct native argument"
assert any(code[index:index + 3] == (147, 0, 3)
           for index in range(len(code) - 2)), "boolarrchk must target the correct native argument and length"
assert any(code[index:index + 4] == (147, 0, 3, 123)
           for index in range(len(code) - 3)), "bool arrays must be checked before native execution"
assert any(code[index] == 123 and code[index + 2] == 147
           for index in range(len(code) - 3)), "bool arrays must be checked again after native execution"
print("Typed bool compiler checks passed")
PY

cat > "$tmp_dir/legacy.sma" <<'EOF'
#include <amxmodx>

public plugin_init()
{
    new value = 1;
    value++;
}
EOF

"$tmp_dir/pawncc" "$tmp_dir/legacy.sma" \
  "-i$root/plugins/include" -o"$tmp_dir/legacy.amx"

python3 - "$tmp_dir/legacy.amx" <<'PY'
import struct
import sys

with open(sys.argv[1], "rb") as amx_file:
    data = amx_file.read()

header = struct.unpack_from("<I H B B h h 11i", data)
code_start, data_start = header[6:8]
code = struct.unpack("<%di" % ((data_start - code_start) // 4),
                     data[code_start:data_start])
assert not header[4] & 0x40, "untyped code must not set AMX_FLAG_TYPED"
assert header[2] == 10 and header[3] == 10, "untyped code must remain loadable by version-10 AMX runtimes"
assert 145 not in code and 146 not in code and 147 not in code
print("Untyped compiler compatibility check passed")
PY

${CXX:-g++} -std=c++11 -fpermissive -include wchar.h \
  -DLINUX -D_LINUX -DPOSIX -DPLATFORM_POSIX -DJIT -D__64BIT__ -DAMX_NODYNALOAD \
  -DPAWN_CELL_SIZE=32 -I"$root/compiler/amxxpc" \
  -I"$root/compiler/libpc300" \
  "$root/amxmodx/tests/TypedVmTests.cpp" \
  "$root/compiler/amxxpc/amx.cpp" -lm -ldl -o "$tmp_dir/typed_vm_tests"

"$tmp_dir/typed_vm_tests"
echo "Typed bool VM execution checks passed"

cat > "$tmp_dir/unknown_array_size.sma" <<'EOF'
#include <amxmodx>

native native_array(bool:values[]);

stock forward_array(bool:values[])
{
    native_array(values);
}

public plugin_init()
{
    new bool:values[2];
    forward_array(values);
}
EOF

if output=$("$tmp_dir/pawncc" "$tmp_dir/unknown_array_size.sma" \
  "-i$root/plugins/include" -w213- -o"$tmp_dir/unknown_array_size.amx" 2>&1); then
  echo "Typed native arrays with unknown runtime sizes must be rejected" >&2
  exit 1
fi
if [[ "$output" != *"error 048:"* ]]; then
  printf '%s\n' "$output" >&2
  echo "Unknown-size array test failed for an unexpected compiler error" >&2
  exit 1
fi

echo "Unknown-size typed native array rejection check passed"
