#include "core.h"
#include "uart.h"

#include "bvh8.h"

#define minimum(x,y) (x<y?x:y)
#define maximum(x,y) (x<y?x:y)

#define MAX_NODE_TRIS 32
#define MAX_STACK_ENTRIES 16

struct BVH8LeafNode
{
	uint32_t m_numTriangles{0};
	uint32_t m_triangleIndices[MAX_NODE_TRIS]{};
};

inline int32_t BitCountLeadingZeroes32(uint32_t x)
{
	return x ? __builtin_clz(x) : 32;
}

bool SlabTest(SVec128 *p0, SVec128 *p1, SVec128 *rayOrigin, SVec128 *rayDir, SVec128 *invRayDir, SVec128 *hitpos, SVec128 *exitpos)
{
	SVec128 t0, t1, tmin, tmax;
	t0.x = (p0->x-rayOrigin->x)*invRayDir->x;
	t0.y = (p0->y-rayOrigin->y)*invRayDir->y;
	t0.z = (p0->z-rayOrigin->z)*invRayDir->z;
	t1.x = (p1->x-rayOrigin->x)*invRayDir->x;
	t1.y = (p1->y-rayOrigin->y)*invRayDir->y;
	t1.z = (p1->z-rayOrigin->z)*invRayDir->z;
	tmin.x = t0.x<t1.x?t0.x:t1.x;
	tmin.y = t0.y<t1.y?t0.y:t1.y;
	tmin.z = t0.z<t1.z?t0.z:t1.z;
	tmax.x = t0.x>t1.x?t0.x:t1.x;
	tmax.y = t0.y>t1.y?t0.y:t1.y;
	tmax.z = t0.z>t1.z?t0.z:t1.z;
	float enter = maximum(tmin.x, maximum(tmin.y,tmin.z));
	float exit = minimum(tmax.x, minimum(tmax.y,tmax.z));
	hitpos->x = rayOrigin->x + rayDir->x*enter;
	hitpos->y = rayOrigin->y + rayDir->y*enter;
	hitpos->z = rayOrigin->z + rayDir->z*enter;
	exitpos->x = rayOrigin->x + rayDir->x*exit;
	exitpos->y = rayOrigin->y + rayDir->y*exit;
	exitpos->z = rayOrigin->z + rayDir->z*exit;
	return enter <= exit;
}

bool SlabTestFast(SVec128 *p0, SVec128 *p1, SVec128 *rayOrigin, SVec128 *rayDir, SVec128 *invRayDir)
{
	SVec128 t0, t1, tmin, tmax;
	t0.x = (p0->x-rayOrigin->x)*invRayDir->x;
	t0.y = (p0->y-rayOrigin->y)*invRayDir->y;
	t0.z = (p0->z-rayOrigin->z)*invRayDir->z;
	t1.x = (p1->x-rayOrigin->x)*invRayDir->x;
	t1.y = (p1->y-rayOrigin->y)*invRayDir->y;
	t1.z = (p1->z-rayOrigin->z)*invRayDir->z;
	tmin.x = t0.x<t1.x?t0.x:t1.x;
	tmin.y = t0.y<t1.y?t0.y:t1.y;
	tmin.z = t0.z<t1.z?t0.z:t1.z;
	tmax.x = t0.x>t1.x?t0.x:t1.x;
	tmax.y = t0.y>t1.y?t0.y:t1.y;
	tmax.z = t0.z>t1.z?t0.z:t1.z;
	float enter = maximum(tmin.x, maximum(tmin.y,tmin.z));
	float exit = minimum(tmax.x, minimum(tmax.y,tmax.z));
	return enter <= exit;
}

uint32_t GatherChildNodes(SBVH8Database<BVH8LeafNode>* bvh, uint32_t rootNode, SVec128 startPos, uint32_t rayOctantMask, SVec128 deltaVec, SVec128 invDeltaVec, uint32_t* hitcells)
{
	uint32_t octantallocationmask = 0;

	// Generate octant allocation mask and store items in hit order (search on caller side will be in reverse bit scan order, i.e. from MSB to LSB)
	uint32_t childcount = bvh->m_dataLookup[rootNode].m_ChildCount;
	uint32_t firstChildNode = bvh->m_dataLookup[rootNode].m_DataIndex;
	for (uint32_t i = 0; i < childcount; ++i)
	{
		uint32_t idx = firstChildNode + i;
		SVec128 subminbounds = bvh->m_dataLookup[idx].m_BoundsMin;
		SVec128 submaxbounds = bvh->m_dataLookup[idx].m_BoundsMax;
		if (SlabTestFast(&subminbounds, &submaxbounds, &startPos, &deltaVec, &invDeltaVec))
		{
			uint32_t byoctant = (bvh->m_dataLookup[idx].m_SpatialKey & 0x00000007) ^ rayOctantMask;
			hitcells[byoctant] = idx;
			octantallocationmask |= (1 << byoctant);
		}
	}

	return octantallocationmask;
}

