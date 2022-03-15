// Bootloader

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <memory.h>
#include <math.h>

#include "rvcrt0.h"
#include "encoding.h" // For CSR access macros

#include "core.h"
#include "uart.h"
#include "gpu.h"

//#include "memtest/memtest.h"

/*void testmemory()
{
    int i=0;
    uint64_t startclock = E32ReadTime();
    for (uint32_t m=0x0A000000; m<0x0C000000; m+=4)
    {
        *((volatile uint32_t*)m) = 0x00000000;
        if ((m!=0) && ((m%0x100000) == 0))
        {
            ++i;
            UARTWriteDecimal(i);
            UARTWrite(" Mbytes cleared @");
            UARTWriteHex((unsigned int)m);
            UARTWrite("\n");
        }
    }

    uint64_t endclock = E32ReadTime();
    uint32_t deltams = ClockToMs(endclock-startclock);
    UARTWrite("Clearing 32Mbytes took ");
    UARTWriteDecimal((unsigned int)deltams);
    UARTWrite(" ms, at ");

    int rate = (1024*32*1024) / deltams;
    UARTWriteDecimal(rate);
    UARTWrite(" Kbytes/sec\n");

    UARTWrite("\n-------------MemTest--------------\n");
    UARTWrite("Copyright (c) 2000 by Michael Barr\n");
    UARTWrite("----------------------------------\n");

    UARTWrite("Data bus test (0x0A000000-0x0A03FFFF)...");
    int failed = 0;
    for (uint32_t i=0x0A000000; i<0x0A03FFFF; i+=4)
    {
        failed += memTestDataBus((volatile datum*)i);
    }
    UARTWrite(failed == 0 ? "passed (" : "failed (");
    UARTWriteDecimal(failed);
    UARTWrite(" failures)\n");

    UARTWrite("Address bus test (0x0B000000-0x0B03FFFF)...");
    int errortype = 0;
    datum* res = memTestAddressBus((volatile datum*)0x0B000000, 262144, &errortype);
    UARTWrite(res == NULL ? "passed\n" : "failed\n");
    if (res != NULL)
    {
        if (errortype == 0)
            UARTWrite("Reason: Address bit stuck high at 0x");
        if (errortype == 1)
            UARTWrite("Reason: Address bit stuck low at 0x");
        if (errortype == 2)
            UARTWrite("Reason: Address bit shorted at 0x");
        UARTWriteHex((unsigned int)res);
        UARTWrite("\n");
    }

    UARTWrite("Memory device test (0x0C000000-0x0C03FFFF)...");
    datum* res2 = memTestDevice((volatile datum *)0x0C000000, 262144);
    UARTWrite(res2 == NULL ? "passed\n" : "failed\n");
    if (res2 != NULL)
    {
        UARTWrite("Reason: incorrect value read at 0x");
        UARTWriteHex((unsigned int)res2);
        UARTWrite("\n");
    }

    if ((failed != 0) | (res != NULL) | (res2 != NULL))
      UARTWrite("Memory device does not appear to be working correctly.\n");
}*/

#define M_PI 3.14159265358979323846

// Morton order work dispatch

void EMorton2DDecode(const uint32_t morton, uint32_t &x, uint32_t &y)
{
  uint32_t res = morton&0x5555555555555555;
  res=(res|(res>>1)) & 0x3333333333333333;
  res=(res|(res>>2)) & 0x0f0f0f0f0f0f0f0f;
  res=(res|(res>>4)) & 0x00ff00ff00ff00ff;
  res=res|(res>>8);
  x = res;

  res = (morton>>1)&0x5555555555555555;
  res=(res|(res>>1)) & 0x3333333333333333;
  res=(res|(res>>2)) & 0x0f0f0f0f0f0f0f0f;
  res=(res|(res>>4)) & 0x00ff00ff00ff00ff;
  res=res|(res>>8);
  y = res;
}

// CPU synchronization mailbox (uncached access, writes visible to all HARTs the following clock)
volatile uint32_t *mailbox = (volatile uint32_t*)0x80000000;

