// Very very simple vector math library

#include "math.h"

typedef float vec3_t[3];
typedef float vec4_t[4];

typedef vec3_t mat33_t[3];
typedef vec4_t mat44_t[4];

void MatIdentity(mat33_t &m)
{
    m[0][0] = 1.f;
    m[0][1] = 0.f;
    m[0][2] = 0.f;

    m[1][0] = 0.f;
    m[1][1] = 1.f;
    m[1][2] = 0.f;

    m[2][0] = 0.f;
    m[2][1] = 0.f;
    m[2][2] = 1.f;
}

void MatIdentity(mat44_t &m)
{
    m[0][0] = 1.f;
    m[0][1] = 0.f;
    m[0][2] = 0.f;
    m[0][3] = 0.f;

    m[1][0] = 0.f;
    m[1][1] = 1.f;
    m[1][2] = 0.f;
    m[1][3] = 0.f;

    m[2][0] = 0.f;
    m[2][1] = 0.f;
    m[2][2] = 1.f;
    m[2][3] = 0.f;

    m[3][0] = 0.f;
    m[3][1] = 0.f;
    m[3][2] = 0.f;
    m[3][3] = 1.f;
}

void Norm(vec3_t &a)
{
    float vlen = 1.f / sqrtf(a[0]*a[0] + a[1]*a[1] + a[2]*a[2]);
    a[0] = a[0] * vlen;
    a[1] = a[1] * vlen;
    a[2] = a[2] * vlen;
}

float Dot(const vec3_t &a, const vec3_t &b)
{
    return a[0]*b[0] + a[1]*b[1] + a[2]*b[2];
}

void Cross(const vec3_t &a, const vec3_t &b, vec3_t &r)
{
    r[0] = a[1]*b[2]-a[2]*b[1];
    r[1] = a[2]*b[0]-a[0]*b[2];
    r[2] = a[0]*b[1]-a[1]*b[0];
}

void MatrixLookAt(const vec3_t &eye, const vec3_t &at, const vec3_t &up, mat44_t &m)
{
    vec3_t xaxis, yaxis, zaxis;

    zaxis[0] = eye[0] - at[0];
    zaxis[1] = eye[1] - at[1];
    zaxis[2] = eye[2] - at[2];
    Norm(zaxis);

    Cross(up, zaxis, xaxis);
    Norm(xaxis);

    Cross(zaxis, xaxis, yaxis);

    float dx = Dot(xaxis, eye);
    float dy = Dot(yaxis, eye);
    float dz = Dot(zaxis, eye);

    m[0][0] = xaxis[0];
    m[0][1] = yaxis[0];
    m[0][2] = zaxis[0];
    m[0][3] = 0.f;

    m[1][0] = xaxis[1];
    m[1][1] = yaxis[1];
    m[1][2] = zaxis[1];
    m[1][3] = 0.f;

    m[2][0] = xaxis[2];
    m[2][1] = yaxis[2];
    m[2][2] = zaxis[2];
    m[2][3] = 0.f;

    m[3][0] = dx;
    m[3][1] = dy;
    m[3][2] = dz;
    m[3][3] = 1.f;
}

void MatrixPerspectiveRH(const float w, const float h, const float zn, const float zf, mat44_t &m)
{
    m[0][0] = 2.f*zn/w;
    m[0][1] = 0.f;
    m[0][2] = 0.f;
    m[0][3] = 0.f;

    m[1][0] = 0.f;
    m[1][1] = 2.f*zn/h;
    m[1][2] = 0.f;
    m[1][3] = 0.f;

    m[2][0] = 0.f;
    m[2][1] = 0.f;
    m[2][2] = zf/(zn-zf);
    m[2][3] = -1.f;

    m[3][0] = 0.f;
    m[3][1] = 0.f;
    m[3][2] = zn*zf/(zn-zf);
    m[3][3] = 0.f;
}

void MatrixMultiply(const mat44_t &a, const mat44_t &b, mat44_t &m)
{
    for ( int i = 0; i < 4; ++i )
        for ( int j = 0; j < 4; ++j )
            m[i][j] = a[i][0]*b[0][j] + a[i][1]*b[1][j] + a[i][2]*b[2][j] + a[i][3]*b[3][j];
}

void Vec4Transform(const vec4_t &v, const mat44_t &m, vec4_t &r)
{
    for ( int i = 0; i < 4; ++i )
       r[i] = v[0] * m[0][i] + v[1] * m[1][i] + v[2] + m[2][i] + v[3] * m[3][i];
    r[0] = r[0]/r[3];
    r[1] = r[1]/r[3];
    r[2] = r[2]/r[3];
}
