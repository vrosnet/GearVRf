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
CustomShader::CustomShader(const std::string& vertex_shader,
        const std::string& fragment_shader) :
        program_(0), u_mvp_(0), u_right_(0), textureVariables_(), attributeVariables_() {
    vertexShader_ = vertex_shader;
    fragmentShader_ = fragment_shader;
}

CustomShader::~CustomShader() {
    delete program_;
}

void CustomShader::initializeOnDemand() {
    if (nullptr == program_) {
        program_ = new GLProgram(vertexShader_.c_str(), fragmentShader_.c_str());
        vertexShader_.clear();
        fragmentShader_.clear();

        u_mvp_ = glGetUniformLocation(program_->id(), "u_mvp");
        u_right_ = glGetUniformLocation(program_->id(), "u_right");
    }

    if (textureVariablesDirty_) {
        for (auto it = textureVariables_.begin(); it != textureVariables_.end(); ++it) {
            if (-1 == it->location) {
                it->location = it->variableType.f_getLocation(program_->id(), it->variable);
            }
        }
        textureVariablesDirty_ = false;
    }

    if (uniformVariablesDirty_) {
        for (auto it = uniformVariables_.begin(); it != uniformVariables_.end(); ++it) {
            if (-1 == it->location) {
                it->location = it->variableType.f_getLocation(program_->id(), it->variable);
            }
        }
        uniformVariablesDirty_ = false;
    }

    if (attributeVariablesDirty_) {
        for (auto it = attributeVariables_.begin(); it != attributeVariables_.end(); ++it) {
            if (-1 == it->location) {
                it->location = it->variableType.f_getLocation(program_->id(), it->variable);
            }
        }
        attributeVariablesDirty_ = false;
    }
}

void CustomShader::addTextureKey(const std::string& variable_name, const std::string& key) {
    LOGV("CustomShader::addTextureKey: variable: %s key: %s", variable_name.c_str(), key.c_str());
    Descriptor<TextureVariable> d(variable_name, key);

    d.variableType.f_getLocation = [] (GLuint programId, const std::string& variable) {
        int i = glGetUniformLocation(programId, variable.c_str());
        LOGV("CustomShader::texture:f_getLocation: variable: %s location: %d", variable.c_str(), i);
        return i;
    };

    d.variableType.f_bind = [] (int& textureIndex, const Material& material, const std::string& variable, GLuint location) {
        LOGV("CustomShader::texture::f_bind: textureIndex: %d variable: %s location: %d", textureIndex, variable.c_str(), location);
        glActiveTexture(getGLTexture(textureIndex));
        Texture* texture = material.getTexture(variable);
        glBindTexture(texture->getTarget(), texture->getId());
        glUniform1i(location, textureIndex++);
    };

    textureVariables_.insert(d);
    textureVariablesDirty_ = true;
}

void CustomShader::addAttributeFloatKey(const std::string& variable_name,
        const std::string& key) {
    AttributeVariableBind f =
            [] (Mesh& mesh, const std::string& variable, GLuint location) {
                LOGV("CustomShader::attributeFloatKey: bind variable: %s location: %d", variable.c_str(), location);
                mesh.setVertexAttribLocF(location, variable);
            };
    addAttributeKey(variable_name, key, f);
}

void CustomShader::addAttributeVec2Key(const std::string& variable_name,
        const std::string& key) {
    AttributeVariableBind f =
            [] (Mesh& mesh, const std::string& variable, GLuint location) {
                LOGV("CustomShader::attributeVec2Key: bind variable: %s location: %d", variable.c_str(), location);
                mesh.setVertexAttribLocV2(location, variable);
            };
    addAttributeKey(variable_name, key, f);
}

void CustomShader::addAttributeKey(const std::string& variable_name,
        const std::string& key, AttributeVariableBind f) {
    Descriptor<AttributeVariable> d(variable_name, key);

    d.variableType.f_getLocation = [] (GLuint programId, const std::string& variable) {
        int i = glGetAttribLocation(programId, variable.c_str());
        LOGV("CustomShader::attribute:f_getLocation: variable: %s location: %d", variable.c_str(), i);
        return i;
    };
    d.variableType.f_bind = f;

    attributeVariables_.insert(d);
    attributeVariablesDirty_ = true;
}

void CustomShader::addAttributeVec3Key(const std::string& variable_name,
        const std::string& key) {
    AttributeVariableBind f =
            [] (Mesh& mesh, const std::string& variable, GLuint location) {
                LOGV("CustomShader::attributeVec3Key: bind variable: %s location: %d", variable.c_str(), location);
                mesh.setVertexAttribLocV3(location, variable);
            };
    addAttributeKey(variable_name, key, f);
}

