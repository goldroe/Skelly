#define MAX_BONES 100

cbuffer constants_Per_Frame {
    float3 eye_pos_w;
};

cbuffer Constants_Per_Obj {
    matrix wvp;
    matrix bone_matrices[MAX_BONES];
};

struct Vertex_In {
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

Vertex_Out vs_main(Vertex_In vin) {
    Vertex_Out vout;
    vout.position = mul(wvp, float4(vin.position, 1.0));
    vout.normal = vin.normal;
    vout.uv = vin.uv;
    return vout;
}

float4 ps_main(Vertex_Out pin) : SV_TARGET {
    return texture_diffuse.Sample(model_sampler, pin.uv);
}
