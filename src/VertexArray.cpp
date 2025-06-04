#include "VertexArray.h"
#include <GL/glew.h>

namespace Gizmo{

	VertexArray::VertexArray() : m_vertexBufferIndex(0) {
		glCreateVertexArrays(1, &m_vertexArrayID);
	};

	VertexArray::~VertexArray() {
		glDeleteVertexArrays(1, &m_vertexArrayID);
	};

	void VertexArray::Bind() const {
		glBindVertexArray(m_vertexArrayID); 
	};

	void VertexArray::Unbind() const {
		glBindVertexArray(0);
	};

	void VertexArray::AddVertexBuffer(const Ref<VertexBuffer>& vertexBuffer) {
		glBindVertexArray(m_vertexArrayID);
		vertexBuffer->Bind();

		const BufferLayout& layout = vertexBuffer->GetLayout();
		for (const BufferAttribute& attrib : layout)
		{
			switch (attrib.type)
			{
			case ShaderDataType::Float:
				assertm(false, "Not implemented");
				break;
			case ShaderDataType::Float2:
				glEnableVertexAttribArray(m_vertexBufferIndex);
				glVertexAttribPointer(m_vertexBufferIndex,
					attrib.getComponentCount(),
					ShaderDataTypeToGLType(attrib.type),
					attrib.normalized ? GL_TRUE : GL_FALSE,
					layout.GetStride(),
					(const void*)attrib.offset);
				m_vertexBufferIndex++;
				break;
			case ShaderDataType::Float3: {
				glEnableVertexAttribArray(m_vertexBufferIndex);
				glVertexAttribPointer(m_vertexBufferIndex,
					attrib.getComponentCount(),
					ShaderDataTypeToGLType(attrib.type),
					attrib.normalized ? GL_TRUE : GL_FALSE,
					layout.GetStride(),
					(const void*)attrib.offset);
				m_vertexBufferIndex++;
				break;
			}
			case ShaderDataType::Float4: {
				glEnableVertexAttribArray(m_vertexBufferIndex);
				glVertexAttribPointer(m_vertexBufferIndex,
					attrib.getComponentCount(),
					ShaderDataTypeToGLType(attrib.type),
					attrib.normalized ? GL_TRUE : GL_FALSE,
					layout.GetStride(),
					(const void*)attrib.offset);
				m_vertexBufferIndex++;
				break;
			}
			case ShaderDataType::Int:
				assertm(false, "Not implemented");
				break;
			case ShaderDataType::Int2:
				assertm(false, "Not implemented");
				break;
			case ShaderDataType::Int3:
				assertm(false, "Not implemented");
				break;
			default:
				assertm(false, "Unknown ShaderDataType!");
			}
		}

		m_vertexBuffers.push_back(vertexBuffer);
	};
	
	void VertexArray::SetIndexBuffer(const Ref<IndexBuffer>& indexBuffer) {
		glBindVertexArray(m_vertexArrayID);
		indexBuffer->Bind();
		m_indexBuffer = indexBuffer;
	};
}
