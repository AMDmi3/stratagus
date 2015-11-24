#ifndef __SHADERS_H__
#define __SHADERS_H__
#ifdef USE_OPENGL
#define MAX_SHADERS 5
extern unsigned ShaderIndex;
extern void LoadShaders();
extern bool LoadShaderExtensions();
extern void SetupFramebuffer();
extern void RenderFramebufferToScreen();
#endif
#endif