void CustomShader::addAttributeVec4Key(const std::string& variable_name,
        const std::string& key) {
    AttributeVariableBind f =
            [] (Mesh& mesh, const std::string& variable, GLuint location) {
                LOGV("CustomShader::attributeVec4Key: bind variable: %s location: %d", variable.c_str(), location);
                mesh.setVertexAttribLocV4(location, variable);
            };
    addAttributeKey(variable_name, key, f);
}

void CustomShader::addUniformKey(const std::string& variable_name,
        const std::string& key, UniformVariableBind f) {
    LOGV("CustomShader::addUniformKey: variable: %s key: %s", variable_name.c_str(), key.c_str());
    Descriptor<UniformVariable> d(variable_name, key);

    d.variableType.f_getLocation = [] (GLuint programId, const std::string& variable) {
        int i = glGetUniformLocation(programId, variable.c_str());
        LOGV("CustomShader::uniform:f_getLocation: variable: %s location: %d", variable.c_str(), i);
        return i;
    };
    d.variableType.f_bind = f;

    uniformVariables_.insert(d);
    uniformVariablesDirty_ = true;
}

void CustomShader::addUniformFloatKey(const std::string& variable_name,
        const std::string& key) {
    UniformVariableBind f =
            [] (Material& material, const std::string& variable, GLuint location) {
                LOGV("CustomShader::uniformFloatKey: bind variable: %s location: %d", variable.c_str(), location);
                glUniform1f(location, material.getFloat(variable));
            };
    addUniformKey(variable_name, key, f);
}

void CustomShader::addUniformVec2Key(const std::string& variable_name,
        const std::string& key) {
    UniformVariableBind f =
            [] (Material& material, const std::string& variable, GLuint location) {
                LOGV("CustomShader::uniformVec2Key: bind variable: %s location: %d", variable.c_str(), location);
                glm::vec2 v = material.getVec2(variable);
                glUniform2f(location, v.x, v.y);
            };
    addUniformKey(variable_name, key, f);
}

void CustomShader::addUniformVec3Key(const std::string& variable_name,
        const std::string& key) {
    UniformVariableBind f =
            [] (Material& material, const std::string& variable, GLuint location) {
                LOGV("CustomShader::uniformVec3Key: bind variable: %s location: %d", variable.c_str(), location);
                glm::vec3 v = material.getVec3(variable);
                glUniform3f(location, v.x, v.y, v.z);
            };
    addUniformKey(variable_name, key, f);
}

void CustomShader::addUniformVec4Key(const std::string& variable_name,
        const std::string& key) {
    UniformVariableBind f =
            [] (Material& material, const std::string& variable, GLuint location) {
                LOGV("CustomShader::uniformVec4Key: bind variable: %s location: %d", variable.c_str(), location);
                glm::vec4 v = material.getVec4(variable);
                glUniform4f(location, v.x, v.y, v.z, v.w);
            };
    addUniformKey(variable_name, key, f);
}

void CustomShader::addUniformMat4Key(const std::string& variable_name,
        const std::string& key) {
    UniformVariableBind f =
            [] (Material& material, const std::string& variable, GLuint location) {
                LOGV("CustomShader::uniformMat4Key: bind variable: %s location: %d", variable.c_str(), location);
                glm::mat4 m = material.getMat4(variable);
                glUniformMatrix4fv(location, 1, GL_FALSE, glm::value_ptr(m));
            };
    addUniformKey(variable_name, key, f);
}

void CustomShader::render(const glm::mat4& mvp_matrix, RenderData* render_data,
        Material* material, bool right) {
    initializeOnDemand();

    for (auto it = textureVariables_.begin(); it != textureVariables_.end(); ++it) {
        Texture* texture = material->getTextureNoError((*it).variable);
        // If any texture is not ready, do not render the material at all
        if (texture == NULL || !texture->isReady()) {
            return;
        }
    }

    Mesh* mesh = render_data->mesh();

    glUseProgram(program_->id());

    if(mesh->isVaoDirty()) {
        for (auto it = attributeVariables_.begin(); it != attributeVariables_.end(); ++it) {
            auto d = *it;
            d.variableType.f_bind(*mesh, d.variable, d.location);
        }
        mesh->unSetVaoDirty();
    }
    mesh->generateVAO();  // setup VAO

    ///////////// uniform /////////
    for (auto it = uniformVariables_.begin(); it != uniformVariables_.end(); ++it) {
        auto d = *it;
        d.variableType.f_bind(*material, d.variable, d.location);
    }

    if (u_mvp_ != -1) {
        glUniformMatrix4fv(u_mvp_, 1, GL_FALSE, glm::value_ptr(mvp_matrix));
    }
    if (u_right_ != 0) {
        glUniform1i(u_right_, right ? 1 : 0);
    }

    int texture_index = 0;
    for (auto it = textureVariables_.begin(); it != textureVariables_.end(); ++it) {
        auto d = *it;
        d.variableType.f_bind(texture_index, *material, d.variable, d.location);
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
