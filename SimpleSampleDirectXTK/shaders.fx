#define PI 3.14159
#define SCENE_SIZE 50.0f

const float GaussWeight[25] = {
	0.003, 0.013, 0.022, 0.013, 0.003,
	0.013, 0.060, 0.098, 0.060, 0.013,
	0.022, 0.098, 0.162, 0.098, 0.022,
	0.013, 0.060, 0.098, 0.060, 0.013,
	0.003, 0.013, 0.022, 0.013, 0.003,
};
float		boxPos;
float4x4	worldMat;                  
float4x4	mvpMat;    				

Texture2D terrainTex;	
Texture2D waterSampleTex;		
Texture2D waterHeightTex;		
TextureCube skyTex;		

struct VertexPos
{
	float3 PosL : POSITION;
};

struct VertexPosHL
{
	float4 position : SV_POSITION;
	float3 PosL : POSITION;
};

struct BoxVSInput
{
	float3 worldPos	:	POSITION;
	float4 color	:	COLOR;
};

struct BoxVSOutput
{
	float4 position :	SV_POSITION;
	float4 color	:	COLOR;
};

struct VSInput
{
    float2 texCoord		: POSITION;
};

struct QuadVSInput
{
    float2 texCoord		: POSITION;
    float2 Tex		: TEXTURE0;
};

struct PacketVSInput
{
	float4 position	: POSITION;  
	float4 Att	: TEXTURE0;  
	float4 Att2	: TEXTURE1;
};

struct PSInput
{
	float4 position : SV_POSITION;
    float2 Tex		 : TEXTURE0;
};

struct PSPosInput
{
	float4 position : SV_POSITION;
	float2 Tex		 : TEXTURE0;
	float3 worldPos		 : TEXTURE1; 
};

struct PS_PACKET_INPUT
{
	float4 position	 : SV_POSITION;
	float4 worldPos			 : TEXTURE0;
	float4 Att			 : TEXTURE1; 
	float4 Att2			 : TEXTURE2; 
};

struct PS_OUTPUT
{
	float4 color	: SV_Target0;
};

RasterizerState rsWireframe
{
	FillMode = WireFrame;
};

RasterizerState rsSolid
{
	FillMode = Solid;
};

SamplerState PointSampler
{
    Filter = MIN_MAG_MIP_POINT;
    AddressU = Clamp;
    AddressV = Clamp;
    AddressW = Clamp;
};

SamplerState LinearSampler
{
    Filter = MIN_MAG_MIP_LINEAR;
    AddressU = Clamp;
    AddressV = Clamp;
    AddressW = Clamp;
};

VertexPosHL SkyVS(VertexPos vIn)
{
	VertexPosHL vOut;
	float4 posH = mul(float4(vIn.PosL, 1.0f), mvpMat);
	vOut.position = posH.xyww;
	vOut.PosL = vIn.PosL;
	return vOut;
}

BoxVSOutput CubeVS(BoxVSInput In)
{
	BoxVSOutput Out;
	Out.position = mul(float4(In.worldPos.x,In.worldPos.y+ boxPos,In.worldPos.z, 1.0f), mvpMat);
	Out.color = In.color;                  
	return Out;
}

PacketVSInput PacketVS(PacketVSInput In)
{
    return In;
}

PSPosInput WaveMeshVS(VSInput In )
{
	PSPosInput Out;

	Out.worldPos = 0.5*SCENE_SIZE*float3(In.texCoord.x, 0, In.texCoord.y);
	Out.position = mul( float4(Out.worldPos,1.0), mvpMat);
	Out.Tex = In.texCoord;

	return Out;
}

PSPosInput MicroMeshVS(VSInput In)
{
	PSPosInput Out;
	Out.Tex = In.texCoord;
	Out.worldPos = float3(0, 0, 0);
	Out.position = float4(0, 0, 0, 0);

	float4 pos = waterSampleTex.SampleLevel(PointSampler, In.texCoord.xy, 0.0);
	if (pos.w > 0.0)
	{
		Out.worldPos = pos.xyz + waterHeightTex.SampleLevel(LinearSampler, In.texCoord.xy, 0).xyz;
		Out.position = mul(float4(Out.worldPos.xyz, 1.0), mvpMat);
	}
	
	return Out;
}

