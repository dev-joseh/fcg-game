#version 330 core

// Atributos de fragmentos recebidos como entrada ("in") pelo Fragment Shader.
// Neste exemplo, este atributo foi gerado pelo rasterizador como a
// interpolação da posição global e a normal de cada vértice, definidas em
// "shader_vertex.glsl" e "main.cpp".
in vec4 position_world;
in vec4 normal;

// Posição do vértice atual no sistema de coordenadas local do modelo.
in vec4 position_model;

// Coordenadas de textura obtidas do arquivo OBJ (se existirem!)
in vec2 texcoords;

// Matrizes computadas no código C++ e enviadas para a GPU
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

// Identificador que define qual objeto está sendo desenhado no momento
#define SPHERE 0
#define BUNNY  1
#define PLANE  2
#define FLASHLIGHT  3
#define CROSSHAIR  4

uniform int object_id;

// Parâmetros da axis-aligned bounding box (AABB) do modelo
uniform vec4 bbox_min;
uniform vec4 bbox_max;

// Variáveis para acesso das imagens de textura
uniform sampler2D TextureImage0;
uniform sampler2D TextureImage1;
uniform sampler2D TextureImage2;
uniform sampler2D TextureImage3;

// Mapa de normais da textura do chão
uniform sampler2D TextureImage0_NormalMap;

// O valor de saída ("out") de um Fragment Shader é a cor final do fragmento.
out vec4 color;

// Função para a lanterna
float luz_lanterna(vec4 l, vec4 sv, float potencia);

// Parâmetros criados:
uniform int lanterna_ligada;
int potencia_lanterna;

// Constantes
#define M_PI   3.14159265358979323846
#define M_PI_2 1.57079632679489661923

