/* Copyright 2015 Samsung Electronics Co., LTD
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */


/***************************************************************************
 * Logging macros.
 ***************************************************************************/

#ifndef LOG_H_
#define LOG_H_

#include <android/log.h>
#include <exception>

#include "GLES3/gl3.h"

#define  LOG_TAG    "gvrf"
#define  LOGV(...)  __android_log_print(ANDROID_LOG_VERBOSE,LOG_TAG,__VA_ARGS__)
#define  LOGD(...)  __android_log_print(ANDROID_LOG_DEBUG,LOG_TAG,__VA_ARGS__)
#define  LOGI(...)  __android_log_print(ANDROID_LOG_INFO,LOG_TAG,__VA_ARGS__)
#define  LOGW(...)  __android_log_print(ANDROID_LOG_WARN,LOG_TAG,__VA_ARGS__)
#define  LOGE(...)  __android_log_print(ANDROID_LOG_ERROR,LOG_TAG,__VA_ARGS__)

static const char * GlErrorString( GLenum error )
{
    switch ( error )
    {
        case GL_NO_ERROR:                       return "GL_NO_ERROR";
        case GL_INVALID_ENUM:                   return "GL_INVALID_ENUM";
        case GL_INVALID_VALUE:                  return "GL_INVALID_VALUE";
        case GL_INVALID_OPERATION:              return "GL_INVALID_OPERATION";
        case GL_INVALID_FRAMEBUFFER_OPERATION:  return "GL_INVALID_FRAMEBUFFER_OPERATION";
        case GL_OUT_OF_MEMORY:                  return "GL_OUT_OF_MEMORY";
        default: return "unknown";
    }
}
static void GLCheckErrors(const char* name)
{
    for ( int i = 0; i < 10; i++ )
    {
        const GLenum error = glGetError();
        if ( error == GL_NO_ERROR )
        {
            break;
        }
        LOGE( "mmarinov GL %s error: %s", name, GlErrorString( error ) );
        //std::terminate();
    }
}
#define GL( func )      func; GLCheckErrors(#func);
#define GL_V( func )      func; GLCheckErrors(#func);
//#define GL( func )      func;
//#define GL_V( func )      func;

#endif
