#include "Buffer.h"
#include <GL/glew.h>

namespace Gizmo {

	VertexBuffer::VertexBuffer(uint32_t size) {
		glCreateBuffers(1, &m_vertexBufferID);
		glBindBuffer(GL_ARRAY_BUFFER, m_vertexBufferID);
		glBufferData(GL_ARRAY_BUFFER, size, nullptr, GL_DYNAMIC_DRAW);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
	};

	VertexBuffer::VertexBuffer(float* vertices, uint32_t size) {
		glCreateBuffers(1, &m_vertexBufferID);
		glBindBuffer(GL_ARRAY_BUFFER, m_vertexBufferID);
		glBufferData(GL_ARRAY_BUFFER, size, vertices, GL_DYNAMIC_DRAW);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
	};

	VertexBuffer::~VertexBuffer() {
		glDeleteBuffers(1, &m_vertexBufferID);
	};

	void VertexBuffer::Bind() const {
		glBindBuffer(GL_ARRAY_BUFFER, m_vertexBufferID);
	};
	void VertexBuffer::Unbind() const {
		glBindBuffer(GL_ARRAY_BUFFER, 0);
	};

	void VertexBuffer::SetData(const void* data, uint32_t size, uint32_t offset) {
		glBindBuffer(GL_ARRAY_BUFFER, m_vertexBufferID);
		glBufferSubData(GL_ARRAY_BUFFER, offset, size, data);
	};

	IndexBuffer::IndexBuffer(uint32_t* indices, uint32_t count) : m_count(count), mIndexForamt(IndexType::UInt32) {

		glCreateBuffers(1, &m_indexBufferID);
		glBindBuffer(GL_ARRAY_BUFFER, m_indexBufferID);
		glBufferData(GL_ARRAY_BUFFER, m_count * sizeof(uint32_t), indices, GL_DYNAMIC_DRAW);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
	};

	IndexBuffer::IndexBuffer(uint16_t* indices, uint32_t count) : m_count(count), mIndexForamt(IndexType::UInt16) {
		glCreateBuffers(1, &m_indexBufferID);
		glBindBuffer(GL_ARRAY_BUFFER, m_indexBufferID);
		glBufferData(GL_ARRAY_BUFFER, m_count * sizeof(uint16_t), indices, GL_DYNAMIC_DRAW);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
	};

	IndexBuffer::IndexBuffer(uint8_t* indices, uint32_t count, IndexType indexFormat) : m_count(count), mIndexForamt(indexFormat) {
		glCreateBuffers(1, &m_indexBufferID);
		glBindBuffer(GL_ARRAY_BUFFER, m_indexBufferID);
		if(indexFormat == IndexType::UInt16){
			glBufferData(GL_ARRAY_BUFFER, m_count * sizeof(uint16_t), reinterpret_cast<uint16_t*>(indices), GL_DYNAMIC_DRAW);
		}else{
			glBufferData(GL_ARRAY_BUFFER, m_count * sizeof(uint32_t), reinterpret_cast<uint32_t*>(indices), GL_DYNAMIC_DRAW);
		}
		glBindBuffer(GL_ARRAY_BUFFER, 0);
	}

	IndexBuffer::~IndexBuffer() {
		glDeleteBuffers(1, &m_indexBufferID);
	};

	void IndexBuffer::Bind() const {
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_indexBufferID);
	};

	void IndexBuffer::Unbind() const {
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	};

}