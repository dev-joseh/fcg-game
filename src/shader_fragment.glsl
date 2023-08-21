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
#define BULLET 1
#define PLANE  2
#define FLASHLIGHT  3
#define REVOLVER  4
#define SCREEN  5
#define SKULL  6
#define EYE 7
#define SMOKE 8
#define ARVORE 9
#define CABINE 10
#define CARRO 11
#define C_VIDROS 12

uniform int object_id;

// Parâmetros da axis-aligned bounding box (AABB) do modelo
uniform vec4 bbox_min;
uniform vec4 bbox_max;

// Variáveis para acesso das imagens de textura
uniform sampler2D chao;
uniform sampler2D ceu;
uniform sampler2D lanterna;
uniform sampler2D crosshair;
uniform sampler2D skull_diff;
uniform sampler2D smoke;
uniform sampler2D bark;
uniform sampler2D folhas;
uniform sampler2D cabine_diff;
uniform sampler2D carro_BP_diff;
uniform sampler2D carro_SP_diff;
uniform sampler2D carro_W_diff;

// Mapa de normais
uniform sampler2D chao_normal;
uniform sampler2D skull_normal;
uniform sampler2D cabine_normal;

// Mapa de especular
uniform sampler2D cabine_spec;

// O valor de saída ("out") de um Fragment Shader é a cor final do fragmento.
out vec4 color;

// Função para a lanterna
float luz_lanterna(vec4 l, vec4 sv, float potencia);

