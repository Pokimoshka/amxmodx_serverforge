#include <stdio.h>

#include "CDataPack.h"
#include "datastructs.h"

#if defined(__GNUC__)
static bool g_failNextRealloc;

extern "C" void *__real_realloc(void *ptr, size_t size);

extern "C" void *__wrap_realloc(void *ptr, size_t size)
{
	if (g_failNextRealloc)
	{
		g_failNextRealloc = false;
		return NULL;
	}

	return __real_realloc(ptr, size);
}
#endif

#define CHECK(expr) \
	do { \
		if (!(expr)) { \
			fprintf(stderr, "Check failed at line %d: %s\n", __LINE__, #expr); \
			return 1; \
		} \
	} while (0)

int main()
{
	CDataPack pack;
	size_t initialPosition = pack.GetPosition();

	CHECK(!pack.SetPosition(0));
	CHECK(!pack.SetPosition(static_cast<size_t>(-1)));
	CHECK(pack.GetPosition() == initialPosition);

	CellArray array(1, 1);
	cell *item = array.push();
	CHECK(item != NULL);
	*item = 42;

	size_t oldSize = array.size();
	cell *oldData = array.base();
#if defined(__GNUC__)
	g_failNextRealloc = true;
	CHECK(!array.resize(2));
	CHECK(!g_failNextRealloc);
	CHECK(array.size() == oldSize);
	CHECK(array.base() == oldData);
	CHECK(array.at(0)[0] == 42);
#endif

	CHECK(!array.resize(static_cast<size_t>(-1) - 1));
	CHECK(array.size() == oldSize);
	CHECK(array.base() == oldData);
	CHECK(array.at(0)[0] == 42);

	CellArray oversizedArray(static_cast<size_t>(-1), 1);
	CHECK(oversizedArray.push() == NULL);
	CHECK(oversizedArray.size() == 0);

	return 0;
}
