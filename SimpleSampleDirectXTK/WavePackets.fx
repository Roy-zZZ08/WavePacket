//***********************************************************MyRef.h****************************
// 
// Defines Effect11 Rendering PipeLine Settings
// 
//***************************************************************************************


#define PI 3.14159
#define SCENE_EXTENT 50.0f

const float GaussWeight[25] = {
	0.003, 0.013, 0.022, 0.013, 0.003,
	0.013, 0.060, 0.098, 0.060, 0.013,
	0.022, 0.098, 0.162, 0.098, 0.022,
	0.013, 0.060, 0.098, 0.060, 0.013,
	0.003, 0.013, 0.022, 0.013, 0.003,
};

// ================================================================================
//
// Global variables
//
// ================================================================================

float		g_BoxPos;
float4x4	m_World;                  
float4x4	m_MVP;    				

Texture2D t_DepthTerrain;	
Texture2D t_WaterSamplePos;		
Texture2D t_WaterHeight;		
TextureCube g_TexCube;		

// ================================================================================
//
// Vertex and Pixel shader structures
//
// ================================================================================

struct VertexPos
{
	float3 PosL : POSITION;
};

struct VertexPosHL
{
	float4 PosH : SV_POSITION;
	float3 PosL : POSITION;
};

struct VS_CUBE_INPUT
{
	float3 PosW	:	POSITION;
	float4 Col	:	COLOR;
};

struct VS_CUBE_OUT
{
	float4 PosH :	SV_POSITION;
	float4 Col	:	COLOR;
};

struct VS_INPUT
{
    float2 PosTex		: POSITION;
};

struct VS_QUAD_INPUT
{
    float2 PosTex		: POSITION;
    float2 Tex		: TEXTURE0;
};

struct VS_PACKET_INPUT
{
	float4 PosV	: POSITION;  
	float4 Att	: TEXTURE0;  
	float4 Att2	: TEXTURE1;
};

struct VertexPosNormalTex
{
	float3 PosL : POSITION;
	float3 NormalL : NORMAL;
	float2 Tex : TEXCOORD;
};

struct VertexPosHWNormalTex
{
	float4 PosH : SV_POSITION;
	float3 PosW : POSITION; 
	float3 NormalW : NORMAL; 
	float2 Tex : TEXCOORD;
};

//--------------------------------------

struct PS_INPUT
{
	float4 PosH : SV_POSITION;
    float2 Tex		 : TEXTURE0;
};

struct PS_POS_INPUT
{
	float4 PosH : SV_POSITION;
	float2 Tex		 : TEXTURE0;
	float3 PosW		 : TEXTURE1; 
};

struct PS_PACKET_INPUT
{
	float4 PosH	 : SV_POSITION;
	float4 PosW			 : TEXTURE0;
	float4 Att			 : TEXTURE1; 
	float4 Att2			 : TEXTURE2; 
};

struct PS_OUTPUT
{
	float4 Col	: SV_Target0;
};


// ================================================================================
//
// Samplers
//
// ================================================================================


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

// ================================================================================
//
// Vertex shader
//
// ================================================================================

VertexPosHL SkyVS(VertexPos vIn)
{
	VertexPosHL vOut;
	float4 posH = mul(float4(vIn.PosL, 1.0f), m_MVP);
	vOut.PosH = posH.xyww;
	vOut.PosL = vIn.PosL;
	return vOut;
}

VS_CUBE_OUT CubeVS(VS_CUBE_INPUT In)
{
	VS_CUBE_OUT Out;
	Out.PosH = mul(float4(In.PosW.x,In.PosW.y+ g_BoxPos,In.PosW.z, 1.0f), m_MVP);
	Out.Col = In.Col;                  
	return Out;
}

VertexPosHWNormalTex VS_3D(VertexPosNormalTex vIn)
{
	VertexPosHWNormalTex vOut;
	float4 posW = mul(float4(vIn.PosL, 1.0f), m_World);
	float3 normalW = mul(vIn.NormalL, (float3x3) m_World);

	vOut.PosH = mul(float4(vIn.PosL, 1.0f), m_MVP);
	vOut.PosW = posW.xyz;
	vOut.NormalW = normalW;
	vOut.Tex = vIn.Tex;
	return vOut;
}

VS_PACKET_INPUT PacketVS(VS_PACKET_INPUT In)
{
    return In;
}

PS_POS_INPUT WaveMeshVS(VS_INPUT In )
{
	PS_POS_INPUT Out;

	Out.PosW = 0.5*SCENE_EXTENT*float3(In.PosTex.x, 0, In.PosTex.y);
	Out.PosH = mul( float4(Out.PosW,1.0), m_MVP);
	Out.Tex = In.PosTex;

	return Out;
}