// Number of HARTs in E32E
// HART#0 - main HART
// HART#1..HART#6 - worker HARTs
static const int numharts = 6;

/* Modified by Engin Cilasun to fit the E32 graphics architecture  */
/* from Bruno Levy's original port of 2020                         */
/* Original tinyraytracer: https://github.com/ssloy/tinyraytracer  */

/*******************************************************************/

typedef int BOOL;

static inline float max(float x, float y) { return x>y?x:y; }
static inline float min(float x, float y) { return x<y?x:y; }

// The Bayer matrix for ordered dithering
const uint8_t dither[4][4] = {
  { 0, 8, 2,10},
  {12, 4,14, 6},
  { 3,11, 1, 9},
  {15, 7,13, 5}
};

/*******************************************************************/

// Size of the screen
// Replace with your own variables or values
#define graphics_width  320
#define graphics_height 240

// Normally you will not need to modify anything beyond that point.
/*******************************************************************/

typedef struct { float x,y,z; }   vec3;
typedef struct { float x,y,z,w; } vec4;

static inline vec3 make_vec3(float x, float y, float z) {
  vec3 V;
  V.x = x; V.y = y; V.z = z;
  return V;
}

static inline vec4 make_vec4(float x, float y, float z, float w) {
  vec4 V;
  V.x = x; V.y = y; V.z = z; V.w = w;
  return V;
}

static inline vec3 vec3_neg(vec3 V) {
  return make_vec3(-V.x, -V.y, -V.z);
}

static inline vec3 vec3_add(vec3 U, vec3 V) {
  return make_vec3(U.x+V.x, U.y+V.y, U.z+V.z);
}

static inline vec3 vec3_sub(vec3 U, vec3 V) {
  return make_vec3(U.x-V.x, U.y-V.y, U.z-V.z);
}

static inline float vec3_dot(vec3 U, vec3 V) {
  return U.x*V.x+U.y*V.y+U.z*V.z;
}

static inline vec3 vec3_scale(float s, vec3 U) {
  return make_vec3(s*U.x, s*U.y, s*U.z);
}

static inline float vec3_length(vec3 U) {
  return sqrtf(U.x*U.x+U.y*U.y+U.z*U.z);
}

static inline vec3 vec3_normalize(vec3 U) {
  return vec3_scale(1.0f/vec3_length(U),U);
}

/*************************************************************************/

typedef struct Light {
    vec3 position;
    float intensity;
} Light;

Light make_Light(vec3 position, float intensity) {
  Light L;
  L.position = position;
  L.intensity = intensity;
  return L;
}

/*************************************************************************/

typedef struct {
    float refractive_index;
    vec4  albedo;
    vec3  diffuse_color;
    float specular_exponent;
} Material;

Material make_Material(float r, vec4 a, vec3 color, float spec) {
  Material M;
  M.refractive_index = r;
  M.albedo = a;
  M.diffuse_color = color;
  M.specular_exponent = spec;
  return M;
}

Material make_Material_default() {
  Material M;
  M.refractive_index = 1;
  M.albedo = make_vec4(1,0,0,0);
  M.diffuse_color = make_vec3(0,0,0);
  M.specular_exponent = 0;
  return M;
}

/*************************************************************************/

typedef struct {
  vec3 center;
  float radius;
  Material material;
} Sphere;

Sphere make_Sphere(vec3 c, float r, Material M) {
  Sphere S;
  S.center = c;
  S.radius = r;
  S.material = M;
  return S;
}

BOOL Sphere_ray_intersect(Sphere* S, vec3 orig, vec3 dir, float* t0) {
  vec3 L = vec3_sub(S->center, orig);
  float tca = vec3_dot(L,dir);
  float d2 = vec3_dot(L,L) - tca*tca;
  float r2 = S->radius*S->radius;
  if (d2 > r2) return 0;
  float thc = sqrtf(r2 - d2);
  *t0       = tca - thc;
  float t1 = tca + thc;
  if (*t0 < 0) *t0 = t1;
  if (*t0 < 0) return 0;
  return 1;
}

