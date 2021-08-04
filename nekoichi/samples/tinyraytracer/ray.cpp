#include "geometry.h"

#include <limits>
#include <vector>
#include <algorithm>
#include <math.h>
#include "gpu.h"

uint32_t vramPage = 0;
uint8_t *GRAM = (uint8_t *)GraphicsRAMStart;

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

    for (size_t j = 0; j<height; j++)
    {
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
            GRAM[i+j*width] = (B<<6) | (G<<3) | R;
        }
    }

    for (size_t j = 0; j<height; j++)
    {
        uint32_t sysramsource = uint32_t(GRAM) + j*width;
        GPUSetRegister(4, sysramsource);
        uint32_t vramramtarget = (j*512)>>2;
        GPUSetRegister(5, vramramtarget);
        uint32_t dmacount = width/4;
        GPUKickDMA(4, 5, dmacount, false);
    }
    vramPage = (vramPage+1)%2;
    GPUSetRegister(2, vramPage);
    GPUSetVideoPage(2);
}

int main()
{
	GPUSetRegister(2, vramPage);
	GPUSetVideoPage(2);

    // Make RGB palette
    int target = 0;
    for (int b=0;b<4;++b)
    for (int g=0;g<8;++g)
    for (int r=0;r<8;++r)
    {
        uint32_t color = MAKERGBPALETTECOLOR(r*32, g*32, b*64);
        GPUSetRegister(1, color);
        GPUSetPaletteEntry(1, target);
        ++target;
    }

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
    return 0;
}
