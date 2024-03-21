#ifndef SIMPLE_MATH_H
#define SIMPLE_MATH_H

#define SM_PI   3.14159265358979323846264338327950288f
#define SM_PIDIV2 1.5707963267948966192313216916398f

#define SM_ABS(X) ((X) >= 0 ? X : -(X))

internal float
rad_to_deg(float rad) {
    float result = rad * 180.0f / SM_PI;
    return result;
}

internal float
deg_to_rad(float deg) {
    float result = deg * SM_PI / 180.0f;
    return result;
}

union v2 {
    float e[2];
    struct {
        float x, y;
    };
    struct {
        float u, v;
    };
    struct {
        float width, height;
    };
};

union v3 {
    float e[3];
    struct {
        float x, y, z;
    };
    struct {
        float u, v, w;
    };
    struct {
        v2 xy;
        float _unused0;
    };
};

union v4 {
    float e[4];
    struct {
        float x, y, z, w;
    };
    struct {
        float r, g, b, a;
    };
    struct {
        v2 xy;
        float _unused0;
        float _unused1;
    };
    struct {
        v3 xyz;
        float _unused2;
    };
};

union m4 {
    float e[4][4];
};

internal v2
V2(float x, float y) {
    v2 result;
    result.x = x;
    result.y = y;
    return result;
}

internal v3
V3(float x, float y, float z) {
    v3 result;
    result.x = x;
    result.y = y;
    result.z = z;
    return result;
}


internal v4
V4(float x, float y, float z, float w) {
    v4 result;
    result.x = x;
    result.y = y;
    result.z = z;
    result.w = w;
    return result;
}

internal v4
V4(v3 v, float w) {
    v4 result;
    result.x = v.x;
    result.y = v.y;
    result.z = v.z;
    result.w = w;
    return result;
}

internal v3
v3_negate(v3 v) {
    v3 result;
    result.x = -v.x;
    result.y = -v.y;
    result.z = -v.z;
    return result;
}

internal v4
v4_negate(v4 v) {
    v4 result;
    result.x = -v.x;
    result.y = -v.y;
    result.z = -v.z;
    result.w = -v.w;
    return result;
}

internal m4
m4_id() {
    m4 result{};
    result.e[0][0] = 1.0f;
    result.e[1][1] = 1.0f;
    result.e[2][2] = 1.0f;
    result.e[3][3] = 1.0f;
    return result;
}

internal m4
m4_mul(m4 b, m4 a) {
    m4 result{};
    result.e[0][0] = a.e[0][0] * b.e[0][0] + a.e[0][1] * b.e[1][0] + a.e[0][2] * b.e[2][0] + a.e[0][3] * b.e[3][0]; 
    result.e[0][1] = a.e[0][0] * b.e[0][1] + a.e[0][1] * b.e[1][1] + a.e[0][2] * b.e[2][1] + a.e[0][3] * b.e[3][1]; 
    result.e[0][2] = a.e[0][0] * b.e[0][2] + a.e[0][1] * b.e[1][2] + a.e[0][2] * b.e[2][2] + a.e[0][3] * b.e[3][2]; 
    result.e[0][3] = a.e[0][0] * b.e[0][3] + a.e[0][1] * b.e[1][3] + a.e[0][2] * b.e[2][3] + a.e[0][3] * b.e[3][3]; 
    result.e[1][0] = a.e[1][0] * b.e[0][0] + a.e[1][1] * b.e[1][0] + a.e[1][2] * b.e[2][0] + a.e[1][3] * b.e[3][0]; 
    result.e[1][1] = a.e[1][0] * b.e[0][1] + a.e[1][1] * b.e[1][1] + a.e[1][2] * b.e[2][1] + a.e[1][3] * b.e[3][1]; 
    result.e[1][2] = a.e[1][0] * b.e[0][2] + a.e[1][1] * b.e[1][2] + a.e[1][2] * b.e[2][2] + a.e[1][3] * b.e[3][2]; 
    result.e[1][3] = a.e[1][0] * b.e[0][3] + a.e[1][1] * b.e[1][3] + a.e[1][2] * b.e[2][3] + a.e[1][3] * b.e[3][3]; 
    result.e[2][0] = a.e[2][0] * b.e[0][0] + a.e[2][1] * b.e[1][0] + a.e[2][2] * b.e[2][0] + a.e[2][3] * b.e[3][0]; 
    result.e[2][1] = a.e[2][0] * b.e[0][1] + a.e[2][1] * b.e[1][1] + a.e[2][2] * b.e[2][1] + a.e[2][3] * b.e[3][1]; 
    result.e[2][2] = a.e[2][0] * b.e[0][2] + a.e[2][1] * b.e[1][2] + a.e[2][2] * b.e[2][2] + a.e[2][3] * b.e[3][2]; 
    result.e[2][3] = a.e[2][0] * b.e[0][3] + a.e[2][1] * b.e[1][3] + a.e[2][2] * b.e[2][3] + a.e[2][3] * b.e[3][3]; 
    result.e[3][0] = a.e[3][0] * b.e[0][0] + a.e[3][1] * b.e[1][0] + a.e[3][2] * b.e[2][0] + a.e[3][3] * b.e[3][0]; 
    result.e[3][1] = a.e[3][0] * b.e[0][1] + a.e[3][1] * b.e[1][1] + a.e[3][2] * b.e[2][1] + a.e[3][3] * b.e[3][1]; 
    result.e[3][2] = a.e[3][0] * b.e[0][2] + a.e[3][1] * b.e[1][2] + a.e[3][2] * b.e[2][2] + a.e[3][3] * b.e[3][2]; 
    result.e[3][3] = a.e[3][0] * b.e[0][3] + a.e[3][1] * b.e[1][3] + a.e[3][2] * b.e[2][3] + a.e[3][3] * b.e[3][3]; 
    return result;
}

