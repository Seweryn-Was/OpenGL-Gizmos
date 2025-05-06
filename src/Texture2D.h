#pragma once

#include <iostream>
#include <string>

#include <gl/glew.h>

class Texture2D
{
public:
    Texture2D(const char* path, uint32_t slotID = 0);
    ~Texture2D();

    void Bind();
    void Unbind();

    inline int getWidth() const { return mWidth; }
    inline int getHeight() const { return mHeight; }
    inline GLuint getTexture() const { return mTextureID; }

private:
    GLuint loadTexture();
    uint32_t mTextureID;
    uint32_t mSlotID;
    uint32_t mWidth;
    uint32_t mHeight;
    uint32_t mBPP;
    const char* mFilePath;
};