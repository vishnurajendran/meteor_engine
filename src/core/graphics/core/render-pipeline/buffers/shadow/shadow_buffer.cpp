#include <GL/glew.h>
#include "shadow_buffer.h"

void SShadowBuffer::initDynamicShadows(int res)
{
    // ---- Spot shadow maps ---------------------------------------------------
    for (int i = 0; i < MAX_SPOT_SHADOWS; ++i)
    {
        glGenTextures(1, &spotDepthTextures[i]);
        glBindTexture(GL_TEXTURE_2D, spotDepthTextures[i]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, res, res,
                     0, GL_DEPTH_COMPONENT, GL_UNSIGNED_INT, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
        float border[] = {1.0f, 1.0f, 1.0f, 1.0f};
        glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, border);
        glBindTexture(GL_TEXTURE_2D, 0);

        glGenFramebuffers(1, &spotFBOs[i]);
        glBindFramebuffer(GL_FRAMEBUFFER, spotFBOs[i]);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
                               GL_TEXTURE_2D, spotDepthTextures[i], 0);
        glDrawBuffer(GL_NONE);
        glReadBuffer(GL_NONE);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    // ---- Point shadow cube maps --------------------------------------------
    for (int i = 0; i < MAX_POINT_SHADOWS; ++i)
    {
        glGenTextures(1, &pointCubemapTextures[i]);
        glBindTexture(GL_TEXTURE_CUBE_MAP, pointCubemapTextures[i]);
        for (int f = 0; f < 6; ++f)
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + f, 0, GL_DEPTH_COMPONENT24,
                         res, res, 0, GL_DEPTH_COMPONENT, GL_UNSIGNED_INT, nullptr);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
        glBindTexture(GL_TEXTURE_CUBE_MAP, 0);

        glGenFramebuffers(1, &pointFBOs[i]);
    }
}

void SShadowBuffer::destroyDynamicShadows()
{
    for (int i = 0; i < MAX_SPOT_SHADOWS; ++i)
    {
        if (spotFBOs[i])          { glDeleteFramebuffers(1, &spotFBOs[i]);         spotFBOs[i] = 0; }
        if (spotDepthTextures[i]) { glDeleteTextures(1,    &spotDepthTextures[i]); spotDepthTextures[i] = 0; }
    }
    for (int i = 0; i < MAX_POINT_SHADOWS; ++i)
    {
        if (pointFBOs[i])            { glDeleteFramebuffers(1, &pointFBOs[i]);            pointFBOs[i] = 0; }
        if (pointCubemapTextures[i]) { glDeleteTextures(1,    &pointCubemapTextures[i]); pointCubemapTextures[i] = 0; }
    }
}