// Parâmetros criados:
uniform int lanterna_ligada;
uniform int smoke_life;
int potencia_lanterna;
uniform int nozzle_flash;
bool opaco=false;
uniform bool tronco;

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

    // == CENÁRIO ==
    if ( object_id == SPHERE )
    {
        opaco = true;
        vec4 bbox_center = (bbox_min + bbox_max) / 2.0;

        vec4 r = position_model - bbox_center;

        theta = atan(position_model.x,position_model.z);
        phi   = asin(position_model.y/length(r));

        U = (theta+M_PI)/(2*M_PI);
        V = (phi+M_PI_2)/M_PI;

        // Propriedades espectrais da esfera skybox
        Kd = vec3(0.0,0.0,0.0);
        Ks = vec3(0.0,0.0,0.0);
        Ka = vec3(0.2,0.02,0.02);
        q = 1.0;
    }
    else if ( object_id == PLANE )
    {
        opaco = true;
        vec2 texcoordsRepetidas = fract(texcoords*250); // Coordenadas repetidas do plano de chao

        // Coordenadas de textura do plano, obtidas do arquivo OBJ.
        U = texcoordsRepetidas[0];
        V = texcoordsRepetidas[1];

        // Propriedades espectrais do chão
        Kd = vec3(0.1,0.1,0.1);
        Ks = vec3(0.5,0.5,0.5);
        Ka = vec3(0.09,0.01,0.01);
        q = 80.0;

        n = normalize(inverse(transpose(model)) * texture(chao_normal, vec2(U,V)));
    }
    else if ( object_id == ARVORE )
    {
        opaco = true;
        // Propriedades espectrais da lanterna
        Kd = vec3(0.1,0.1,0.1);
        Ks = vec3(0.5,0.5,0.5);
        Ka = vec3(0.09,0.01,0.01);
        q = 30.0;
        U = texcoords.x;
        V = texcoords.y;

        if(tronco)
        {
            vec4 bbox_center = (bbox_min + bbox_max) / 2.0;

            vec4 r = position_model - bbox_center;

            theta = atan(position_model.x,position_model.z);
            phi   = asin(position_model.y/length(r));

            U = (theta+M_PI)/(2*M_PI);
            V = (phi+M_PI_2)/M_PI;
        }
    }
    else if ( object_id == CABINE )
    {
        opaco = true;
        // Propriedades espectrais do chão
        Kd = vec3(0.1,0.1,0.1);
        Ks = texture(cabine_spec, vec2(U,V)).rgb;
        Ka = vec3(0.09,0.01,0.01);
        q = 30.0;

        U = texcoords.x;
        V = texcoords.y;
    }
    else if ( object_id == CARRO )
    {
        opaco = true;
        // Propriedades espectrais do chão
        Kd = vec3(0.1,0.1,0.1);
        Ks = vec3(0.9,0.5,0.5);
        Ka = vec3(0.09,0.01,0.01);
        q = 30.0;

        U = texcoords.x;
        V = texcoords.y;
    }
    else if ( object_id == CARRO )
    {
        // Propriedades espectrais do chão
        Kd = vec3(0.1,0.1,0.1);
        Ks = vec3(0.9,0.5,0.5);
        Ka = vec3(0.09,0.01,0.01);
        q = 30.0;

        U = texcoords.x;
        V = texcoords.y;
    }
    // == JOGADOR ==
    else if ( object_id == FLASHLIGHT )
    {
        opaco = true;
        // Propriedades espectrais da lanterna
        Kd = vec3(0.0,0.0,0.0);
        Ks = vec3(0.0,0.0,0.0);
        Ka = vec3(1,1,1);
        q = 1.0;
        U = texcoords.x;
        V = texcoords.y;
    }
    else if ( object_id == REVOLVER )
    {
        opaco = true;
        // Propriedades espectrais do revolver
        Kd = vec3(0.0,0.0,0.0);
        Ks = vec3(0.0,0.0,0.0);
        Ka = vec3(1,1,1);
        q = 1.0;
        U = texcoords.x;
        V = texcoords.y;
    }
    else if ( object_id == SCREEN )
    {
        // Coordenadas de textura da tela, obtidas do arquivo OBJ.
        U = texcoords.x;
        V = texcoords.y;
    }
    else if ( object_id == BULLET )
    {
        opaco = true;
        // Propriedades espectrais da bala
        Kd = vec3(0.6,0.3,0.3);
        Ks = vec3(0.9,0.5,0.5);
        Ka = vec3(0.3,0.1,0.1);
        q = 10.0;
    }
    else if ( object_id == SMOKE )
    {
        // Seleciona qual textura da partícula de fumaça usar, baseado na sua vida
        if(smoke_life <= 8)
        {
            U = (texcoords.x/8)+smoke_life*0.125;
            V = (texcoords.y/8)+0.875f;
        }
        else if(smoke_life <= 16&&smoke_life > 8)
        {
            U = (texcoords.x/8)+(smoke_life-8)*0.125;
            V = (texcoords.y/8)+0.75f;
        }
        else if(smoke_life <= 32&&smoke_life > 16)
        {
            U = (texcoords.x/8)+(smoke_life-16)*0.125;
            V = (texcoords.y/8)+0.625f;
        }
        else if(smoke_life <= 40&&smoke_life > 32)
        {
            U = (texcoords.x/8)+(smoke_life-32)*0.125;
            V = (texcoords.y/8)+0.5f;
        }
        else if(smoke_life > 40)
        {
            U = (texcoords.x/8)+(smoke_life-40)*0.125;
            V = (texcoords.y/8)+0.375f;
        }
    }
    // == FANTASMAS ==
    else if ( object_id == SKULL )
    {
        U = texcoords.x;
        V = texcoords.y;
        // Propriedades espectrais da caveira
        Kd = vec3(0.8,0.8,0.8);
        Ks = vec3(0.8,0.8,0.8);
        Ka = vec3(1,1,1);
        q = 25.0;
    }
    else if ( object_id == EYE )
    {
        // Propriedades espectrais dos olhos
        Kd = vec3(0.3,0.3,0.3);
        Ks = vec3(0.3,0.3,0.3);
        Ka = vec3(0.01,0.01,0.01);
        q = 25.0;
    }

    // Espectro da fonte de iluminação
    vec3 I = vec3(1.0,1.0,1.0); // Espectro da fonte de luz

    // Espectro da luz ambiente
    vec3 Ia = vec3(0.1,0.1,0.1); // Espectro da luz ambiente

    // Termo difuso utilizando a lei dos cossenos de Lambert
    vec3 lambert_diffuse_term = Kd * I * max(0, dot(n,l)); // Termo difuso de Lambert

    // Termo ambiente
    vec3 ambient_term = Ka * Ia; // Termo ambiente

    // Termo especular utilizando o modelo de iluminação de Phong. Para Mapa especular = substituir 'Ks'
    vec3 phong_specular_term  = Ks * I * pow(max(0,dot(r,v)), q) * max(0, dot(n,l)); // Termo especular de Phong

    // Define se a lanterna está ligada.
    if (lanterna_ligada==1)
        potencia_lanterna = 25;
    else
        potencia_lanterna = 0;

    // Define a iluminação dos objetos pela lanterna (A = Ambiente, D = Difusa, S = Difusa + Especular)
    vec3 A = ambient_term;
    vec3 D = lambert_diffuse_term * 0.5 * luz_lanterna(l, sv, potencia_lanterna);
    vec3 S = (lambert_diffuse_term * 0.5 + phong_specular_term) * luz_lanterna(l, sv, potencia_lanterna);
    vec3 NF = ambient_term*nozzle_flash*5;

    // Define a textura dos objetos opacos
    if(opaco)
        color.a = 1;

    // == CENÁRIO ==
    if( object_id == SPHERE )
        color.rgb = texture(ceu, vec2(U,V)).rgb*(2*A);
    else if( object_id == PLANE )
        color.rgb = texture(chao, vec2(U,V)).rgb*(A+D+NF);
    else if( object_id == CABINE )
        color.rgb = texture(cabine_diff, vec2(U,V)).rgb*(A+D+S+NF);
    else if( object_id == CARRO )
        color.rgb = texture(carro_BP_diff, vec2(U,V)).rgb*(A+D+S+NF);
    else if( object_id == C_VIDROS )
        color.rgb = vec3(0.0f,0.0f,0.0f)+D+S+NF;
    else if( object_id == ARVORE && tronco )
        color.rgb = texture(bark, vec2(U,V)).rgb*(A+D+NF);
    else if( object_id == ARVORE && tronco == false )
    {
            color.rgb = texture(folhas, vec2(U,V)).rgb*(A+D+NF);
            if(texture(folhas, vec2(U,V)).r > 0.3)
                discard;
    }

    // == JOGADOR ==
    else if( object_id == FLASHLIGHT )
        color.rgb = texture(lanterna, vec2(U,V)).rgb*(3*A+NF);
    else if( object_id == REVOLVER )
        color.rgb = texture(lanterna, vec2(U,V)).rgb*(3*A+NF);
    else if( object_id == BULLET )
        color.rgb = vec3(1.0f,1.0f,0.0f)*(A+D+S);
    else if( object_id == SMOKE )
    {
        color.rgb = texture(smoke, vec2(U,V)).rgb;
        color.a = 0.2;
        if(color.r > 0.2&&color.b < 0.1)
            discard;
    }
    else if( object_id == SCREEN ){
        color.rgb = texture(crosshair, vec2(U,V)).rgb;
        if (color.r < 0.1f)
            discard;
    }

    // == FANTASMAS ==
    else if( object_id == SKULL )
    {
        color.rgb = texture(skull_diff, vec2(U,V)).rgb*(S+D+NF);
        color.a = nozzle_flash+0.2*luz_lanterna(l, sv, potencia_lanterna);
    }
    else if( object_id == EYE )
    {
        color.rgb = vec3(0.9f,0.9f,0.0f)*(0.1-S-D);
        color.a = 0.2+luz_lanterna(l, sv, potencia_lanterna);
    }

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
