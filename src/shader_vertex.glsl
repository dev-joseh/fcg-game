#version 330 core

// Atributos de vértice recebidos como entrada ("in") pelo Vertex Shader.
// Veja a função BuildTrianglesAndAddToVirtualScene() em "main.cpp".
layout (location = 0) in vec4 model_coefficients;
layout (location = 1) in vec4 normal_coefficients;
layout (location = 2) in vec2 texture_coefficients;
// layout (location = 3) in vec4 normal_bitangente;

// Matrizes computadas no código C++ e enviadas para a GPU
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform int object_id;

// Atributos de vértice que serão gerados como saída ("out") pelo Vertex Shader.
// ** Estes serão interpolados pelo rasterizador! ** gerando, assim, valores
// para cada fragmento, os quais serão recebidos como entrada pelo Fragment
// Shader. Veja o arquivo "shader_fragment.glsl".
out vec4 position_world;
out vec4 position_model;
out vec4 normal;
out vec2 texcoords;
//
out vec4 cor_tiro; // usando a modelo de Gouraud Shading

void main()
{
    // A variável gl_Position define a posição final de cada vértice
    // OBRIGATORIAMENTE em "normalized device coordinates" (NDC), onde cada
    // coeficiente estará entre -1 e 1 após divisão por w.
    gl_Position = projection * view * model * model_coefficients;

    // Agora definimos outros atributos dos vértices que serão interpolados pelo
    // rasterizador para gerar atributos únicos para cada fragmento gerado.

    // Posição do vértice atual no sistema de coordenadas global (World).
    position_world = model * model_coefficients;

    // Posição do vértice atual no sistema de coordenadas local do modelo.
    position_model = model_coefficients;

    normal = inverse(transpose(model)) * normal_coefficients;
    normal.w = 0.0;
    texcoords = texture_coefficients;

    #define BULLET 1
    if ( object_id == BULLET )
    {
        // Normal do vértice atual no sistema de coordenadas global (World).
        // Veja slides 123-151 do documento Aula_07_Transformacoes_Geometricas_3D.pdf.
        vec4 origin = vec4(0.0, 0.0, 0.0, 1.0);
        vec4 camera_position = inverse(view) * origin;


        // Coordenadas de textura obtidas do arquivo OBJ (se existirem!)




        // Parâmetros que definem as propriedades espectrais da superfície
        vec3 Kd; // Refletância difusa
        vec3 Ks; // Refletância especular
        vec3 Ka; // Refletância ambiente
        float q; // Expoente especular para o modelo de iluminação de Phong

            // O fragmento atual é coberto por um ponto que percente à superfície de um
        // dos objetos virtuais da cena. Este ponto, p, possui uma posição no
        // sistema de coordenadas global (World coordinates). Esta posição é obtida
        // através da interpolação, feita pelo rasterizador, da posição de cada
        // vértice.
        vec4 p = position_world;

        // Normal do fragmento atual, interpolada pelo rasterizador a partir das
        // normais de cada vértice.
        vec4 n = normalize(normal);

        // Vetor que define o sentido da câmera em relação ao ponto atual.
        vec4 v = normalize(camera_position - p);

        // Vetor que define o sentido da fonte de luz em relação ao ponto atual.
        vec4 l = normalize(vec4(0.0f,5.0f,0.0f, 0.0f) - vec4(-1.0f,4.0f,0.0f, 0.0f));

        // Vetor que define o sentido da reflexão especular ideal.
        vec4 r = -l+2*n*(dot(n, l)); // Vetor de reflexão especular ideal

        // Espectro da fonte de iluminação
        vec3 I = vec3(1.0,1.0,1.0); // Espectro da fonte de luz

        vec3 Ia = vec3(0.1,0.1,0.1); // Espectro da luz ambiente

        /* Define se a lanterna está ligada.
        if (lanterna_ligada==1)
            potencia_lanterna = 25;
        else
            potencia_lanterna = 0;*/


            // Propriedades espectrais da bala
        Kd = vec3(0.6,0.3,0.3);
        Ks = vec3(0.9,0.5,0.5);
        Ka = vec3(0.3,0.1,0.1);
        q = 10.0;


        // Termo difuso utilizando a lei dos cossenos de Lambert
        vec3 lambert_diffuse_term = Kd * I * max(0, dot(n,l)); // Termo difuso de Lambert

        // Termo ambiente
        vec3 ambient_term = Ka * Ia; // Termo ambiente

        // Termo especular utilizando o modelo de iluminação de Phong. Para Mapa especular = substituir 'Ks'
        vec3 phong_specular_term  = Ks * I * pow(max(0,dot(r,v)), q) * max(0, dot(n,l)); // Termo especular de Phong

        cor_tiro.a = 1;
        cor_tiro.rgb = lambert_diffuse_term + ambient_term + phong_specular_term;
    }
}