PS_POS_INPUT MicroMeshVS(VS_INPUT In)
{
	PS_POS_INPUT Out;
	Out.Tex = In.PosTex;
	Out.PosW = float3(0, 0, 0);
	Out.PosH = float4(0, 0, 0, 0);

	float4 pos = t_WaterSamplePos.SampleLevel(PointSampler, In.PosTex.xy, 0.0);
	if (pos.w > 0.0)
	{
		Out.PosW = pos.xyz + t_WaterHeight.SampleLevel(LinearSampler, In.PosTex.xy, 0).xyz;
		Out.PosH = mul(float4(Out.PosW.xyz, 1.0), m_MVP);
	}
	
	return Out;
}

PS_POS_INPUT TerrainVS(VS_INPUT In)
{
	PS_POS_INPUT Out;
	Out.Tex = 0.5 * (In.PosTex.xy + float2(1.0, 1.0));
	float Height = t_DepthTerrain.SampleLevel(LinearSampler, Out.Tex, 0).y;
	Out.PosW = SCENE_EXTENT*0.5*float3(In.PosTex.x,Height-0.35, In.PosTex.y);
	Out.PosH = mul(float4(Out.PosW,1.0), m_MVP);
	return Out;
}

PS_INPUT RenderQuadVS(VS_QUAD_INPUT In)
{
	PS_INPUT Out;
	Out.Tex = In.Tex;
	Out.PosH = float4(In.PosTex.xy, 0.5, 1.0);
	return Out;
}


// ================================================================================
//
// Geometry shader
//
// ================================================================================

float		g_diffX;
float		g_diffY;

[maxvertexcount(4)]
void PacketGS( point VS_PACKET_INPUT input[1], inout TriangleStream<PS_PACKET_INPUT> tStream )
{
	float3 CenterPos = float3(input[0].PosV.x, 0, input[0].PosV.y );	
	float3 Depth = float3(input[0].PosV.z, 0, input[0].PosV.w );
	float3 Width =  float3(Depth.z, 0, -Depth.x);				
	float Thickness = input[0].Att.w;							
							
	PS_PACKET_INPUT p0;
	p0.Att = input[0].Att.xyzw;
	p0.Att2 = input[0].Att2.xyzw;
	float3 p1 = CenterPos + 0.0 * Depth - Thickness * Width;
	p0.PosW = float4(-1,1, -input[0].Att.w, 0);
	p0.PosH = mul(float4(p1,1.0), m_MVP);
	tStream.Append( p0 );
	float3 p2 = CenterPos - Thickness * Depth - Thickness * Width;
	p0.PosW = float4(-1,-1, -input[0].Att.w, -input[0].Att.w);
	p0.PosH = mul(float4(p2,1.0), m_MVP);
	tStream.Append( p0 );
	float3 p3 = CenterPos + 0.0 * Depth + Thickness * Width;
	p0.PosW = float4(1,1, input[0].Att.w, 0);
	p0.PosH = mul(float4(p3,1.0), m_MVP);
	tStream.Append( p0 );
	float3 p4 = CenterPos - Thickness * Depth + Thickness * Width;
	p0.PosW = float4(1,-1, input[0].Att.w, -input[0].Att.w);
	p0.PosH = mul(float4(p4,1.0), m_MVP);
	tStream.Append( p0 );

	tStream.RestartStrip(); 
}


[maxvertexcount(3)]
void MicroMeshGS( triangle PS_POS_INPUT input[3], inout TriangleStream<PS_POS_INPUT> tStream )
{
	if ((input[0].PosH.w<0.01) || (input[1].PosH.w<0.01) || (input[2].PosH.w<0.01))
		return;
	tStream.Append(input[0]);
	tStream.Append(input[1]);
	tStream.Append(input[2]);
}


// ================================================================================
//
// Pixel Shaders
//
// ================================================================================

float4 SkyPS(VertexPosHL pIn) : SV_Target
{
	return g_TexCube.Sample(LinearSampler, pIn.PosL);
}

float4 CubePS(VS_CUBE_OUT In) :SV_Target
{
	return In.Col;
}

float4 PS_3D(VertexPosHWNormalTex pIn) : SV_Target
{
	return float4(1.0,1.0,1.0,1.0);
}

