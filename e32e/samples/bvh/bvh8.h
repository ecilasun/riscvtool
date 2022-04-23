#include <math.h>
#include <float.h>
#include <stdint.h>

#include <array>
#include <vector>
#include <algorithm>

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
};
