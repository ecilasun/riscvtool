#include "core.h"
#include "uart.h"
#include "geometry.h"

#include <limits>
#include <vector>
#include <algorithm>
#include <math.h>
#include "gpu.h"

uint32_t vramPage = 0;
uint8_t *rtbuffer = (uint8_t*)GRAMStart; // We can use the whole G-RAM for graphics data, programs live in a separate memory, P-RAM
uint64_t totaltime = 0;

GPUCommandPackage gpuSetupProg;
GPUCommandPackage dmaProg;
GPUCommandPackage swapProg;

void PrepareCommandPackages()
{
    // RGB palette
    int target = 0;
    for (int b=0;b<4;++b)
    for (int g=0;g<8;++g)
    for (int r=0;r<8;++r)
    {
        GRAMStart[target] = MAKERGBPALETTECOLOR(r*32, g*32, b*64);
        ++target;
    }

    // GPU setup program

    GPUInitializeCommandPackage(&gpuSetupProg);
    GPUWritePrologue(&gpuSetupProg);
    GPUWriteInstruction(&gpuSetupProg, GPU_INSTRUCTION(G_MISC, G_R0, 0x0, 0x0, G_VPAGE)); // Write to page 0
    GPUWriteInstruction(&gpuSetupProg, GPU_INSTRUCTION(G_DMA, G_R0, G_R0, G_DMAGRAMTOPALETTE, 0x100)); // move palette from top of G-RAM to palette memory at 0
    GPUWriteEpilogue(&gpuSetupProg);
    GPUCloseCommandPackage(&gpuSetupProg);

    // DMA program

    int xoffset = 0;
    int yoffset = 0;
    GPUInitializeCommandPackage(&dmaProg);
    GPUWritePrologue(&dmaProg);
    for (int L=0; L<200; ++L)
    {
        // Source in G-RAM (note, these are byte addresses, align appropriately as needed)
        uint32_t gramsource = uint32_t(rtbuffer+L*320);
        GPUWriteInstruction(&dmaProg, GPU_INSTRUCTION(G_SETREG, G_R15, G_HIGHBITS, G_R15, HIHALF((uint32_t)gramsource)));
        GPUWriteInstruction(&dmaProg, GPU_INSTRUCTION(G_SETREG, G_R15, G_LOWBITS, G_R15, LOHALF((uint32_t)gramsource)));    // setregi r15, (uint32_t)mandelbuffer
        // Target in V-RAM (note, these are byte addresses, align appropriately as needed)
        uint32_t vramramtarget = ((L+yoffset)*512)+xoffset;
        GPUWriteInstruction(&dmaProg, GPU_INSTRUCTION(G_SETREG, G_R14, G_HIGHBITS, G_R14, HIHALF((uint32_t)vramramtarget)));
        GPUWriteInstruction(&dmaProg, GPU_INSTRUCTION(G_SETREG, G_R14, G_LOWBITS, G_R14, LOHALF((uint32_t)vramramtarget)));   // setregi r14, (uint32_t)vramtarget
        // DMA - 320/4 words
        uint32_t dmacount = 80;
        GPUWriteInstruction(&dmaProg, GPU_INSTRUCTION(G_DMA, G_R15, G_R14, G_DMAGRAMTOVRAM, dmacount));                                   // dma r15, r14, dmacount
    }
    GPUWriteEpilogue(&dmaProg);
    GPUCloseCommandPackage(&dmaProg);
}

struct Light {
    vec3 position;
    float intensity;
};

struct Material {
    float refractive_index = 1;
    vec4 albedo = {1,0,0,0};
    vec3 diffuse_color = {0,0,0};
    float specular_exponent = 0;
};

struct Sphere {
    vec3 center;
    float radius;
    Material material;
};

bool ray_sphere_intersect(const vec3 &orig, const vec3 &dir, const Sphere &s, float &t0) {
    vec3 L = s.center - orig;
    float tca = L*dir;
    float d2 = L*L - tca*tca;
    if (d2 > s.radius*s.radius) return false;
    float thc = sqrtf(s.radius*s.radius - d2);
    t0       = tca - thc;
    float t1 = tca + thc;
    if (t0 < 1e-3) t0 = t1;  // offset the original point to avoid occlusion by the object itself
    if (t0 < 1e-3) return false;
    return true;
}

