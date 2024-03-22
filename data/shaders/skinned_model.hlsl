#define MAX_BONES 100

cbuffer Skinned_Constants {
    matrix mvp;
    matrix bone_matrices[MAX_BONES];
    float4 color;
};

struct Skinned_Vertex_In {
    float3 position : POSITION;
    float3 normal : NORMAL;
    float2 uv : TEXCOORD;
    float3 weights : WEIGHTS;
    uint4 bone_indices : BONEINDICES;
};

struct Vertex_Out {
    float4 position : SV_POSITION;
    float3 normal : NORMAL;
    float2 uv : TEXCOORD;
};

SamplerState model_sampler;
Texture2D texture_diffuse;

Vertex_Out vs_main(Skinned_Vertex_In vin) {
    Vertex_Out vout;
    vout.position = mul(mvp, float4(vin.position, 1.0));
    vout.normal = vin.normal;
    vout.uv = vin.uv;
    return vout;
}

float4 ps_main(Vertex_Out pin) {
    return texture_diffuse.Sample(model_sampler, pin.uv);
}
