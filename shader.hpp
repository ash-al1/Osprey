//===----------------------------------------------------------------------===//
//
// Shader class for loading shader programs in OpenGL
// Source code is modified from learnopengl.com
// 
//===----------------------------------------------------------------------===//

#ifndef SHADER_HPP
#define SHADER_HPP

#include <glad/glad.h>


#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <string>
  

class Shader
{
public:
    // the program ID
    unsigned int ID;
  
    // constructor reads and builds the shader
    Shader(const char* vertexPath, const char* fragmentPath);
    ~Shader();

    // use/activate the shader
    void use() const;

    // utility uniform functions
    void setBool(const std::string &name, bool value) const;  
    void setInt(const std::string &name, int value) const;   
    void setFloat(const std::string &name, float value) const;
    void setVec3(const std::string &name, glm::vec3 value) const;
    void setVec3(const std::string &name, float x, float y, float z) const;
    void setMat4(const std::string &name, glm::mat4 value) const;

private:
    void checkCompileErrors(unsigned int shader, std::string type);
};
  
#endif