void main()
{
    // Obtemos a posição da câmera utilizando a inversa da matriz que define o
    // sistema de coordenadas da câmera.
    vec4 origin = vec4(0.0, 0.0, 0.0, 1.0);
    vec4 camera_position = inverse(view) * origin;
    vec4 cameraForward = vec4(view[0][2], view[1][2], view[2][2], 0.0f);

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

    // Ponto que define a posição da fonte de luz spotlight.
    vec4 sl = camera_position;

    // Vetor que define o sentido da fonte de luz spotlight.
    vec4 sv = cameraForward;

    // Vetor que define o sentido da fonte de luz em relação ao ponto atual.
    vec4 l = v;

    // Vetor que define o sentido da reflexão especular ideal.
    vec4 r = -l+2*n*(dot(n, l)); // Vetor de reflexão especular ideal

    // Coordenadas de textura U e V
    float U = 0.0;
    float V = 0.0;

    // Parâmetros que definem as propriedades espectrais da superfície
    vec3 Kd, Ks, Ka; // Refletâncias difusa, especular e ambiente.
    float q; // Expoente especular para o modelo de iluminação de Phong

    // Coordenadas de textura esféricas
    float theta, phi, px, py, pz;

    if ( object_id == SPHERE )
    {
        // PREENCHA AQUI as coordenadas de textura da esfera, computadas com
        // projeção esférica EM COORDENADAS DO MODELO. Utilize como referência
        // o slides 134-150 do documento Aula_20_Mapeamento_de_Texturas.pdf.
        // A esfera que define a projeção deve estar centrada na posição
        // "bbox_center" definida abaixo.

        vec4 bbox_center = (bbox_min + bbox_max) / 2.0;

        vec4 r = position_model - bbox_center;

        theta = atan(position_model.x,position_model.z);
        phi   = asin(position_model.y/length(r));

        U = (theta+M_PI)/(2*M_PI);
        V = (phi+M_PI_2)/M_PI;

        // Propriedades espectrais da esfera
        Kd = vec3(0.0,0.0,0.0);
        Ks = vec3(0.0,0.0,0.0);
        Ka = vec3(0.2,0.02,0.02);
        q = 1.0;
    }
    else if ( object_id == BUNNY )
    {
        // Coordenadas de textura do coelho usando BBox
        float minx = bbox_min.x;
        float maxx = bbox_max.x;

        float miny = bbox_min.y;
        float maxy = bbox_max.y;

        float minz = bbox_min.z;
        float maxz = bbox_max.z;

        vec4 a = vec4(minx, miny, minz, 1.0);
        vec4 b = vec4(maxx, maxy, maxz, 1.0);

        U = (position_model.x-a.x)/(b.x-a.x);
        V = (position_model.y-a.y)/(b.y-a.y);

        // Propriedades espectrais do coelho
        Kd = vec3(0.08,0.4,0.8);
        Ks = vec3(0.8,0.8,0.8);
        Ka = vec3(0.04,0.2,0.4);
        q = 32.0;
    }
    else if ( object_id == FLASHLIGHT )
    {
        // Propriedades espectrais da lanterna
        Kd = vec3(0.0,0.0,0.0);
        Ks = vec3(0.0,0.0,0.0);
        Ka = vec3(1,1,1);
        q = 1.0;
        U = texcoords.x;
        V = texcoords.y;
    }
    else if ( object_id == PLANE )
    {
        vec2 texcoordsRepetidas = fract(texcoords*20); // Coordenadas repetidas do plano de chao

        // Coordenadas de textura do plano, obtidas do arquivo OBJ.
        U = texcoordsRepetidas[0];
        V = texcoordsRepetidas[1];

        // Propriedades espectrais da lanterna
        Kd = vec3(0.1,0.1,0.1);
        Ks = vec3(0.5,0.5,0.5);
        Ka = vec3(0.09,0.01,0.01);
        q = 32.0;
    }
    else if ( object_id == CROSSHAIR )
    {
        vec2 texcoordsRepetidas = fract(texcoords*20); // Coordenadas repetidas do plano de chao

        // Coordenadas de textura do plano, obtidas do arquivo OBJ.
        U = texcoordsRepetidas[0];
        V = texcoordsRepetidas[1];
    }

    // Espectro da fonte de iluminação
    vec3 I = vec3(1.0,1.0,1.0); // Espectro da fonte de luz

    // Espectro da luz ambiente
    vec3 Ia = vec3(0.15,0.15,0.15); // Espectro da luz ambiente

    // Termo difuso utilizando a lei dos cossenos de Lambert
    vec3 lambert_diffuse_term = Kd * I * max(0, dot(n,l)); // Termo difuso de Lambert

    // Termo ambiente
    vec3 ambient_term = Ka * Ia; // Termo ambiente

    // Termo especular utilizando o modelo de iluminação de Phong
    vec3 phong_specular_term  = Ks * I * pow(max(0,dot(r,v)), q) * max(0, dot(n,l)); // Termo especular de Phong

    // Define se a lanterna está ligada.
    if (lanterna_ligada==1)
        potencia_lanterna = 20;
    else
        potencia_lanterna = 0;

    // Define a iluminação dos objetos (A = Ambiente, D = Difusa, S = Difusa + Especular)
    vec3 A = ambient_term;
    vec3 D = lambert_diffuse_term * 0.2;
    vec3 S = (lambert_diffuse_term * 0.8 + phong_specular_term) * luz_lanterna(l, sv, potencia_lanterna);

    // Define a textura de cada objeto
    if( object_id == SPHERE )
        color.rgb = texture(TextureImage1, vec2(U,V)).rgb*(A+D+S);
    else if( object_id == BUNNY )
        color.rgb = texture(TextureImage3, vec2(U,V)).rgb*(A+D+S);
    else if( object_id == PLANE )
        color.rgb = texture(TextureImage0, vec2(U,V)).rgb*(D+S)
        + texture(TextureImage0_NormalMap, vec2(U,V)).rgb*(A+D);

    else if( object_id == FLASHLIGHT )
        color.rgb = texture(TextureImage2, vec2(U,V)).rgb*(A+D+S);

    // A implementar ...
    else if( object_id == CROSSHAIR )
        color.rgb = texture(TextureImage3, vec2(U,V)).rgb;



    // NOTE: Se você quiser fazer o rendering de objetos transparentes, é
    // necessário:
    // 1) Habilitar a operação de "blending" de OpenGL logo antes de realizar o
    //    desenho dos objetos transparentes, com os comandos abaixo no código C++:
    //      glEnable(GL_BLEND);
    //      glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    // 2) Realizar o desenho de todos objetos transparentes *após* ter desenhado
    //    todos os objetos opacos; e
    // 3) Realizar o desenho de objetos transparentes ordenados de acordo com
    //    suas distâncias para a câmera (desenhando primeiro objetos
    //    transparentes que estão mais longe da câmera).
    // Alpha default = 1 = 100% opaco = 0% transparente
    color.a = 1;

    // Cor final com correção gamma, considerando monitor sRGB.
    // Veja https://en.wikipedia.org/w/index.php?title=Gamma_correction&oldid=751281772#Windows.2C_Mac.2C_sRGB_and_TV.2Fvideo_standard_gammas
    color.rgb = pow(color.rgb, vec3(1.0,1.0,1.0)/2.2);
}

// Valor de iluminação da lanterna, normalizado para valores entre [0.0 1.0]
float luz_lanterna(vec4 l, vec4 sv, float potencia)
{
    float resultado;
    // A potencia dita a força da luz e seu raio
    float minimo = cos(potencia * 3.1415926535/180.0);
    float valor = dot(l, sv);

    if(valor > minimo)
        resultado = valor - minimo;
    else
        resultado = 0.0f;

    return resultado * potencia;
}
