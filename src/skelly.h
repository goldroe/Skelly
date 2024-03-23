#ifndef SKELLY_H
#define SKELLY_H

// #include "simple_math.h"

struct Camera {
    HMM_Vec3 position;

    HMM_Vec3 up;
    HMM_Vec3 forward;
    HMM_Vec3 right;

    f32 yaw;
    f32 pitch;

    HMM_Mat4 view_matrix;
};

struct Texture {
    ID3D11Texture2D *texture_2d;
    ID3D11ShaderResourceView *view;
};

struct Shader_Program {
    ID3D11VertexShader *vertex_shader;
    ID3D11PixelShader *pixel_shader;
    ID3D11InputLayout *input_layout;
};

struct Material {
    HMM_Vec4 color;
    HMM_Vec4 ambient;
    HMM_Vec4 diffuse;
    HMM_Vec4 specular;
};

struct Directional_Light {
    HMM_Vec4 ambient;
    HMM_Vec4 diffuse;
    HMM_Vec4 specular;
    HMM_Vec3 direction;
    f32 pad;
};

struct Basic_Constants_Per_Frame {
    Directional_Light directional_light;
    HMM_Vec3 eye_pos_w;
};

struct Basic_Constants_Per_Obj {
    HMM_Mat4 world;
    HMM_Mat4 world_inv_transpose;
    HMM_Mat4 wvp;
    Material material;
};

struct Shader_Basic {
    Shader_Program program;
    ID3D11Buffer *per_frame;
    ID3D11Buffer *per_obj;
};

struct Basic_Vertex {
    HMM_Vec3 position;
    HMM_Vec3 normal;
    HMM_Vec2 uv;
};

struct Basic_Mesh {
    std::vector<Basic_Vertex> vertices;
    std::vector<u32> indices;
    ID3D11Buffer *vertex_buffer;
    ID3D11Buffer *index_buffer;
    Texture texture;
};

struct Basic_Model {
    std::vector<Basic_Mesh> meshes;
    Material material;
    // Texture diffuse_texture;
};


struct Shader_Skinned {
    Shader_Program program;
    ID3D11Buffer *per_frame;
    ID3D11Buffer *per_obj;
};

#define MAX_BONES 100
#define MAX_BONE_INFLUENCE 4

struct Skinned_Constants_Per_Frame {
    Directional_Light directional_light;
    HMM_Vec3 eye_pos_w;
};

struct Skinned_Constants_Per_Obj {
    HMM_Mat4 world;
    HMM_Mat4 world_inv_transpose;
    HMM_Mat4 wvp;
    HMM_Mat4 bone_matrices[MAX_BONES];
    Material material;
};

struct Skinned_Vertex {
    HMM_Vec3 position;
    HMM_Vec3 normal;
    HMM_Vec2 uv;
    f32 bone_weights[MAX_BONES];
    u32 bone_ids[MAX_BONE_INFLUENCE];
};

struct Skinned_Mesh {
    std::vector<Skinned_Vertex> vertices;
    std::vector<u32> indices;
    ID3D11Buffer *vertex_buffer;
    ID3D11Buffer *index_buffer;
    Texture texture;
};

struct Skinned_Model {
    std::vector<Skinned_Mesh> meshes;
    Texture diffuse_texture;
};

enum Keycode {
    Key_Begin = 0,
    
    Key_Enter  = 0x0D,
    Key_Shift  = 0x10,
    Key_Ctrl   = 0x11,
    Key_Alt    = 0x12,

    Key_Escape = 0x1B,

    Key_Left = 0x25,
    Key_Up,
    Key_Right,
    Key_Down,

    Key_0 = 0x30,
    Key_1,
    Key_2,
    Key_3,
    Key_4,
    Key_5,
    Key_6,
    Key_7,
    Key_8,
    Key_9,

    Key_A = 0x41,
    Key_B,
    Key_C,
    Key_D,
    Key_E,
    Key_F,
    Key_G,
    Key_H,
    Key_I,
    Key_J,
    Key_K,
    Key_L,
    Key_M,
    Key_N,
    Key_O,
    Key_P,
    Key_Q,
    Key_R,
    Key_S,
    Key_T,
    Key_U,
    Key_V,
    Key_W,
    Key_X,
    Key_Y,
    Key_Z,

    Key_Last,
};

#endif SKELLY_H
