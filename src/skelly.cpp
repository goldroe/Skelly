#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif // _MSC_VER

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <timeapi.h>
#include <windowsx.h>
#include <fileapi.h>
#include <d3d11.h>
#include <d3dcompiler.h>
#undef near
#undef far
#pragma comment(lib, "d3dcompiler.lib")
#pragma comment(lib, "d3d11.lib")
#endif // _WIN32

#include <stdio.h>
#include <assert.h>

#include <stdint.h>
typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
typedef int8_t  s8;
typedef int16_t s16;
typedef int32_t s32;
typedef int64_t s64;
typedef s8  b8;
typedef s32 b32;
typedef float f32;
typedef double f64;
#define internal static
#define global static
#define local_persist static

#include <string>
#include <vector>

#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

#define STB_IMAGE_IMPLEMENTATION
#pragma warning(push)
#pragma warning(disable : 4244)
#include <stb_image.h>
#pragma warning(pop)

#define HANDMADE_MATH_USE_DEGREES
#include "HandmadeMath.h"

#include "skelly.h"

#include "input.cpp"

#define CLAMP(V, Min, Max) ((V) < (Min) ? (Min) : (V) > (Max) ? (Max) : (V))

#define WIDTH 1600
#define HEIGHT 900

global bool window_should_close;

global IDXGISwapChain *swapchain;
global ID3D11Device *d3d_device;
global ID3D11DeviceContext *d3d_context;
global ID3D11RenderTargetView *render_target;

global LARGE_INTEGER global_clock_start;
global LARGE_INTEGER performance_frequency;


global Assimp::Importer assimp_importer;

internal void
win32_set_start_clock(LARGE_INTEGER start) {
    global_clock_start = start;
}

internal f32
win32_get_seconds_elapsed(LARGE_INTEGER start, LARGE_INTEGER end) {
    float result = (float)(end.QuadPart - start.QuadPart) / (float)performance_frequency.QuadPart;
    return result;
}

internal LARGE_INTEGER
win32_get_wall_clock() {
    LARGE_INTEGER result;
    QueryPerformanceCounter(&result);
    return result;
}

internal f32
win32_get_current_time() {
    LARGE_INTEGER now = win32_get_wall_clock();
    f32 result = win32_get_seconds_elapsed(global_clock_start, now);
    return result;
}

internal void
win32_get_window_dim(HWND window, int *w, int *h) {
    RECT rc;
    GetClientRect(window, &rc);
    int width = rc.right - rc.left;
    int height = rc.bottom - rc.top;
    if (w) *w = width;
    if (h) *h = height;
}

internal void
os_popup(char *str) {
    MessageBoxA(NULL, str, NULL, MB_OK | MB_ICONERROR | MB_ICONEXCLAMATION);
}

internal void
os_popupf(char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    int len = vsnprintf(nullptr, 0, fmt, args);
    char *buffer = new char[len + 1];
    vsnprintf(buffer, len, fmt, args);
    buffer[len] = 0;
    va_end(args);
    os_popup(buffer);
    free(buffer);
}

LRESULT CALLBACK
window_proc(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam) {
    LRESULT result = 0;
    switch (Msg) {
    case WM_ACTIVATE: {
        bool active = wParam > 0;
        if (active) {
            _input.capture_cursor = true;
        } else {
            _input.capture_cursor = false;
        }
        break;
    }
       
    case WM_MOUSEMOVE:
    {
        int px = GET_X_LPARAM(lParam);
        int py = GET_Y_LPARAM(lParam);
        if (_input.capture_cursor) {
            _input.last_cursor_px = _input.cursor_px;
            _input.last_cursor_py = _input.cursor_py;
            _input.cursor_px = px;
            _input.cursor_py = py;
            _input.delta_x = _input.cursor_px - _input.last_cursor_px;
            _input.delta_y = _input.cursor_py - _input.last_cursor_py;
        }
        break;
    }

    case WM_KEYDOWN:
    case WM_KEYUP:
    {
        int vk_code = (int)wParam;
        bool key_down = (((u32)lParam >> 31) == 0);
        bool key_down_previous = (((u32)lParam >> 30) == 1);
        if (vk_code > (int)Key_Begin && vk_code < (int)Key_Last) {
            _keys_pressed[vk_code] = key_down_previous;
            _keys_down[vk_code] = key_down;
        }
        break;
    }
    
    case WM_SIZE:
    {
        // NOTE: Resize render target view
        if (swapchain) {
            d3d_context->OMSetRenderTargets(0, 0, 0);

            // Release all outstanding references to the swap chain's buffers.
            render_target->Release();

            // Preserve the existing buffer count and format.
            // Automatically choose the width and height to match the client rect for HWNDs.
            HRESULT hr = swapchain->ResizeBuffers(0, 0, 0, DXGI_FORMAT_UNKNOWN, 0);
                                            
            // Perform error handling here!

            // Get buffer and create a render-target-view.
            ID3D11Texture2D* buffer;
            hr = swapchain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**) &buffer);
            // Perform error handling here!

            hr = d3d_device->CreateRenderTargetView(buffer, NULL, &render_target);
            // Perform error handling here!
            buffer->Release();

            d3d_context->OMSetRenderTargets(1, &render_target, NULL);
        }
        break;
    }

    case WM_CLOSE:
        PostQuitMessage(0);
        break;
    default:
        result = DefWindowProcA(hWnd, Msg, wParam, lParam);
    }
    return result;
}

