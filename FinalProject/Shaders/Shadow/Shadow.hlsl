struct VertexIn {
  float3 PosL : POSITION;
};

struct VertexOut {
  float4 PosH : SV_POSITION;
};

cbuffer cbPerObject : register(b0){ matrix gWorld; };

cbuffer cbPass : register(b1){ matrix gViewProj; };

VertexOut VS(VertexIn vin)
{
  VertexOut vout = (VertexOut)0.0f;
  float4 posW = mul(float4(vin.PosL, 1.0f), gWorld);
  vout.PosH   = mul(posW, gViewProj);

  return vout;
}

void PS(VertexOut pin) {}