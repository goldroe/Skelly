#ifndef SKELLY_H
#define SKELLY_H

#include <vector>
#include <map>

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
};

struct Shader_Skinned {
    Shader_Program program;
    ID3D11Buffer *per_frame;
    ID3D11Buffer *per_obj;
};

#define MAX_BONES 100
#define MAX_BONE_WEIGHTS 4

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
    f32 bone_weights[MAX_BONE_WEIGHTS];
    s32 bone_ids[MAX_BONE_WEIGHTS];
};

struct Skinned_Mesh {
    std::vector<Skinned_Vertex> vertices;
    std::vector<u32> indices;
    ID3D11Buffer *vertex_buffer;
    ID3D11Buffer *index_buffer;
    Texture texture;
};

struct Bone_Info {
    int id;
    HMM_Mat4 offset_matrix;
};

struct Key_Vector {
    f32 time;
    HMM_Vec3 value;
};

struct Key_Quat {
    f32 time;
    HMM_Quat value;
};

struct Bone {
    std::string name;
    int id;

    std::vector<Key_Vector> translation_keys;
    std::vector<Key_Vector> scale_keys;
    std::vector<Key_Quat>   rotation_keys;

    int translation_count;
    int scale_count;
    int rotation_count;

    HMM_Mat4 local_transform;
};

struct Skinned_Model {
    std::vector<Skinned_Mesh> meshes;
    Material material;

    std::map<std::string, Bone_Info> bone_info_map;
    int bone_counter;
};

struct Animation_Node {
    std::string name;
    HMM_Mat4 transform;
    int children_count;
    std::vector<Animation_Node> children;
};

struct Animation {
    std::string name;
    f32 duration;
    f32 ticks_per_second;

    std::vector<Bone> bones;
    Animation_Node node;

    std::map<std::string, Bone_Info> bone_info_map;
};

struct Animator {
    f32 time;
    Animation *animation;
    std::vector<HMM_Mat4> final_bone_matrices;
};

#endif SKELLY_H
