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


	IndexBuffer::IndexBuffer(uint32_t* indices, uint32_t count) : m_count(count) {
		glCreateBuffers(1, &m_indexBufferID);
		glBindBuffer(GL_ARRAY_BUFFER, m_indexBufferID);
		glBufferData(GL_ARRAY_BUFFER, m_count, indices, GL_DYNAMIC_DRAW);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
	};

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