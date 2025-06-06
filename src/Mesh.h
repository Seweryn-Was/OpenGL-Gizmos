#include <string>
#include <glm/gtc/matrix_transform.hpp>

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

	struct Bone {
		Bone(uint32_t nodeIndex, glm::mat4 invBindPose) : mNodeIndex(nodeIndex), mInvBindPose(invBindPose)  {}

		uint32_t mNodeIndex;
		glm::mat4 mInvBindPose; 
	};

	struct Node {
		Node(std::string name, int32_t parentIndex, glm::mat4 localTransform) 
			: mName(name), mParentIndex(parentIndex), mLocalTransform(localTransform), mGlobalTransform(glm::mat4(1.0f)) {}

		std::string mName; 
		int32_t mParentIndex; 
		glm::mat4 mLocalTransform; 
		glm::mat4 mGlobalTransform; //cache


	};

	class Skeleton {
	public:
		Skeleton() {
			mNodes.clear(); 
		}

		int addNode(const std::string& name, const uint32_t& parentIndex, const glm::mat4& localTransform) {
			uint32_t index = mNodes.size();
			mNodeNameToIndex[name] = index;
			mNodes.push_back(Node(name, parentIndex, localTransform));  
			return index; 
		}

		int addNode(const Node& node) { 
			uint32_t index = mNodes.size();
			mNodeNameToIndex[node.mName] = index;
			mNodes.push_back(node); 
			return index; 
		}

		void setNodeLocalTrans(uint32_t index, glm::mat4 localTrans) {
			mNodes[index].mLocalTransform = localTrans; 
		}

		uint32_t getNodeIndex(std::string name) { return mNodeNameToIndex[name]; }

		Node getNode(int index) { return mNodes[index]; }

		void calculateGlobalTransforms() {
			calculateRecursive(0, glm::mat4(1.0f));
		}

		const glm::mat4& getGlobalTransform(int index) const {
			return mNodes[index].mGlobalTransform;
		}

		int getNodeCount() {
			return mNodes.size(); 
		}

	private:
		std::vector<Node> mNodes;
		std::unordered_map<std::string, uint32_t> mNodeNameToIndex; 

		void calculateRecursive(int nodeIndex, const glm::mat4& parentTransform) {
			Node& node = mNodes[nodeIndex];
			node.mGlobalTransform = parentTransform * node.mLocalTransform;

			for (int i = 0; i < mNodes.size(); ++i) {
				if (mNodes[i].mParentIndex == nodeIndex)
					calculateRecursive(i, node.mGlobalTransform);
			}
		}
	};

	class SkinnedMesh : public StaticMesh {
	public: 
		SkinnedMesh(const std::vector<float>& vertecies, 
			const std::vector<SubMesh>& subMeshes, 
			const BufferLayout& layout, 
			const std::vector<Bone> bones) 
			: StaticMesh(vertecies, subMeshes, layout), 
			mBones(bones){}

		std::vector<glm::mat4> calculateSkinningMatrices(const Skeleton& skeleton) const {
			std::vector<glm::mat4> skinningMatrices;
			for (size_t i = 0; i < mBones.size(); ++i) {
				uint32_t nodeIndex = mBones[i].mNodeIndex;
				glm::mat4 globalTransform = skeleton.getGlobalTransform(nodeIndex);
				glm::mat4 skinMatrix = globalTransform * mBones[i].mInvBindPose;
				skinningMatrices.push_back(skinMatrix);
			}
			return skinningMatrices;
		}

		int getBoneCount() { return mBones.size(); }

	private: 
		std::vector<Bone> mBones; // reference to scene node to get its local transform 
	};

}