#include "GameObject.h"

using namespace DirectX;

GameObject::GameObject()
	: m_IndexCount(),
	m_VertexStride()
{
}

void GameObject::Release()
{
	if (m_pTexture) {
		m_pTexture->Release();
		m_pTexture = nullptr;
	}
	if (m_pVertexBuffer) {
		m_pVertexBuffer->Release();
		m_pVertexBuffer = nullptr;
	}
	if (m_pIndexBuffer) {
		m_pIndexBuffer->Release();
		m_pIndexBuffer = nullptr;
	}
}

GameObject::~GameObject()
{
	if (m_pTexture) {
		m_pTexture->Release();
		m_pTexture = nullptr;
	}
	if (m_pVertexBuffer) {
		m_pVertexBuffer->Release();
		m_pVertexBuffer = nullptr;
	}
	if (m_pIndexBuffer) {
		m_pIndexBuffer->Release();
		m_pIndexBuffer = nullptr;
	}
}
//Transform& GameObject::GetTransform()
//{
//	return m_Transform;
//}
//
//const Transform& GameObject::GetTransform() const
//{
//	return m_Transform;
//}

void GameObject::SetTexture(ID3D11ShaderResourceView* texture)
{
	m_pTexture = texture;
}


//void GameObject::Draw(ID3D11DeviceContext* deviceContext, BasicEffect& effect)
//{
//	// 设置顶点/索引缓冲区
//	UINT strides = m_VertexStride;
//	UINT offsets = 0;
//	deviceContext->IASetVertexBuffers(0, 1, m_pVertexBuffer.GetAddressOf(), &strides, &offsets);
//	deviceContext->IASetIndexBuffer(m_pIndexBuffer.Get(), DXGI_FORMAT_R32_UINT, 0);
//
//	// 更新数据并应用
//	effect.SetWorldMatrix(m_Transform.GetLocalToWorldMatrixXM());
//	effect.SetTexture(m_pTexture.Get());
//	effect.Apply(deviceContext);
//
//	deviceContext->DrawIndexed(m_IndexCount, 0, 0);
//}



