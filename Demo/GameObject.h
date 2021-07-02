#ifndef GAMEOBJECT_H
#define GAMEOBJECT_H

#include "Geometry.h"
#include "Transform.h"

using namespace DirectX;


class GameObject
{
public:

	GameObject();

	~GameObject();

	void Release();


	// ���û�����
	template<class VertexType, class IndexType>
	void SetBuffer(ID3D11Device* device, const Geometry::MeshData<VertexType, IndexType>& meshData);


	// ��������
	void SetTexture(ID3D11ShaderResourceView* texture);

	// ����
	//void Draw(ID3D11DeviceContext* deviceContext, BasicEffect& effect);


private:

	ID3D11ShaderResourceView *m_pTexture;		// ����
	ID3D11Buffer *m_pVertexBuffer;				// ���㻺����
	ID3D11Buffer *m_pIndexBuffer;				// ����������
	UINT m_VertexStride;								// �����ֽڴ�С
	UINT m_IndexCount;								    // ������Ŀ	
};

template<class VertexType, class IndexType>
inline void GameObject::SetBuffer(ID3D11Device* device, const Geometry::MeshData<VertexType, IndexType>& meshData)
{

	// ���D3D�豸
	if (device == nullptr)
		return;

	// ���ö��㻺��������
	m_VertexStride = sizeof(VertexType);
	D3D11_BUFFER_DESC vbd;
	ZeroMemory(&vbd, sizeof(vbd));
	vbd.Usage = D3D11_USAGE_IMMUTABLE;
	vbd.ByteWidth = (UINT)meshData.vertexVec.size() * m_VertexStride;
	vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vbd.CPUAccessFlags = 0;
	// �½����㻺����
	D3D11_SUBRESOURCE_DATA InitData;
	ZeroMemory(&InitData, sizeof(InitData));
	InitData.pSysMem = meshData.vertexVec.data();
	device->CreateBuffer(&vbd, &InitData, &m_pVertexBuffer);

	// ������������������
	m_IndexCount = (UINT)meshData.indexVec.size();
	D3D11_BUFFER_DESC ibd;
	ZeroMemory(&ibd, sizeof(ibd));
	ibd.Usage = D3D11_USAGE_IMMUTABLE;
	ibd.ByteWidth = m_IndexCount * sizeof(IndexType);
	ibd.BindFlags = D3D11_BIND_INDEX_BUFFER;
	ibd.CPUAccessFlags = 0;
	// �½�����������
	InitData.pSysMem = meshData.indexVec.data();
	device->CreateBuffer(&ibd, &InitData, &m_pIndexBuffer);


}

#endif
