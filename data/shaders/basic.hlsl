cbuffer Basic_Constants {
    matrix mvp;
    float4 color;
};

struct Vertex_In {
    float3 position : POSITION;
    float3 normal : NORMAL;
    float2 uv : TEXCOORD;
};

struct Vertex_Out {
    float4 position : SV_POSITION;
    float3 normal : NORMAL;
    float2 uv : TEXCOORD;
};

SamplerState basic_sampler;
Texture2D texture_diffuse;

Vertex_Out vs_main(Vertex_In vin) {
    Vertex_Out vout;
    vout.position = mul(mvp, float4(vin.position, 1));
    vout.normal = vin.normal;
    vout.uv = vin.uv;
    return vout;
}

float4 ps_main(Vertex_Out pin) : SV_TARGET {
    return color * texture_diffuse.Sample(model_sampler, pin.uv);
}
