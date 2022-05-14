#include <math.h>
#include <float.h>
#include <stdint.h>

#include <array>
#include <vector>
#include <algorithm>

#include <stdio.h>

#include "math.h"

#define BVH8IllegalIndex 0xFFFFFFFF

struct SVec128
{
	float x,y,z,w;
};

#define EInline forceinline

struct BVH8NodeInfo
{
	uint32_t m_SpatialKey{0};
	uint32_t m_DataIndex{BVH8IllegalIndex};
	uint32_t m_ChildCount{0};
	uint32_t m_unused_padding_0;
	SVec128 m_BoundsMin{FLT_MAX,FLT_MAX,FLT_MAX,0.f};
	SVec128 m_BoundsMax{-FLT_MAX,-FLT_MAX,-FLT_MAX,0.f};
};

template <typename T>
struct SBVH8Database
{
	static const uint32_t MaxOctreeLevels = 16;
	std::vector<BVH8NodeInfo> m_dataLookup;
	std::vector<T> m_data;
	SVec128 m_GridAABBMin, m_GridCellSize;
	uint32_t m_LodStart[MaxOctreeLevels]{};
	uint32_t m_LodEnd[MaxOctreeLevels]{};
	uint32_t m_RootBVH8Node{};

	explicit SBVH8Database()
	{
		m_data.reserve(16384);
		m_dataLookup.reserve(16384);
	}

	uint32_t LoadBVH8(const char *_fname)
	{
		FILE *fp = fopen(_fname, "rb");
		if (fp)
		{
			uint32_t lookupcount = 0;
			uint32_t datacount = 0;
			float cellsize = 1.f;
			float minx = 0.f;
			float miny = 0.f;
			float minz = 0.f;
			fread(&m_RootBVH8Node, sizeof(uint32_t), 1, fp);
			for (uint32_t i=0;i<MaxOctreeLevels;++i)
			{
				fread(&m_LodStart[i], sizeof(uint32_t), 1, fp);
				fread(&m_LodEnd[i], sizeof(uint32_t), 1, fp);
			}
			fread(&lookupcount, sizeof(uint32_t), 1, fp);
			fread(&datacount, sizeof(uint32_t), 1, fp);

			fread(&cellsize, sizeof(float), 1, fp);
			m_GridCellSize.x = cellsize;
			m_GridCellSize.y = cellsize;
			m_GridCellSize.z = cellsize;
			m_GridCellSize.w = 0.f;

			fread(&minx, sizeof(float), 1, fp);
			fread(&miny, sizeof(float), 1, fp);
			fread(&minz, sizeof(float), 1, fp);
			m_GridAABBMin.x = minx;
			m_GridAABBMin.y = miny;
			m_GridAABBMin.z = minz;
			m_GridAABBMin.w = 0.f;

			m_dataLookup.resize(lookupcount);
			fread(m_dataLookup.data(), sizeof(BVH8NodeInfo), m_dataLookup.size(), fp);

			m_data.resize(datacount);
			fread(m_data.data(), sizeof(T), m_data.size(), fp);

			fclose(fp);
			return 1;
		}
		return 0;
	}
};
