#include "core.h"
#include "uart.h"

#include "bvh8.h"

#define MAX_NODE_TRIS 32
#define MAX_STACK_ENTRIES 16

struct BVH8LeafNode
{
	uint32_t m_numTriangles{0};
	uint32_t m_triangleIndices[MAX_NODE_TRIS]{};
};

/*void Barycentrics(SVec128& P, SVec128& v1, SVec128& v2, SVec128& v3, SVec128& uvw)
{
    SVec128 e1 = EVecSub(v3, v1);
    SVec128 e2 = EVecSub(v2, v1);
    SVec128 e = EVecSub(P, v1);
    float d00 = EVecGetFloatX(EVecDot3(e1, e1));
    float d01 = EVecGetFloatX(EVecDot3(e1, e2));
    float d11 = EVecGetFloatX(EVecDot3(e2, e2));
    float d20 = EVecGetFloatX(EVecDot3(e, e1));
    float d21 = EVecGetFloatX(EVecDot3(e, e2));
    float invdenom = 1.0 / (d00 * d11 - d01 * d01);

	float bu = (d11 * d20 - d01 * d21) * invdenom;
	float bv = (d00 * d21 - d01 * d20) * invdenom;
	float bw = 1.f - bu - bv;

	uvw = EVecConst(bu, bv, bw, 0.f);
}

float TriHit(SVec128& origin, SVec128& direction, SVec128& v1, SVec128& v2, SVec128& v3, float max_t)
{
    SVec128 e1 = EVecSub(v3, v1);
    SVec128 e2 = EVecSub(v2, v1);
    SVec128 s1 = EVecCross3(direction, e2);
	SVec128 K = EVecDot3(s1, e1);

#if defined(DOUBLE_SIDED)
	// No facing check in this case
#else
	if (EVecGetFloatX(K) >= 0.f)
		return max_t; // Ignore backfacing (TODO: enable/disable this)
#endif

    SVec128 invd = EVecRcp(K);
    SVec128 d = EVecSub(origin, v1);
    SVec128 b1 = EVecMul(EVecDot3(d, s1), invd);
    SVec128 s2 = EVecCross3(d, e1);
    SVec128 b2 = EVecMul(EVecDot3(direction, s2), invd);
    SVec128 temp = EVecMul(EVecDot3(e2, s2), invd);

	float fb1 = EVecGetFloatX(b1);
	float fb2 = EVecGetFloatX(b2);
	float ftemp = EVecGetFloatX(temp);

    if (fb1 < 0.f || fb1 > 1.f ||
        fb2 < 0.f || fb1 + fb2 > 1.f ||
        ftemp < 0.f || ftemp > max_t)
		return max_t; // Missed
    else
		return ftemp; // Hit
}

bool SlabTestFast(SVec128& p0, SVec128& p1, SVec128& rayOrigin, SVec128& rayDir, SVec128& invRayDir)
{
	SVec128 t0 = EVecMul(EVecSub(p0, rayOrigin), invRayDir);
	SVec128 t1 = EVecMul(EVecSub(p1, rayOrigin), invRayDir);
	SVec128 tmin = EVecMin(t0, t1);
	SVec128 tmax = EVecMax(t0, t1);
	float enter = EVecMaxComponent3(tmin);
	float exit = EVecMinComponent3(tmax);
	return enter <= exit;
}

uint32_t gatherChildNodes(SBVH8Database<BVH8LeafNode>* bvh, uint32_t rootNode, SVec128 startPos, uint32_t rayOctantMask, SVec128 deltaVec, SVec128 invDeltaVec, uint32_t* hitcells)
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
		if (SlabTestFast(subminbounds, submaxbounds, startPos, deltaVec, invDeltaVec))
		{
			uint32_t byoctant = (bvh->m_dataLookup[idx].m_SpatialKey & 0x00000007) ^ rayOctantMask;
			hitcells[byoctant] = idx;
			octantallocationmask |= (1 << byoctant);
		}
	}

	return octantallocationmask;
}

int traceBVH8(SBVH8Database<BVH8LeafNode>* bvh, uint32_t& marchCount, float& t, SVec128& startPos, uint32_t& hitID, SVec128& deltaVec, SVec128& invDeltaVec, SVec128& hitPos)
{
	uint32_t traversalStack[MAX_STACK_ENTRIES]{};
	int stackpointer = 0;

	// Store root node to start travelsal with
	traversalStack[stackpointer++] = bvh->m_LodStart[bvh->m_RootBVH8Node];

	// Generate ray octant mask
	uint32_t ray_octant = (deltaVec.x < 0.f ? 1:0) | (deltaVec.y < 0.f ? 2:0) | (deltaVec.z < 0.f ? 4:0);

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
			uint32_t octantallocationmask = gatherChildNodes(bvh, currentNode, startPos, ray_octant, deltaVec, invDeltaVec, hitcells);

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
			//if (leafNodeHitTest(currentNode, m_dataLookup[currentNode].m_DataIndex, this, ray, hit))
			//	return 1;

			// Only hit the BVH8 leaf node with no data access
#ifdef IGNORE_CHILD_DATA
			SVec128 subminbounds = bvh->m_dataLookup[currentNode].m_BoundsMin;
			SVec128 submaxbounds = bvh->m_dataLookup[currentNode].m_BoundsMax;
			SVec128 exitpos;
			hitID=currentNode;
			SlabTest(subminbounds, submaxbounds, startPos, deltaVec, invDeltaVec, hitPos, exitpos);
			t = EVecGetFloatX(EVecLen3(EVecSub(hitPos, startPos)));
			return 1; // NOTE: do not return to generate an x-ray view
#else

			// Default inline hit test returning hit position
			uint32_t modelNode = bvh->m_dataLookup[currentNode].m_DataIndex;
			float last_t = 1.f;
			uint32_t hitTriangleIndex = 0xFFFFFFFF;
			for (uint32_t tri=0; tri<bvh->m_data[modelNode].m_numTriangles; ++tri)
			{
				uint32_t triangleIndex = bvh->m_data[modelNode].m_triangleIndices[tri];

				SVec128 v1 = testtris[triangleIndex].coords[0];
				SVec128 v2 = testtris[triangleIndex].coords[1];
				SVec128 v3 = testtris[triangleIndex].coords[2];

				// Hit if closer than before
				float curr_t = TriHit(startPos, deltaVec, v1, v2, v3, last_t);
				if (curr_t < last_t)
				{
					last_t = curr_t;
					hitTriangleIndex = triangleIndex;
				}
			}

			if (hitTriangleIndex == 0xFFFFFFFF) // Nothing hit
				continue;

			// TODO: hit position, hit normal etc

			t = last_t;
			hitID = hitTriangleIndex;
			hitPos = EVecAdd(startPos, EVecMul(EVecConst(t, t, t, 0.f), deltaVec));
			// Positive hit
			return 1;
#endif
		}
	}

	return 0;
}*/

int main()
{
    UARTWrite("BVH8 ray tracing test\n");

	// Create the BVH container
	SBVH8Database<BVH8LeafNode>* testBVH8;
	testBVH8 = new SBVH8Database<BVH8LeafNode>;

	// TODO: load contents from storage

	// TODO: trace into the BVH

	// Done
	delete testBVH8;

    return 0;
}