PS_OUTPUT PacketDisplacementPS(PS_PACKET_INPUT In)
{
	PS_OUTPUT Out;
	float centerDiff = length(In.PosW.zw - float2(0.0f, In.Att2.x)) - abs(In.PosW.w - In.Att2.x);   //	centerDiff = 0; -> straight waves
	float phase = -In.Att.z + (In.PosW.w + centerDiff)*2.0*PI / In.Att.y;
	float3 rippleAdd =	1.0*(1.0 + cos(In.PosW.x*PI)) *(1.0 + cos(In.PosW.y*PI))*In.Att.x // gaussian envelope
						* float3(0, cos(phase), 0);									 	 // ripple function
	Out.Col.xyzw = float4(rippleAdd, 1.0);
	return Out;
}


PS_OUTPUT WavePositionPS(PS_POS_INPUT In)
{
	PS_OUTPUT Out; 
	Out.Col = float4(In.PosW.x, In.PosW.y, In.PosW.z, 1.0);
	return Out;
}


//water rendering
PS_OUTPUT WaterPS(PS_POS_INPUT In)
{
	PS_OUTPUT Out;
	float3 pos = t_WaterSamplePos.SampleLevel(LinearSampler, In.Tex.xy, 0).xyz + t_WaterHeight.SampleLevel(LinearSampler, In.Tex.xy, 0).xyz;
	float3 nVec = cross(ddx(pos), -ddy(pos));
	if (dot(nVec, nVec) <= 0)
		nVec = float3(0, -1, 0);
	else
		nVec = normalize(nVec);
	// view vector
	float3 vDir = normalize(In.PosW - m_World[3].xyz);	
	// reflection vector
	float3 rDir = vDir - (2.0*dot(vDir, nVec))*nVec;	
	// diffuse && reflective lighting
	float fac = 1.0 - (1.0 - abs(nVec.y) + abs(rDir.y))*(1.0 - abs(nVec.y) + abs(rDir.y));
	Out.Col.xyz = fac*fac*float3(0.1, 0.4, 0.5);

	//specular 
	const float3 glareDir1 = normalize(float3(-1, -0.75, 1));
	const float3 glareDir2 = normalize(float3(1, -0.75, -1));
	const float3 glareDir3 = normalize(float3(1, -0.75, 1));
	const float3 glareDir4 = normalize(float3(-1, -0.75, -1));

	Out.Col.xyz += 100.0*pow(
	 max(
		dot(-rDir, glareDir4), max(
		dot(-rDir, glareDir3), max(
		dot(-rDir, glareDir2), max(
		0.0, dot(-rDir, glareDir1))))), 5000);

	float waterDepth = 1.0 + 0.9*pow(t_DepthTerrain.SampleLevel(LinearSampler, In.PosW.xz / SCENE_EXTENT + float2(0.5, 0.5), 0).z, 4);
	Out.Col.xyz = waterDepth*Out.Col.xyz;
	Out.Col.w = 1.0;

	return Out;
}



// rendering the landscape 
PS_OUTPUT TerrainPS(PS_POS_INPUT In)
{
	PS_OUTPUT Out; 
	if (In.PosW.y < -0.1)
		clip(-1);
	float3 pos = In.PosW;
	Out.Col.xyz = (0.05)*float3(0.05, In.PosW.y, 0.15);
	
	float3 nVec = cross(ddx(pos), -ddy(pos));
	if (dot(nVec, nVec) <= 0)
		nVec = float3(0, -1, 0);
	else
		nVec = normalize(nVec);
	float3 vDir = normalize(In.PosW - m_World[3].xyz);	
	float3 rDir = vDir - (2.0 * dot(vDir, nVec)) * nVec;	
	float3 color = float3(0.5, 0.6, 0.8);
	float fac = 1.0 - (1.0 - abs(nVec.y) + abs(rDir.y)) * (1.0 - abs(nVec.y) + abs(rDir.y));
	Out.Col.xyz += fac * fac * float3(0.1, 0.3, 0.02);
	Out.Col.w = 1.0;
	return Out;
}


// downsampling (anti aliasing)
PS_OUTPUT RenderAAPS(PS_INPUT In) 
{ 
    PS_OUTPUT Out;
	Out.Col = float4(0,0,0,1.0);
	for (float y=-2; y<2.5; y++)
		for (float x = -2; x < 2.5; x++)
		{
			Out.Col += GaussWeight[((int)(y)+2) * 5 + (int)(x)+2] * t_WaterHeight.SampleLevel(
				LinearSampler, float2(In.Tex.x, In.Tex.y) + float2(x * g_diffX, y * g_diffY), 0);
		}
		    
	return Out;
}



//--------------------------------------------------------------------------------------
// States
//--------------------------------------------------------------------------------------

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


//--------------------------------------------------------------------------------------
// Techniques
//--------------------------------------------------------------------------------------

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