PSPosInput TerrainVS(VSInput In)
{
	PSPosInput Out;
	Out.Tex = 0.5 * (In.texCoord.xy + float2(1.0, 1.0));
	float Height = terrainTex.SampleLevel(LinearSampler, Out.Tex, 0).y;
	Out.worldPos = SCENE_SIZE*0.5*float3(In.texCoord.x,Height-0.35, In.texCoord.y);
	Out.position = mul(float4(Out.worldPos,1.0), mvpMat);
	return Out;
}

PSInput RenderQuadVS(QuadVSInput In)
{
	PSInput Out;
	Out.Tex = In.Tex;
	Out.position = float4(In.texCoord.xy, 0.5, 1.0);
	return Out;
}


float		g_diffX;
float		g_diffY;

[maxvertexcount(4)]
void PacketGS( point PacketVSInput input[1], inout TriangleStream<PS_PACKET_INPUT> tStream )
{
	float3 CenterPos = float3(input[0].position.x, 0, input[0].position.y );	
	float3 Depth = float3(input[0].position.z, 0, input[0].position.w );
	float3 Width =  float3(Depth.z, 0, -Depth.x);				
	float Thickness = input[0].Att.w;							
							
	PS_PACKET_INPUT p0;
	p0.Att = input[0].Att.xyzw;
	p0.Att2 = input[0].Att2.xyzw;
	float3 p1 = CenterPos + 0.0 * Depth - Thickness * Width;
	p0.worldPos = float4(-1,1, -input[0].Att.w, 0);
	p0.position = mul(float4(p1,1.0), mvpMat);
	tStream.Append( p0 );
	float3 p2 = CenterPos - Thickness * Depth - Thickness * Width;
	p0.worldPos = float4(-1,-1, -input[0].Att.w, -input[0].Att.w);
	p0.position = mul(float4(p2,1.0), mvpMat);
	tStream.Append( p0 );
	float3 p3 = CenterPos + 0.0 * Depth + Thickness * Width;
	p0.worldPos = float4(1,1, input[0].Att.w, 0);
	p0.position = mul(float4(p3,1.0), mvpMat);
	tStream.Append( p0 );
	float3 p4 = CenterPos - Thickness * Depth + Thickness * Width;
	p0.worldPos = float4(1,-1, input[0].Att.w, -input[0].Att.w);
	p0.position = mul(float4(p4,1.0), mvpMat);
	tStream.Append( p0 );

	tStream.RestartStrip(); 
}


[maxvertexcount(3)]
void MicroMeshGS( triangle PSPosInput input[3], inout TriangleStream<PSPosInput> tStream )
{
	if ((input[0].position.w<0.01) || (input[1].position.w<0.01) || (input[2].position.w<0.01))
		return;
	tStream.Append(input[0]);
	tStream.Append(input[1]);
	tStream.Append(input[2]);
}

float4 SkyPS(VertexPosHL pIn) : SV_Target
{
	return skyTex.Sample(LinearSampler, pIn.PosL);
}

float4 CubePS(BoxVSOutput In) :SV_Target
{
	return In.color;
}

PS_OUTPUT PacketDisplacementPS(PS_PACKET_INPUT In)
{
	PS_OUTPUT Out;
	float centerDiff = length(In.worldPos.zw - float2(0.0f, In.Att2.x)) - abs(In.worldPos.w - In.Att2.x);
	float phase = -In.Att.z + (In.worldPos.w + centerDiff)*2.0*PI / In.Att.y;
	float3 rippleAdd =	1.0*(1.0 + cos(In.worldPos.x*PI)) *(1.0 + cos(In.worldPos.y*PI))*In.Att.x
						* float3(0, cos(phase), 0);									 	 
	Out.color.xyzw = float4(rippleAdd, 1.0);
	return Out;
}


