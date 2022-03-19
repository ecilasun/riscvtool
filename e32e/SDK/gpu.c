#include "gpu.h"

volatile uint8_t *GPUFB0 = (volatile uint8_t* )0x81000000;
volatile uint32_t *GPUFB0WORD = (volatile uint32_t* )0x81000000;
volatile uint8_t *GPUFB1 = (volatile uint8_t* )0x81020000;
volatile uint32_t *GPUFB1WORD = (volatile uint32_t* )0x81020000;
volatile uint32_t *GPUPAL_32 = (volatile uint32_t* )0x81040000;
volatile uint32_t *GPUCTL = (volatile uint32_t* )0x81040100; // Up to 0x8104FFFF

#define max(_a_, _b_) (_a_) > (_b_) ? (_a_) : (_b_)
#define min(_a_, _b_) (_a_) < (_b_) ? (_a_) : (_b_)

/*#define screenWidth 320
#define screenHeight 240

static const int subStep = 256;
static const int subMask = subStep - 1;

struct Point2D {
    int x, y;
    Point2D() : x(0), y(0) {}
    Point2D(int _x, int _y) : x(_x), y(_y) {}
};

int orient2d(const Point2D* a, const Point2D* b, const Point2D& c)
{
    return (b->x-a->x)*(c.y-a->y) - (b->y-a->y)*(c.x-a->x);
}

void drawTri(const Point2D* v0, const Point2D* v1, const Point2D* v2, const uint32_t workunit)
{
  // Compute triangle bounding box
  int minX = min(v0->x, min(v1->x, v2->x));
  int minY = min(v0->y, min(v1->y, v2->y));
  int maxX = max(v0->x, max(v1->x, v2->x));
  int maxY = max(v0->y, max(v1->y, v2->y));

  // 16*16 tile blocks, 20x15 pixels each
  int tileX = (workunit%16)*20;
  int tileY = (workunit/16)*15;

  // Clip against screen bounds : TODO: Clip against HART tile bounds
  minX = max(minX, tileX);
  minY = max(minY, tileY);
  maxX = min(maxX, tileX+19);
  maxY = min(maxY, tileY+14);

  int bias0 = 0;//isTopLeft(v1, v2) ? 0 : -1;
  int bias1 = 0;//isTopLeft(v2, v0) ? 0 : -1;
  int bias2 = 0;//isTopLeft(v0, v1) ? 0 : -1;

  uint32_t W = workunit & 0xFF;
  uint32_t wcol = W | (W<<8) | (W<<16) | (W<<24);

  Point2D p;
  for (p.y = minY; p.y <= maxY; p.y++)
  {
    for (p.x = minX; p.x <= maxX; p.x+=4)
    {
      // Determine barycentric coordinates
      int w0 = orient2d(v1, v2, p) + bias0;
      int w1 = orient2d(v2, v0, p) + bias1;
      int w2 = orient2d(v0, v1, p) + bias2;

      // If p is on or inside all edges, render pixel.
      if (w0 >= 0 && w1 >= 0 && w2 >= 0)
        GPUFB0WORD[(p.x>>2)+p.y*80] = wcol;
    }
  }
}*/