int traceBVH8NoTris(SBVH8Database<BVH8LeafNode>* bvh, uint32_t& marchCount, float& t, SVec128& startPos, uint32_t& hitID, SVec128& deltaVec, SVec128& invDeltaVec, SVec128& hitPos)
{
	uint32_t traversalStack[MAX_STACK_ENTRIES]{};
	int stackpointer = 0;

	// Store root node to start travelsal with
	traversalStack[stackpointer++] = bvh->m_LodStart[bvh->m_RootBVH8Node];

	// Generate ray octant mask
	uint32_t ray_octant = (deltaVec.x<0.f ? 1:0) | (deltaVec.y<0.f ? 2:0) | (deltaVec.z<0.f ? 4:0);

	hitID = 0xFFFFFFFF; // Miss
	hitPos.x = startPos.x + deltaVec.x; // End of ray
	hitPos.y = startPos.y + deltaVec.y;
	hitPos.z = startPos.z + deltaVec.z;
	t = 1.f;

	// Trace until stack underflows
	while (stackpointer > 0)
	{
		// Number of loops through until we find a hit
		++marchCount;

		--stackpointer;
		uint32_t currentNode = traversalStack[stackpointer];

		if (bvh->m_dataLookup[currentNode].m_ChildCount != 0)
		{
			uint32_t hitcells[8];
			uint32_t octantallocationmask = GatherChildNodes(bvh, currentNode, startPos, ray_octant, deltaVec, invDeltaVec, hitcells);

			uint32_t idx = BitCountLeadingZeroes32(octantallocationmask);
			while (idx != 32)
			{
				// Convert to cell index
				uint32_t cellindex = 31 - idx;

				// Stack overflow
				if (stackpointer > MAX_STACK_ENTRIES)
					return -1;

				// Push valid child nodes onto stack
				traversalStack[stackpointer++] = hitcells[cellindex];

				// Clear this bit for next iteration and get new index
				octantallocationmask ^= (1 << cellindex);
				idx = BitCountLeadingZeroes32(octantallocationmask);
			}
		}
		else // Leaf node reached
		{
			// Time to invoke a 'hit test' callback and stop if we have an actual hit
			// It is up to the hit test callback to determine which primitive in this cell is closest etc
			/*if (leafNodeHitTest(currentNode, m_dataLookup[currentNode].m_DataIndex, this, ray, hit))
				return 1;*/

			SVec128 subminbounds = bvh->m_dataLookup[currentNode].m_BoundsMin;
			SVec128 submaxbounds = bvh->m_dataLookup[currentNode].m_BoundsMax;
			SVec128 exitpos;
			hitID=currentNode;
			SlabTest(&subminbounds, &submaxbounds, &startPos, &deltaVec, &invDeltaVec, &hitPos, &exitpos);
			float dx = hitPos.x-startPos.x;
			float dy = hitPos.y-startPos.y;
			float dz = hitPos.z-startPos.z;
			float L = dx*dx+dy*dy+dz*dz;
			t = L<0.f ? 0.f : sqrtf(L);
		#ifndef XRAY_MODE
			return 1; // NOTE: do not return to generate an x-ray view
		#endif
		}
	}

	return 0;
}

int main()
{
    UARTWrite("BVH8 ray tracing test\n");

	// Create the BVH container
	SBVH8Database<BVH8LeafNode>* testBVH8;
	testBVH8 = new SBVH8Database<BVH8LeafNode>;
	testBVH8->LoadBVH8("sibenik.cache.bv8");

	UARTWrite("Test scene loaded\n");

	// TODO: load contents from storage

	// TODO: trace into the BVH

	// Done
	delete testBVH8;

    return 0;
}
