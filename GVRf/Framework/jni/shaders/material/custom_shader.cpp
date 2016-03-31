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
#include "objects/textures/texture.h"
#include "objects/components/render_data.h"
#include "util/gvr_gl.h"

#include <sys/time.h>

namespace gvr {
CustomShader::CustomShader(std::string vertex_shader,
        std::string fragment_shader) :
        program_(0), u_mvp_(0), u_right_(
                0), texture_keys_(), attribute_variables_(), uniform_float_keys_(), uniform_vec2_keys_(), uniform_vec3_keys_(), uniform_vec4_keys_(), uniform_mat4_keys_() {
    program_ = new GLProgram(vertex_shader.c_str(), fragment_shader.c_str());
    u_mvp_ = glGetUniformLocation(program_->id(), "u_mvp");
    u_right_ = glGetUniformLocation(program_->id(), "u_right");
}

CustomShader::~CustomShader() {
    delete program_;
}

void CustomShader::addTextureKey(std::string variable_name, std::string key) {
    Descriptor<TextureVariable> d(variable_name, key);

    d.variableType.f_location = [] (GLuint programId, const std::string& variable) {
        return glGetUniformLocation(programId, variable.c_str());
    };
    d.location = d.variableType.f_location(program_->id(), d.variable);

    d.variableType.f_update = [] (int& textureIndex, const Material& material, const std::string& variable, GLuint location) {
        glActiveTexture(getGLTexture(textureIndex));
        Texture* texture = material.getTexture(variable);
        glBindTexture(texture->getTarget(), texture->getId());
        glUniform1i(location, textureIndex++);
    };

    texture_keys_.insert(d);
}

void CustomShader::addAttributeFloatKey(std::string variable_name,
        std::string key) {
    AttributeVariableUpdater f =
            [] (Mesh& mesh, const std::string& variable, GLuint location) {
                mesh.setVertexAttribLocF(location, variable);
            };
    addAttributeKey(variable_name, key, f);
}

void CustomShader::addAttributeVec2Key(std::string variable_name,
        std::string key) {
    AttributeVariableUpdater f =
            [] (Mesh& mesh, const std::string& variable, GLuint location) {
                mesh.setVertexAttribLocV2(location, variable);
            };
    addAttributeKey(variable_name, key, f);
}

void CustomShader::addAttributeKey(std::string variable_name,
        std::string key, AttributeVariableUpdater f) {
    Descriptor<AttributeVariable> d(variable_name, key);

    d.variableType.f_location = [] (GLuint programId, const std::string& variable) {
        return glGetAttribLocation(programId, variable.c_str());
    };
    d.location = d.variableType.f_location(program_->id(), d.variable);
    d.variableType.f_update = f;

    attribute_variables_.insert(d);
}

void CustomShader::addAttributeVec3Key(std::string variable_name,
        std::string key) {
    AttributeVariableUpdater f =
            [] (Mesh& mesh, const std::string& variable, GLuint location) {
                mesh.setVertexAttribLocV3(location, variable);
            };
    addAttributeKey(variable_name, key, f);
}

void CustomShader::addAttributeVec4Key(std::string variable_name,
        std::string key) {
    AttributeVariableUpdater f =
            [] (Mesh& mesh, const std::string& variable, GLuint location) {
                mesh.setVertexAttribLocV4(location, variable);
            };
    addAttributeKey(variable_name, key, f);
}

void CustomShader::addUniformFloatKey(std::string variable_name,
        std::string key) {
    int location = glGetUniformLocation(program_->id(), variable_name.c_str());
    uniform_float_keys_[location] = key;
}

void CustomShader::addUniformVec2Key(std::string variable_name,
        std::string key) {
    int location = glGetUniformLocation(program_->id(), variable_name.c_str());
    uniform_vec2_keys_[location] = key;
}

void CustomShader::addUniformVec3Key(std::string variable_name,
        std::string key) {
    int location = glGetUniformLocation(program_->id(), variable_name.c_str());
    uniform_vec3_keys_[location] = key;
}

void CustomShader::addUniformVec4Key(std::string variable_name,
        std::string key) {
    int location = glGetUniformLocation(program_->id(), variable_name.c_str());
    uniform_vec4_keys_[location] = key;
}

void CustomShader::addUniformMat4Key(std::string variable_name,
        std::string key) {
    int location = glGetUniformLocation(program_->id(), variable_name.c_str());
    uniform_mat4_keys_[location] = key;
}

void CustomShader::render(const glm::mat4& mvp_matrix, RenderData* render_data,
        Material* material, bool right) {
    for (auto it = texture_keys_.begin(); it != texture_keys_.end(); ++it) {
        Texture* texture = material->getTextureNoError((*it).variable);
        // If any texture is not ready, do not render the material at all
        if (texture == NULL || !texture->isReady()) {
            return;
        }
    }

    Mesh* mesh = render_data->mesh();

    glUseProgram(program_->id());

    if(mesh->isVaoDirty()) {
        for (auto it = attribute_variables_.begin(); it != attribute_variables_.end(); ++it) {
            auto d = *it;
            d.variableType.f_update(*mesh, d.variable, d.location);
        }
        mesh->unSetVaoDirty();
    }
    mesh->generateVAO();  // setup VAO

    ///////////// uniform /////////
    for (auto it = uniform_float_keys_.begin(); it != uniform_float_keys_.end();
            ++it) {
        glUniform1f(it->first, material->getFloat(it->second));
    }

    if (u_mvp_ != -1) {
        glUniformMatrix4fv(u_mvp_, 1, GL_FALSE, glm::value_ptr(mvp_matrix));
    }
    if (u_right_ != 0) {
        glUniform1i(u_right_, right ? 1 : 0);
    }

    int texture_index = 0;
    for (auto it = texture_keys_.begin(); it != texture_keys_.end(); ++it) {
        auto d = *it;
        d.variableType.f_update(texture_index, *material, d.variable, d.location);
    }

    for (auto it = uniform_vec2_keys_.begin(); it != uniform_vec2_keys_.end();
            ++it) {
        glm::vec2 v = material->getVec2(it->second);
        glUniform2f(it->first, v.x, v.y);
    }

    for (auto it = uniform_vec3_keys_.begin(); it != uniform_vec3_keys_.end();
            ++it) {
        glm::vec3 v = material->getVec3(it->second);
        glUniform3f(it->first, v.x, v.y, v.z);
    }

    for (auto it = uniform_vec4_keys_.begin(); it != uniform_vec4_keys_.end();
            ++it) {
        glm::vec4 v = material->getVec4(it->second);
        glUniform4f(it->first, v.x, v.y, v.z, v.w);
    }

    for (auto it = uniform_mat4_keys_.begin(); it != uniform_mat4_keys_.end();
            ++it) {
        glm::mat4 m = material->getMat4(it->second);
        glUniformMatrix4fv(it->first, 1, GL_FALSE, glm::value_ptr(m));
    }

    glBindVertexArray(mesh->getVAOId(material->shader_type()));
    glDrawElements(render_data->draw_mode(), mesh->indices().size(), GL_UNSIGNED_SHORT,
            0);
    glBindVertexArray(0);

    checkGlError("CustomShader::render");
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
