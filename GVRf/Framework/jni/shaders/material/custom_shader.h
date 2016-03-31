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
#include <memory>
#include <string>

#include "GLES3/gl3.h"
#include "glm/glm.hpp"
#include "glm/gtc/type_ptr.hpp"

#include "objects/eye_type.h"
#include "objects/hybrid_object.h"

namespace gvr {

class GLProgram;
class RenderData;
class Material;

class CustomShader: public HybridObject {
public:
    explicit CustomShader(const std::string& vertex_shader,
            const std::string& fragment_shader);
    virtual ~CustomShader();

    void addTextureKey(const std::string& variable_name, const std::string& key);
    void addAttributeFloatKey(const std::string& variable_name, const std::string& key);
    void addAttributeVec2Key(const std::string& variable_name, const std::string& key);
    void addAttributeVec3Key(const std::string& variable_name, const std::string& key);
    void addAttributeVec4Key(const std::string& variable_name, const std::string& key);
    void addUniformFloatKey(const std::string& variable_name, const std::string& key);
    void addUniformVec2Key(const std::string& variable_name, const std::string& key);
    void addUniformVec3Key(const std::string& variable_name, const std::string& key);
    void addUniformVec4Key(const std::string& variable_name, const std::string& key);
    void addUniformMat4Key(const std::string& variable_name, const std::string& key);
    void render(const glm::mat4& mvp_matrix, RenderData* render_data, Material* material, bool right);
    static int getGLTexture(int n);

private:
    CustomShader(const CustomShader& custom_shader);
    CustomShader(CustomShader&& custom_shader);
    CustomShader& operator=(const CustomShader& custom_shader);
    CustomShader& operator=(CustomShader&& custom_shader);

    void completeInitialization();

private:
    GLProgram* program_;
    GLuint u_mvp_;
    GLuint u_right_;

    std::string vertexShader_, fragmentShader_;

    struct VariableKeyPair {
        explicit VariableKeyPair(const std::string& v, const std::string& k) {
            variable = v;
            key = k;
        }
        std::string variable;
        std::string key;

        bool operator<(const VariableKeyPair& vkp) const {
            return variable < vkp.variable && key < vkp.key;
        }
    };

    class VariableKeyPairComparator {
       public:
        bool operator()(const VariableKeyPair& x, const VariableKeyPair& y) {
            return x < y;
        }
    };

    bool texture_keys_dirty_ = false;
    std::map<VariableKeyPair, int, VariableKeyPairComparator> texture_keys_;
    bool attribute_float_dirty_ = false;
    std::map<VariableKeyPair, int> attribute_float_keys_;
    std::map<VariableKeyPair, int> attribute_vec2_keys_;
    std::map<VariableKeyPair, int> attribute_vec3_keys_;
    std::map<VariableKeyPair, int> attribute_vec4_keys_;
    std::map<VariableKeyPair, int> uniform_float_keys_;
    std::map<VariableKeyPair, int> uniform_vec2_keys_;
    std::map<VariableKeyPair, int> uniform_vec3_keys_;
    std::map<VariableKeyPair, int> uniform_vec4_keys_;
    std::map<VariableKeyPair, int> uniform_mat4_keys_;
};

}
#endif
