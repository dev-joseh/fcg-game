
// Headers da biblioteca GLM: cria��o de matrizes e vetores.
#include <glm/mat4x4.hpp>
#include <glm/vec4.hpp>
#include <glm/gtc/type_ptr.hpp>

// Arquivos "headers" padr�es de C
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
    glm::vec3 minimo; // Coordenadas m�nimas (canto inferior esquerdo)
    glm::vec3 maximo; // Coordenadas m�ximas (canto superior direito)

        AABB(const glm::vec3& minimo, const glm::vec3& maximo)
            : minimo(minimo), maximo(maximo) {}

    // Fun��o para verificar a colis�o com outra AABB
    bool EstaColidindoComAABB(const AABB& other) const {
        // Verifique se as caixas n�o colidem em algum eixo
        if (maximo.x < other.minimo.x || minimo.x > other.maximo.x) return false;
        if (maximo.y < other.minimo.y || minimo.y > other.maximo.y) return false;
        if (maximo.z < other.minimo.z || minimo.z > other.maximo.z) return false;

        // Se n�o houver sobreposi��o em nenhum dos eixos, elas est�o colidindo
        return true;
    }
    // Fun��o para verificar a colis�o com um plano
    bool EstaColidindoComPlano(const Plano& plane) const {
        // Calcula o ponto m�dio da AABB
        glm::vec3 center = (minimo + maximo) * 0.5f;

        // Calcula a dist�ncia entre o ponto m�dio e o plano
        float distance = glm::dot(plane.normal, center - plane.point);

        // Se a dist�ncia for menor que a metade da diagonal da AABB
        // na dire��o oposta � normal do plano, ent�o h� colis�o
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
        // Fun��o para verificar a colis�o com uma esfera
        float angulo;

        // Encontrar o ponto mais pr�ximo na AABB ao centro da esfera
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

        // Verificar se a dist�ncia entre o ponto mais pr�ximo e o centro da esfera � menor ou igual ao raio
        float distance = glm::distance(closestPoint, Esfera::center);

        if (distance <= Esfera::radius)
        {
            glm::vec4 direction = posicao_futura - Esfera::center;
            glm::vec4 vetor = closestPoint - Esfera::center;
            angulo = acos(glm::dot(direction, vetor));
        }

        if(distance > radius)
            angulo = -1.0f;

        return angulo; // Retorna -1 se n�o houver colis�o
    }

};

