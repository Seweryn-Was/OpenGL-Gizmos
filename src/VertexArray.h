#pragma once 

#include <cassert>
#define assertm(exp, msg) assert((void(msg), exp))

#include <vector>
#include "Base.h"
#include "Buffer.h"

namespace Gizmo{
	class VertexArray {
	public:
		VertexArray();
		~VertexArray();

		void Bind() const;
		void Unbind() const;

		void AddVertexBuffer(const Ref<VertexBuffer>& vertexBuffer);
		void SetIndexBuffer(const Ref<IndexBuffer>& indexBuffer);

		const std::vector<Ref<VertexBuffer>>& GetVertexBuffers() const { return m_vertexBuffers; }
		const Ref<IndexBuffer>& GetIndexBuffer() const { return m_indexBuffer; }

	private:
		uint32_t m_vertexArrayID;
		uint32_t m_vertexBufferIndex; 
		std::vector<Ref<VertexBuffer>> m_vertexBuffers; 
		Ref<IndexBuffer> m_indexBuffer;

	};
}