internal m4
m4_translate(v3 t) {
    m4 result = m4_id();
    result.e[3][0] = t.x;
    result.e[3][1] = t.y;
    result.e[3][2] = t.z;
    return result;
}

internal m4
m4_scale(v3 s) {
    m4 result{};
    result.e[0][0] = s.x;
    result.e[1][1] = s.y;
    result.e[2][2] = s.z;
    result.e[3][3] = 1.0f;
    return result;
}

internal v2
v2_mul_s(v2 v, float s) {
    v2 result;
    result.x = v.x * s;
    result.y = v.y * s;
    return result;
}

internal v2
v2_div_s(v2 v, float s) {
    v2 result;
    result.x = v.x / s;
    result.y = v.y / s;
    return result;
}

internal v2
v2_add(v2 a, v2 b) {
    v2 result;
    result.x = a.x + b.x;
    result.y = a.y + b.y;
    return result;
}

internal v2
v2_subtract(v2 a, v2 b) {
    v2 result;
    result.x = a.x - b.x;
    result.y = a.y - b.y;
    return result;
}

internal v3
v3_mul_s(v3 v, float s) {
    v3 result;
    result.x = v.x * s;
    result.y = v.y * s;
    result.z = v.z * s;
    return result;
}

internal v3
v3_add(v3 a, v3 b) {
    v3 result;
    result.x = a.x + b.x;
    result.y = a.y + b.y;
    result.z = a.z + b.z;
    return result; 
}

internal v3
v3_subtract(v3 a, v3 b) {
    v3 result;
    result.x = a.x - b.x;
    result.y = a.y - b.y;
    result.z = a.z - b.z;
    return result; 
}

internal float
v3_length(v3 v) {
    float result;
    result = sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
    return result;
}

internal v3
v3_normal(v3 v) {
    v3 result;
    float length = v3_length(v);
    result.x = v.x / length;
    result.y = v.y / length;
    result.z = v.z / length;
    return result;
}

internal float
v4_length(v4 v) {
    float result;
    result = sqrt(v.x * v.x + v.y * v.y + v.z * v.z + v.w * v.w);
    return result;
}

internal v4
v4_normal(v4 v) {
    v4 result;
    float length = v4_length(v);
    result.x = v.x / length;
    result.y = v.y / length;
    result.z = v.z / length;
    result.w = v.w / length;
    return result;
}

internal v3
v3_cross(v3 a, v3 b) {
    v3 result;
    result.x = (a.y * b.z) - (a.z * b.y);
    result.y = (a.z * b.x) - (a.x * b.z);
    result.z = (a.x * b.y) - (a.y * b.x);
    return result;
}

internal float
v3_dot(v3 a, v3 b) {
    float result;
    result = a.x * b.x + a.y * b.y + a.z * b.z;
    return result; 
}

internal float
v4_dot(v4 a, v4 b) {
    float result;
    result = a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;
    return result; 
}

