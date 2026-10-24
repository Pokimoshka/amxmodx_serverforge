#include <stdio.h>
#include <string.h>

#include <vector>

#include "amx.h"

int getMaxCodeSize(void)
{
	return 0;
}

int asm_runJIT(void *, void *, void *)
{
	return 0;
}

cell amx_exec_jit(cell *, cell *, cell, cell)
{
	return AMX_ERR_GENERAL;
}

#define CHECK(expr) \
	do { \
		if (!(expr)) { \
			fprintf(stderr, "Check failed at line %d: %s\n", __LINE__, #expr); \
			return 1; \
		} \
	} while (0)
#define CHECK_ERROR(expr, expected) \
	do { \
		int actual = (expr); \
		if (actual != (expected)) \
			fprintf(stderr, "Expected VM error %d, got %d\n", (expected), actual); \
		CHECK(actual == (expected)); \
	} while (0)

static int unused_callback(AMX *, cell, cell *, cell *)
{
	return AMX_ERR_CALLBACK;
}

static int run_program(const cell *instructions, size_t instruction_count, cell *result,
                       cell stack_offset = -1, const cell *initial_data = NULL,
                       cell frame_offset = 0)
{
	const size_t code_offset = sizeof(AMX_HEADER);
	const size_t code_size = instruction_count * sizeof(cell);
	std::vector<unsigned char> image(code_offset + code_size);
	std::vector<cell> data(64, 0);
	if (initial_data != NULL)
		memcpy(data.data(), initial_data, data.size() * sizeof(cell));
	AMX_HEADER *header = reinterpret_cast<AMX_HEADER *>(image.data());
	memset(header, 0, sizeof(*header));
	header->size = static_cast<int32_t>(image.size());
	header->magic = AMX_MAGIC;
	header->file_version = CUR_FILE_VERSION;
	header->amx_version = MIN_AMX_VERSION;
	header->flags = AMX_FLAG_TYPED;
	header->cod = static_cast<int32_t>(code_offset);
	header->dat = static_cast<int32_t>(code_offset + code_size);
	header->hea = header->dat + 16 * sizeof(cell);
	header->stp = header->dat + static_cast<int32_t>(data.size() * sizeof(cell));
	header->cip = 0;
	memcpy(image.data() + code_offset, instructions, code_size);

	AMX amx;
	memset(&amx, 0, sizeof(amx));
	amx.base = image.data();
	amx.data = reinterpret_cast<unsigned char *>(data.data());
	amx.callback = unused_callback;
	amx.flags = AMX_FLAG_TYPED | AMX_FLAG_RELOC | AMX_FLAG_NTVREG;
	amx.hlw = 16 * sizeof(cell);
	amx.hea = amx.hlw;
	amx.stp = static_cast<cell>(data.size() * sizeof(cell) - sizeof(cell));
	amx.stk = stack_offset < 0 ? amx.stp : stack_offset;
	if (stack_offset >= 0) {
		amx.cip = 0;
		amx.frm = frame_offset;
		amx.reset_stk = amx.stk;
		amx.reset_hea = amx.hea;
		return amx_Exec(&amx, result, AMX_EXEC_CONT);
	}
	return amx_Exec(&amx, result, AMX_EXEC_MAIN);
}

static int run_bool_check(cell value)
{
	const cell program[] = {11, value, 145, 120, 0};
	cell result = 0;
	return run_program(program, sizeof(program) / sizeof(program[0]), &result);
}

static int run_bool_reference_check(cell value)
{
	const cell program[] = {146, 0, 120, 0};
	cell data[64] = {0};
	data[1] = value;
	data[60] = sizeof(cell);
	data[61] = sizeof(cell);
	cell result = 0;
	return run_program(program, sizeof(program) / sizeof(program[0]), &result,
	                   60 * sizeof(cell), data);
}

static int run_bool_array_check(cell third_value, cell array_address)
{
	const cell program[] = {147, 0, 3, 120, 0};
	cell data[64] = {0};
	data[1] = 0;
	data[2] = 1;
	data[3] = third_value;
	data[60] = sizeof(cell);
	data[61] = array_address;
	cell result = 0;
	return run_program(program, sizeof(program) / sizeof(program[0]), &result,
	                   60 * sizeof(cell), data);
}

static int run_bool_frame_array_check(cell third_value, cell array_address,
                                      cell arg_index = 0,
                                      cell args_bytes = sizeof(cell),
                                      cell frame_offset = 45 * sizeof(cell))
{
	const cell program[] = {148, arg_index, 3, 120, 0};
	cell data[64] = {0};
	data[4] = 0;
	data[5] = 1;
	data[6] = third_value;
	data[frame_offset / sizeof(cell) + 2] = args_bytes;
	data[frame_offset / sizeof(cell) + 3 + arg_index] = array_address;
	cell result = 0;
	return run_program(program, sizeof(program) / sizeof(program[0]), &result,
	                   frame_offset, data, frame_offset);
}

int main()
{
	CHECK_ERROR(run_bool_check(0), AMX_ERR_NONE);
	CHECK_ERROR(run_bool_check(1), AMX_ERR_NONE);
	CHECK_ERROR(run_bool_check(2), AMX_ERR_TYPE);
	CHECK_ERROR(run_bool_reference_check(1), AMX_ERR_NONE);
	CHECK_ERROR(run_bool_reference_check(2), AMX_ERR_TYPE);
	CHECK_ERROR(run_bool_array_check(0, sizeof(cell)), AMX_ERR_NONE);
	CHECK_ERROR(run_bool_array_check(2, sizeof(cell)), AMX_ERR_TYPE);
	CHECK_ERROR(run_bool_array_check(0, 62 * sizeof(cell)), AMX_ERR_MEMACCESS);
	CHECK_ERROR(run_bool_frame_array_check(0, 4 * sizeof(cell)), AMX_ERR_NONE);
	CHECK_ERROR(run_bool_frame_array_check(2, 4 * sizeof(cell)), AMX_ERR_TYPE);
	CHECK_ERROR(run_bool_frame_array_check(0, 63 * sizeof(cell)), AMX_ERR_MEMACCESS);
	CHECK_ERROR(run_bool_frame_array_check(0, 4 * sizeof(cell), 1),
	            AMX_ERR_PARAMS);
	CHECK_ERROR(run_bool_frame_array_check(0, 4 * sizeof(cell), 0, 1),
	            AMX_ERR_PARAMS);
	CHECK_ERROR(run_bool_frame_array_check(0, 4 * sizeof(cell), 0,
	                                      sizeof(cell), 60 * sizeof(cell)),
	            AMX_ERR_PARAMS);
	return 0;
}