vec3 reflect(vec3 I, vec3 N) {
  return vec3_sub(I, vec3_scale(2.f*vec3_dot(I,N),N));
}

vec3 refract(vec3 I, vec3 N, float eta_t, float eta_i /* =1.f */) {
  // Snell's law
  float cosi = -max(-1.f, min(1.f, vec3_dot(I,N)));
  // if the ray comes from the inside the object, swap the air and the media  
  if (cosi<0) return refract(I, vec3_neg(N), eta_i, eta_t); 
    float eta = eta_i / eta_t;
    float k = 1 - eta*eta*(1 - cosi*cosi);
    // k<0 = total reflection, no ray to refract.
    // I refract it anyways, this has no physical meaning
    return k<0 ? make_vec3(1,0,0)
              : vec3_add(vec3_scale(eta,I),vec3_scale((eta*cosi - sqrtf(k)),N));
}

BOOL scene_intersect(
   vec3 orig, vec3 dir, Sphere* spheres, int nb_spheres,
   vec3* hit, vec3* N, Material* material
) {
  float spheres_dist = 1e30;
  for(int i=0; i<nb_spheres; ++i) {
    float dist_i;
    if(
       Sphere_ray_intersect(&spheres[i], orig, dir, &dist_i) &&
       (dist_i < spheres_dist)
    ) {
      spheres_dist = dist_i;
      *hit = vec3_add(orig,vec3_scale(dist_i,dir));
      *N = vec3_normalize(vec3_sub(*hit, spheres[i].center));
      *material = spheres[i].material;
    }
  }
  float checkerboard_dist = 1e30;
  if (fabs(dir.y)>1e-3)  {
    float d = -(orig.y+4)/dir.y; // the checkerboard plane has equation y = -4
    vec3 pt = vec3_add(orig, vec3_scale(d,dir));
    if (d>0 && fabs(pt.x)<10 && pt.z<-10 && pt.z>-30 && d<spheres_dist) {
      checkerboard_dist = d;
      *hit = pt;
      *N = make_vec3(0,1,0);
      material->diffuse_color =
	(((int)(.5*hit->x+1000) + (int)(.5*hit->z)) & 1)
	             ? make_vec3(.3, .3, .3)
	             : make_vec3(.3, .2, .1);
    }
  }
  return min(spheres_dist, checkerboard_dist)<1000;
}

