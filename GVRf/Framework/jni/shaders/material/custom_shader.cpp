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
 * A shader which an user can add in run-time.
 ***************************************************************************/

#include "custom_shader.h"

#include "gl/gl_program.h"
#include "objects/material.h"
#include "objects/mesh.h"
#include "objects/textures/texture.h"
#include "objects/components/render_data.h"
#include "util/gvr_gl.h"

namespace gvr {

static const int UNINITIALIZED = -1;

CustomShader::CustomShader(const std::string& vertex_shader,
        const std::string& fragment_shader) :
        program_(0), u_mvp_(0), u_right_(
                0), texture_keys_(), attribute_float_keys_(), attribute_vec2_keys_(), attribute_vec3_keys_(), attribute_vec4_keys_(), uniform_float_keys_(), uniform_vec2_keys_(), uniform_vec3_keys_(), uniform_vec4_keys_(), uniform_mat4_keys_() {
    vertexShader_ = vertex_shader;
    fragmentShader_ = fragment_shader;
}

CustomShader::~CustomShader() {
    delete program_;
}

void CustomShader::addTextureKey(const std::string& variable_name, const std::string& key) {
    texture_keys_dirty_ = true;
    VariableKeyPair pair(variable_name, key);
    texture_keys_[pair] = UNINITIALIZED;
}

void CustomShader::addAttributeFloatKey(const std::string& variable_name,
        const std::string& key) {
    attribute_float_dirty_ = true;
    VariableKeyPair pair(variable_name, key);
    attribute_float_keys_[pair] = UNINITIALIZED;
}

void CustomShader::addAttributeVec2Key(const std::string& variable_name,
        const std::string& key) {
    int location = glGetAttribLocation(program_->id(), variable_name.c_str());
    VariableKeyPair pair(variable_name, key);
    attribute_vec2_keys_[pair] = UNINITIALIZED;
}

void CustomShader::addAttributeVec3Key(const std::string& variable_name,
        const std::string& key) {
    int location = glGetAttribLocation(program_->id(), variable_name.c_str());
    VariableKeyPair pair(variable_name, key);
    attribute_vec3_keys_[pair] = UNINITIALIZED;
}

void CustomShader::addAttributeVec4Key(const std::string& variable_name,
        const std::string& key) {
    int location = glGetAttribLocation(program_->id(), variable_name.c_str());
    VariableKeyPair pair(variable_name, key);
    attribute_vec4_keys_[pair] = UNINITIALIZED;
}

void CustomShader::addUniformFloatKey(const std::string& variable_name,
        const std::string& key) {
    int location = glGetUniformLocation(program_->id(), variable_name.c_str());
    VariableKeyPair pair(variable_name, key);
    uniform_float_keys_[pair] = UNINITIALIZED;
}

void CustomShader::addUniformVec2Key(const std::string& variable_name,
        const std::string& key) {
    int location = glGetUniformLocation(program_->id(), variable_name.c_str());
    VariableKeyPair pair(variable_name, key);
    uniform_vec2_keys_[pair] = UNINITIALIZED;
}

void CustomShader::addUniformVec3Key(const std::string& variable_name,
        const std::string& key) {
    int location = glGetUniformLocation(program_->id(), variable_name.c_str());
    VariableKeyPair pair(variable_name, key);
    uniform_vec3_keys_[pair] = UNINITIALIZED;
}

void CustomShader::addUniformVec4Key(const std::string& variable_name,
        const std::string& key) {
    int location = glGetUniformLocation(program_->id(), variable_name.c_str());
    VariableKeyPair pair(variable_name, key);
    uniform_vec4_keys_[pair] = UNINITIALIZED;
}

void CustomShader::addUniformMat4Key(const std::string& variable_name,
        const std::string& key) {
    int location = glGetUniformLocation(program_->id(), variable_name.c_str());
    VariableKeyPair pair(variable_name, key);
    uniform_mat4_keys_[pair] = UNINITIALIZED;
}

void CustomShader::completeInitialization() {
    if (nullptr == program_) {
        program_ = new GLProgram(vertexShader_.c_str(), fragmentShader_.c_str());
        vertexShader_.clear();
        fragmentShader_.clear();

        u_mvp_ = glGetUniformLocation(program_->id(), "u_mvp");
        u_right_ = glGetUniformLocation(program_->id(), "u_right");
    }

    if (texture_keys_dirty_) {
        for (auto it = texture_keys_.begin(); it != texture_keys_.end(); ++it) {
            if (UNINITIALIZED == it->second) {
                int location = glGetUniformLocation(program_->id(), it->first.variable.c_str());
                it->second = location;
            }
        }
        texture_keys_dirty_ = false;
    }

    if (attribute_float_dirty_) {
        for (auto it = attribute_float_keys_.begin(); it != attribute_float_keys_.end(); ++it) {
            if (UNINITIALIZED == it->second) {
                int location = glGetAttribLocation(program_->id(), it->first.variable.c_str());
                it->second = location;
            }
        }

        attribute_float_dirty_ = false;
    }
}

void CustomShader::render(const glm::mat4& mvp_matrix, RenderData* render_data,
        Material* material, bool right) {
    completeInitialization();

    for (auto it = texture_keys_.begin(); it != texture_keys_.end(); ++it) {
        Texture* texture = material->getTextureNoError(it->first.key);
        // If any texture is not ready, do not render the material at all
        if (texture == NULL || !texture->isReady()) {
            return;
        }
    }

    Mesh* mesh = render_data->mesh();

    glUseProgram(program_->id());
//
//    if(mesh->isVaoDirty()) {
//        for (auto it = attribute_float_keys_.begin(); it != attribute_float_keys_.end(); ++it) {
//            mesh->setVertexAttribLocF(it->first, it->second);
//        }
//
//        for (auto it = attribute_vec2_keys_.begin(); it != attribute_vec2_keys_.end(); ++it) {
//            mesh->setVertexAttribLocV2(it->first, it->second);
//        }
//
//        for (auto it = attribute_vec3_keys_.begin(); it != attribute_vec3_keys_.end(); ++it) {
//            mesh->setVertexAttribLocV3(it->first, it->second);
//        }
//
//        for (auto it = attribute_vec4_keys_.begin(); it != attribute_vec4_keys_.end(); ++it) {
//            mesh->setVertexAttribLocV4(it->first, it->second);
//        }
//        mesh->unSetVaoDirty();
//    }
//    mesh->generateVAO();  // setup VAO
//
//    ///////////// uniform /////////
//    for (auto it = uniform_float_keys_.begin(); it != uniform_float_keys_.end();
//            ++it) {
//        glUniform1f(it->first, material->getFloat(it->second));
//    }
//
//    if (u_mvp_ != -1) {
//        glUniformMatrix4fv(u_mvp_, 1, GL_FALSE, glm::value_ptr(mvp_matrix));
//    }
//    if (u_right_ != 0) {
//        glUniform1i(u_right_, right ? 1 : 0);
//    }
//
//    int texture_index = 0;
//    for (auto it = texture_keys_.begin(); it != texture_keys_.end(); ++it) {
//        glActiveTexture(getGLTexture(texture_index));
//        Texture* texture = material->getTexture(it->second);
//        glBindTexture(texture->getTarget(), texture->getId());
//        glUniform1i(it->first, texture_index++);
//    }
//
//    for (auto it = uniform_vec2_keys_.begin(); it != uniform_vec2_keys_.end();
//            ++it) {
//        glm::vec2 v = material->getVec2(it->second);
//        glUniform2f(it->first, v.x, v.y);
//    }
//
//    for (auto it = uniform_vec3_keys_.begin(); it != uniform_vec3_keys_.end();
//            ++it) {
//        glm::vec3 v = material->getVec3(it->second);
//        glUniform3f(it->first, v.x, v.y, v.z);
//    }
//
//    for (auto it = uniform_vec4_keys_.begin(); it != uniform_vec4_keys_.end();
//            ++it) {
//        glm::vec4 v = material->getVec4(it->second);
//        glUniform4f(it->first, v.x, v.y, v.z, v.w);
//    }
//
//    for (auto it = uniform_mat4_keys_.begin(); it != uniform_mat4_keys_.end();
//            ++it) {
//        glm::mat4 m = material->getMat4(it->second);
//        glUniformMatrix4fv(it->first, 1, GL_FALSE, glm::value_ptr(m));
//    }
//
//    glBindVertexArray(mesh->getVAOId());
//    glDrawElements(render_data->draw_mode(), mesh->indices().size(), GL_UNSIGNED_SHORT,
//            0);
//    glBindVertexArray(0);
//
//    checkGlError("CustomShader::render");
}

int CustomShader::getGLTexture(int n) {
    switch (n) {
    case 0:
        return GL_TEXTURE0;
    case 1:
        return GL_TEXTURE1;
    case 2:
        return GL_TEXTURE2;
    case 3:
        return GL_TEXTURE3;
    case 4:
        return GL_TEXTURE4;
    case 5:
        return GL_TEXTURE5;
    case 6:
        return GL_TEXTURE6;
    case 7:
        return GL_TEXTURE7;
    case 8:
        return GL_TEXTURE8;
    case 9:
        return GL_TEXTURE9;
    case 10:
        return GL_TEXTURE10;
    default:
        return GL_TEXTURE0;
    }
}

} /* namespace gvr */