vec3 reflect(const vec3 &I, const vec3 &N) {
    return I - N*2.f*(I*N);
}

vec3 refract(const vec3 &I, const vec3 &N, const float eta_t, const float eta_i=1.f) { // Snell's law
    float cosi = - std::max(-1.f, std::min(1.f, I*N));
    if (cosi<0) return refract(I, -N, eta_i, eta_t); // if the ray comes from the inside the object, swap the air and the media
    float eta = eta_i / eta_t;
    float k = 1 - eta*eta*(1 - cosi*cosi);
    return k<0 ? vec3{1,0,0} : I*eta + N*(eta*cosi - std::sqrt(k)); // k<0 = total reflection, no ray to refract. I refract it anyways, this has no physical meaning
}

bool scene_intersect(const vec3 &orig, const vec3 &dir, const std::vector<Sphere> &spheres, vec3 &hit, vec3 &N, Material &material) {
    float spheres_dist = std::numeric_limits<float>::max();
    for (const Sphere &s : spheres) {
        float dist_i;
        if (ray_sphere_intersect(orig, dir, s, dist_i) && dist_i < spheres_dist) {
            spheres_dist = dist_i;
            hit = orig + dir*dist_i;
            N = (hit - s.center).normalize();
            material = s.material;
        }
    }

    float checkerboard_dist = std::numeric_limits<float>::max();
    if (std::abs(dir.y)>1e-3) { // avoid division by zero
        float d = -(orig.y+4)/dir.y; // the checkerboard plane has equation y = -4
        vec3 pt = orig + dir*d;
        if (d>1e-3 && fabs(pt.x)<10 && pt.z<-10 && pt.z>-30 && d<spheres_dist) {
            checkerboard_dist = d;
            hit = pt;
            N = vec3{0,1,0};
            material.diffuse_color = (int(.5*hit.x+1000) + int(.5*hit.z)) & 1 ? vec3{.3, .3, .3} : vec3{.3, .2, .1};
        }
    }
    return std::min(spheres_dist, checkerboard_dist)<1000;
}

vec3 cast_ray(const vec3 &orig, const vec3 &dir, const std::vector<Sphere> &spheres, const std::vector<Light> &lights, size_t depth=0) {
    vec3 point, N;
    Material material;

    if (depth>4 || !scene_intersect(orig, dir, spheres, point, N, material))
        return vec3{0.2, 0.7, 0.8}; // background color

    vec3 reflect_dir = reflect(dir, N).normalize();
    vec3 refract_dir = refract(dir, N, material.refractive_index).normalize();
    vec3 reflect_color = cast_ray(point, reflect_dir, spheres, lights, depth + 1);
    vec3 refract_color = cast_ray(point, refract_dir, spheres, lights, depth + 1);

    float diffuse_light_intensity = 0, specular_light_intensity = 0;
    for (const Light light : lights) {
        vec3 light_dir      = (light.position - point).normalize();

        vec3 shadow_pt, trashnrm;
        Material trashmat;
        if (scene_intersect(point, light_dir, spheres, shadow_pt, trashnrm, trashmat) &&
                (shadow_pt-point).norm() < (light.position-point).norm()) // checking if the point lies in the shadow of the light
            continue;

        diffuse_light_intensity  += light.intensity * std::max(0.f, light_dir*N);
        specular_light_intensity += std::pow(std::max(0.f, -reflect(-light_dir, N)*dir), material.specular_exponent)*light.intensity;
    }
    return material.diffuse_color * diffuse_light_intensity * material.albedo[0] + vec3{1., 1., 1.}*specular_light_intensity * material.albedo[1] + reflect_color*material.albedo[2] + refract_color*material.albedo[3];
}