PS_OUTPUT WavePositionPS(PSPosInput In)
{
	PS_OUTPUT Out; 
	Out.color = float4(In.worldPos.x, In.worldPos.y, In.worldPos.z, 1.0);
	return Out;
}

PS_OUTPUT WaterPS(PSPosInput In)
{
	PS_OUTPUT Out;
	float3 pos = waterSampleTex.SampleLevel(LinearSampler, In.Tex.xy, 0).xyz + waterHeightTex.SampleLevel(LinearSampler, In.Tex.xy, 0).xyz;
	float3 nVec = cross(ddx(pos), -ddy(pos));
	if (dot(nVec, nVec) <= 0)
		nVec = float3(0, -1, 0);
	else
		nVec = normalize(nVec);
	float3 vDir = normalize(In.worldPos - worldMat[3].xyz);	
	float3 rDir = vDir - (2.0*dot(vDir, nVec))*nVec;	
	float fac = 1.0 - (1.0 - abs(nVec.y) + abs(rDir.y))*(1.0 - abs(nVec.y) + abs(rDir.y));
	Out.color.xyz = fac*fac*float3(0.1, 0.4, 0.5);

	const float3 glareDir1 = normalize(float3(-1, -0.75, 1));
	const float3 glareDir2 = normalize(float3(1, -0.75, -1));
	const float3 glareDir3 = normalize(float3(1, -0.75, 1));
	const float3 glareDir4 = normalize(float3(-1, -0.75, -1));

	Out.color.xyz += 100.0*pow(
	 max(
		dot(-rDir, glareDir4), max(
		dot(-rDir, glareDir3), max(
		dot(-rDir, glareDir2), max(
		0.0, dot(-rDir, glareDir1))))), 5000);

	float waterDepth = 1.0 + 0.9*pow(terrainTex.SampleLevel(LinearSampler, In.worldPos.xz / SCENE_SIZE + float2(0.5, 0.5), 0).z, 4);
	Out.color.xyz = waterDepth*Out.color.xyz;
	Out.color.w = 1.0;

	return Out;
}

PS_OUTPUT TerrainPS(PSPosInput In)
{
	PS_OUTPUT Out; 
	if (In.worldPos.y < -0.1)
		clip(-1);
	float3 pos = In.worldPos;
	Out.color.xyz = (0.05)*float3(0.05, In.worldPos.y, 0.15);
	
	float3 nVec = cross(ddx(pos), -ddy(pos));
	if (dot(nVec, nVec) <= 0)
		nVec = float3(0, -1, 0);
	else
		nVec = normalize(nVec);
	float3 vDir = normalize(In.worldPos - worldMat[3].xyz);	
	float3 rDir = vDir - (2.0 * dot(vDir, nVec)) * nVec;	
	float3 color = float3(0.5, 0.6, 0.8);
	float fac = 1.0 - (1.0 - abs(nVec.y) + abs(rDir.y)) * (1.0 - abs(nVec.y) + abs(rDir.y));
	Out.color.xyz += fac * fac * float3(0.1, 0.3, 0.02);
	Out.color.w = 1.0;
	return Out;
}

PS_OUTPUT RenderAAPS(PSInput In) 
{ 
    PS_OUTPUT Out;
	Out.color = float4(0,0,0,1.0);
	for (float y=-2; y<2.5; y++)
		for (float x = -2; x < 2.5; x++)
		{
			Out.color += GaussWeight[((int)(y)+2) * 5 + (int)(x)+2] * waterHeightTex.SampleLevel(
				LinearSampler, float2(In.Tex.x, In.Tex.y) + float2(x * g_diffX, y * g_diffY), 0);
		}
		    
	return Out;
}

RasterizerState state
{
	FillMode = Solid;
	CullMode = None;
	MultisampleEnable = FALSE;
};


BlendState NoBlending
{
	AlphaToCoverageEnable = FALSE;
	BlendEnable[0] = FALSE;
};


BlendState BlendAdd
{
	AlphaToCoverageEnable = FALSE;
	BlendEnable[0] = TRUE;
	SrcBlend = ONE;
	DestBlend = ONE;
	BlendOp = ADD;
	SrcBlendAlpha = ONE;
	DestBlendAlpha = ONE;
	BlendOpAlpha = ADD;
	RenderTargetWriteMask[0] = 0x0F;
};

