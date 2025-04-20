#pragma once
#include <GL/glew.h>

#include <vector>

#include <cassert>
#define assertm(exp, msg) assert((void(msg), exp))

namespace Gizmo {
	enum ShaderDataType { None = 0, Float, Float2, Float3, Int, Int2, Int3 };

	static uint32_t getShaderDataTypeSize(ShaderDataType type) {

		switch (type) {
		case ShaderDataType::Float:		return 4;
		case ShaderDataType::Float2:	return 8;
		case ShaderDataType::Float3:	return 12;
		case ShaderDataType::Int:		return 4;
		case ShaderDataType::Int2:		return 8;
		case ShaderDataType::Int3:		return 12;
		}

		assertm(false, "Unknown ShaderDataType");
		return 0;
	}

	static GLenum ShaderDataTypeToGLType(ShaderDataType type) {
		switch (type) {
		case ShaderDataType::Float:		return GL_FLOAT;
		case ShaderDataType::Float2:	return GL_FLOAT;
		case ShaderDataType::Float3:	return GL_FLOAT;
		case ShaderDataType::Int:		return GL_INT;
		case ShaderDataType::Int2:		return GL_INT;
		case ShaderDataType::Int3:		return GL_INT;
		};

		assertm(false, "Unknown ShaderDataType");
		return 0;
	}


	struct BufferAttribute {
		ShaderDataType type;
		uint32_t size;
		size_t offset;
		bool normalized;

		BufferAttribute(ShaderDataType type, bool normalized) :
			type(type), size(getShaderDataTypeSize(type)), offset(0), normalized(normalized) {};

		uint32_t getComponentCount() const {
			switch (type) {
			case ShaderDataType::Float:		return 1;
			case ShaderDataType::Float2:	return 2;
			case ShaderDataType::Float3:	return 3;
			case ShaderDataType::Int:		return 1;
			case ShaderDataType::Int2:		return 2;
			case ShaderDataType::Int3:		return 3;
			}

			assert(false, "Unknown ShaderDataType");
			return 0;
		}

	};

	typedef BufferAttribute BufferAttrib;

	class BufferLayout {
	public:
		BufferLayout() = default;
		BufferLayout(std::initializer_list<BufferAttribute> attributes) : m_attributes(attributes) {
			calculateOffsetAndStride();
		};

		uint32_t GetStride() const { return m_stride; }
		const std::vector<BufferAttribute>& GetElements() const { return m_attributes; }

		std::vector<BufferAttribute>::iterator begin() { return m_attributes.begin(); }
		std::vector<BufferAttribute>::iterator end() { return m_attributes.end(); }
		std::vector<BufferAttribute>::const_iterator begin() const { return m_attributes.begin(); }
		std::vector<BufferAttribute>::const_iterator end() const { return m_attributes.end(); }

	private:

		void calculateOffsetAndStride() {
			uint32_t offset = 0;
			m_stride = 0;
			for (BufferAttribute attrib : m_attributes) {
				attrib.offset = offset;
				m_stride += attrib.size;
				offset += attrib.size;
			}
		};

		std::vector<BufferAttribute> m_attributes;
		uint32_t m_stride;
	};

	class VertexBuffer {
	public:
		~VertexBuffer();

		VertexBuffer(uint32_t size);
		VertexBuffer(float* vertices, uint32_t size);

		void Bind() const;
		void Unbind() const;

		void SetData(const void* data, uint32_t size, uint32_t offset = 0);

		const BufferLayout& GetLayout() const { return m_Layout; };
		void SetLayout(const BufferLayout& layout) { m_Layout = layout; };

	private:
		uint32_t m_vertexBufferID;
		BufferLayout m_Layout;
	};

	class IndexBuffer {
	public:
		IndexBuffer(uint32_t* indices, uint32_t count);
		~IndexBuffer();

		void Bind() const;
		void Unbind() const;

		virtual uint32_t GetCount() const { return m_count; }
	private:
		uint32_t m_indexBufferID;
		uint32_t m_count;
	};

}