vec3 cast_ray(
   vec3 orig, vec3 dir, Sphere* spheres, int nb_spheres,
   Light* lights, int nb_lights, int depth /* =0 */
) {
  vec3 point,N;
  Material material = make_Material_default();
  if (
    depth>2 ||
    !scene_intersect(orig, dir, spheres, nb_spheres, &point, &N, &material)
  ) {
    float s = 0.5*(dir.y + 1.0);
    return vec3_add(
	vec3_scale(s,make_vec3(0.2, 0.7, 0.8)),
        vec3_scale(s,make_vec3(0.0, 0.0, 0.5))
    );
  }

  vec3 reflect_dir=vec3_normalize(reflect(dir, N));
  vec3 refract_dir=vec3_normalize(refract(dir,N,material.refractive_index,1));
  
  // offset the original point to avoid occlusion by the object itself 
  vec3 reflect_orig =
    vec3_dot(reflect_dir,N) < 0
               ? vec3_sub(point,vec3_scale(1e-3,N))
               : vec3_add(point,vec3_scale(1e-3,N)); 
  vec3 refract_orig =
    vec3_dot(refract_dir,N) < 0
               ? vec3_sub(point,vec3_scale(1e-3,N))
               : vec3_add(point,vec3_scale(1e-3,N));
  vec3 reflect_color = cast_ray(
       reflect_orig, reflect_dir, spheres, nb_spheres,
       lights, nb_lights, depth + 1
  );
  vec3 refract_color = cast_ray(
       refract_orig, refract_dir, spheres, nb_spheres,
       lights, nb_lights, depth + 1
  );
  
  float diffuse_light_intensity = 0, specular_light_intensity = 0;
  for (int i=0; i<nb_lights; i++) {
    vec3  light_dir = vec3_normalize(vec3_sub(lights[i].position,point));
    float light_distance = vec3_length(vec3_sub(lights[i].position,point));

    vec3 shadow_orig =
      vec3_dot(light_dir,N) < 0
                ? vec3_sub(point,vec3_scale(1e-3,N))
                : vec3_add(point,vec3_scale(1e-3,N)) ;
    // checking if the point lies in the shadow of the lights[i]
    vec3 shadow_pt, shadow_N;
    Material tmpmaterial;
    if (
       scene_intersect(
	 shadow_orig, light_dir, spheres, nb_spheres,
	 &shadow_pt, &shadow_N, &tmpmaterial
       ) && (
  	 vec3_length(vec3_sub(shadow_pt,shadow_orig)) < light_distance
	     )
    ) continue ;
    
    diffuse_light_intensity  +=
                  lights[i].intensity * max(0.f, vec3_dot(light_dir,N));
     
    float abc = max(
	           0.f, vec3_dot(vec3_neg(reflect(vec3_neg(light_dir), N)),dir)
	        );
    float def = material.specular_exponent;
    if(abc > 0.0f && def > 0.0f) {
      specular_light_intensity += powf(abc,def)*lights[i].intensity;
    }
  }
  vec3 result = vec3_scale(
      diffuse_light_intensity * material.albedo.x, material.diffuse_color
  );
  result = vec3_add(
       result, vec3_scale(specular_light_intensity * material.albedo.y,
       make_vec3(1,1,1))
  );
  result = vec3_add(result, vec3_scale(material.albedo.z, reflect_color));
  result = vec3_add(result, vec3_scale(material.albedo.w, refract_color));
  return result;
}

// Replace with your own code.
void graphics_set_pixel(int x, int y, vec3 &V) {
  uint16_t R = (uint16_t)(max(0.0f, min(1.0f, V.x))*255.0f);
  uint16_t G = (uint16_t)(max(0.0f, min(1.0f, V.y))*255.0f);
  uint16_t B = (uint16_t)(max(0.0f, min(1.0f, V.z))*255.0f);

  uint16_t ROFF = min(dither[x&3][y&3] + R, 255);
  uint16_t GOFF = min(dither[x&3][y&3] + G, 255);
  uint16_t BOFF = min(dither[x&3][y&3] + B, 255);

  R = ROFF/32;
  G = GOFF/32;
  B = BOFF/64;
  uint32_t RGB = (uint8_t)((B<<6) | (G<<3) | R);

  GPUFB0[x+y*graphics_width] = RGB;
}

void render(uint32_t hartid, uint32_t tileid, Sphere* spheres, int nb_spheres, Light* lights, int nb_lights) {
  const float fov  = M_PI/3.;

  // Assume 16x16 tiles -> tile dim: 20x15

  int TX = (tileid%16)*20;
  int TY = (tileid/16)*15;

  float dir_z = -graphics_height/(2.*tan(fov/2.));
  for (int j = TY; j<TY+15; j++)
  {
    float dir_y = -(j + 0.5) + graphics_height/2.;
    for (int i = TX; i<TX+20; i++)
    {
      float dir_x =  (i + 0.5) - graphics_width/2.;
      vec3 V = cast_ray( make_vec3(0,0,0), vec3_normalize(make_vec3(dir_x, dir_y, dir_z)), spheres, nb_spheres, lights, nb_lights, 0 );
      graphics_set_pixel(i, j, V);
    }
  }
}

int nb_spheres = 4;
Sphere spheres[4];

int nb_lights = 3;
Light lights[3];

