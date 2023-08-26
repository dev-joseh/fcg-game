#ifndef COLLISIONS_H
#define COLLISIONS_H


#include <glm/mat4x4.hpp>
#include <glm/vec4.hpp>
#include <glm/gtc/matrix_transform.hpp>


struct SceneObject
{
    std::string  name;        // Nome do objeto
    size_t       first_index; // �ndice do primeiro v�rtice dentro do vetor indices[] definido em BuildTrianglesAndAddToVirtualScene()
    size_t       num_indices; // N�mero de �ndices do objeto dentro do vetor indices[] definido em BuildTrianglesAndAddToVirtualScene()
    GLenum       rendering_mode; // Modo de rasteriza��o (GL_TRIANGLES, GL_TRIANGLE_STRIP, etc.)
    GLuint       vertex_array_object_id; // ID do VAO onde est�o armazenados os atributos do modelo
    glm::vec3    bbox_min; // Axis-Aligned Bounding Box do objeto
    glm::vec3    bbox_max;
};


bool BoundingBoxIntersection (SceneObject &objeto1, SceneObject &objeto2)
{
    if (objeto1.bbox_max.x < objeto2.bbox_min.x || objeto1.bbox_min.x > objeto2.bbox_max.x) {
        return false; // Separados no eixo X
    }
    if (objeto1.bbox_max.y < objeto2.bbox_min.y || objeto1.bbox_min.y > objeto2.bbox_max.y) {
        return false; // Separados no eixo Y
    }
    if (objeto1.bbox_max.z < objeto2.bbox_min.z || objeto1.bbox_min.z > objeto2.bbox_max.z) {
        return false; // Separados no eixo Z
    }

    return true; // N�o houve separa��o em nenhum dos eixos, portanto, est�o se interseccionando
}

#endif  COLLISIONS_H
