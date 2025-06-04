#include "Mesh.h"

namespace Gizmo{
	StaticMesh::StaticMesh(const std::vector<float>& vertecies, const std::vector<SubMesh>& subMeshes, const BufferLayout& layout)
		: mVertices(vertecies), mSubMeshes(subMeshes), mVertCount(vertecies.size()/(layout.GetStride()/sizeof(float))) {
		mVao = CreateRef<VertexArray>();

		mVbo = CreateRef<VertexBuffer>(mVertices.data(), mVertices.size() * sizeof(float));
		mVbo->SetLayout(layout);

		mIbo.resize(mSubMeshes.size());

		for (int i = 0; i < mIbo.size(); i++) {
			mIbo[i] = CreateRef<IndexBuffer>( mSubMeshes[i].getIndexData8(), mSubMeshes[i].getCount(), mSubMeshes[i].mIndexFormat);
		}

		mVao->AddVertexBuffer(mVbo);
		mVao->SetIndexBuffer(mIbo[0]);
	};

	void StaticMesh::bindSubMesh(int index) { mVao->SetIndexBuffer(mIbo[index]); }

	SubMesh StaticMesh::getSubMesh(int index) { return mSubMeshes[index]; };
}