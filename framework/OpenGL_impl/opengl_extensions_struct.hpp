#ifndef FRAMEWORK_OPENGL_EXTENSIONS_STRUCT_H
#define FRAMEWORK_OPENGL_EXTENSIONS_STRUCT_H

#include <GL/glcorearb.h>

#include <cstdint>
#include "glm/vec2.hpp"
#include "glm/vec3.hpp"

namespace framework {
    struct OpenGLAPIExtensions {
        //extensions for task mesh pipeline invocations
        PFNGLDRAWMESHTASKSNVPROC glDrawMeshTasksNV = nullptr;
        PFNGLDRAWMESHTASKSINDIRECTNVPROC glDrawMeshTasksIndirectNV = nullptr;
        PFNGLMULTIDRAWMESHTASKSINDIRECTNVPROC glMultiDrawMeshTasksIndirectNV = nullptr;
        PFNGLMULTIDRAWMESHTASKSINDIRECTCOUNTNVPROC glMultiDrawMeshTasksIndirectCountNV = nullptr;

        PFNGLDRAWMESHTASKSEXTPROC glDrawMeshTasksEXT = nullptr;
        PFNGLDRAWMESHTASKSINDIRECTEXTPROC glDrawMeshTasksIndirectEXT = nullptr;
        PFNGLMULTIDRAWMESHTASKSINDIRECTEXTPROC glMultiDrawMeshTasksIndirectEXT = nullptr;
        PFNGLMULTIDRAWMESHTASKSINDIRECTCOUNTEXTPROC glMultiDrawMeshTasksIndirectCountEXT = nullptr;

        //extensions for shader includes
        PFNGLNAMEDSTRINGARBPROC glNamedStringARB = nullptr;
        PFNGLDELETENAMEDSTRINGARBPROC glDeleteNamedStringARB = nullptr;
        PFNGLCOMPILESHADERINCLUDEARBPROC glCompileShaderIncludeARB = nullptr;

        //extensions for bindless textures
        PFNGLGETTEXTUREHANDLEARBPROC glGetTextureHandleARB = nullptr;

        PFNGLMAKETEXTUREHANDLERESIDENTARBPROC glMakeTextureHandleResidentARB = nullptr;
        PFNGLMAKETEXTUREHANDLENONRESIDENTARBPROC glMakeTextureHandleNonResidentARB = nullptr;
        PFNGLISTEXTUREHANDLERESIDENTARBPROC glIsTextureHandleResidentARB = nullptr;
    };

    struct OpenGLAPIQueriedParameters {
        glm::ivec3 MAX_COMPUTE_WORK_GROUP_COUNTS = { 1, 1, 1 };
        glm::i64vec2 MAX_COMPUTE_WORK_GROUP_COUNTS_PER_SLICE_AND_VOLUME = { 1LL, 1LL };
    };
}

#endif //FRAMEWORK_OPENGL_EXTENSIONS_STRUCT_H
