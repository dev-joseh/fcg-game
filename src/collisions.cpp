
// Headers da biblioteca GLM: criação de matrizes e vetores.
#include <glm/mat4x4.hpp>
#include <glm/vec4.hpp>
#include <glm/gtc/type_ptr.hpp>

// Arquivos "headers" padrões de C
#include <cmath>
#include <cstdio>
#include <cstdlib>





struct Plano {
    glm::vec3 normal; // Vetor normal ao plano
    glm::vec3 point;  // Ponto pertencente ao plano

    Plano(const glm::vec3& normal, const glm::vec3& point)
        : normal(normal), point(point) {}
};

// Define a estrutura de um objeto com Axis Aligned Bounding Box
struct AABB {
    glm::vec3 minimo; // Coordenadas mínimas (canto inferior esquerdo)
    glm::vec3 maximo; // Coordenadas máximas (canto superior direito)

        AABB(const glm::vec3& minimo, const glm::vec3& maximo)
            : minimo(minimo), maximo(maximo) {}

    // Função para verificar a colisão com outra AABB
    bool EstaColidindoComAABB(const AABB& other) const {
        // Verifique se as caixas não colidem em algum eixo
        if (maximo.x < other.minimo.x || minimo.x > other.maximo.x) return false;
        if (maximo.y < other.minimo.y || minimo.y > other.maximo.y) return false;
        if (maximo.z < other.minimo.z || minimo.z > other.maximo.z) return false;

        // Se não houver sobreposição em nenhum dos eixos, elas estão colidindo
        return true;
    }
    // Função para verificar a colisão com um plano
    bool EstaColidindoComPlano(const Plano& plane) const {
        // Calcula o ponto médio da AABB
        glm::vec3 center = (minimo + maximo) * 0.5f;

        // Calcula a distância entre o ponto médio e o plano
        float distance = glm::dot(plane.normal, center - plane.point);

        // Se a distância for menor que a metade da diagonal da AABB
        // na direção oposta à normal do plano, então há colisão
        float diagonalLength = glm::length(maximo - minimo);
        return (std::abs(distance) <= diagonalLength / 2.0f);
    }
};

struct Esfera {
    glm::vec4 center; // Centro da esfera
    float radius;     // Raio da esfera

    Esfera(const glm::vec4& center, float radius)
        : center(center), radius(radius) {}

    float VaiColidirComAABB(const AABB& other, glm::vec4 posicao_futura){
        // Função para verificar a colisão com uma esfera
        float angulo;

        // Encontrar o ponto mais próximo na AABB ao centro da esfera
        glm::vec4 closestPoint;

        for (int i = 0; i < 3; i++)
        {
            if (Esfera::center[i] < other.minimo[i])
                closestPoint[i] = other.minimo[i];
            else if (Esfera::center[i] > other.maximo[i])
                closestPoint[i] = other.maximo[i];
            else
                closestPoint[i] = Esfera::center[i];
        }
        closestPoint[3] = 1.0f;

        // Verificar se a distância entre o ponto mais próximo e o centro da esfera é menor ou igual ao raio
        float distance = glm::distance(closestPoint, Esfera::center);

        if (distance <= Esfera::radius)
        {
            glm::vec4 direction = posicao_futura - Esfera::center;
            glm::vec4 vetor = closestPoint - Esfera::center;
            angulo = acos(glm::dot(direction, vetor));
        }

        if(distance > radius)
            angulo = -1.0f;

        return angulo; // Retorna -1 se não houver colisão
    }

};