void render(const std::vector<Sphere> &spheres, const std::vector<Light> &lights) {
    const int   width    = 320;
    const int   height   = 200;
    const float fov      = 3.1415927f/3.f;

    // Set up
    GPUClearMailbox();
    GPUSubmitCommands(&gpuSetupProg);
    GPUKick();

    for (size_t j = 0; j<height; j++)
    {
        uint64_t startclock = ReadClock();
        for (size_t i = 0; i<width; i++)
        {
            float dir_x =  (i + 0.5f) - width/2.f;
            float dir_y = -(j + 0.5f) + height/2.f;    // this flips the image at the same time
            float dir_z = -height/(2.f*tan(fov/2.f));
            vec3 V = cast_ray(vec3{0,0,0}, vec3{dir_x, dir_y, dir_z}.normalize(), spheres, lights);
            float cmax = std::max(V[0], std::max(V[1], V[2]));
            if (cmax>1) V = V*(1.f/cmax);

            int R = int(255.f*V[0]) / 32;
            int G = int(255.f*V[1]) / 32;
            int B = int(255.f*V[2]) / 64;
            int RGB = (B<<6) | (G<<3) | R;
            rtbuffer[i+j*width] = (RGB<<24)|(RGB<<16)|(RGB<<8)|RGB;
        }
        uint64_t endclock = ReadClock();
        totaltime += (endclock-startclock);

        // Wait for previous work from end of last frame (or from the GPU setup program above) to be done.
        // Most likely they'll be complete by the time we get here due to the work involved in between frames.
        GPUWaitMailbox();

        // Show the mandelbrot image from G-RAM buffer
        GPUClearMailbox();
        GPUSubmitCommands(&dmaProg);
        GPUKick();

        GPUWaitMailbox(); // Wait before kicking the swap program

        // Wire up a 'swap' program on the fly
        vramPage = (vramPage+1)%2;
        GPUInitializeCommandPackage(&swapProg);
        GPUWritePrologue(&swapProg);
        GPUWriteInstruction(&swapProg, GPU_INSTRUCTION(G_SETREG, G_R15, G_HIGHBITS, G_R15, HIHALF(vramPage)));
        GPUWriteInstruction(&swapProg, GPU_INSTRUCTION(G_SETREG, G_R15, G_LOWBITS, G_R15, LOHALF(vramPage)));
        GPUWriteInstruction(&swapProg, GPU_INSTRUCTION(G_MISC, G_R15, 0x0, 0x0, G_VPAGE));
        GPUWriteEpilogue(&swapProg);
        GPUCloseCommandPackage(&swapProg);

        GPUClearMailbox();
        GPUSubmitCommands(&swapProg);
        GPUKick();
    }
}

int main()
{
    PrepareCommandPackages();

    const Material      ivory = {1.0, {0.6,  0.3, 0.1, 0.0}, {0.4, 0.4, 0.3},   50.};
    const Material      glass = {1.5, {0.0,  0.5, 0.1, 0.8}, {0.6, 0.7, 0.8},  125.};
    const Material red_rubber = {1.0, {0.9,  0.1, 0.0, 0.0}, {0.3, 0.1, 0.1},   10.};
    const Material     mirror = {1.0, {0.0, 10.0, 0.8, 0.0}, {1.0, 1.0, 1.0}, 1425.};

    std::vector<Sphere> spheres = {
        Sphere{vec3{-3,    0,   -16}, 2,      ivory},
        Sphere{vec3{-1.0, -1.5, -12}, 2,      glass},
        Sphere{vec3{ 1.5, -0.5, -18}, 3, red_rubber},
        Sphere{vec3{ 7,    5,   -18}, 4,     mirror}
    };

    std::vector<Light> lights = {
        {{-20, 20,  20}, 1.5},
        {{ 30, 50, -25}, 1.8},
        {{ 30, 20,  30}, 1.7}
    };

    render(spheres, lights);

    UARTWrite("Total time: ~");
    uint32_t totalms = ClockToMs(totaltime);
    UARTWriteDecimal(totalms/1000);
    UARTWrite(" seconds\n");

    return 0;
}
