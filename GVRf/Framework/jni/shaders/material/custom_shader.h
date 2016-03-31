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

typedef std::function<void(Mesh&, const std::string&, GLuint)> AttributeVariableBind;
typedef std::function<void(Material&, const std::string&, GLuint)> UniformVariableBind;

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

    void addAttributeKey(const std::string& variable_name, const std::string& key, AttributeVariableBind f);
    void addUniformKey(const std::string& variable_name, const std::string& key, UniformVariableBind f);

    void initializeOnDemand();

    template <class T> struct Descriptor {
        Descriptor(const std::string& v, const std::string& k) {
            variable = v;
            key = k;
        }

        std::string variable;
        std::string key;
        mutable int location = -1;
        T variableType;
    };

    template <class T> struct DescriptorComparator {
        bool operator() (const Descriptor<T>& lhs, const Descriptor<T>& rhs) const {
            return lhs.variable < rhs.variable && lhs.key < rhs.key;
        }
    };

    struct TextureVariable {
        std::function<int(GLuint, const std::string&)> f_getLocation;
        std::function<void(int&, const Material&, const std::string&, GLuint)> f_bind;
    };

    struct AttributeVariable {
        std::function<int(GLuint, const std::string&)> f_getLocation;
        AttributeVariableBind f_bind;
    };

    struct UniformVariable {
        std::function<int(GLuint, const std::string&)> f_getLocation;
        UniformVariableBind f_bind;
    };

private:
    GLProgram* program_;
    GLuint u_mvp_;
    GLuint u_right_;

    bool textureVariablesDirty_ = false;
    std::set<Descriptor<TextureVariable>, DescriptorComparator<TextureVariable>> textureVariables_;
    bool attributeVariablesDirty_ = false;
    std::set<Descriptor<AttributeVariable>, DescriptorComparator<AttributeVariable>> attributeVariables_;
    bool uniformVariablesDirty_ = false;
    std::set<Descriptor<UniformVariable>, DescriptorComparator<UniformVariable>> uniformVariables_;

    std::string vertexShader_;
    std::string fragmentShader_;
};

}
#endif
