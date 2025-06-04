#include "Base.h"
#include "VertexArray.h"
#include "Buffer.h"

#include <vector>

namespace Gizmo {

	struct SubMesh {
		SubMesh(const std::vector<uint32_t>& indices, uint32_t materialIndex) : mMaterialIndex(materialIndex), mCount(indices.size()), mIndexFormat(IndexType::UInt32) {
			mIndices.resize(indices.size() * sizeof(uint32_t));
			std::memcpy(mIndices.data(), indices.data(), mIndices.size());
		}

		SubMesh(const std::vector<uint16_t>& indices, uint32_t materialIndex) : mMaterialIndex(materialIndex), mCount(indices.size()), mIndexFormat(IndexType::UInt16) {
			mIndices.resize(indices.size() * sizeof(uint16_t));
			std::memcpy(mIndices.data(), indices.data(), mIndices.size());
		}

		std::vector<uint8_t> mIndices;
		uint32_t mMaterialIndex = 0;
		uint32_t mCount;
		IndexType mIndexFormat; 

		uint16_t* getIndexData16() { return reinterpret_cast<uint16_t*>(mIndices.data()); }
		uint32_t* getIndexData32() { return reinterpret_cast<uint32_t*>(mIndices.data()); }
		uint8_t*  getIndexData8() { return mIndices.data(); }

		uint32_t getCount() const { return mCount; }
	};

	class StaticMesh {
	public:
		StaticMesh(const std::vector<float>& vertecies, const std::vector<SubMesh>& subMeshes, const BufferLayout& layout);

		void bindSubMesh(int index);
		uint32_t subMeshCount() { return mSubMeshes.size(); };

		SubMesh getSubMesh(int index);

	private:
		Ref<VertexArray> mVao;
		Ref<VertexBuffer> mVbo;
		std::vector<float> mVertices;
		std::vector<SubMesh> mSubMeshes;
		std::vector<Ref<IndexBuffer>> mIbo;
		IndexType mIndexFormat;
		uint32_t mVertCount;
	};
}