internal m4
m4_transpose(m4 m) {
    m4 result = m;
    result.e[0][1] = m.e[1][0];
    result.e[0][2] = m.e[2][0];
    result.e[0][3] = m.e[3][0];
    result.e[1][0] = m.e[0][1];
    result.e[1][2] = m.e[2][1];
    result.e[1][3] = m.e[3][1];
    result.e[2][0] = m.e[0][2];
    result.e[2][1] = m.e[1][2];
    result.e[2][3] = m.e[3][2];
    result.e[3][0] = m.e[0][3];
    result.e[3][1] = m.e[1][3];
    result.e[3][2] = m.e[2][3];
    return result;
}

internal m4
m4_rotation_axis(float angle, v3 axis) {
    m4 result = m4_id();
    axis = v3_normal(axis);

    float sin_t = sinf(deg_to_rad(angle));
    float cos_t = cosf(deg_to_rad(angle));
    float cos_inv = 1.0f - cos_t;

    result.e[0][0] = (cos_inv * axis.x * axis.x) + cos_t;
    result.e[0][1] = (cos_inv * axis.x * axis.y) + (axis.z * sin_t);
    result.e[0][2] = (cos_inv * axis.x * axis.z) - (axis.y * sin_t);

    result.e[1][0] = (cos_inv * axis.y * axis.x) - (axis.z * sin_t);
    result.e[1][1] = (cos_inv * axis.y * axis.y) - cos_t;
    result.e[1][2] = (cos_inv * axis.y * axis.z) + (axis.x * sin_t);

    result.e[2][0] = (cos_inv * axis.z * axis.x) + (axis.y * sin_t);
    result.e[2][1] = (cos_inv * axis.z * axis.y) - (axis.x * sin_t);
    result.e[2][2] = (cos_inv * axis.z * axis.z) + cos_t;

    return result; 
}

internal v4
m4_mul_v4(m4 m, v4 v) {
    v4 result;
    result.e[0] = v.e[0] * m.e[0][0] + v.e[1] * m.e[0][1] + v.e[2] * m.e[0][2] + v.e[3] * m.e[0][3];
    result.e[1] = v.e[0] * m.e[1][0] + v.e[1] * m.e[1][1] + v.e[2] * m.e[1][2] + v.e[3] * m.e[1][3];
    result.e[2] = v.e[0] * m.e[2][0] + v.e[1] * m.e[2][1] + v.e[2] * m.e[2][2] + v.e[3] * m.e[2][3];
    result.e[3] = v.e[0] * m.e[3][0] + v.e[1] * m.e[3][1] + v.e[2] * m.e[3][2] + v.e[3] * m.e[3][3];
    return result;
}

internal m4
perspective_projection_rh(float fov, float aspect_ratio, float near, float far) {
    m4 result{};
    float c = 1.0f / tanf(deg_to_rad(fov / 2.0f));
    result.e[0][0] = c / aspect_ratio;
    result.e[1][1] = c;
    result.e[2][3] = -1.0f;

    result.e[2][2] = (far) / (near - far);
    result.e[3][2] = (near * far) / (near - far);
    
    return result;
}

internal m4
look_at_rh(v3 eye, v3 target, v3 up) {
    v3 F = v3_normal(v3_subtract(target, eye));
    v3 R = v3_normal(v3_cross(F, up));
    v3 U = v3_cross(R, F);

    m4 result = m4_id();
    result.e[0][0] = R.x;
    result.e[1][0] = R.y;
    result.e[2][0] = R.z;
    result.e[0][1] = U.x;
    result.e[1][1] = U.y;
    result.e[2][1] = U.z;
    result.e[0][2] = -F.x;
    result.e[1][2] = -F.y;
    result.e[2][2] = -F.z;
    result.e[3][0] = -v3_dot(R, eye);
    result.e[3][1] = -v3_dot(U, eye);
    result.e[3][2] =  v3_dot(F, eye);
    return result;
}

internal m4
look_at_lh(v3 eye, v3 target, v3 up) {
    v3 F = v3_normal(v3_subtract(target, eye));
    v3 R = v3_normal(v3_cross(up, F));
    v3 U = v3_cross(F, R);

    m4 result = m4_id();
    result.e[0][0] = R.x;
    result.e[1][0] = R.y;
    result.e[2][0] = R.z;
    result.e[0][1] = U.x;
    result.e[1][1] = U.y;
    result.e[2][1] = U.z;
    result.e[0][2] = F.x;
    result.e[1][2] = F.y;
    result.e[2][2] = F.z;
    result.e[3][0] = -v3_dot(R, eye);
    result.e[3][1] = -v3_dot(U, eye);
    result.e[3][2] = -v3_dot(F, eye);
    return result;
}


#endif // SIMPLE_MATH_H