internal std::string
read_file(std::string file_name) {
    std::string result{};
    HANDLE file_handle = CreateFileA((LPCSTR)file_name.data(), GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (file_handle != INVALID_HANDLE_VALUE) {
        u64 bytes_to_read;
        if (GetFileSizeEx(file_handle, (PLARGE_INTEGER)&bytes_to_read)) {
            assert(bytes_to_read <= UINT32_MAX);
            char *buffer = new char[bytes_to_read + 1];
            buffer[bytes_to_read] = 0;
            DWORD bytes_read;
            if (ReadFile(file_handle, buffer, (DWORD)bytes_to_read, &bytes_read, NULL) && (DWORD)bytes_to_read ==  bytes_read) {
                result = std::string(buffer);
            } else {
                // TODO: error handling
                printf("ReadFile: error reading file, %s!\n", file_name.data());
            }
       } else {
            // TODO: error handling
            printf("GetFileSize: error getting size of file: %s!\n", file_name.data());
       }
       CloseHandle(file_handle);
    } else {
        // TODO: error handling
        printf("CreateFile: error opening file: %s!\n", file_name.data());
    }
    return result;
}

internal void
upload_constants(ID3D11Buffer *constant_buffer, void *constants, s32 constants_size) {
    D3D11_MAPPED_SUBRESOURCE res{};
    if (d3d_context->Map(constant_buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &res) == S_OK) {
        memcpy(res.pData, constants, constants_size);
        d3d_context->Unmap(constant_buffer, 0);
    } else {
        fprintf(stderr, "Failed to map constant buffer\n");
    }
}

internal ID3D11Buffer *
create_constant_buffer(unsigned int size) {
    ID3D11Buffer *constant_buffer = nullptr;
    D3D11_BUFFER_DESC desc{};
    desc.ByteWidth = size;
    desc.Usage = D3D11_USAGE_DYNAMIC;
    desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    if (d3d_device->CreateBuffer(&desc, nullptr, &constant_buffer) != S_OK) {
        fprintf(stderr, "Failed to create constant buffer\n");
    }
    return constant_buffer;
}

internal Shader_Program
create_shader_program(std::string source_name, std::string vs_entry, std::string ps_entry, D3D11_INPUT_ELEMENT_DESC *items, int item_count) {
    std::string source = read_file(source_name);
    Shader_Program program{};
    ID3DBlob *vs_blob, *ps_blob, *error_blob;
    UINT flags = 0;
    #if _DEBUG
    flags |= D3DCOMPILE_DEBUG;
    #endif
    if (D3DCompile(source.data(), source.length(), source_name.data(), NULL, D3D_COMPILE_STANDARD_FILE_INCLUDE, vs_entry.data(), "vs_5_0", flags, 0, &vs_blob, &error_blob) != S_OK) {
        fprintf(stderr, "Failed to compile Vertex Shader \"%s\"\n%s", source_name.data(), (char *)error_blob->GetBufferPointer());
        return program;
    }
    if (D3DCompile(source.data(), source.length(), source_name.data(), NULL, D3D_COMPILE_STANDARD_FILE_INCLUDE, ps_entry.data(), "ps_5_0", flags, 0, &ps_blob, &error_blob) != S_OK) {
        fprintf(stderr, "Failed to compile Pixel Shader \"%s\"\n%s", source_name.data(), (char *)error_blob->GetBufferPointer());
        return program;
    }

    if (d3d_device->CreateVertexShader(vs_blob->GetBufferPointer(), vs_blob->GetBufferSize(), NULL, &program.vertex_shader) != S_OK) {
        fprintf(stderr, "Failed to compile Vertex Shader\n");
    }
    if (d3d_device->CreatePixelShader(ps_blob->GetBufferPointer(), ps_blob->GetBufferSize(), NULL, &program.pixel_shader) != S_OK) {
        fprintf(stderr, "Failed to create Pixel Shader\n");
    }

    ID3D11InputLayout *input_layout = nullptr;
    if (d3d_device->CreateInputLayout(items, item_count, vs_blob->GetBufferPointer(), vs_blob->GetBufferSize(), &program.input_layout) != S_OK) {
        fprintf(stderr, "Failed to create Input Layout\n");
    }
    return program;
}

internal bool
load_texture(std::string file_name, Texture *texture) {
    bool success = true;
    ID3D11ShaderResourceView *srv = nullptr;
    int width, height, nc;
    u8 *image_data = stbi_load(file_name.data(), &width, &height, &nc, 4);
    if (!image_data) {
        success = false;
    }
    ID3D11Texture2D *tex_2d = nullptr;
    D3D11_TEXTURE2D_DESC desc{};
    desc.Width = width;
    desc.Height = height;
    desc.MipLevels = 1;
    desc.ArraySize = 1;
    desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    desc.SampleDesc.Count = 1;
    desc.SampleDesc.Quality = 0;
    desc.Usage = D3D11_USAGE_IMMUTABLE;
    desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    D3D11_SUBRESOURCE_DATA data{};
    data.pSysMem = image_data;
    data.SysMemPitch = width * sizeof(u32);
    data.SysMemSlicePitch = 0;
    if (d3d_device->CreateTexture2D(&desc, &data, &tex_2d) == S_OK) {
        if (d3d_device->CreateShaderResourceView(tex_2d, NULL, &srv) == S_OK) {
        } else {
            success = false;
        }
    } else {
        success = false;
    }
        
    if (image_data) {
        stbi_image_free(image_data);
    }
    if (!success) {
        printf("Error creating texture '%s'\n", file_name.data());
    }

    if (texture) {
        texture->view = srv;
        texture->texture_2d = tex_2d;
    }
    return success;
}

internal std::string
get_parent_path(std::string file_name) {
    std::string parent = file_name.substr(0, file_name.find_last_of("/\\"));
    return parent;
}

internal Basic_Model
load_basic_model(std::string file_name) {
    u32 import_flags = aiProcess_Triangulate | aiProcess_FlipUVs;
    const aiScene *scene = assimp_importer.ReadFile(file_name.data(), import_flags);
    if (!scene) {
        fprintf(stderr, "Failed to load mesh %s: %s\n", file_name.data(), assimp_importer.GetErrorString());
    }
    std::string model_path = get_parent_path(file_name);

    Basic_Model model{};
    model.meshes.reserve(scene->mNumMeshes);
    for (u32 mesh_index = 0; mesh_index < scene->mNumMeshes; mesh_index++) {
        aiMesh *ai_mesh = scene->mMeshes[mesh_index];
        Basic_Mesh mesh{};
        mesh.vertices.reserve(ai_mesh->mNumVertices);
        for (u32 vert_index = 0; vert_index < ai_mesh->mNumVertices; vert_index++) {
            aiVector3D p = ai_mesh->mVertices[vert_index];
            aiVector3D n = ai_mesh->mNormals[vert_index];
            aiVector3D t = {};
            if (ai_mesh->HasTextureCoords(0)) {
                t = ai_mesh->mTextureCoords[0][vert_index];
            }
            Basic_Vertex v;
            v.position = HMM_V3(p.x, p.y, p.z);
            v.normal = HMM_V3(n.x, n.y, n.z);
            v.uv = HMM_V2(t.x, t.y);
            mesh.vertices.push_back(v);
        }

        for (u32 i = 0; i < ai_mesh->mNumFaces; i++) {
            aiFace *face = ai_mesh->mFaces + i;
            assert(face->mNumIndices <= 3);
            mesh.indices.push_back(face->mIndices[0]);
            mesh.indices.push_back(face->mIndices[1]);
            mesh.indices.push_back(face->mIndices[2]);
        }

        aiMaterial *material = scene->mMaterials[ai_mesh->mMaterialIndex];
        if (material->GetTextureCount(aiTextureType_DIFFUSE) > 0) {
            aiString name;
            if (material->GetTexture(aiTextureType_DIFFUSE, 0, &name) == AI_SUCCESS) {
                std::string material_file = model_path + "/" + name.data;
                printf("material:%s\n", material_file.data());
                if (!load_texture(material_file, &mesh.texture)) {
                    printf("Error loading texture:%s\n", material_file.data());
                }
            }
        }

        // NOTE: Create vertex buffer
        {
            D3D11_BUFFER_DESC desc{};
            desc.ByteWidth = sizeof(Basic_Vertex) * (int)mesh.vertices.size();
            desc.Usage = D3D11_USAGE_DEFAULT;
            desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
            D3D11_SUBRESOURCE_DATA data{};
            data.pSysMem = mesh.vertices.data();
            if (d3d_device->CreateBuffer(&desc, &data, &mesh.vertex_buffer) != S_OK) {
                fprintf(stderr, "Failed to create vertex buffer\n");
            }
        }

        // NOTE: Create index buffer
        {
            D3D11_BUFFER_DESC desc{};
            desc.ByteWidth = sizeof(u32) * (int)mesh.indices.size();
            desc.Usage = D3D11_USAGE_DEFAULT;
            desc.BindFlags = D3D11_BIND_INDEX_BUFFER;
            D3D11_SUBRESOURCE_DATA data{};
            data.pSysMem = mesh.indices.data();
            if (d3d_device->CreateBuffer(&desc, &data, &mesh.index_buffer) != S_OK) {
                fprintf(stderr, "Failed to create index buffer\n");
            }
        }

        model.meshes.push_back(mesh);
    }
    return model; 
}


internal void
set_vertex_bone_data(Skinned_Vertex *vertex, s32 bone_id, f32 bone_weight) {
    for (int i = 0; i < MAX_BONE_WEIGHTS; i++) {
        if (vertex->bone_ids[i] == -1) {
            vertex->bone_ids[i] = bone_id;
            vertex->bone_weights[i] = bone_weight;
            break;
        }
    }
}

internal HMM_Mat4
convert_assimp_matrix(aiMatrix4x4 m) {
    HMM_Mat4 M;
    M.Columns[0] = HMM_V4(m.a1, m.b1, m.c1, m.d1);
    M.Columns[1] = HMM_V4(m.a2, m.b2, m.c2, m.d2);
    M.Columns[2] = HMM_V4(m.a3, m.b3, m.c3, m.d3);
    M.Columns[3] = HMM_V4(m.a4, m.b4, m.c4, m.d4);
    return M;
}

internal HMM_Vec3
convert_assimp_v3(aiVector3D v) {
    HMM_Vec3 V;
    V.X = v.x;
    V.Y = v.y;
    V.Z = v.z;
    return V;
}

internal HMM_Quat
convert_assimp_quat(aiQuaternion q) {
    HMM_Quat Q;
    Q.X = (float)q.x;
    Q.Y = (float)q.y;
    Q.Z = (float)q.z;
    Q.W = (float)q.w;
    return Q;
}

internal Bone
bone_create(std::string name, int id, aiNodeAnim *channel) {
    Bone bone;
    bone.name = name;
    bone.id = id;
    bone.local_transform = HMM_M4D(1.0f);

    for (u32 key_index = 0; key_index < channel->mNumPositionKeys; key_index++) {
        Key_Vector key;
        key.time = (f32)channel->mPositionKeys[key_index].mTime;
        key.value = convert_assimp_v3(channel->mPositionKeys[key_index].mValue);
        bone.translation_keys.push_back(key);
    }
    bone.translation_count = channel->mNumPositionKeys;
    
    for (u32 key_index = 0; key_index < channel->mNumScalingKeys; key_index++) {
        Key_Vector key;
        key.time = (f32)channel->mScalingKeys[key_index].mTime;
        key.value = convert_assimp_v3(channel->mScalingKeys[key_index].mValue);
        bone.scale_keys.push_back(key);
    }
    bone.scale_count = channel->mNumScalingKeys;

    for (u32 key_index = 0; key_index < channel->mNumRotationKeys; key_index++) {
        Key_Quat key;
        key.time = (f32)channel->mRotationKeys[key_index].mTime;
        key.value = convert_assimp_quat(channel->mRotationKeys[key_index].mValue);
        bone.rotation_keys.push_back(key);
    }
    bone.rotation_count = channel->mNumRotationKeys;

    return bone;
}


internal void
read_animation_node(Animation_Node *anim_node, aiNode *node) {
    anim_node->name = node->mName.data;
    anim_node->transform = convert_assimp_matrix(node->mTransformation);
    for (u32 i = 0; i < node->mNumChildren; i++) {
        Animation_Node child;
        read_animation_node(&child, node->mChildren[i]);
        anim_node->children.push_back(child);
    }
    anim_node->children_count = node->mNumChildren;
}

internal Animator
animator_create(Animation *animation) {
    Animator animator;
    animator.time = 0.0f;
    animator.animation = animation;
    animator.final_bone_matrices.reserve(MAX_BONES);
    HMM_Mat4 m = HMM_M4D(1.0f);
    for (int i = 0; i < MAX_BONES; i++) {
        animator.final_bone_matrices.push_back(m);
    }
    return animator;
}

internal Animation
load_animation(std::string file_name, Skinned_Model *model) {
    u32 import_flags = aiProcess_Triangulate | aiProcess_FlipUVs;
    const aiScene *scene = assimp_importer.ReadFile(file_name.data(), import_flags);
    if (!scene) {
        fprintf(stderr, "Failed to load animation %s: %s\n", file_name.data(), assimp_importer.GetErrorString());
    }

    Animation animation{};

    if (!scene->HasAnimations()) {
        fprintf(stderr, "File has no animations %s: %s\n", file_name.data(), assimp_importer.GetErrorString());
        return animation;
    }

    aiAnimation *ai_anim = scene->mAnimations[0];

    animation.name = ai_anim->mName.C_Str();
    animation.duration = (f32)ai_anim->mDuration;
    animation.ticks_per_second = (f32)ai_anim->mTicksPerSecond;

    for (u32 channel_index = 0; channel_index < ai_anim->mNumChannels; channel_index++) {
        aiNodeAnim *channel = ai_anim->mChannels[channel_index];
        std::string node_name = channel->mNodeName.data;

        if (model->bone_info_map.find(node_name) == model->bone_info_map.end()) {
            Bone_Info info{};
            info.id = model->bone_counter;
            model->bone_info_map.insert({node_name, info});
        }

        Bone bone = bone_create(node_name, model->bone_info_map[node_name].id, channel);
        animation.bones.push_back(bone);
    }

    read_animation_node(&animation.node, scene->mRootNode);

    animation.bone_info_map = model->bone_info_map;
    
    return animation;
}

internal Skinned_Model
load_skinned_model(std::string file_name) {
    Assimp::Importer importer;
    u32 import_flags = aiProcess_Triangulate | aiProcess_FlipUVs;
    const aiScene *scene = importer.ReadFile(file_name.data(), import_flags);
    if (!scene) {
        fprintf(stderr, "Failed to load mesh %s: %s\n", file_name.data(), importer.GetErrorString());
    }
    std::string model_path = get_parent_path(file_name);

    Skinned_Model model{};
    model.meshes.reserve(scene->mNumMeshes);
    for (u32 mesh_index = 0; mesh_index < scene->mNumMeshes; mesh_index++) {
        aiMesh *ai_mesh = scene->mMeshes[mesh_index];
        Skinned_Mesh mesh{};
        mesh.vertices.reserve(ai_mesh->mNumVertices);
        for (u32 vert_index = 0; vert_index < ai_mesh->mNumVertices; vert_index++) {
            aiVector3D p = ai_mesh->mVertices[vert_index];
            aiVector3D n = ai_mesh->mNormals[vert_index];
            aiVector3D t = {};
            if (ai_mesh->HasTextureCoords(0)) {
                t = ai_mesh->mTextureCoords[0][vert_index];
            }
            Skinned_Vertex v;
            v.position = HMM_V3(p.x, p.y, p.z);
            v.normal = HMM_V3(n.x, n.y, n.z);
            v.uv = HMM_V2(t.x, t.y);
            v.bone_weights[0] = v.bone_weights[1] = v.bone_weights[2] = v.bone_weights[3] = 0;
            v.bone_ids[0] = v.bone_ids[1] = v.bone_ids[2] = v.bone_ids[3] = -1;
            mesh.vertices.push_back(v);
        }

        // NOTE: Load bone data
        for (u32 bone_index = 0; bone_index < ai_mesh->mNumBones; bone_index++) {
            aiBone *bone = ai_mesh->mBones[bone_index];
            std::string bone_name = bone->mName.C_Str();
            int bone_id = -1;
            if (model.bone_info_map.find(bone_name) != model.bone_info_map.end()) {
                bone_id = model.bone_info_map[bone_name].id;
            } else {
                Bone_Info bone_info;
                bone_id = bone_info.id = model.bone_counter;
                bone_info.offset_matrix = convert_assimp_matrix(bone->mOffsetMatrix);
                model.bone_info_map.insert({bone_name, bone_info});
                model.bone_counter++;
            }

            assert(bone_id != -1);
            for (u32 weight_index = 0; weight_index < bone->mNumWeights; weight_index++) {
                aiVertexWeight ai_weight = bone->mWeights[weight_index];
                Skinned_Vertex *vertex = &mesh.vertices[ai_weight.mVertexId];
                set_vertex_bone_data(vertex, bone_id, ai_weight.mWeight);
            }
        }

        for (u32 i = 0; i < ai_mesh->mNumFaces; i++) {
            aiFace *face = ai_mesh->mFaces + i;
            assert(face->mNumIndices <= 3);
            mesh.indices.push_back(face->mIndices[0]);
            mesh.indices.push_back(face->mIndices[1]);
            mesh.indices.push_back(face->mIndices[2]);
        }

        aiMaterial *material = scene->mMaterials[ai_mesh->mMaterialIndex];
        if (material->GetTextureCount(aiTextureType_DIFFUSE) > 0) {
            aiString name;
            if (material->GetTexture(aiTextureType_DIFFUSE, 0, &name) == AI_SUCCESS) {
                std::string material_file = model_path + "/" + name.data;
                printf("material:%s\n", material_file.data());
                if (!load_texture(material_file, &mesh.texture)) {
                    printf("Error loading texture:%s\n", material_file.data());
                }
            }
        }

        // NOTE: Create vertex buffer
        {
            D3D11_BUFFER_DESC desc{};
            desc.ByteWidth = sizeof(Skinned_Vertex) * (int)mesh.vertices.size();
            desc.Usage = D3D11_USAGE_DEFAULT;
            desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
            D3D11_SUBRESOURCE_DATA data{};
            data.pSysMem = mesh.vertices.data();
            if (d3d_device->CreateBuffer(&desc, &data, &mesh.vertex_buffer) != S_OK) {
                fprintf(stderr, "Failed to create vertex buffer\n");
            }
        }

        // NOTE: Create index buffer
        {
            D3D11_BUFFER_DESC desc{};
            desc.ByteWidth = sizeof(u32) * (int)mesh.indices.size();
            desc.Usage = D3D11_USAGE_DEFAULT;
            desc.BindFlags = D3D11_BIND_INDEX_BUFFER;
            D3D11_SUBRESOURCE_DATA data{};
            data.pSysMem = mesh.indices.data();
            if (d3d_device->CreateBuffer(&desc, &data, &mesh.index_buffer) != S_OK) {
                fprintf(stderr, "Failed to create index buffer\n");
            }
        }

        model.meshes.push_back(mesh);
    }
    return model; 
}

internal f32
compute_bone_lerp_factor(f32 time, f32 last_time, f32 next_time) {
    f32 result = (time - last_time) / (next_time - last_time);
    return result;
}

internal int
bone_get_translation_key(Bone *bone, f32 time) {
    for (int key_index = 0; key_index < bone->translation_count - 1; key_index++) {
        if (bone->translation_keys[key_index].time <= time && time <= bone->translation_keys[key_index + 1].time) {
            return key_index;
        }
    }
    return 0;
}

internal int
bone_get_scale_key(Bone *bone, f32 time) {
    for (int key_index = 0; key_index < bone->scale_count - 1; key_index++) {
        if (bone->scale_keys[key_index].time <= time && time <= bone->scale_keys[key_index + 1].time) {
            return key_index;
        }
    }
    return 0;
}

internal int
bone_get_rotation_key(Bone *bone, f32 time) {
    for (int key_index = 0; key_index < bone->rotation_count - 1; key_index++) {
        if (bone->rotation_keys[key_index].time <= time && time <= bone->rotation_keys[key_index + 1].time) {
            return key_index;
        }
    }
    return 0;
}

internal HMM_Mat4
bone_lerp_translation(Bone *bone, f32 time) {
    int key = bone_get_translation_key(bone, time);
    Key_Vector last_key = bone->translation_keys[key];
    Key_Vector next_key = bone->translation_keys[key + 1];

    f32 lerp_factor = compute_bone_lerp_factor(time, last_key.time, next_key.time);
    HMM_Vec3 lerp_vector = HMM_LerpV3(last_key.value, lerp_factor, next_key.value);
    HMM_Mat4 T = HMM_Translate(lerp_vector);
    return T;
}

internal HMM_Mat4
bone_lerp_scale(Bone *bone, f32 time) {
    int key = bone_get_scale_key(bone, time);
    Key_Vector last_key = bone->scale_keys[key];
    Key_Vector next_key = bone->scale_keys[key + 1];

    f32 lerp_factor = compute_bone_lerp_factor(time, last_key.time, next_key.time);
    HMM_Vec3 lerp_vector = HMM_LerpV3(last_key.value, lerp_factor, next_key.value);
    HMM_Mat4 S = HMM_Scale(lerp_vector);
    return S;
}

internal HMM_Mat4
bone_lerp_rotation(Bone *bone, f32 time) {
    int key = bone_get_rotation_key(bone, time);
    Key_Quat last_key = bone->rotation_keys[key];
    Key_Quat next_key = bone->rotation_keys[key + 1];

    f32 slerp_factor = compute_bone_lerp_factor(time, last_key.time, next_key.time);
    HMM_Quat slerp_quat = HMM_SLerp(last_key.value, slerp_factor, next_key.value);
    HMM_Mat4 R = HMM_QToM4(slerp_quat);
    return R;
}

internal void
bone_update(Bone *bone, f32 time) {
    HMM_Mat4 translation = bone_lerp_translation(bone, time);
    HMM_Mat4 scale = bone_lerp_scale(bone, time);
    HMM_Mat4 rotation = bone_lerp_rotation(bone, time);
    bone->local_transform = translation * scale * rotation;
}

internal Bone *
find_bone(Animation *animation, std::string name) {
    for (int i = 0; i < animation->bones.size(); i++) {
        if (animation->bones[i].name == name) {
            return &animation->bones[i];
        }
    }
    return nullptr;
}

internal void
compute_bone_transform(Animator *animator, Animation_Node *node, HMM_Mat4 parent_transform) {
    HMM_Mat4 node_transform = node->transform;
    
    Bone *bone = find_bone(animator->animation, node->name);
    if (bone) {
        bone_update(bone, animator->time);
        node_transform = bone->local_transform;
    }

    HMM_Mat4 global_transform = parent_transform * node_transform;

    Animation *animation = animator->animation;
    if (animation->bone_info_map.find(node->name) != animation->bone_info_map.end()) {
        Bone_Info bone_info = animation->bone_info_map[node->name];
        animator->final_bone_matrices[bone_info.id] = global_transform * bone_info.offset_matrix;
    }

    for (int i = 0; i < node->children_count; i++) {
        compute_bone_transform(animator, &node->children[i], global_transform);
    }
}

internal void
animation_update(Animator *animator, f32 dt) {
    Animation *animation = animator->animation;
    animator->time += animation->ticks_per_second * dt;
    animator->time = fmod(animator->time, animation->duration);
    compute_bone_transform(animator, &animation->node, HMM_M4D(1.0f));
}

int main() {
    QueryPerformanceFrequency(&performance_frequency);
    UINT desired_scheduler_ms = 1;
    timeBeginPeriod(desired_scheduler_ms);

    HRESULT hr = 0;
#define CLASSNAME "skelly_hwnd_class"
    HINSTANCE hinstance = GetModuleHandle(NULL);
    WNDCLASSA window_class{};
    window_class.style = CS_HREDRAW | CS_VREDRAW;
    window_class.lpfnWndProc = window_proc;
    window_class.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    window_class.lpszClassName = CLASSNAME;
    window_class.hInstance = hinstance;
    window_class.hCursor = LoadCursorA(NULL, IDC_ARROW);
    if (!RegisterClassA(&window_class)) {
        os_popupf("RegisterClassA failed, err:%d\n", GetLastError());
    }

    HWND window = 0;
    {
        RECT rc = {0, 0, WIDTH, HEIGHT};
        if (AdjustWindowRect(&rc, WS_OVERLAPPEDWINDOW, FALSE)) {
            window = CreateWindowA(CLASSNAME, "Skelly", WS_OVERLAPPEDWINDOW | WS_VISIBLE, CW_USEDEFAULT, CW_USEDEFAULT, rc.right - rc.left, rc.bottom - rc.top, NULL, NULL, hinstance, NULL);
        } else {
            fprintf(stderr, "AdjustWindowRect failed, err:%d\n", GetLastError());
        }
        if (!window) {
            os_popupf("CreateWindowA failed, err:%d\n", GetLastError());
        }
    }

    // NOTE: initialize cursor
    _input.capture_cursor = true;
    ShowCursor(FALSE);
    {
        POINT pt;
        GetCursorPos(&pt);
        ScreenToClient(window, &pt);
        _input.cursor_px = _input.last_cursor_px = pt.x;
        _input.cursor_py = _input.last_cursor_py = pt.y;
    }

    // INITIALIZE SWAP CHAIN
    DXGI_SWAP_CHAIN_DESC swapchain_desc{};
    {
        DXGI_MODE_DESC buffer_desc{};
        buffer_desc.Width = WIDTH;
        buffer_desc.Height = HEIGHT;
        buffer_desc.RefreshRate.Numerator = 75;
        buffer_desc.RefreshRate.Denominator = 1;
        buffer_desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        buffer_desc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
        buffer_desc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;

        swapchain_desc.BufferDesc = buffer_desc;
        swapchain_desc.SampleDesc.Count = 1;
        swapchain_desc.SampleDesc.Quality = 0;
        swapchain_desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        swapchain_desc.BufferCount = 1;
        swapchain_desc.OutputWindow = window;
        swapchain_desc.Windowed = TRUE;
        swapchain_desc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
    }

    hr = D3D11CreateDeviceAndSwapChain(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, D3D11_CREATE_DEVICE_DEBUG, NULL, NULL, D3D11_SDK_VERSION, &swapchain_desc, &swapchain, &d3d_device, NULL, &d3d_context);

    ID3D11Texture2D *backbuffer;
    hr = swapchain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void **)&backbuffer);
    
    hr = d3d_device->CreateRenderTargetView(backbuffer, NULL, &render_target);
    backbuffer->Release();

    ID3D11DepthStencilView *depth_stencil_view = nullptr;

    // DEPTH STENCIL BUFFER
    {
        D3D11_TEXTURE2D_DESC desc{};
        desc.Width = WIDTH;
        desc.Height = HEIGHT;
        desc.MipLevels = 1;
        desc.ArraySize = 1;
        desc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
        desc.SampleDesc.Count = 1;
        desc.SampleDesc.Quality = 0;
        desc.Usage = D3D11_USAGE_DEFAULT;
        desc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
        desc.CPUAccessFlags = 0;
        desc.MiscFlags = 0;

        ID3D11Texture2D *depth_stencil_buffer = nullptr;
        hr = d3d_device->CreateTexture2D(&desc, NULL, &depth_stencil_buffer);
        hr = d3d_device->CreateDepthStencilView(depth_stencil_buffer, NULL, &depth_stencil_view);
    }

    // RASTERIZER STATE
    ID3D11RasterizerState *rasterizer_state = nullptr;
    {
        D3D11_RASTERIZER_DESC desc{};
        desc.FillMode = D3D11_FILL_SOLID;
        desc.CullMode = D3D11_CULL_NONE;
        desc.FrontCounterClockwise = true;
        desc.ScissorEnable = false;
        desc.DepthClipEnable = false;
        d3d_device->CreateRasterizerState(&desc, &rasterizer_state);
    }

    // DEPTH-STENCIL STATE
    ID3D11DepthStencilState *depth_stencil_state = nullptr;
    {
        D3D11_DEPTH_STENCIL_DESC desc{};
        desc.DepthEnable = true;
        desc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
        desc.DepthFunc = D3D11_COMPARISON_LESS;
        desc.StencilEnable = false;
        desc.FrontFace.StencilFailOp = desc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
        desc.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
        desc.BackFace = desc.FrontFace;
        d3d_device->CreateDepthStencilState(&desc, &depth_stencil_state);
    }

    // BLEND STATE
    ID3D11BlendState *blend_state = nullptr;
    {
        D3D11_BLEND_DESC desc{};
        desc.AlphaToCoverageEnable = false;
        desc.IndependentBlendEnable = false;
        desc.RenderTarget[0].BlendEnable = true;
        desc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
        desc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
        desc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
        desc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ZERO;
        desc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
        desc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
        desc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
        if (d3d_device->CreateBlendState(&desc, &blend_state) != S_OK) {
            printf("Error CreateBlendState\n");
        }
    }

    ID3D11SamplerState *model_sampler = nullptr;
    {
        D3D11_SAMPLER_DESC desc{};
        desc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
        desc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
        desc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
        desc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
        desc.ComparisonFunc = D3D11_COMPARISON_NEVER;
        desc.MinLOD = 0;
        desc.MaxLOD = D3D11_FLOAT32_MAX;
        if (d3d_device->CreateSamplerState(&desc, &model_sampler) != S_OK) {
            printf("Failed to create sampler\n");
        }
    }

    // Create Basic Shader
    D3D11_INPUT_ELEMENT_DESC basic_input_layout[] = {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, offsetof(Basic_Vertex, position), D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, offsetof(Basic_Vertex, normal), D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, offsetof(Basic_Vertex, uv), D3D11_INPUT_PER_VERTEX_DATA, 0 },
    };
    Shader_Basic shader_basic;
    shader_basic.program = create_shader_program("data/shaders/basic.hlsl", "vs_main", "ps_main", basic_input_layout, ARRAYSIZE(basic_input_layout));
    shader_basic.per_frame = create_constant_buffer(sizeof(Basic_Constants_Per_Frame));
    shader_basic.per_obj = create_constant_buffer(sizeof(Basic_Constants_Per_Obj));

    // Create Skinned Shader
    D3D11_INPUT_ELEMENT_DESC skinned_input_layout[] = {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, offsetof(Skinned_Vertex, position), D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, offsetof(Skinned_Vertex, normal), D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, offsetof(Skinned_Vertex, uv), D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "WEIGHTS", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, offsetof(Skinned_Vertex, bone_weights), D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "BONEINDICES", 0, DXGI_FORMAT_R32G32B32A32_SINT, 0, offsetof(Skinned_Vertex, bone_ids), D3D11_INPUT_PER_VERTEX_DATA, 0 },
    };
    Shader_Skinned shader_skinned;
    shader_skinned.program = create_shader_program("data/shaders/skinned_model.hlsl", "vs_main", "ps_main", skinned_input_layout, ARRAYSIZE(skinned_input_layout));
    shader_skinned.per_frame = create_constant_buffer(sizeof(Skinned_Constants_Per_Frame));
    shader_skinned.per_obj = create_constant_buffer(sizeof(Skinned_Constants_Per_Obj));

    Texture white_texture;
    load_texture("data/white.png", &white_texture);

    Skinned_Model model = load_skinned_model("data/Vampire/dancing_vampire.dae");
    Animation animation = load_animation("data/Vampire/dancing_vampire.dae", &model);
    Animator animator = animator_create(&animation);

    Camera camera{};
    camera.position = HMM_V3(0.0f, 0.0f, 3.0f);
    camera.up = HMM_V3(0.0f, 1.0f, 0.0f);
    camera.right = HMM_V3(1.0f, 0.0f, 0.0f);
    camera.forward = HMM_V3(0.0f, 0.0f, -1.0f);
    // NOTE: point toward -Z axis
    camera.yaw = -90.0f;
    camera.pitch =  0.0f;
    
    model.material.color = HMM_V4(0.8f, 0.5f, 0.3f, 1.0f);
    model.material.ambient = HMM_V4(0.4f, 0.4f, 0.4f, 1.0f);
    model.material.diffuse =  HMM_V4(0.6f, 0.6f, 0.6f, 1.0f);
    model.material.specular = HMM_V4(1.0f, 1.0f, 1.0f, 128.0f);

    Directional_Light directional_light{};
    directional_light.ambient = HMM_V4(0.6f, 0.6f, 0.6f, 1.0f);
    directional_light.diffuse = HMM_V4(0.45f, 0.45f, 0.45f, 1.0f);
    directional_light.specular = HMM_V4(0.6f, 0.6f, 0.6f, 1.0f);
    directional_light.direction = HMM_V3(0.0f, 0.0f, -1.0f);

    f32 animation_time = 0.0f;

    f32 delta_t = 0.0f;
    LARGE_INTEGER last_counter = win32_get_wall_clock();
    win32_set_start_clock(last_counter);

    while (!window_should_close) {
        MSG message{};
        while (PeekMessageA(&message, NULL, 0, 0, PM_REMOVE)) {
            if (message.message == WM_QUIT) {
                window_should_close = true;
            }
            TranslateMessage(&message);
            DispatchMessageA(&message);
        }

        int render_width, render_height;
        win32_get_window_dim(window, &render_width, &render_height);
        HMM_Vec2 render_dim = HMM_V2((float)render_width, (float)render_height);

        HMM_Mat4 projection = HMM_Perspective_RH_ZO(45.0f, render_dim.X/render_dim.Y, 0.1f, 1000.0f);

        // ==============================
        // INPUT
        // ==============================
        if (input_button_down(Key_Escape)) {
            window_should_close = true;
        }

        f32 forward_dt = 0.0f;
        f32 right_dt = 0.0f;

        HMM_Vec2 cursor_delta = get_cursor_delta();

        if (input_button_down(Key_W)) {
            forward_dt += 1.0f;
        }
        if (input_button_down(Key_S)) {
            forward_dt -= 1.0f;
        }

        if (input_button_down(Key_A)) {
            right_dt -= 1.0f;
        }
        if (input_button_down(Key_D)) {
            right_dt += 1.0f;
        }

        if (input_button_down(Key_Q)) {
            camera.position.Y -= 0.1f;
        }
        if (input_button_down(Key_E)) {
            camera.position.Y += 0.1f;
        }

        {
            f32 rotation_dp = 10.0f;
            // Rotate around Y-axis
            camera.yaw   += rotation_dp * cursor_delta.X * delta_t;
            camera.yaw = fmod(camera.yaw, 360.0f);
            camera.pitch -= rotation_dp * cursor_delta.Y * delta_t;
            camera.pitch = CLAMP(camera.pitch, -89.0f, 89.0f);
            
            HMM_Vec3 dir{};
            dir.X = HMM_CosF(camera.yaw) * HMM_CosF(camera.pitch);
            dir.Y = HMM_SinF(camera.pitch); 
            dir.Z = HMM_SinF(camera.yaw) * HMM_CosF(camera.pitch);

            camera.forward = HMM_NormV3(dir);
            camera.right = HMM_NormV3(HMM_Cross(camera.forward, camera.up));
        }

        f32 camera_speed = 1.0f;
        if (input_button_down(Key_Shift)) {
            camera_speed *= 10.0f;
        }
        if (input_button_down(Key_Ctrl)) {
            camera_speed *= 0.1f;
        }

        HMM_Vec3 distance{};
        distance += camera.forward * forward_dt;
        distance += camera.right * right_dt;
        if (HMM_LenV3(distance) > 0.0f) {
            HMM_Vec3 move_dir = HMM_NormV3(distance);
            camera.position += move_dir * camera_speed * delta_t;
        }

        camera.view_matrix = HMM_LookAt_RH(camera.position, camera.position + camera.forward, camera.up);

        // NOTE: Light direction moves circularly
        // f32 t = 45.0f * win32_get_current_time();
        // directional_light.direction = HMM_Norm(HMM_V3(HMM_CosF(t), 0.0f, HMM_SinF(t)));

        directional_light.direction = camera.forward;

        animation_update(&animator, delta_t);

        // RENDER
        float bg_color[4] = {};
        d3d_context->ClearRenderTargetView(render_target, bg_color);
        d3d_context->ClearDepthStencilView(depth_stencil_view, D3D11_CLEAR_DEPTH|D3D11_CLEAR_STENCIL, 1.0f, 0);
        d3d_context->OMSetRenderTargets(1, &render_target, depth_stencil_view);

        D3D11_VIEWPORT viewport{};
        viewport.TopLeftX = 0.0f;
        viewport.TopLeftY = 0.0f;
        viewport.Width = render_dim.X;
        viewport.Height = render_dim.Y;
        viewport.MinDepth = 0.0f;
        viewport.MaxDepth = 1.0f;
        d3d_context->RSSetViewports(1, &viewport);

        d3d_context->RSSetState(rasterizer_state);

        d3d_context->OMSetBlendState(blend_state, NULL, 0xffffffff);
        d3d_context->OMSetDepthStencilState(depth_stencil_state, 0);

        HMM_Mat4 view = camera.view_matrix;
        HMM_Mat4 trans = HMM_Translate(camera.position);
        HMM_Mat4 wvp = projection * view;

        Skinned_Constants_Per_Frame skinned_per_frame{};
        skinned_per_frame.directional_light = directional_light;
        skinned_per_frame.eye_pos_w = camera.position;
        upload_constants(shader_skinned.per_frame, &skinned_per_frame, sizeof(Skinned_Constants_Per_Frame));

        Skinned_Constants_Per_Obj skinned_per_obj{};
        skinned_per_obj.world = HMM_M4D(1.0f);
        skinned_per_obj.world_inv_transpose = HMM_Transpose(HMM_InvGeneral(skinned_per_obj.world));
        skinned_per_obj.wvp = wvp;
        memcpy(skinned_per_obj.bone_matrices, animator.final_bone_matrices.data(), animator.final_bone_matrices.size() * sizeof(HMM_Mat4));
        skinned_per_obj.material = model.material;
        upload_constants(shader_skinned.per_obj, &skinned_per_obj, sizeof(Skinned_Constants_Per_Obj));

        d3d_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        d3d_context->IASetInputLayout(shader_skinned.program.input_layout);

        d3d_context->VSSetConstantBuffers(0, 1, &shader_skinned.per_frame);
        d3d_context->VSSetConstantBuffers(1, 1, &shader_skinned.per_obj);

        d3d_context->PSSetConstantBuffers(0, 1, &shader_skinned.per_frame);
        d3d_context->PSSetConstantBuffers(1, 1, &shader_skinned.per_obj);

        d3d_context->VSSetShader(shader_skinned.program.vertex_shader, nullptr, 0);
        d3d_context->PSSetShader(shader_skinned.program.pixel_shader, nullptr, 0);

        d3d_context->PSSetSamplers(0, 1, &model_sampler);

        UINT stride = sizeof(Skinned_Vertex);
        UINT offset = 0;
        for (int mesh_index = 0; mesh_index < model.meshes.size(); mesh_index++) {
            Skinned_Mesh mesh = model.meshes[mesh_index];
            d3d_context->IASetIndexBuffer(mesh.index_buffer, DXGI_FORMAT_R32_UINT, 0);
            d3d_context->IASetVertexBuffers(0, 1, &mesh.vertex_buffer, &stride, &offset);
            if (mesh.texture.view) {
                d3d_context->PSSetShaderResources(0, 1, &mesh.texture.view);
            }  else {
                d3d_context->PSSetShaderResources(0, 1, &white_texture.view);
            }
            d3d_context->DrawIndexed((int)mesh.indices.size(), 0, 0);
        }

        swapchain->Present(0, 0);

        input_reset_keys_pressed();
        _input.delta_x = 0;
        _input.delta_y = 0;
        if (_input.capture_cursor) {
            int w, h;
            win32_get_window_dim(window, &w, &h);
            int center_x = w / 2;
            int center_y = h / 2;
            _input.cursor_px = _input.last_cursor_px = center_x;
            _input.cursor_py = _input.last_cursor_py = center_y;
            POINT pt = {center_x, center_y};
            ClientToScreen(window, &pt);
            SetCursorPos(pt.x, pt.y);
            // https://github.com/libsdl-org/SDL/blob/38e3c6a4aa338d062ca2eba80728bfdf319f7104/src/video/windows/SDL_windowsmouse.c#L319
            // SetCursorPos(pt.x + 1, pt.y);
            // SetCursorPos(pt.x, pt.y);
        }

        LARGE_INTEGER end_counter = win32_get_wall_clock();
        float seconds_elapsed = win32_get_seconds_elapsed(last_counter, end_counter);
        delta_t = seconds_elapsed;
#if 0
        printf("MS: %f\n", 1000.0f * seconds_elapsed);
#endif
        last_counter = end_counter;
    }

    return 0; 
}