DepthStencilState EnableDepth
{
    DepthEnable = TRUE;
    DepthWriteMask = ALL;
    DepthFunc = LESS_EQUAL;
};

DepthStencilState DisableDepth
{
	DepthEnable = FALSE;
};

technique11 RenderSky
{
	pass P1
	{
		SetVertexShader(CompileShader(vs_4_0, SkyVS()));
		SetGeometryShader(NULL);
		SetPixelShader(CompileShader(ps_4_0, SkyPS()));
		SetRasterizerState(state);
		SetBlendState(NoBlending, float4(1.0f, 1.0f, 1.0f, 1.0f), 0xFFFFFFFF);
		SetDepthStencilState(EnableDepth, 0);
	}
}

technique11 RenderBox
{
	pass P1
	{
		SetVertexShader(CompileShader(vs_4_0, CubeVS()));
		SetGeometryShader(NULL);
		SetPixelShader(CompileShader(ps_4_0, CubePS()));
		SetRasterizerState(state);
		SetBlendState(NoBlending, float4(1.0f, 1.0f, 1.0f, 1.0f), 0xFFFFFFFF);
		SetDepthStencilState(EnableDepth, 0);
	}
}

technique11 RasterizeWaveMeshPosition
{
	pass P1
	{
		SetVertexShader(CompileShader(vs_4_0, WaveMeshVS( )));
		SetGeometryShader(NULL);
		SetPixelShader(CompileShader(ps_4_0, WavePositionPS( )));
		SetRasterizerState(state);
		SetBlendState(NoBlending, float4(1.0f, 1.0f, 1.0f, 1.0f), 0xFFFFFFFF);
		SetDepthStencilState(DisableDepth, 0);
	}
}

technique11 AddPacketDisplacement
{
	pass P1
	{
        SetVertexShader( CompileShader( vs_4_0, PacketVS( ) ) );
        SetGeometryShader( CompileShader( gs_4_0, PacketGS( ) ) );
        SetPixelShader( CompileShader( ps_4_0, PacketDisplacementPS( ) ) );
        SetRasterizerState( state );
        SetBlendState( BlendAdd, float4( 1.0f, 1.0f, 1.0f, 1.0f ), 0xFFFFFFFF );
        SetDepthStencilState( DisableDepth, 0 );
    }
}

technique11 DisplayMicroMesh
{
	pass P1
	{
        SetVertexShader( CompileShader( vs_4_0, MicroMeshVS( ) ) );
        SetGeometryShader( CompileShader( gs_4_0, MicroMeshGS()) );
        SetPixelShader( CompileShader( ps_4_0, WaterPS() ) );
        SetRasterizerState( state );
        SetBlendState( NoBlending, float4( 1.0f, 1.0f, 1.0f, 1.0f ), 0xFFFFFFFF );
        SetDepthStencilState( EnableDepth, 0 );
    }
}


technique11 DisplayTerrain
{
	pass P1
	{
        SetVertexShader( CompileShader( vs_4_0, TerrainVS( ) ) );
        SetGeometryShader( NULL );
        SetPixelShader( CompileShader( ps_4_0, TerrainPS( ) ) );
        SetRasterizerState( state );
        SetBlendState( NoBlending, float4( 1.0f, 1.0f, 1.0f, 1.0f ), 0xFFFFFFFF );
        SetDepthStencilState( EnableDepth, 0 );
    }
}

technique11 RenderAA
{
    pass P0
    {
        SetVertexShader( CompileShader( vs_4_0, RenderQuadVS() ) );
        SetGeometryShader( NULL );
        SetPixelShader( CompileShader( ps_4_0, RenderAAPS() ) );
		SetRasterizerState( state );
        SetBlendState( NoBlending, float4( 1.0f, 1.0f, 1.0f, 1.0f ), 0xFFFFFFFF );
        SetDepthStencilState( DisableDepth, 0 );
    }
}