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

#ifndef CUSTOM_SHADER_H_
#define CUSTOM_SHADER_H_

#include <map>
#include <set>
#include <memory>
#include <string>

#include "GLES3/gl3.h"
#include "glm/glm.hpp"
#include "glm/gtc/type_ptr.hpp"

#include "objects/eye_type.h"
#include "objects/hybrid_object.h"
#include "objects/material.h"
#include "objects/mesh.h"

namespace gvr {

class GLProgram;
class RenderData;

typedef std::function<void(Mesh&, const std::string&, GLuint)> AttributeVariableUpdater;

class CustomShader: public HybridObject {
public:
    explicit CustomShader(std::string vertex_shader,
            std::string fragment_shader);
    virtual ~CustomShader();

    void addTextureKey(std::string variable_name, std::string key);

    void addAttributeKey(std::string variable_name, std::string key, AttributeVariableUpdater f);

    void addAttributeFloatKey(std::string variable_name, std::string key);
    void addAttributeVec2Key(std::string variable_name, std::string key);
    void addAttributeVec3Key(std::string variable_name, std::string key);
    void addAttributeVec4Key(std::string variable_name, std::string key);
    void addUniformFloatKey(std::string variable_name, std::string key);
    void addUniformVec2Key(std::string variable_name, std::string key);
    void addUniformVec3Key(std::string variable_name, std::string key);
    void addUniformVec4Key(std::string variable_name, std::string key);
    void addUniformMat4Key(std::string variable_name, std::string key);
    void render(const glm::mat4& mvp_matrix, RenderData* render_data, Material* material, bool right);
    static int getGLTexture(int n);

private:
    CustomShader(const CustomShader& custom_shader);
    CustomShader(CustomShader&& custom_shader);
    CustomShader& operator=(const CustomShader& custom_shader);
    CustomShader& operator=(CustomShader&& custom_shader);

    template <class T> struct Descriptor {
        Descriptor(const std::string& v, const std::string& k) {
            variable = v;
            key = k;
        }

        std::string variable;
        std::string key;
        int location = -1;
        T variableType;
    };

    template <class T> struct DescriptorComparator {
        bool operator() (const Descriptor<T>& lhs, const Descriptor<T>& rhs) const {
            return lhs.variable < rhs.variable && lhs.key < rhs.key;
        }
    };

    struct TextureVariable {
        std::function<int(GLuint, const std::string&)> f_location;
        std::function<void(int&, const Material&, const std::string&, GLuint)> f_update;
    };

    struct AttributeVariable {
        std::function<int(GLuint, const std::string&)> f_location;
        AttributeVariableUpdater f_update;
    };

private:
    GLProgram* program_;
    GLuint u_mvp_;
    GLuint u_right_;

    std::set<Descriptor<TextureVariable>, DescriptorComparator<TextureVariable>> texture_keys_;
    std::set<Descriptor<AttributeVariable>, DescriptorComparator<AttributeVariable>> attribute_variables_;

    std::map<int, std::string> uniform_float_keys_;
    std::map<int, std::string> uniform_vec2_keys_;
    std::map<int, std::string> uniform_vec3_keys_;
    std::map<int, std::string> uniform_vec4_keys_;
    std::map<int, std::string> uniform_mat4_keys_;
};

}
#endif