void init_scene() {
    Material ivory = make_Material(
       1.0, make_vec4(0.6,  0.3, 0.1, 0.0), make_vec3(0.4, 0.4, 0.3),   50.
    );
    Material glass = make_Material(
       1.5, make_vec4(0.0,  0.5, 0.1, 0.8), make_vec3(0.6, 0.7, 0.8),  125.
    );
    Material red_rubber = make_Material(
       1.0, make_vec4(0.9,  0.1, 0.0, 0.0), make_vec3(0.3, 0.1, 0.1),   10.
    );
    Material mirror = make_Material(
       1.0, make_vec4(0.0, 10.0, 0.8, 0.0), make_vec3(1.0, 1.0, 1.0),  142.
    );

    spheres[0] = make_Sphere(make_vec3(-3,    0,   -16), 2,      ivory);
    spheres[1] = make_Sphere(make_vec3(-1.0, -1.5, -12), 2,      glass);
    spheres[2] = make_Sphere(make_vec3( 1.5, -0.5, -18), 3, red_rubber);
    spheres[3] = make_Sphere(make_vec3( 7,    5,   -18), 4,     mirror);

    lights[0] = make_Light(make_vec3(-20, 20,  20), 1.5);
    lights[1] = make_Light(make_vec3( 30, 50, -25), 1.8);
    lights[2] = make_Light(make_vec3( 30, 20,  30), 1.7);
}

// Worker CPU entry point
void workermain()
{
  // Wait for mailbox to notify us that HART#0 is ready
  uint32_t hartid = read_csr(mhartid);

  // Wait to be woken up
  uint32_t hartbit = (1<<hartid);
  while ((mailbox[numharts]&hartbit) == 0) { }

  init_scene();
  uint32_t workunit = 0xFFFFFFFF;
  do{
    // Wait for valid work unit
    do {
      workunit = mailbox[hartid];
    } while(workunit == 0xFFFFFFFF);
    // Rendering incoming work
    render(hartid, workunit, spheres, nb_spheres, lights, nb_lights);
    // Work done
    mailbox[hartid] = 0xFFFFFFFF;
  } while(1);
}

// Main CPU entry point
int main()
{
  // Reset work indices
  for (int i=0;i<numharts;++i)
    mailbox[i] = 0xFFFFFFFF;

  // Handle HART#0 specific code before we wake up other HARTs with work
  uint32_t hartid = read_csr(mhartid);

  // Set RGB palette
  int target = 0;
  for (int b=0;b<4;++b)
    for (int g=0;g<8;++g)
      for (int r=0;r<8;++r)
      {
          GPUPAL_32[target] = MAKERGBPALETTECOLOR(r*32, g*32, b*64);
          ++target;
      }

  UARTWrite("Rendering...\n");

  // Wake up other HARTs
  uint32_t hartenablemask = 0;
  for (uint32_t i=0; i<numharts; ++i)
    if (hartid!=i)
      hartenablemask |= (1<<i);
  mailbox[numharts] = hartenablemask;

  uint64_t startclock = E32ReadTime();

  // Work distributor
  int done = 0;
  uint32_t workunit = 0;
  do {
    // Distribute some work for each hart, which also wakes them up
    for (uint32_t i=1; i<numharts; ++i)
    {
      uint32_t currentworkunit = mailbox[i];
      if (currentworkunit == 0xFFFFFFFF)
      {
        uint32_t x, y;
        EMorton2DDecode(workunit, x, y);
        if (workunit < 256) // 16x16 tiles
          mailbox[i] = x+16*y;
        else
          done = 1;
        workunit++;
      }
    }
  } while (!done);

  uint64_t endclock = E32ReadTime();
  uint32_t deltams = ClockToMs(endclock-startclock);
  UARTWrite("tinyraytracerttymulti\n");
  UARTWrite("Resolution: 320*240\n");
  UARTWrite("HARTs: ");
  UARTWriteDecimal(numharts);
  UARTWrite("\nTime: ");
  UARTWriteDecimal((unsigned int)deltams);
  UARTWrite(" ms\n");

  // Try out the DDR3 memory
  //testmemory();

  while(1) { }

  return 0;
}
