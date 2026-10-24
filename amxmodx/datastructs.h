// vim: set ts=4 sw=4 tw=99 noet:
//
// AMX Mod X, based on AMX Mod by Aleksander Naszko ("OLO").
// Copyright (C) The AMX Mod X Development Team.
//
// This software is licensed under the GNU General Public License, version 3 or higher.
// Additional exceptions apply. For full license details, see LICENSE.txt or visit:
//     https://alliedmods.net/amxmodx-license

#ifndef DATASTRUCTS_H
#define DATASTRUCTS_H

#include "natives_handles.h"

class CellArray
{
public:
	CellArray(size_t blocksize, size_t basesize = 0) : m_Data(nullptr), m_BlockSize(blocksize), m_AllocSize(0), m_BaseSize(basesize > 0 ? basesize : 8), m_Size(0)
	{
	}

	~CellArray()
	{
		free(m_Data);
	}

	size_t size() const
	{
		return m_Size;
	}

	cell *push()
	{
		if (!GrowIfNeeded(1))
		{
			return NULL;
		}
		cell *arr = &m_Data[m_Size * m_BlockSize];
		m_Size++;
		return arr;
	}

	cell *at(size_t b) const
	{
		return &m_Data[b * m_BlockSize];
	}

	size_t blocksize() const
	{
		return m_BlockSize;
	}

	void clear()
	{
		m_Size = 0;
	}

	bool swap(size_t item1, size_t item2)
	{
		/* Make sure there is extra space available */
		if (!GrowIfNeeded(1))
		{
			return false;
		}

		cell *pri = at(item1);
		cell *alt = at(item2);

		/* Get our temporary array 1 after the limit */
		cell *temp = &m_Data[m_Size * m_BlockSize];

		memcpy(temp, pri, sizeof(cell)* m_BlockSize);
		memcpy(pri, alt, sizeof(cell)* m_BlockSize);
		memcpy(alt, temp, sizeof(cell)* m_BlockSize);

		return true;
	}

	void remove(size_t index)
	{
		/* If we're at the end, take the easy way out */
		if (index == m_Size - 1)
		{
			m_Size--;
			return;
		}

		/* Otherwise, it's time to move stuff! */
		size_t remaining_indexes = (m_Size - 1) - index;
		cell *src = at(index + 1);
		cell *dest = at(index);
		memmove(dest, src, sizeof(cell)* m_BlockSize * remaining_indexes);

		m_Size--;
	}

	cell *insert_at(size_t index)
	{
		/* Make sure it'll fit */
		if (!GrowIfNeeded(1))
		{
			return nullptr;
		}

		/* move everything up */
		cell *src = at(index);
		cell *dst = at(index + 1);
		memmove(dst, src, sizeof(cell)* m_BlockSize * (m_Size - index));

		m_Size++;

		return src;
	}

	bool resize(size_t count)
	{
		if (count <= m_Size)
		{
			m_Size = count;
			return true;
		}

		if (!GrowIfNeeded(count - m_Size))
		{
			return false;
		}

		m_Size = count;
		return true;
	}

	CellArray *clone()
	{
		CellArray *array = new CellArray(m_BlockSize);
		array->m_AllocSize = m_AllocSize;
		array->m_Size = m_Size;
		array->m_Data = (cell *)malloc(sizeof(cell)* m_BlockSize * m_AllocSize);

		if (!array->m_Data)
		{
			delete array;
			return nullptr;
		}

		memcpy(array->m_Data, m_Data, sizeof(cell)* m_BlockSize * m_Size);
		return array;
	}

	cell *base()
	{
		return m_Data;
	}

	size_t mem_usage()
	{
		return m_AllocSize * m_BlockSize * sizeof(cell);
	}

private:
	bool GrowIfNeeded(size_t count)
	{
		const size_t maxSize = static_cast<size_t>(-1);
		if (count > maxSize - m_Size)
		{
			return false;
		}

		size_t allocSize = m_AllocSize;
		if (!allocSize)
		{
			allocSize = m_BaseSize;
		}

		/* Shortcut out if we can store this */
		if (m_AllocSize && m_Size + count <= m_AllocSize)
		{
			return true;
		}
		/* If it's not enough, keep doubling */
		while (m_Size + count > allocSize)
		{
			if (allocSize > maxSize / 2)
			{
				return false;
			}
			allocSize *= 2;
		}

		if (m_BlockSize && allocSize > maxSize / m_BlockSize)
		{
			return false;
		}
		size_t cellCount = m_BlockSize * allocSize;
		if (cellCount > maxSize / sizeof(cell))
		{
			return false;
		}

		/* finally, allocate the new block */
		cell *newData;
		if (m_Data)
		{
			newData = (cell *)realloc(m_Data, sizeof(cell) * cellCount);
		}
		else {
			newData = (cell *)malloc(sizeof(cell) * cellCount);
		}
		if (!newData)
		{
			return false;
		}

		m_Data = newData;
		m_AllocSize = allocSize;
		return true;
	}
private:
	cell  *m_Data;
	size_t m_BlockSize;
	size_t m_AllocSize;
	size_t m_BaseSize;
	size_t m_Size;
};

extern NativeHandle<CellArray> ArrayHandles;

#endif
