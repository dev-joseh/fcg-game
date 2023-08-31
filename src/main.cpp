//     Universidade Federal do Rio Grande do Sul
//             Instituto de Informática
//       Departamento de Informática Aplicada
//
//    INF01047 Fundamentos de Computação Gráfica
//               Prof. Eduardo Gastal
//
//                   JOGO DE FCG
//

// Arquivos "headers" padrões de C podem ser incluídos em um
// programa C++, sendo necessário somente adicionar o caractere
// "c" antes de seu nome, e remover o sufixo ".h". Exemplo:
//    #include <stdio.h> // Em C
//  vira
//    #include <cstdio> // Em C++
//
#include <iostream>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <stdlib.h>     /* srand, rand */
#include <time.h>       /* time */

// Headers abaixo são específicos de C++
#include <map>
#include <stack>
#include <string>
#include <vector>
#include <limits>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <algorithm>

// Headers das bibliotecas OpenGL
#include <glad/glad.h>   // Criação de contexto OpenGL 3.3
#include <GLFW/glfw3.h>  // Criação de janelas do sistema operacional

// Headers da biblioteca GLM: criação de matrizes e vetores.
#include <glm/mat4x4.hpp>
#include <glm/vec4.hpp>
#include <glm/gtc/type_ptr.hpp>

// Headers da biblioteca para carregar modelos obj
#include <tiny_obj_loader.h>
#include <stb_image.h>

// Headers locais, definidos na pasta "include/"
#include "utils.h"
#include "matrices.h"

// Incluimos o arquivo de testes de colisões
#include "collisions.h"
#include "collisions.cpp"

#define PI 3.14159265359f

inline const char * const BoolToString(bool b)
{
  return b ? "true" : "false";
}

// Estrutura que representa um modelo geométrico carregado a partir de um
// arquivo ".obj". Veja https://en.wikipedia.org/wiki/Wavefront_.obj_file .
struct ObjModel
{
    tinyobj::attrib_t                 attrib;
    std::vector<tinyobj::shape_t>     shapes;
    std::vector<tinyobj::material_t>  materials;

    // Este construtor lê o modelo de um arquivo utilizando a biblioteca tinyobjloader.
    // Veja: https://github.com/syoyo/tinyobjloader
    ObjModel(const char* filename, const char* basepath = NULL, bool triangulate = true)
    {
        printf("Carregando objetos do arquivo \"%s\"...\n", filename);

        // Se basepath == NULL, então setamos basepath como o dirname do
        // filename, para que os arquivos MTL sejam corretamente carregados caso
        // estejam no mesmo diretório dos arquivos OBJ.
        std::string fullpath(filename);
        std::string dirname;
        if (basepath == NULL)
        {
            auto i = fullpath.find_last_of("/");
            if (i != std::string::npos)
            {
                dirname = fullpath.substr(0, i+1);
                basepath = dirname.c_str();
            }
        }

        std::string warn;
        std::string err;
        bool ret = tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, filename, basepath, triangulate);

        if (!err.empty())
            fprintf(stderr, "\n%s\n", err.c_str());

        if (!ret)
            throw std::runtime_error("Erro ao carregar modelo.");

        for (size_t shape = 0; shape < shapes.size(); ++shape)
        {
            if (shapes[shape].name.empty())
            {
                fprintf(stderr,
                        "*********************************************\n"
                        "Erro: Objeto sem nome dentro do arquivo '%s'.\n"
                        "Veja https://www.inf.ufrgs.br/~eslgastal/fcg-faq-etc.html#Modelos-3D-no-formato-OBJ .\n"
                        "*********************************************\n",
                    filename);
                throw std::runtime_error("Objeto sem nome.");
            }
            printf("- Objeto '%s'\n", shapes[shape].name.c_str());
        }

        printf("OK.\n");
    }
};

typedef struct ammo
{
    // Variáveis que definem a posição, orientação, tempo de atividade e rotação das balas.
    glm::vec4 pos, orientacao;
    float rotacao, timer;
    bool ativa;
    Esfera bounding_sphere;
    AABB aabb;

    ammo() : pos(glm::vec4(0.0f)), rotacao(rotacao), timer(timer), ativa(ativa), bounding_sphere(glm::vec4(0.0f), 0.05f), aabb(glm::vec3(0.0f), glm::vec3(0.0f)) {}

} AMMO;

typedef struct jogador
{
    // Variáveis que definem o jogador
    glm::vec4 pos, camera;
    int vidas;
    int ammo;
    AABB aabb;
    Esfera bounding_sphere;

    jogador() : pos(glm::vec4(0.0f)), camera(glm::vec4(0.0f)), vidas(0), ammo(0), aabb(glm::vec3(0.0f), glm::vec3(0.0f)), bounding_sphere(bounding_sphere) {}

} JOGADOR;

typedef struct monstro
{
    // Variáveis que definem a posição dos monstros
    glm::vec4 pos, orientacao;
    float rotacao;
    bool vivo;
    Esfera bounding_sphere;
    AABB aabb;

    monstro() : pos(glm::vec4(0.0f)), orientacao(glm::vec4(0.0f)), rotacao(rotacao), vivo(vivo), bounding_sphere(glm::vec4(0.0f), 0.05f), aabb(glm::vec3(0,0,0), glm::vec3(0,0,0)) {}

} MONSTRO;

typedef struct carro
{
    // Variáveis que definem o carro que o jogador precisa consertar
    glm::vec4 pos;
    float estado;
    AABB aabb;

    carro() : pos(glm::vec4(0.0f)), estado(estado), aabb(glm::vec3(0.0f), glm::vec3(0.0f)) {}

} CAR;

typedef struct arvore
{
    // Variáveis que definem a posição das arvores
    glm::vec4 pos;
    float rotacao;
    AABB aabb;

    arvore() : pos(glm::vec4(0.0f)), rotacao(rotacao), aabb(glm::vec3(0.0f), glm::vec3(0.0f)) {}

} ARVORE;

typedef struct Particle {
    glm::vec2 pos, vel;
    glm::vec4 color;
    int     life;
    float scale;

    Particle() : pos(0.0f), vel(0.0f), color(1.0f), life(0.0f), scale(1.0) {}

} PARTICLE;

// Declaração de funções utilizadas para pilha de matrizes de modelagem.
void PushMatrix(glm::mat4 M);
void PopMatrix(glm::mat4& M);

// Declaração de várias funções utilizadas em main().  Essas estão definidas
// logo após a definição de main() neste arquivo.
void BuildTrianglesAndAddToVirtualScene(ObjModel*); // Constrói representação de um ObjModel como malha de triângulos para renderização
void ComputeNormals(ObjModel* model); // Computa normais de um ObjModel, caso não existam.
void LoadShadersFromFiles(); // Carrega os shaders de vértice e fragmento, criando um programa de GPU
void LoadTextureImage(const char* filename); // Função que carrega imagens de textura
void DrawVirtualObject(const char* object_name); // Desenha um objeto armazenado em g_VirtualScene
GLuint LoadShader_Vertex(const char* filename);   // Carrega um vertex shader
GLuint LoadShader_Fragment(const char* filename); // Carrega um fragment shader
void LoadShader(const char* filename, GLuint shader_id); // Função utilizada pelas duas acima
GLuint CreateGpuProgram(GLuint vertex_shader_id, GLuint fragment_shader_id); // Cria um programa de GPU
void PrintObjModelInfo(ObjModel*); // Função para debugging

// Declaração de funções auxiliares para renderizar texto dentro da janela
// OpenGL. Estas funções estão definidas no arquivo "textrendering.cpp".
void TextRendering_Init();
float TextRendering_LineHeight(GLFWwindow* window);
float TextRendering_CharWidth(GLFWwindow* window);
void TextRendering_PrintString(GLFWwindow* window, const std::string &str, float x, float y, float scale = 1.0f);
void TextRendering_PrintMatrix(GLFWwindow* window, glm::mat4 M, float x, float y, float scale = 1.0f);
void TextRendering_PrintVector(GLFWwindow* window, glm::vec4 v, float x, float y, float scale = 1.0f);
void TextRendering_PrintMatrixVectorProduct(GLFWwindow* window, glm::mat4 M, glm::vec4 v, float x, float y, float scale = 1.0f);
void TextRendering_PrintMatrixVectorProductMoreDigits(GLFWwindow* window, glm::mat4 M, glm::vec4 v, float x, float y, float scale = 1.0f);
void TextRendering_PrintMatrixVectorProductDivW(GLFWwindow* window, glm::mat4 M, glm::vec4 v, float x, float y, float scale = 1.0f);


// Funções abaixo renderizam como texto na janela OpenGL algumas matrizes e
// outras informações do programa. Definidas após main().
void TextRendering_ShowModelViewProjection(GLFWwindow* window, glm::mat4 projection, glm::mat4 view, glm::mat4 model, glm::vec4 p_model);
void TextRendering_ShowProjection(GLFWwindow* window);
void TextRendering_ShowFramesPerSecond(GLFWwindow* window);
void TextRendering_ShowCarTip(GLFWwindow* window, float estado_carro);

// Funções callback para comunicação com o sistema operacional e interação do
// usuário. Veja mais comentários nas definições das mesmas, abaixo.
void FramebufferSizeCallback(GLFWwindow* window, int width, int height);
void ErrorCallback(int error, const char* description);
void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mode);
void MouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
void CursorPosCallback(GLFWwindow* window, double xpos, double ypos);
void ScrollCallback(GLFWwindow* window, double xoffset, double yoffset);

// FUNÇÕES CRIADAS:
void centerMouse(GLFWwindow* window, int screenWidth, int screenHeight); // Centraliza o mouse na tela
void TextRendering_ShowSecondsEllapsed(GLFWwindow* window); // Mostra quandos segundos se passaram
void TextRendering_ShowAMMO(GLFWwindow* window, int ammo);  // Mostra a munição do jogador
void TextRendering_Menu(GLFWwindow* window, int mov_escrita);
glm::vec4 calculateBezierPoint(const glm::vec4& P0, const glm::vec4& P1, const glm::vec4& P2, const glm::vec4& P3, const glm::vec4& P4, const glm::vec4& P5, float t);
//bool BoundingBoxIntersection (SceneObject &objeto1, SceneObject &objeto2);

// Definimos uma estrutura que armazenará dados necessários para renderizar
// cada objeto da cena virtual.
/*struct SceneObject
{
    std::string  name;        // Nome do objeto
    size_t       first_index; // Índice do primeiro vértice dentro do vetor indices[] definido em BuildTrianglesAndAddToVirtualScene()
    size_t       num_indices; // Número de índices do objeto dentro do vetor indices[] definido em BuildTrianglesAndAddToVirtualScene()
    GLenum       rendering_mode; // Modo de rasterização (GL_TRIANGLES, GL_TRIANGLE_STRIP, etc.)
    GLuint       vertex_array_object_id; // ID do VAO onde estão armazenados os atributos do modelo
    glm::vec3    bbox_min; // Axis-Aligned Bounding Box do objeto
    glm::vec3    bbox_max;
};*/

// Abaixo definimos variáveis globais utilizadas em várias funções do código.

// A cena virtual é uma lista de objetos nomeados, guardados em um dicionário
// (map).  Veja dentro da função BuildTrianglesAndAddToVirtualScene() como que são incluídos
// objetos dentro da variável g_VirtualScene, e veja na função main() como
// estes são acessados.
std::map<std::string, SceneObject> g_VirtualScene;


// Pilha que guardará as matrizes de modelagem.
std::stack<glm::mat4>  g_MatrixStack;

// Razão de proporção da janela (largura/altura). Veja função FramebufferSizeCallback().
float g_ScreenRatio = 1.0f;

// Ângulos de Euler que controlam a rotação de um dos cubos da cena virtual
float g_AngleX = 0.0f;
float g_AngleY = 0.0f;
float g_AngleZ = 0.0f;

// "g_LeftMouseButtonPressed = true" se o usuário está com o botão esquerdo do mouse
// pressionado no momento atual. Veja função MouseButtonCallback().
bool g_LeftMouseButtonPressed = false;
bool g_RightMouseButtonPressed = false; // Análogo para botão direito do mouse
bool g_MiddleMouseButtonPressed = false; // Análogo para botão do meio do mouse

// Variáveis que definem a câmera em coordenadas esféricas, controladas pelo
// usuário através do mouse (veja função CursorPosCallback()). A posição
// efetiva da câmera é calculada dentro da função main(), dentro do loop de
// renderização.
float g_CameraTheta = 0.0f; // Ângulo no plano ZX em relação ao eixo Z
float g_CameraPhi = 0.0f;   // Ângulo em relação ao eixo Y
float g_CameraDistance = 3.5f; // Distância da câmera para a origem

// ------ Variáveis para comandos do jogador
float delta_t;

bool tecla_W_pressionada = false;
bool tecla_A_pressionada = false;
bool tecla_S_pressionada = false;
bool tecla_D_pressionada = false;
bool tecla_F_pressionada = false;     // Lanterna (ligar/desligar)
bool tecla_R_pressionada = false;     // Recarrega arma
bool tecla_E_pressionada = false;     // Conserta o carro
bool tecla_SPACE_pressionada = false; // Pulo
bool tecla_SHIFT_pressionada = false; // Corrida

bool tecla_K_pressionada = false; // TESTE PRA TELA FINAL DO JOGO, SERÁ ARRUMADO

float g_Theta = PI / 4;
float g_Phi = PI / 6;

double g_LastCursorPosX, g_LastCursorPosY;
// Usada para verificar se o mouse está centralizado.
bool cursorCentered;

// Outras variáveis
bool jogador_andando;
float movimento_crosshair;
bool lanterna_ligada;
int bala_atual=0;
bool reload_active = false;
bool jogador_proximo_do_carro;

// Variáveis que controlam rotação do antebraço
float g_ForearmAngleZ = 0.0f;
float g_ForearmAngleX = 0.0f;

// Variáveis que controlam translação do torso
float g_TorsoPositionX = 0.0f;
float g_TorsoPositionY = 0.0f;

// Variável que controla o tipo de projeção utilizada: perspectiva ou ortográfica.
bool g_UsePerspectiveProjection = true;

// Variável que controla se o texto informativo será mostrado na tela.
bool g_ShowInfoText = true;

// Variáveis que definem um programa de GPU (shaders). Veja função LoadShadersFromFiles().
GLuint g_GpuProgramID = 0;
GLint g_model_uniform;
GLint g_view_uniform;
GLint g_projection_uniform;
GLint g_object_id_uniform;
GLint g_bbox_min_uniform;
GLint g_bbox_max_uniform;
// Variáveis que eu criei para enviar para o fragment shader
GLint lanterna_ligada_uniform;
GLint smoke_life_uniform;
GLint nozzle_flash_uniform;
GLint tronco_uniform;
GLint parte_carro_uniform;
GLint tela_de_menu_uniform;

// Número de texturas carregadas pela função LoadTextureImage()
GLuint g_NumLoadedTextures = 0;

int main(int argc, char* argv[])
{
    // Inicializamos a biblioteca GLFW, utilizada para criar uma janela do
    // sistema operacional, onde poderemos renderizar com OpenGL.
    int success = glfwInit();
    if (!success)
    {
        fprintf(stderr, "ERROR: glfwInit() failed.\n");
        std::exit(EXIT_FAILURE);
    }

    // Definimos o callback para impressão de erros da GLFW no terminal
    glfwSetErrorCallback(ErrorCallback);

    // Pedimos para utilizar OpenGL versão 3.3 (ou superior)
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);

    #ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    #endif

    // Pedimos para utilizar o perfil "core", isto é, utilizaremos somente as
    // funções modernas de OpenGL.
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // Criamos uma janela do sistema operacional, com 800 colunas e 600 linhas
    // de pixels, e com título "INF01047 ...".
    GLFWwindow* window;
    window = glfwCreateWindow(1200, 800, "FCG GAME", NULL, NULL);
    if (!window)
    {
        glfwTerminate();
        fprintf(stderr, "ERROR: glfwCreateWindow() failed.\n");
        std::exit(EXIT_FAILURE);
    }

    // Definimos a função de callback que será chamada sempre que o usuário
    // pressionar alguma tecla do teclado ...
    glfwSetKeyCallback(window, KeyCallback);
    // ... ou clicar os botões do mouse ...
    glfwSetMouseButtonCallback(window, MouseButtonCallback);
    // ... ou movimentar o cursor do mouse em cima da janela ...
    glfwSetCursorPosCallback(window, CursorPosCallback);
    // Centraliza a posição do mouse na tela
    centerMouse(window, 1200, 800);
    // Esconde o cursor do mouse equanto estiver dentro da tela
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // Indicamos que as chamadas OpenGL deverão renderizar nesta janela
    glfwMakeContextCurrent(window);

    // Carregamento de todas funções definidas por OpenGL 3.3, utilizando a
    // biblioteca GLAD.
    gladLoadGLLoader((GLADloadproc) glfwGetProcAddress);

    // Definimos a função de callback que será chamada sempre que a janela for
    // redimensionada, por consequência alterando o tamanho do "framebuffer"
    // (região de memória onde são armazenados os pixels da imagem).
    glfwSetFramebufferSizeCallback(window, FramebufferSizeCallback);
    FramebufferSizeCallback(window, 1200, 800); // Forçamos a chamada do callback acima, para definir g_ScreenRatio.

    // Imprimimos no terminal informações sobre a GPU do sistema
    const GLubyte *vendor      = glGetString(GL_VENDOR);
    const GLubyte *renderer    = glGetString(GL_RENDERER);
    const GLubyte *glversion   = glGetString(GL_VERSION);
    const GLubyte *glslversion = glGetString(GL_SHADING_LANGUAGE_VERSION);

    printf("GPU: %s, %s, OpenGL %s, GLSL %s\n", vendor, renderer, glversion, glslversion);

    // Carregamos os shaders de vértices e de fragmentos que serão utilizados
    // para renderização. Veja slides 180-200 do documento Aula_03_Rendering_Pipeline_Grafico.pdf.
    //
    LoadShadersFromFiles();

    // Carregamos duas imagens para serem utilizadas como textura
    LoadTextureImage("../../data/Textures/chao.jpg");                         // chao_diff
    LoadTextureImage("../../data/Textures/ceu.hdr");                          // ceu
    LoadTextureImage("../../data/Textures/flashlight_H.jpg");                 // lanterna
    LoadTextureImage("../../data/Textures/CrossHair.png");                    // crosshair
    LoadTextureImage("../../data/Textures/chao_normal.jpg");                  // chao_normal

    // Fantasmas
    LoadTextureImage("../../data/Textures/skull_diff.png");                   // skull_diff
    LoadTextureImage("../../data/Textures/skull_nm.png");                     // skull_normal

    // Partículas de fumaça
    LoadTextureImage("../../data/Textures/smoke.png");                        // smoke

    // Árvores
    LoadTextureImage("../../data/Textures/bark1.jpg");                        // bark
    LoadTextureImage("../../data/Textures/leaf.png");                         // folhas

    // Cabine
    LoadTextureImage("../../data/Textures/WoodCabin.jpg");                    // cabin_diff
    LoadTextureImage("../../data/Textures/WoodCabinNM.jpg");                  // cabin_normal
    LoadTextureImage("../../data/Textures/WoodCabinSM.jpg");                  // cabin_spec

    // Texturas do carro
    LoadTextureImage("../../data/Textures/car_tex/Backlight1.jpg");           // car_BL1
    LoadTextureImage("../../data/Textures/car_tex/Backlight2.jpg");           // car_BL2
    LoadTextureImage("../../data/Textures/car_tex/GuidLight.jpg");            // car_GL
    LoadTextureImage("../../data/Textures/car_tex/Headlight.jpg");            // car_HL
    LoadTextureImage("../../data/Textures/car_tex/LightBelow.jpg");           // car_BL
    LoadTextureImage("../../data/Textures/car_tex/Plaque.png");               // car_Plaque
    LoadTextureImage("../../data/Textures/car_tex/SamandLogo.png");           // car_Logo
    LoadTextureImage("../../data/Textures/car_tex/Tire.png");                 // car_Tire

    // Tela de fim de jogo
    LoadTextureImage("../../data/Textures/tela_fim_de_jogo.png");             // tela final

    // Construímos a representação de objetos geométricos através de malhas de triângulos
    ObjModel spheremodel("../../data/Objects/sphere.obj");
   // PrintObjModelInfo(&spheremodel);
    ComputeNormals(&spheremodel);
    BuildTrianglesAndAddToVirtualScene(&spheremodel);

    ObjModel planemodel("../../data/Objects/plane.obj");
  //  PrintObjModelInfo(&planemodel);
    ComputeNormals(&planemodel);
    BuildTrianglesAndAddToVirtualScene(&planemodel);

    ObjModel flashlightmodel("../../data/Objects/flashlight.obj");
   // PrintObjModelInfo(&flashlightmodel);
    ComputeNormals(&flashlightmodel);
    BuildTrianglesAndAddToVirtualScene(&flashlightmodel);

    ObjModel revolvermodel("../../data/Objects/revolver.obj");
    ComputeNormals(&revolvermodel);
    BuildTrianglesAndAddToVirtualScene(&revolvermodel);

    ObjModel screenmodel("../../data/Objects/screen.obj");
    //PrintObjModelInfo(&screenmodel);
    ComputeNormals(&screenmodel);
    BuildTrianglesAndAddToVirtualScene(&screenmodel);

    ObjModel skullmodel("../../data/Objects/skull.obj");
    //PrintObjModelInfo(&skullmodel);
    ComputeNormals(&skullmodel);
    BuildTrianglesAndAddToVirtualScene(&skullmodel);

    ObjModel eyemodel("../../data/Objects/eye.obj");
    ComputeNormals(&eyemodel);
    BuildTrianglesAndAddToVirtualScene(&eyemodel);

    ObjModel bulletmodel("../../data/Objects/bullet.obj");
    ComputeNormals(&bulletmodel);
    BuildTrianglesAndAddToVirtualScene(&bulletmodel);

    ObjModel treesmodel("../../data/Objects/trees.obj");
    ComputeNormals(&treesmodel);
    BuildTrianglesAndAddToVirtualScene(&treesmodel);

    ObjModel cabinmodel("../../data/Objects/cabin.obj");
    ComputeNormals(&cabinmodel);
    BuildTrianglesAndAddToVirtualScene(&cabinmodel);

    ObjModel carmodel("../../data/Objects/car.obj");
    ComputeNormals(&carmodel);
    BuildTrianglesAndAddToVirtualScene(&carmodel);

    ObjModel tela_final("../../data/Objects/tela_fim_de_jogo.obj");
    ComputeNormals(&tela_final);
    BuildTrianglesAndAddToVirtualScene(&tela_final);

    if ( argc > 1 )
    {
        ObjModel model(argv[1]);
        BuildTrianglesAndAddToVirtualScene(&model);
    }

    // Inicializamos o código para renderização de texto.
    TextRendering_Init();

    // Habilitamos o Z-buffer. Veja slides 104-116 do documento Aula_09_Projecoes.pdf.
    glEnable(GL_DEPTH_TEST);

    // Habilitamos o Backface Culling. Veja slides 23-34 do documento Aula_13_Clipping_and_Culling.pdf e slides 112-123 do documento Aula_14_Laboratorio_3_Revisao.pdf.
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);

    srand(time(NULL));

    # define N_MONSTROS     5        // Monstros teste no mapa
    # define N_AMMO         6        // Munição máxima do revolver
    # define NUM_ARVORES   30        // Quantidade de árvores no mapa
    # define ARVORES_DIST  15.0f     // Distância das árvores da cabine
    # define SPEED_BASE     3.0f     // Velocidade base para algumas variáveis

    // Definimos a posição inicial do jogador, da câmera e a quantidade de vidas e munições
    JOGADOR jogador;
    jogador.pos = glm::vec4(-2.0f, 1.0f, 0.0f, 1.0f);
    jogador.camera = glm::vec4(0.0f, 2.4f, 0.0f, 1.0f);
    jogador.ammo = 6;
    jogador.vidas = 3;

    // Definimos o plano do chão para os testes de colisão
               // Seu vetor normal          // Sua posição
    Plano chao(glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(0.0f, 0.0f, 0.0f));

   /* for (const auto& entry : g_VirtualScene) {
        const std::string& objectName = entry.first;
        const SceneObject& sceneObject = entry.second;

        std::cout << "Object Name: " << objectName << std::endl;
        std::cout << "First Index: " << sceneObject.first_index << std::endl;
        std::cout << "Number of Indices: " << sceneObject.num_indices << std::endl;
        // Imprimir outras informações do objeto, se necessário

        // Chame a função para imprimir as informações do ObjModel

        std::cout << std::endl; // Espaço entre os objetos
    }*/

    // Definimos a matriz monstro que irá guardar informações dos monstros
    MONSTRO monstro[N_MONSTROS];
    for(int i=0; i<N_MONSTROS; i++)
    {
        monstro[i].pos = {rand()%100, (rand()%10*0.1+0.1)*1.4f, rand()%100,1.0f};
        monstro[i].orientacao = {0.0f, 0.0f, 0.0f, 0.0f};
    }

    MONSTRO monstro_bezier;
    monstro_bezier.pos = glm::vec4(5.0f, 5.0f, 1.0f, 1.0f);
    monstro_bezier.orientacao = {0.0f, 0.0f, 0.0f, 0.0f};


    CAR carro;
    carro.pos = {6.0f, 0.0f, 0.0f, 1.0f};
    carro.estado = 0.0f;

    // Definimos a matriz ammo que irá guardar informações sobre a munição
    AMMO ammo[N_AMMO];
    for(int i=0; i<N_AMMO; i++)
        ammo[i].ativa = false;
    float cooldown_tiro = 0.0f; // Atraso entre cada tiro
    float reload_delay = 0.0f;  // Atraso entre o recarregamento de cada bala
    float reload_move = 0.0f;   // Movimento do revolver durante o recarregamento
    float recoil = 0.0f;        // Movimento do revolver durante o recuo do tiro
    bool recoil_active = false;

    // Árvores
    // Desenhamos as árvores em um círculo com raio = ARVORES_DIST ao redor da cabine e do carro
    ARVORE arvores[NUM_ARVORES];
    for(int i=0; i<NUM_ARVORES; i++)
    {
        float angulo=i*(2*PI/NUM_ARVORES);
        arvores[i].pos = glm::vec4(cos(angulo)*ARVORES_DIST, 0.0f, sin(angulo)*ARVORES_DIST, 1.0f);
        arvores[i].rotacao = rand()%6*(2*PI/6);
    }

    // Cenário AABBs
    std::vector<AABB> cenario;

    // Carro
    //cenario.push_back(AABB(glm::vec3(5.0f, 0.0f, -2.5f), glm::vec3(7.0f, 2.0f, 2.75f)));
    //cenario.push_back(BoundingBoxIntersection(g_VirtualScene["Plaque"], g_VirtualScene["the_sphere"])))
    // Casa chão
    //cenario.push_back(AABB(glm::vec3(-3.0f, 0.0f, -3.3f), glm::vec3(3.3f, 0.65f, 3.3f)));
    // Casa parede sul
    //cenario.push_back(AABB(glm::vec3(-3.0f, 0.0f, -3.3f), glm::vec3(3.0f, 5.0f, -2.95f)));
    // Casa parede leste
    //cenario.push_back(AABB(glm::vec3(-2.9f, 0.0f, -3.0f), glm::vec3(-2.0f, 4.0f, 3.0f)));
    /*
    // Casa parede oeste
    cenario.push_back(AABB(glm::vec3(-3.0f, 0.0f, -3.0f), glm::vec3(3.0f, 0.65f, 3.0f)));

    // Casa parede norte/esquerda da porta
    cenario[NUM_ARVORES+5] =
    // Casa parede norte/direita da porta
    cenario[NUM_ARVORES+6] =
    // Casa escadaria
    cenario[NUM_ARVORES+7] =
    */

    // Definimos variáveis de partícula
    #define SMOKE_P_COUNT 6
    PARTICLE smoke[SMOKE_P_COUNT];
    bool smoke_active=false;
    float smoke_timer=0.0f;

    // Variáveis auxiliares no efeito de caminhada
    bool shake_cima=false;
    bool _fullscreen=false;

    // Definir velocidade e tempo para câmera livre
    float speed, speed_base = SPEED_BASE;  // Velocidade base para os monstros, jogador e para algumas variáveis do revolver
    float Yspeed, gravity = 3.0f;         // Aceleração da queda
    float prev_time = (float)glfwGetTime();

    // Variáveis menu
    bool sair_menu = false;
    float camera_pos;
    int mov_escrita = 0;



    //Variaveis de fim de jogo
    bool fim_de_jogo = false;

    for (const auto& entry : g_VirtualScene) {
        const std::string& objectName = entry.first; // Chave (nome do objeto)
        const SceneObject& sceneObj = entry.second;  // Valor (estrutura SceneObject)

        const glm::vec3& bboxMin = sceneObj.bbox_min;
        const glm::vec3& bboxMax = sceneObj.bbox_max;

        std::cout << "Object Name: " << objectName << std::endl;
        std::cout << "Bounding Box Min: (" << bboxMin.x << ", " << bboxMin.y << ", " << bboxMin.z << ")" << std::endl;
        std::cout << "Bounding Box Max: (" << bboxMax.x << ", " << bboxMax.y << ", " << bboxMax.z << ")" << std::endl;
}


    // Ficamos em um loop infinito, renderizando, até que o usuário feche a janela
    while (!glfwWindowShouldClose(window))
    {
        glUniform1i(tela_de_menu_uniform, true);
        if (tecla_SPACE_pressionada)       // Pressionar espaço p sair do menu e começar o jogo
            sair_menu = true;

        glfwSetWindowMonitor(window, _fullscreen ? glfwGetPrimaryMonitor() : NULL, 0, 0, 4000, 4000, GLFW_DONT_CARE);
        // Variáveis de tempo
        float current_time = (float)glfwGetTime();
        delta_t = current_time - prev_time;
        prev_time = current_time;

        glClearColor(1.0f, 1.0f, 1.0f, 1.0f);

        // "Pintamos" todos os pixels do framebuffer com a cor definida acima,
        // e também resetamos todos os pixels do Z-buffer (depth buffer).
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Pedimos para a GPU utilizar o programa de GPU criado acima (contendo
        // os shaders de vértice e fragmentos).
        glUseProgram(g_GpuProgramID);

        // Computamos a posição da câmera utilizando coordenadas esféricas.  As
        // variáveis g_CameraDistance, g_CameraPhi, e g_CameraTheta são
        // controladas pelo mouse do usuário. Veja as funções CursorPosCallback()
        // e ScrollCallback().

        //posição da camera look-at
        #define CAMERA_VEL 0.5f
        #define CAMERA_H   0.707f
        camera_pos += delta_t*CAMERA_VEL;
        float r = g_CameraDistance;
        float y = r*sin(CAMERA_H);
        float z = r*cos(CAMERA_H)*cos(camera_pos);
        float x = r*cos(CAMERA_H)*sin(camera_pos);

        // Variáveis de câmera Look-At
        glm::vec4 camera_position_c  = glm::vec4(4*x,4*y,4*z,1.0f); // Ponto "c", centro da câmera
        glm::vec4 camera_lookat_l    = glm::vec4(0.0f,0.0f,0.0f,1.0f); // Ponto "l", para onde a câmera (look-at) estará sempre olhando
        glm::vec4 camera_view_vector = camera_lookat_l - camera_position_c; // Vetor "view", sentido para onde a câmera está virada
        glm::vec4 camera_up_vector   = glm::vec4(0.0f,1.0f,0.0f,0.0f); // Vetor "up" fixado para apontar para o "céu" (eito Y global)

        // Computamos a matriz "View" utilizando os parâmetros da câmera para definir o sistema de coordenadas da câmera.
        glm::mat4 view = Matrix_Camera_View(camera_position_c, camera_view_vector, camera_up_vector);
        glUniformMatrix4fv(g_view_uniform       , 1 , GL_FALSE , glm::value_ptr(view));

        // Definimos a matriz de projeção como perspectiva
        float field_of_view = PI / 3.0f;
        float nearplane = -0.1f;  // Posição do "near plane"
        float farplane  = -30.0f; // Posição do "far plane"
        glm::mat4 perspective = Matrix_Perspective(field_of_view, g_ScreenRatio, nearplane, farplane);
        glUniformMatrix4fv(g_projection_uniform , 1 , GL_FALSE , glm::value_ptr(perspective));

        glm::mat4 model = Matrix_Identity(); // Transformação identidade de modelagem

        #define SPHERE 0
        #define BULLET 1
        #define PLANE  2
        #define ARVORE  9
        #define CABINE  10
        #define CARRO  11

        // SPHERE
        glDisable(GL_DEPTH_TEST);
        glDisable(GL_CULL_FACE);
        model = Matrix_Translate(camera_position_c[0], camera_position_c[1], camera_position_c[2]);
        glUniformMatrix4fv(g_model_uniform, 1 , GL_FALSE , glm::value_ptr(model));
        glUniform1i(g_object_id_uniform, SPHERE);
        DrawVirtualObject("the_sphere");
        glEnable(GL_CULL_FACE);
        glEnable(GL_DEPTH_TEST);

        // PLANE
        model = Matrix_Translate(0.0f, 0.0f, 0.0f)
              * Matrix_Scale(350.0f,1.0f,350.0f);
        glUniformMatrix4fv(g_model_uniform, 1 , GL_FALSE , glm::value_ptr(model));
        glUniform1i(g_object_id_uniform, PLANE);
        DrawVirtualObject("the_plane");

        // CABINE
        model = Matrix_Translate(0.0f, 0.0f, 0.0f)
              * Matrix_Scale(0.1f,0.1f,0.1f);
        glUniformMatrix4fv(g_model_uniform, 1 , GL_FALSE , glm::value_ptr(model));
        glUniform1i(g_object_id_uniform, CABINE);
        DrawVirtualObject("WoodCabin");
        DrawVirtualObject("Roof");

        // CARRO & VIDROS
        model = Matrix_Translate(carro.pos[0], 0.0f, carro.pos[2])
              * Matrix_Scale(0.01f,0.01f,0.01f);
        glUniformMatrix4fv(g_model_uniform, 1 , GL_FALSE , glm::value_ptr(model));
        glUniform1i(g_object_id_uniform, CARRO);
        // Body
        glUniform1i(parte_carro_uniform, 1);
        DrawVirtualObject("Body1");
        DrawVirtualObject("Steel");
        DrawVirtualObject("UnderCar");
        DrawVirtualObject("Hood");
        DrawVirtualObject("Body");
        // Vidros
        glUniform1i(parte_carro_uniform, 2);
        DrawVirtualObject("Glass");
        DrawVirtualObject("Plastik");

        DrawVirtualObject("Light1");
        DrawVirtualObject("Light2");
        DrawVirtualObject("Light3");
        // Logo
        glUniform1i(parte_carro_uniform, 3);
        DrawVirtualObject("Logo");
        // Placa
        glUniform1i(parte_carro_uniform, 4);
        DrawVirtualObject("Plaque");
        DrawVirtualObject("Plaque1");
        // Pisca
        glUniform1i(parte_carro_uniform, 5);
        DrawVirtualObject("GuidLight1");
        DrawVirtualObject("GuidLight");
        // Faróis
        glUniform1i(parte_carro_uniform, 6);
        DrawVirtualObject("Light");

        glUniform1i(parte_carro_uniform, 7);
        // Pneus
        glUniform1i(parte_carro_uniform, 8);
        DrawVirtualObject("Tire");
        DrawVirtualObject("Tire1");
        DrawVirtualObject("Tire2");
        DrawVirtualObject("Tire3");

        // ARVORES
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        for(int i=0; i<NUM_ARVORES; i++)
        {
            // TRONCO
            model = Matrix_Translate(arvores[i].pos.x,0.0f,arvores[i].pos.z)
                  * Matrix_Scale(1.0f,1.0f,1.0f)
                  * Matrix_Rotate_Y(arvores[i].rotacao);
            glUniformMatrix4fv(g_model_uniform, 1 , GL_FALSE , glm::value_ptr(model));
            glUniform1i(g_object_id_uniform, ARVORE);
            glUniform1i(tronco_uniform, true);
            DrawVirtualObject("bark1");
            // FOLHAS
            glUniformMatrix4fv(g_model_uniform, 1 , GL_FALSE , glm::value_ptr(model));
            glUniform1i(g_object_id_uniform, ARVORE);
            glUniform1i(tronco_uniform, false);
            DrawVirtualObject("leaves1");
        }
        // Resetamos a matriz View para que os objetos carregados a partir daqui não se movimentem na tela.
        glUniformMatrix4fv(g_view_uniform       , 1 , GL_FALSE , glm::value_ptr(Matrix_Identity()));

        //texto do menu

        TextRendering_Menu(window, mov_escrita);
        mov_escrita++;

        glfwSwapBuffers(window);

        TextRendering_ShowSecondsEllapsed(window);

        // Verificamos com o sistema operacional se houve alguma interação do
        // usuário (teclado, mouse, ...). Caso positivo, as funções de callback
        // definidas anteriormente usando glfwSet*Callback() serão chamadas
        // pela biblioteca GLFW.
        glfwPollEvents();


        // =========================================== GAMEPLAY ===========================================
        while (sair_menu && !glfwWindowShouldClose(window)){

        if (tecla_K_pressionada){
            fim_de_jogo = true;
            break;
        }

        glfwSetWindowMonitor(window, _fullscreen ? glfwGetPrimaryMonitor() : NULL, 0, 0, 4000, 4000, GLFW_DONT_CARE);

        // Serve para mudar a iluminação global durante a gameplay
        glUniform1i(tela_de_menu_uniform, false);
        // Variáveis de tempo
        float current_time = (float)glfwGetTime();
        delta_t = current_time - prev_time;
        prev_time = current_time;

        // Aqui executamos as operações de renderização

        // Definimos a cor do "fundo" do framebuffer como branco.  Tal cor é
        // definida como coeficientes RGBA: Red, Green, Blue, Alpha; isto é:
        // Vermelho, Verde, Azul, Alpha (valor de transparência).
        // Conversaremos sobre sistemas de cores nas aulas de Modelos de Iluminação.
        //
        //           R     G     B     A
        glClearColor(1.0f, 1.0f, 1.0f, 1.0f);

        // "Pintamos" todos os pixels do framebuffer com a cor definida acima,
        // e também resetamos todos os pixels do Z-buffer (depth buffer).
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Pedimos para a GPU utilizar o programa de GPU criado acima (contendo
        // os shaders de vértice e fragmentos).
        glUseProgram(g_GpuProgramID);

        // Computamos a posição da câmera utilizando coordenadas esféricas.  As
        // variáveis g_CameraDistance, g_CameraPhi, e g_CameraTheta são
        // controladas pelo mouse do usuário. Veja as funções CursorPosCallback()
        // e ScrollCallback().
        float vy = sin(g_CameraPhi);
        float vz = cos(g_CameraPhi)*cos(g_CameraTheta);
        float vx = cos(g_CameraPhi)*sin(g_CameraTheta);

        // Abaixo definimos as varáveis que efetivamente definem a câmera virtual.
        // Veja slides 195-227 e 229-234 do documento Aula_08_Sistemas_de_Coordenadas.pdf.
        // glm::vec4 camera_look        =  // Direção para onde a câmera estará olhando
        glm::vec4 camera_view_vector = glm::vec4(vx,vy,vz,0.0f); // Vetor "view", sentido para onde a câmera está virada
        glm::vec4 camera_up_vector   = glm::vec4(0.0f,1.0f,0.0f,0.0f); // Vetor "up"
        glm::vec4 vw = -glm::normalize(camera_view_vector);
        glm::vec4 vu = glm::normalize(crossproduct(camera_up_vector, vw));
        glm::vec4 vv = crossproduct(vw, vu);

        // A câmera pode sofrer efeitos como shaking durante o dano ou na caminhada, mas isso não irá mudar a posição do jogador.

        // --------------------------------------------------------  JOGADOR  -----------------------------------------------------------

        // Definimos a bounding box e a bounding sphere do jogador
        jogador.aabb = AABB(glm::vec3(jogador.pos[0]-0.34f, jogador.pos[1], jogador.pos[2]-0.34f), glm::vec3(jogador.pos[0]+0.34f,jogador.pos[1]+1.4f,jogador.pos[2]+0.34f));
        jogador.bounding_sphere = Esfera(glm::vec4(jogador.pos[0],jogador.pos[1]+0.1f,jogador.pos[2], 1.0f), 0.35f);
        carro.aabb = AABB(glm::vec3(carro.pos[0]-1.00f, carro.pos[1], carro.pos[2]-2.5f), glm::vec3(carro.pos[0]+1.0f,carro.pos[1]+0.2f,carro.pos[2]+2.5f));
        /*glm::vec3 bbox_min = glm::vec3(carro.pos[0] - 1.00f, carro.pos[1] -5.0, carro.pos[2] - 2.5f);
        glm::vec3 bbox_max = glm::vec3(carro.pos[0] + 1.0f, carro.pos[1] +5.0f, carro.pos[2] + 2.5f);
        glm::vec3 bbox_center = glm::vec3(carro.pos[0], carro.pos[1], carro.pos[2]);

        glm::vec3 x_normal = glm::normalize(glm::vec3(1.0f, 0.0f, 0.0f));
        glm::vec3 y_normal = glm::normalize(glm::vec3(0.0f, 1.0f, 0.0f));
        glm::vec3 z_normal = glm::normalize(glm::vec3(0.0f, 0.0f, 1.0f));

        glm::vec3 normals[6] = {
            (bbox_center - bbox_min).x < (bbox_max - bbox_center).x ? x_normal : -x_normal,
            (bbox_center - bbox_min).z < (bbox_max - bbox_center).z ? z_normal : -z_normal,
            (bbox_center - bbox_max).x < (bbox_min - bbox_center).x ? x_normal : -x_normal,
            (bbox_center - bbox_max).z < (bbox_min - bbox_center).z ? z_normal : -z_normal
        };
        std::vector<glm::vec3> normaisDosLadosDoCubo;
            for (int i = 0; i < 6; ++i) {
                normaisDosLadosDoCubo.push_back(normals[i]);
            }
*/


        // Faz o jogador correr quando pressiona SHIFT e não está recarregando
        speed = speed_base;
        if (tecla_SHIFT_pressionada&&reload_active==false)
        {
            speed = speed_base*2;
        }
        glm::vec4 movimentacao = glm::vec4(1.0f,.0f,1.0f,0.0f); // Valor de movimentação padrão, quando não há obstáculos
        //glm::vec4 movimentacao_invertida = glm::vec4(-1.0f, 0.0f, -1.0f, 0.0f);
        /*const SceneObject& personagem = g_VirtualScene["the_sphere"];
        const glm::vec3 bboxMin = personagem.bbox_min;
        const glm::vec3 bboxMax = personagem.bbox_max;

        std::cout << "Object Name: " << "Plaque" << std::endl;
        std::cout << "Bounding Box Min: (" << bboxMin.x << ", " << bboxMin.y << ", " << bboxMin.z << ")" << std::endl;
        std::cout << "Bounding Box Max: (" << bboxMax.x << ", " << bboxMax.y << ", " << bboxMax.z << ")" << std::endl;
        std::cout << "Posicao do jogador: (" << jogador.pos.x << ", " << jogador.pos.y << ", " << jogador.pos.z << ")" << std::endl;*/

        if (tecla_W_pressionada){
            jogador.pos += - vw * (speed * delta_t) * movimentacao;
        }

        if (tecla_S_pressionada){
            jogador.pos += vw * (speed * delta_t) * movimentacao;
        }

        if (tecla_D_pressionada){
            jogador.pos += vu * (speed * delta_t) * movimentacao;
        }

        if (tecla_A_pressionada){
            jogador.pos += -vu * (speed * delta_t) * movimentacao;
        }

        // Prende o jogador em um 'loop' dentro da área de jogo, caso ele se afaste demais da cabine
        if ( sqrt(pow(jogador.pos[0],2)+pow(jogador.pos[2],2)) >= ARVORES_DIST + 27.0f)
        {
            jogador.pos[0] = -jogador.pos[0];
            jogador.pos[2] = -jogador.pos[2];
        }

        // "Balanço" enquanto o jogador anda no chão.
        if((tecla_A_pressionada||tecla_D_pressionada||tecla_S_pressionada||tecla_W_pressionada) && Yspeed == 0.0f)
            jogador_andando = true;
        else
            jogador_andando = false;

        if(jogador_andando)
        {
            if (shake_cima)
            {
                jogador.camera[1] += 0.1*(speed * delta_t);
                //jogador.pos[1] += 0.1*(speed * delta_t);
                if(jogador.camera[1]>=jogador.pos[1]+1.4f)
                    shake_cima = false;
            }
            else
            {
                jogador.camera[1] -= 0.1*(speed * delta_t);
                if(jogador.camera[1]<jogador.pos[1]+1.3f)
                    shake_cima = true;
            }
        }
        else if (jogador.camera[1] < jogador.pos[1]+1.4f && Yspeed == 0.0f)
        {
            jogador.camera[1] += 0.1*(speed * delta_t);
            shake_cima = false;
        }
        else
            jogador.camera[1] = jogador.pos[1]+1.4f;

        glm::vec4 ipis = glm::vec4(0.0f,0.0f,0.0f,1.0f);
        // Gravidade (reduz a velocidade em Y gradualmente com o tempo até chegar no chão
        if (jogador.aabb.EstaColidindoComPlano(chao))
            Yspeed = 0.0f;
        else
            Yspeed -= gravity * delta_t;
        /*else for(int i=0; i<cenario.size(); i++)
            if (jogador.aabb.EstaColidindoComAABB(cenario[i]))
                    Yspeed = 0.0f;
                else
                    Yspeed -= gravity * delta_t;
*/

        // Pulo (faz com que a velocidade em Y seja a velocidade base)
        if (tecla_SPACE_pressionada)
            if(jogador.aabb.EstaColidindoComPlano(chao))
                Yspeed = speed_base;
            else
                for(int i=0; i<cenario.size(); i++)
                    if (jogador.aabb.EstaColidindoComAABB(cenario[i]))
                        Yspeed = speed_base;

        // jogador.bounding_sphere.VaiColidirComAABB(cenario[i], (jogador.bounding_sphere.center-ipis))==-1)

        // Velocidade no eixo Y
        jogador.pos[1] += Yspeed*delta_t;

        // Ligar/Desligar lanterna
        if(tecla_F_pressionada){
            lanterna_ligada = false;
            std::cout << "Player Position: (" << jogador.pos[0] << ", " << jogador.pos[1] << ", " << -jogador.pos[2] << ")" << std::endl;
        }

        else
            lanterna_ligada = true;

        // Envia a informação da lanterna para o Fragment Shader, para ligar ou desligar a luz.
        glUniform1i(lanterna_ligada_uniform, lanterna_ligada ? 1 : 0);

        // O eixo z da câmera sempre é igual ao eixo da posição do jogador
        jogador.camera[0] = jogador.pos[0];
        jogador.camera[2] = jogador.pos[2];

        // A posição dos objetos irá se mover conforme o jogador caminha ou recarrega a arma
        glm::vec3 lanterna_pos = glm::vec3(0.0f,(jogador.camera[1]-(jogador.pos[1]+1.4f))*0.2f,-0.6f);
        glm::vec3 revolver_pos = glm::vec3(+0.6f,-(jogador.camera[1]-(jogador.pos[1]+1.4f))*0.2f-0.4f+reload_move+recoil,-1.2f);

        // ---------------------------------------------------------  AMMO  -------------------------------------------------------------

        #define RELOAD_ANIMATION 0.6f
        #define RELOAD_SPEED 0.5f
        // Se estiver com a munição cheia, termina o reload
        if(reload_active&&jogador.ammo<6)
        {
            // Animação de reload
            if((reload_move >= -RELOAD_ANIMATION&&jogador.ammo<6))
                reload_move -= RELOAD_ANIMATION * 3 * delta_t;

            // Atraso de reload para cada bala
            reload_delay += delta_t;
            if(reload_delay >= RELOAD_SPEED)
            {
                jogador.ammo++;
                reload_delay = 0.0f;
            }
        }
        else
        {
            if(jogador.ammo==6&&reload_move<-0.0001f)
                reload_move += RELOAD_ANIMATION * 3 * delta_t;
            reload_active = false;
            if(tecla_R_pressionada)
                reload_active = true;
        }

        // Atirar ativa a bala atual, zerando seu timer de atividade e avançando para a próxima bala
        cooldown_tiro += delta_t;
        if(cooldown_tiro >= 0.4f&&reload_active==false&&tecla_E_pressionada==false)
            if(g_LeftMouseButtonPressed)
            {
                for(int i=0; i<N_AMMO; i++)
                    if(ammo[bala_atual].ativa==false&&jogador.ammo>0)
                    {
                        ammo[bala_atual].ativa=true;
                        jogador.ammo--;
                        smoke_active = true;
                        recoil_active = true;
                        ammo[bala_atual].timer=0;
                        ammo[bala_atual].pos = jogador.camera-vw*0.05f+vu*0.06f;
                        ammo[bala_atual].orientacao = normalize((ammo[bala_atual].pos-vu*0.0605f) - jogador.camera);
                        ammo[bala_atual].rotacao = atan2(ammo[bala_atual].orientacao.x, ammo[bala_atual].orientacao.z);


                        cooldown_tiro = 0.0f;
                        bala_atual=++bala_atual%N_AMMO;
                        std::cout << "Bala atual: " << bala_atual << std::endl;
                        break;
                    }
                    else
                        bala_atual=++bala_atual%N_AMMO;
            }
        for(int i=0; i<N_AMMO; i++)
            std::cout << "Bala numero " << i << ": " << ammo[i].pos[0] << ", " << ammo[i].pos[1] << ", " << ammo[i].pos[2] << ")" << std::endl;

        if(cooldown_tiro <= 0.1)
            glUniform1i(nozzle_flash_uniform, 1);
        else
            glUniform1i(nozzle_flash_uniform, 0);

        // Para cada bala, testa se está ativa, se sim, incrementa seu timer e sua posicao e testa se seu tempo de atividade expirou, se estiver inativa, reseta seus
        // parâmetros, testando a colisão com os monstros
        for(int i=0; i<N_AMMO; i++)
        {
            if(ammo[i].ativa == true)
            {
                ammo[i].timer += delta_t;
                ammo[i].pos += ammo[i].orientacao * 30.0f * delta_t;
                ammo[i].aabb.minimo = glm::vec3(ammo[i].pos[0] - 0.1f, ammo[i].pos[1] - 0.1f, ammo[i].pos[2] - 0.1f);
                ammo[i].aabb.maximo = glm::vec3(ammo[i].pos[0] + 0.1f, ammo[i].pos[1] + 0.1f, ammo[i].pos[2] + 0.1f);
                for(int j=0; j<N_MONSTROS; j++){
                        if (ammo[i].aabb.EstaColidindoComAABB(monstro[j].aabb)){
                            monstro[j].pos = {rand()%100, (rand()%10*0.1+0.1)*1.4f, rand()%100,1.0f};
                            ammo[i].pos = jogador.pos;
                        }
                    }
                if(ammo[i].timer >= 1)
                    ammo[i].ativa=false;
            }
        }

        // ========= RECOIL ANIMATION =========
        #define RECOIL_ANIMATION 0.6f
        if(recoil_active)
        {
            if(cooldown_tiro < 0.2f)
                recoil += RECOIL_ANIMATION * delta_t;
            else
            {
                recoil -= RECOIL_ANIMATION * delta_t;
                if(cooldown_tiro >= 0.4f)
                {
                    recoil_active=false;
                    recoil = 0.0f;
                }
            }
        }

        // ========= SMOKE PARTICLES =========
        if(smoke_active)
        {
            // Inicializa a direção das partículas aleatoriamente
            if(smoke_timer == 0.0f)
                for(int i=0; i<SMOKE_P_COUNT; i++)
                {
                    smoke[i].pos[0] = revolver_pos[0]+0.1f;
                    smoke[i].pos[1] = revolver_pos[1]+0.2f;
                    // 8 possíveis direções estão entre [0, 2*PI]
                    float direcao = (rand()%8)*(2*PI/8);
                    smoke[i].vel = {cos(direcao),sin(direcao)};
                    smoke[i].scale = 1.0f;
                }

            // Incrementa o timer que define o tempo de vida da particula
            smoke_timer += delta_t;

            for(int i=0; i<SMOKE_P_COUNT; i++)
            {
                // Essa variável define qual textura (de 40) que será mostrada no momento
                smoke[i].life = int(smoke_timer*125);
                // Movimenta a particula na tela
                smoke[i].pos += smoke[i].vel*delta_t*0.3f;
                smoke[i].scale += smoke[i].scale*(2)*delta_t;
            }
            if(smoke_timer >= 0.4f)
            {
                smoke_timer = 0.0f;
                smoke_active = false;
            }
        }

        // --------------------------------------------------------  MONSTRO  -----------------------------------------------------------

        for(int i=0; i<N_MONSTROS; i++)
        {
            // Faz o monstro estar sempre olhando para o jogador
            monstro[i].orientacao = normalize(monstro[i].pos - jogador.pos);
            // Usado para a matriz de rotação do monstro
            monstro[i].rotacao = atan2(monstro[i].orientacao.x, monstro[i].orientacao.z) + PI;
            // Move o monstro em direção ao jogador
            monstro[i].pos -= monstro[i].orientacao * speed_base * delta_t * glm::vec4(1.0f,0.0f,1.0f,0.0f);
            //colisão
            monstro[i].aabb.minimo = glm::vec3(monstro[i].pos[0] - 0.1f,monstro[i].pos[1] - 0.3f,monstro[i].pos[2] - 0.1f);
            monstro[i].aabb.maximo = glm::vec3(monstro[i].pos[0] + 0.1f,monstro[i].pos[1] + 0.3f,monstro[i].pos[2] + 0.1f);
            if (monstro[i].aabb.EstaColidindoComAABB(jogador.aabb)){
                jogador.vidas--;
                monstro[i].pos = {rand()%100, (rand()%10*0.1+0.1)*1.4f, rand()%100,1.0f};
            }
        }

        glm::vec4 P0(10.0f, 1.0f, 0.0f, 1.0f);           // Ponto inicial
        glm::vec4 P1(5.0f, 2.0f, 12.0f, 1.0f);           // Primeiro ponto intermediário
        glm::vec4 P2(0.0f, -1.0f, 20.0f, 1.0f);           // Segundo ponto intermediário
        glm::vec4 P3(-10.0f, 4.0f, 0.0f, 1.0f);           // Terceiro ponto intermediário
        glm::vec4 P4(-5.0f, -1.0f, -20.0f, 1.0f);           // Quarto ponto intermediário
        glm::vec4 P5 = P0;           // Ponto final
        float t_bezier = fmod(glfwGetTime(), 10.0) / 10.0; // Varia t_bezier de 0 a 1 a cada 10 segundos
        glm::vec4 pointOnBezierCurve = calculateBezierPoint(P0, P1, P2, P3, P4, P5, t_bezier);

        monstro_bezier.orientacao = normalize(monstro_bezier.pos);
        monstro_bezier.rotacao = atan2(monstro_bezier.orientacao.x, monstro_bezier.orientacao.z) + 3.14f;
        monstro_bezier.pos = glm::vec4(pointOnBezierCurve);
        monstro_bezier.aabb.minimo = glm::vec3(monstro_bezier.pos[0] - 0.3f,monstro_bezier.pos[1] - 0.3f,monstro_bezier.pos[2] - 0.3f);
        monstro_bezier.aabb.maximo = glm::vec3(monstro_bezier.pos[0] + 0.3f,monstro_bezier.pos[1] + 0.3f,monstro_bezier.pos[2] + 0.3f);


        // ---------------------------------------------------------- CARRO -------------------------------------------------------------

        // Se o jogador estiver próximo do carro, ele terá a opção de consertá-lo pressionando E.
        if(sqrt(pow(jogador.pos[0]-carro.pos[0],2)+pow(jogador.pos[2]-carro.pos[2],2)) < 3.0f)
            jogador_proximo_do_carro = true;
        else
            jogador_proximo_do_carro = false;

        if(jogador_proximo_do_carro && tecla_E_pressionada)
            carro.estado += speed_base * 0.2 * delta_t;

        // Computamos a matriz "View" utilizando os parâmetros da câmera para definir o sistema de coordenadas da câmera.
        glm::mat4 view = Matrix_Camera_View(jogador.camera, camera_view_vector, vv);

        // Agora computamos a matriz de Projeção.
        glm::mat4 projection;

        // Note que, no sistema de coordenadas da câmera, os planos near e far
        // estão no sentido negativo! Veja slides 176-204 do documento Aula_09_Projecoes.pdf.
        float nearplane = -0.1f;  // Posição do "near plane"
        float farplane  = -25.0f; // Posição do "far plane"


        // Projeção Perspectiva.
        // Para definição do field of view (FOV), veja slides 205-215 do documento Aula_09_Projecoes.pdf.
        float field_of_view = PI / 3.0f;
        glm::mat4 perspective = Matrix_Perspective(field_of_view, g_ScreenRatio, nearplane, farplane);

        // Projeção Ortográfica.
        // Para definição dos valores l, r, b, t ("left", "right", "bottom", "top"),
        // PARA PROJEÇÃO ORTOGRÁFICA veja slides 219-224 do documento Aula_09_Projecoes.pdf.
        // Para simular um "zoom" ortográfico, computamos o valor de "t"
        // utilizando a variável g_CameraDistance.
        float t = 1.5f*g_CameraDistance/2.5f;
        float b = -t;
        float r = t*g_ScreenRatio;
        float l = -r;
        glm::mat4 orthographic = Matrix_Orthographic(l, r, b, t, nearplane, farplane);


        glm::mat4 model = Matrix_Identity(); // Transformação identidade de modelagem

        // Enviamos as matrizes "view" e "projection" para a placa de vídeo
        // (GPU). Veja o arquivo "shader_vertex.glsl", onde estas são
        // efetivamente aplicadas em todos os pontos.
        glUniformMatrix4fv(g_view_uniform       , 1 , GL_FALSE , glm::value_ptr(view));
        glUniformMatrix4fv(g_projection_uniform , 1 , GL_FALSE , glm::value_ptr(perspective));

        #define SPHERE 0
        #define BULLET 1
        #define PLANE  2
        #define FLASHLIGHT  3
        #define REVOLVER  4
        #define SCREEN  5
        #define SKULL  6
        #define EYE  7
        #define SMOKE  8
        #define ARVORE  9
        #define CABINE  10
        #define CARRO  11
        #define TELA_FINAL 12

        // SPHERE
        glDisable(GL_DEPTH_TEST);
        glDisable(GL_CULL_FACE);
        model = Matrix_Translate(jogador.camera[0], jogador.camera[1], jogador.camera[2]);
        glUniformMatrix4fv(g_model_uniform, 1 , GL_FALSE , glm::value_ptr(model));
        glUniform1i(g_object_id_uniform, SPHERE);
        DrawVirtualObject("the_sphere");
        glEnable(GL_CULL_FACE);
        glEnable(GL_DEPTH_TEST);

        // PLANE
        model = Matrix_Translate(0.0f, 0.0f, 0.0f)
              * Matrix_Scale(350.0f,1.0f,350.0f);
        glUniformMatrix4fv(g_model_uniform, 1 , GL_FALSE , glm::value_ptr(model));
        glUniform1i(g_object_id_uniform, PLANE);
        DrawVirtualObject("the_plane");

        // CABINE
        model = Matrix_Translate(0.0f, 0.0f, 0.0f)
              * Matrix_Scale(0.1f,0.1f,0.1f);
        glUniformMatrix4fv(g_model_uniform, 1 , GL_FALSE , glm::value_ptr(model));
        glUniform1i(g_object_id_uniform, CABINE);
        DrawVirtualObject("WoodCabin");
        DrawVirtualObject("Roof");

        // CARRO & VIDROS
        model = Matrix_Translate(carro.pos[0], 0.0f, carro.pos[2])
              * Matrix_Scale(0.01f,0.01f,0.01f);
        glUniformMatrix4fv(g_model_uniform, 1 , GL_FALSE , glm::value_ptr(model));
        glUniform1i(g_object_id_uniform, CARRO);
        // Body
        glUniform1i(parte_carro_uniform, 1);
        DrawVirtualObject("Body1");
        DrawVirtualObject("Steel");
        DrawVirtualObject("UnderCar");
        DrawVirtualObject("Hood");
        DrawVirtualObject("Body");
        // Vidros
        glUniform1i(parte_carro_uniform, 2);
        DrawVirtualObject("Glass");
        DrawVirtualObject("Plastik");

        DrawVirtualObject("Light1");
        DrawVirtualObject("Light2");
        DrawVirtualObject("Light3");
        // Logo
        glUniform1i(parte_carro_uniform, 3);
        DrawVirtualObject("Logo");
        // Placa
        glUniform1i(parte_carro_uniform, 4);
        DrawVirtualObject("Plaque");
        DrawVirtualObject("Plaque1");
        // Pisca
        glUniform1i(parte_carro_uniform, 5);
        DrawVirtualObject("GuidLight1");
        DrawVirtualObject("GuidLight");
        // Faróis
        glUniform1i(parte_carro_uniform, 6);
        DrawVirtualObject("Light");

        glUniform1i(parte_carro_uniform, 7);
        // Pneus
        glUniform1i(parte_carro_uniform, 8);
        DrawVirtualObject("Tire");
        DrawVirtualObject("Tire1");
        DrawVirtualObject("Tire2");
        DrawVirtualObject("Tire3");

        // BULLET
        for(int i=0; i<N_AMMO; i++)
            if(ammo[i].ativa)
            {
                model = Matrix_Translate(ammo[i].pos.x, ammo[i].pos.y, ammo[i].pos.z)
                      * Matrix_Scale(0.04f,0.04f,0.04f)
                      * Matrix_Rotate_Y(ammo[i].rotacao)
                      * Matrix_Rotate_X(PI/2);
                glUniformMatrix4fv(g_model_uniform, 1 , GL_FALSE , glm::value_ptr(model));
                glUniform1i(g_object_id_uniform, BULLET);
                DrawVirtualObject("45_ACP_Low_Poly");
            }

        // ARVORES
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        for(int i=0; i<NUM_ARVORES; i++)
        {
            // TRONCO
            model = Matrix_Translate(arvores[i].pos.x,0.0f,arvores[i].pos.z)
                  * Matrix_Scale(1.0f,1.0f,1.0f)
                  * Matrix_Rotate_Y(arvores[i].rotacao);
            glUniformMatrix4fv(g_model_uniform, 1 , GL_FALSE , glm::value_ptr(model));
            glUniform1i(g_object_id_uniform, ARVORE);
            glUniform1i(tronco_uniform, true);
            DrawVirtualObject("bark1");
            // FOLHAS
            glUniformMatrix4fv(g_model_uniform, 1 , GL_FALSE , glm::value_ptr(model));
            glUniform1i(g_object_id_uniform, ARVORE);
            glUniform1i(tronco_uniform, false);
            DrawVirtualObject("leaves1");
        }

        // SKULL & EYE
        for(int i=0; i<N_MONSTROS; i++)
        {
            model = Matrix_Translate(monstro[i].pos[0], monstro[i].pos[1], monstro[i].pos[2])
                  * Matrix_Rotate_Y(monstro[i].rotacao)
                  * Matrix_Scale(0.02f, 0.02f, 0.02f);
            glUniformMatrix4fv(g_model_uniform, 1 , GL_FALSE , glm::value_ptr(model));
            glUniform1i(g_object_id_uniform, SKULL);
            DrawVirtualObject("skull");
            PushMatrix(model);
                model = model * Matrix_Translate(-3.2f, 1.4f, 9.0f)
                              * Matrix_Rotate_X(PI/2);
                glUniformMatrix4fv(g_model_uniform, 1 , GL_FALSE , glm::value_ptr(model));
                glUniform1i(g_object_id_uniform, EYE);
                DrawVirtualObject("eye");
                model = model * Matrix_Translate(6.4f, 0.0f, 0.f);
                glUniformMatrix4fv(g_model_uniform, 1 , GL_FALSE , glm::value_ptr(model));
                glUniform1i(g_object_id_uniform, EYE);
                DrawVirtualObject("eye");
            PopMatrix(model);
        }

        model = Matrix_Translate(monstro_bezier.pos[0], monstro_bezier.pos[1], monstro_bezier.pos[2])
                      * Matrix_Rotate_Y(monstro_bezier.rotacao)
                      * Matrix_Scale(0.06f, 0.06f, 0.06f);
                glUniformMatrix4fv(g_model_uniform, 1 , GL_FALSE , glm::value_ptr(model));
                glUniform1i(g_object_id_uniform, SKULL);
                DrawVirtualObject("skull");
                PushMatrix(model);
                    model = model * Matrix_Translate(-3.2f, 1.4f, 9.0f)
                                  * Matrix_Rotate_X(3.14/2);
                    glUniformMatrix4fv(g_model_uniform, 1 , GL_FALSE , glm::value_ptr(model));
                    glUniform1i(g_object_id_uniform, EYE);
                    DrawVirtualObject("eye");
                    model = model * Matrix_Translate(6.4f, 0.0f, 0.f);
                    glUniformMatrix4fv(g_model_uniform, 1 , GL_FALSE , glm::value_ptr(model));
                    glUniform1i(g_object_id_uniform, EYE);
                    DrawVirtualObject("eye");
                PopMatrix(model);

        // Resetamos a matriz View para que os objetos carregados a partir daqui não se movimentem na tela.
        glUniformMatrix4fv(g_view_uniform       , 1 , GL_FALSE , glm::value_ptr(Matrix_Identity()));

        // SMOKE
        glClear(GL_DEPTH_BUFFER_BIT);
        if(smoke_active)
            for(int i=0; i<SMOKE_P_COUNT; i++)
            {
                model = Matrix_Translate(smoke[i].pos[0],smoke[i].pos[1]+recoil,-2.0f)
                      * Matrix_Scale(smoke[i].scale,smoke[i].scale,1.0f)
                      * Matrix_Scale(0.1f,0.1f,1.0f)
                      * Matrix_Rotate_X(PI/2);
                glUniformMatrix4fv(g_model_uniform, 1 , GL_FALSE , glm::value_ptr(model));
                glUniform1i(g_object_id_uniform, SMOKE);
                glUniform1i(smoke_life_uniform, smoke[i].life);
                DrawVirtualObject("the_screen");
            }

        // FLASHLIGHT
        model = Matrix_Translate(lanterna_pos[0]-0.6f, lanterna_pos[1]-0.4f, lanterna_pos[2])
            * Matrix_Scale(0.01f,0.01f,0.01f)
            * Matrix_Rotate_X(PI);
        glUniformMatrix4fv(g_model_uniform, 1 , GL_FALSE , glm::value_ptr(model));
        glUniform1i(g_object_id_uniform, FLASHLIGHT);
        DrawVirtualObject("the_light");

        // REVOLVER
        model = Matrix_Translate(revolver_pos[0], revolver_pos[1], revolver_pos[2])
            * Matrix_Scale(0.002f,0.002f,0.002f)
            * Matrix_Rotate_X(reload_move*2+recoil*2)
            * Matrix_Rotate_Y(-PI/2);
        glUniformMatrix4fv(g_model_uniform, 1 , GL_FALSE , glm::value_ptr(model));
        glUniform1i(g_object_id_uniform, REVOLVER);
        DrawVirtualObject("Handle");
        DrawVirtualObject("BodyR");
        DrawVirtualObject("Back_Trigger");
        DrawVirtualObject("Trigger");
        DrawVirtualObject("Chamber_Holder");
        DrawVirtualObject("Chamber");
        DrawVirtualObject("Barrel");

        // SCREEN
        glUniformMatrix4fv(g_projection_uniform , 1 , GL_FALSE , glm::value_ptr(orthographic));
        glDisable(GL_DEPTH_TEST);
        model = Matrix_Translate(0.0f,0.0f,-2.0f)
            * Matrix_Scale(0.1f,0.1f,1.0f)
            * Matrix_Rotate_X(PI/2);
        glUniformMatrix4fv(g_model_uniform, 1 , GL_FALSE , glm::value_ptr(model));
        glUniform1i(g_object_id_uniform, SCREEN);
        DrawVirtualObject("the_screen");
        glEnable(GL_DEPTH_TEST);

        // Desenhamos uma instrução para o jogador se ele estiver próximo do carro
        if(jogador_proximo_do_carro)
            TextRendering_ShowCarTip(window, carro.estado);

        // Imprimimos a quantidade de munição que o jogador possui
        TextRendering_ShowAMMO(window, jogador.ammo);

        // Imprimimos na informação sobre a matriz de projeção sendo utilizada.
        TextRendering_ShowProjection(window);

        // Imprimimos na tela informação sobre o número de quadros renderizados
        // por segundo (frames per second).
        TextRendering_ShowFramesPerSecond(window);

        // Imprimimos na tela quandos segundos se passaram desde o início
        TextRendering_ShowSecondsEllapsed(window);

        // O framebuffer onde OpenGL executa as operações de renderização não
        // é o mesmo que está sendo mostrado para o usuário, caso contrário
        // seria possível ver artefatos conhecidos como "screen tearing". A
        // chamada abaixo faz a troca dos buffers, mostrando para o usuário
        // tudo que foi renderizado pelas funções acima.
        // Veja o link: https://en.wikipedia.org/w/index.php?title=Multiple_buffering&oldid=793452829#Double_buffering_in_computer_graphics
        glfwSwapBuffers(window);

        // Verificamos com o sistema operacional se houve alguma interação do
        // usuário (teclado, mouse, ...). Caso positivo, as funções de callback
        // definidas anteriormente usando glfwSet*Callback() serão chamadas
        // pela biblioteca GLFW.


        glfwPollEvents();
        }


        int i = 0;
        while (fim_de_jogo && !glfwWindowShouldClose(window)){

            glfwSetWindowMonitor(window, _fullscreen ? glfwGetPrimaryMonitor() : NULL, 0, 0, 4000, 4000, GLFW_DONT_CARE);

            // Serve para mudar a iluminação global durante a gameplay
            glUniform1i(tela_de_menu_uniform, false);
            // Variáveis de tempo
            float current_time = (float)glfwGetTime();
            delta_t = current_time - prev_time;
            prev_time = current_time;

            // Aqui executamos as operações de renderização

            // Definimos a cor do "fundo" do framebuffer como branco.  Tal cor é
            // definida como coeficientes RGBA: Red, Green, Blue, Alpha; isto é:
            // Vermelho, Verde, Azul, Alpha (valor de transparência).
            // Conversaremos sobre sistemas de cores nas aulas de Modelos de Iluminação.
            //
            //           R     G     B     A
            glClearColor(1.0f, 1.0f, 1.0f, 1.0f);

            // "Pintamos" todos os pixels do framebuffer com a cor definida acima,
            // e também resetamos todos os pixels do Z-buffer (depth buffer).
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            // Pedimos para a GPU utilizar o programa de GPU criado acima (contendo
            // os shaders de vértice e fragmentos).
            glUseProgram(g_GpuProgramID);

            // Computamos a posição da câmera utilizando coordenadas esféricas.  As
            // variáveis g_CameraDistance, g_CameraPhi, e g_CameraTheta são
            // controladas pelo mouse do usuário. Veja as funções CursorPosCallback()
            // e ScrollCallback().
            float vy = sin(g_CameraPhi);
            float vz = cos(g_CameraPhi)*cos(g_CameraTheta);
            float vx = cos(g_CameraPhi)*sin(g_CameraTheta);

            // Abaixo definimos as varáveis que efetivamente definem a câmera virtual.
            // Veja slides 195-227 e 229-234 do documento Aula_08_Sistemas_de_Coordenadas.pdf.
            // glm::vec4 camera_look        =  // Direção para onde a câmera estará olhando
            glm::vec4 camera_view_vector = glm::vec4(vx,vy,vz,0.0f); // Vetor "view", sentido para onde a câmera está virada
            glm::vec4 camera_up_vector   = glm::vec4(0.0f,1.0f,0.0f,0.0f); // Vetor "up"
            glm::vec4 vw = -glm::normalize(camera_view_vector);
            glm::vec4 vu = glm::normalize(crossproduct(camera_up_vector, vw));
            glm::vec4 vv = crossproduct(vw, vu);

            // A câmera pode sofrer efeitos como shaking durante o dano ou na caminhada, mas isso não irá mudar a posição do jogador.

            // --------------------------------------------------------  JOGADOR  -----------------------------------------------------------

            // Faz o jogador correr quando pressiona SHIFT e não está recarregando
            speed = speed_base;

            // Envia a informação da lanterna para o Fragment Shader, para ligar ou desligar a luz.
            glUniform1i(lanterna_ligada_uniform, lanterna_ligada ? 1 : 0);

            // O eixo z da câmera sempre é igual ao eixo da posição do jogador
            jogador.camera[0] = jogador.pos[0];
            jogador.camera[2] = jogador.pos[2];

            // A posição dos objetos irá se mover conforme o jogador caminha ou recarrega a arma
            glm::vec3 lanterna_pos = glm::vec3(0.0f,(jogador.camera[1]-(jogador.pos[1]+1.4f))*0.2f,-0.6f);
            glm::vec3 revolver_pos = glm::vec3(+0.6f,-(jogador.camera[1]-(jogador.pos[1]+1.4f))*0.2f-0.4f+reload_move+recoil,-1.2f);



            // Computamos a matriz "View" utilizando os parâmetros da câmera para definir o sistema de coordenadas da câmera.
            glm::mat4 view = Matrix_Camera_View(jogador.camera, camera_view_vector, vv);

            // Agora computamos a matriz de Projeção.
            glm::mat4 projection;

            // Note que, no sistema de coordenadas da câmera, os planos near e far
            // estão no sentido negativo! Veja slides 176-204 do documento Aula_09_Projecoes.pdf.
            float nearplane = -0.1f;  // Posição do "near plane"
            float farplane  = -25.0f; // Posição do "far plane"


            // Projeção Perspectiva.
            // Para definição do field of view (FOV), veja slides 205-215 do documento Aula_09_Projecoes.pdf.
            float field_of_view = PI / 3.0f;
            glm::mat4 perspective = Matrix_Perspective(field_of_view, g_ScreenRatio, nearplane, farplane);

            // Projeção Ortográfica.
            // Para definição dos valores l, r, b, t ("left", "right", "bottom", "top"),
            // PARA PROJEÇÃO ORTOGRÁFICA veja slides 219-224 do documento Aula_09_Projecoes.pdf.
            // Para simular um "zoom" ortográfico, computamos o valor de "t"
            // utilizando a variável g_CameraDistance.
            float t = 1.5f*g_CameraDistance/2.5f;
            float b = -t;
            float r = t*g_ScreenRatio;
            float l = -r;
            glm::mat4 orthographic = Matrix_Orthographic(l, r, b, t, nearplane, farplane);


            glm::mat4 model = Matrix_Identity(); // Transformação identidade de modelagem

            // Enviamos as matrizes "view" e "projection" para a placa de vídeo
            // (GPU). Veja o arquivo "shader_vertex.glsl", onde estas são
            // efetivamente aplicadas em todos os pontos.
            glUniformMatrix4fv(g_view_uniform       , 1 , GL_FALSE , glm::value_ptr(view));
            glUniformMatrix4fv(g_projection_uniform , 1 , GL_FALSE , glm::value_ptr(perspective));

            #define SPHERE 0
            #define BULLET 1
            #define PLANE  2
            #define FLASHLIGHT  3
            #define REVOLVER  4
            #define SCREEN  5
            #define SKULL  6
            #define EYE  7
            #define SMOKE  8
            #define ARVORE  9
            #define CABINE  10
            #define CARRO  11
            #define TELA_FINAL 12

            // SPHERE
            glDisable(GL_DEPTH_TEST);
            glDisable(GL_CULL_FACE);
            model = Matrix_Translate(jogador.camera[0], jogador.camera[1], jogador.camera[2]);
            glUniformMatrix4fv(g_model_uniform, 1 , GL_FALSE , glm::value_ptr(model));
            glUniform1i(g_object_id_uniform, SPHERE);
            DrawVirtualObject("the_sphere");
            glEnable(GL_CULL_FACE);
            glEnable(GL_DEPTH_TEST);

            // PLANE
            model = Matrix_Translate(0.0f, 0.0f, 0.0f)
                  * Matrix_Scale(350.0f,1.0f,350.0f);
            glUniformMatrix4fv(g_model_uniform, 1 , GL_FALSE , glm::value_ptr(model));
            glUniform1i(g_object_id_uniform, PLANE);
            DrawVirtualObject("the_plane");

            // CABINE
            model = Matrix_Translate(0.0f, 0.0f, 0.0f)
                  * Matrix_Scale(0.1f,0.1f,0.1f);
            glUniformMatrix4fv(g_model_uniform, 1 , GL_FALSE , glm::value_ptr(model));
            glUniform1i(g_object_id_uniform, CABINE);
            DrawVirtualObject("WoodCabin");
            DrawVirtualObject("Roof");

            // CARRO & VIDROS
            model = Matrix_Translate(carro.pos[0], 0.0f, carro.pos[2])
                  * Matrix_Scale(0.01f,0.01f,0.01f);
            glUniformMatrix4fv(g_model_uniform, 1 , GL_FALSE , glm::value_ptr(model));
            glUniform1i(g_object_id_uniform, CARRO);
            // Body
            glUniform1i(parte_carro_uniform, 1);
            DrawVirtualObject("Body1");
            DrawVirtualObject("Steel");
            DrawVirtualObject("UnderCar");
            DrawVirtualObject("Hood");
            DrawVirtualObject("Body");
            // Vidros
            glUniform1i(parte_carro_uniform, 2);
            DrawVirtualObject("Glass");
            DrawVirtualObject("Plastik");

            DrawVirtualObject("Light1");
            DrawVirtualObject("Light2");
            DrawVirtualObject("Light3");
            // Logo
            glUniform1i(parte_carro_uniform, 3);
            DrawVirtualObject("Logo");
            // Placa
            glUniform1i(parte_carro_uniform, 4);
            DrawVirtualObject("Plaque");
            DrawVirtualObject("Plaque1");
            // Pisca
            glUniform1i(parte_carro_uniform, 5);
            DrawVirtualObject("GuidLight1");
            DrawVirtualObject("GuidLight");
            // Faróis
            glUniform1i(parte_carro_uniform, 6);
            DrawVirtualObject("Light");

            glUniform1i(parte_carro_uniform, 7);
            // Pneus
            glUniform1i(parte_carro_uniform, 8);
            DrawVirtualObject("Tire");
            DrawVirtualObject("Tire1");
            DrawVirtualObject("Tire2");
            DrawVirtualObject("Tire3");


            // ARVORES
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            for(int i=0; i<NUM_ARVORES; i++)
            {
                // TRONCO
                model = Matrix_Translate(arvores[i].pos.x,0.0f,arvores[i].pos.z)
                      * Matrix_Scale(1.0f,1.0f,1.0f)
                      * Matrix_Rotate_Y(arvores[i].rotacao);
                glUniformMatrix4fv(g_model_uniform, 1 , GL_FALSE , glm::value_ptr(model));
                glUniform1i(g_object_id_uniform, ARVORE);
                glUniform1i(tronco_uniform, true);
                DrawVirtualObject("bark1");
                // FOLHAS
                glUniformMatrix4fv(g_model_uniform, 1 , GL_FALSE , glm::value_ptr(model));
                glUniform1i(g_object_id_uniform, ARVORE);
                glUniform1i(tronco_uniform, false);
                DrawVirtualObject("leaves1");
            }

            // SKULL & EYE
            for(int i=0; i<N_MONSTROS; i++)
            {
                model = Matrix_Translate(monstro[i].pos[0], monstro[i].pos[1], monstro[i].pos[2])
                      * Matrix_Rotate_Y(monstro[i].rotacao)
                      * Matrix_Scale(0.02f, 0.02f, 0.02f);
                glUniformMatrix4fv(g_model_uniform, 1 , GL_FALSE , glm::value_ptr(model));
                glUniform1i(g_object_id_uniform, SKULL);
                DrawVirtualObject("skull");
                PushMatrix(model);
                    model = model * Matrix_Translate(-3.2f, 1.4f, 9.0f)
                                  * Matrix_Rotate_X(PI/2);
                    glUniformMatrix4fv(g_model_uniform, 1 , GL_FALSE , glm::value_ptr(model));
                    glUniform1i(g_object_id_uniform, EYE);
                    DrawVirtualObject("eye");
                    model = model * Matrix_Translate(6.4f, 0.0f, 0.f);
                    glUniformMatrix4fv(g_model_uniform, 1 , GL_FALSE , glm::value_ptr(model));
                    glUniform1i(g_object_id_uniform, EYE);
                    DrawVirtualObject("eye");
                PopMatrix(model);
            }

            model = Matrix_Translate(monstro_bezier.pos[0], monstro_bezier.pos[1], monstro_bezier.pos[2])
                          * Matrix_Rotate_Y(monstro_bezier.rotacao)
                          * Matrix_Scale(0.06f, 0.06f, 0.06f);
                    glUniformMatrix4fv(g_model_uniform, 1 , GL_FALSE , glm::value_ptr(model));
                    glUniform1i(g_object_id_uniform, SKULL);
                    DrawVirtualObject("skull");
                    PushMatrix(model);
                        model = model * Matrix_Translate(-3.2f, 1.4f, 9.0f)
                                      * Matrix_Rotate_X(3.14/2);
                        glUniformMatrix4fv(g_model_uniform, 1 , GL_FALSE , glm::value_ptr(model));
                        glUniform1i(g_object_id_uniform, EYE);
                        DrawVirtualObject("eye");
                        model = model * Matrix_Translate(6.4f, 0.0f, 0.f);
                        glUniformMatrix4fv(g_model_uniform, 1 , GL_FALSE , glm::value_ptr(model));
                        glUniform1i(g_object_id_uniform, EYE);
                        DrawVirtualObject("eye");
                    PopMatrix(model);

            // Resetamos a matriz View para que os objetos carregados a partir daqui não se movimentem na tela.
            glUniformMatrix4fv(g_view_uniform       , 1 , GL_FALSE , glm::value_ptr(Matrix_Identity()));


            // FLASHLIGHT
            model = Matrix_Translate(lanterna_pos[0]-0.6f, lanterna_pos[1]-0.4f, lanterna_pos[2])
                * Matrix_Scale(0.01f,0.01f,0.01f)
                * Matrix_Rotate_X(PI);
            glUniformMatrix4fv(g_model_uniform, 1 , GL_FALSE , glm::value_ptr(model));
            glUniform1i(g_object_id_uniform, FLASHLIGHT);
            DrawVirtualObject("the_light");

            // REVOLVER
            model = Matrix_Translate(revolver_pos[0], revolver_pos[1], revolver_pos[2])
                * Matrix_Scale(0.002f,0.002f,0.002f)
                * Matrix_Rotate_X(reload_move*2+recoil*2)
                * Matrix_Rotate_Y(-PI/2);
            glUniformMatrix4fv(g_model_uniform, 1 , GL_FALSE , glm::value_ptr(model));
            glUniform1i(g_object_id_uniform, REVOLVER);
            DrawVirtualObject("Handle");
            DrawVirtualObject("BodyR");
            DrawVirtualObject("Back_Trigger");
            DrawVirtualObject("Trigger");
            DrawVirtualObject("Chamber_Holder");
            DrawVirtualObject("Chamber");
            DrawVirtualObject("Barrel");



            // Texto da tela de fim de jogo
            TextRendering_Menu(window, i);
            i++;

            // Desenhamos uma instrução para o jogador se ele estiver próximo do carro
            if(jogador_proximo_do_carro)
                TextRendering_ShowCarTip(window, carro.estado);

            // Imprimimos a quantidade de munição que o jogador possui
            TextRendering_ShowAMMO(window, jogador.ammo);

            // Imprimimos na informação sobre a matriz de projeção sendo utilizada.
            TextRendering_ShowProjection(window);

            // Imprimimos na tela informação sobre o número de quadros renderizados
            // por segundo (frames per second).
            TextRendering_ShowFramesPerSecond(window);

            // Imprimimos na tela quandos segundos se passaram desde o início
            TextRendering_ShowSecondsEllapsed(window);

            // O framebuffer onde OpenGL executa as operações de renderização não
            // é o mesmo que está sendo mostrado para o usuário, caso contrário
            // seria possível ver artefatos conhecidos como "screen tearing". A
            // chamada abaixo faz a troca dos buffers, mostrando para o usuário
            // tudo que foi renderizado pelas funções acima.
            // Veja o link: https://en.wikipedia.org/w/index.php?title=Multiple_buffering&oldid=793452829#Double_buffering_in_computer_graphics
            glfwSwapBuffers(window);

            // Verificamos com o sistema operacional se houve alguma interação do
            // usuário (teclado, mouse, ...). Caso positivo, as funções de callback
            // definidas anteriormente usando glfwSet*Callback() serão chamadas
            // pela biblioteca GLFW.


            glfwPollEvents();
            }



    }

    // Finalizamos o uso dos recursos do sistema operacional
    glfwTerminate();

    // Fim do programa
    return 0;
}

//funcao de bezier
glm::vec4 calculateBezierPoint(const glm::vec4& P0, const glm::vec4& P1, const glm::vec4& P2, const glm::vec4& P3, const glm::vec4& P4, const glm::vec4& P5, float t)
{

    float tt = t * t;
    float ttt = tt * t;
    float tttt = ttt * t;
    float ttttt = tttt * t;
    float u = 1.0f - t;
    float uu = u * u;
    float uuu = uu * u;
    float uuuu = uuu * u;
    float uuuuu = uuuu * u;

    glm::vec4 point = (uuuuu * P0) + (5.0f * uuuu * t * P1) + (10.0f * uuu * tt * P2) + (10.0f * uu * ttt * P3) + (5.0f * u * tttt * P4) + (ttttt * P5);

    return point;
}


// Função que carrega uma imagem para ser utilizada como textura
void LoadTextureImage(const char* filename)
{
    printf("Carregando imagem \"%s\"... ", filename);

    // Primeiro fazemos a leitura da imagem do disco
    stbi_set_flip_vertically_on_load(true);
    int width;
    int height;
    int channels;
    unsigned char *data = stbi_load(filename, &width, &height, &channels, 3); // 4 canais para transparencia

    if ( data == NULL )
    {
        fprintf(stderr, "ERROR: Cannot open image file \"%s\".\n", filename);
        std::exit(EXIT_FAILURE);
    }

    printf("OK (%dx%d).\n", width, height);

    // Agora criamos objetos na GPU com OpenGL para armazenar a textura
    GLuint texture_id;
    GLuint sampler_id;
    glGenTextures(1, &texture_id);
    glGenSamplers(1, &sampler_id);

    // Veja slides 95-96 do documento Aula_20_Mapeamento_de_Texturas.pdf
    glSamplerParameteri(sampler_id, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glSamplerParameteri(sampler_id, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    // Parâmetros de amostragem da textura.
    glSamplerParameteri(sampler_id, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glSamplerParameteri(sampler_id, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // Agora enviamos a imagem lida do disco para a GPU
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
    glPixelStorei(GL_UNPACK_SKIP_PIXELS, 0);
    glPixelStorei(GL_UNPACK_SKIP_ROWS, 0);

    GLuint textureunit = g_NumLoadedTextures;
    glActiveTexture(GL_TEXTURE0 + textureunit);
    glBindTexture(GL_TEXTURE_2D, texture_id);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_SRGB8, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data); // Usar GL_RGBA para transparencia
    glGenerateMipmap(GL_TEXTURE_2D);
    glBindSampler(textureunit, sampler_id);

    stbi_image_free(data);

    g_NumLoadedTextures += 1;
}

// Função que desenha um objeto armazenado em g_VirtualScene. Veja definição
// dos objetos na função BuildTrianglesAndAddToVirtualScene().
void DrawVirtualObject(const char* object_name)
{
    // "Ligamos" o VAO. Informamos que queremos utilizar os atributos de
    // vértices apontados pelo VAO criado pela função BuildTrianglesAndAddToVirtualScene(). Veja
    // comentários detalhados dentro da definição de BuildTrianglesAndAddToVirtualScene().
    glBindVertexArray(g_VirtualScene[object_name].vertex_array_object_id);

    // Setamos as variáveis "bbox_min" e "bbox_max" do fragment shader
    // com os parâmetros da axis-aligned bounding box (AABB) do modelo.
    glm::vec3 bbox_min = g_VirtualScene[object_name].bbox_min;
    glm::vec3 bbox_max = g_VirtualScene[object_name].bbox_max;
    glUniform4f(g_bbox_min_uniform, bbox_min.x, bbox_min.y, bbox_min.z, 1.0f);
    glUniform4f(g_bbox_max_uniform, bbox_max.x, bbox_max.y, bbox_max.z, 1.0f);

    // Pedimos para a GPU rasterizar os vértices dos eixos XYZ
    // apontados pelo VAO como linhas. Veja a definição de
    // g_VirtualScene[""] dentro da função BuildTrianglesAndAddToVirtualScene(), e veja
    // a documentação da função glDrawElements() em
    // http://docs.gl/gl3/glDrawElements.
    glDrawElements(
        g_VirtualScene[object_name].rendering_mode,
        g_VirtualScene[object_name].num_indices,
        GL_UNSIGNED_INT,
        (void*)(g_VirtualScene[object_name].first_index * sizeof(GLuint))
    );

    // "Desligamos" o VAO, evitando assim que operações posteriores venham a
    // alterar o mesmo. Isso evita bugs.
    glBindVertexArray(0);
}

// Função que carrega os shaders de vértices e de fragmentos que serão
// utilizados para renderização. Veja slides 180-200 do documento Aula_03_Rendering_Pipeline_Grafico.pdf.
//
void LoadShadersFromFiles()
{
    // Note que o caminho para os arquivos "shader_vertex.glsl" e
    // "shader_fragment.glsl" estão fixados, sendo que assumimos a existência
    // da seguinte estrutura no sistema de arquivos:
    //
    //    + FCG_Lab_01/
    //    |
    //    +--+ bin/
    //    |  |
    //    |  +--+ Release/  (ou Debug/ ou Linux/)
    //    |     |
    //    |     o-- main.exe
    //    |
    //    +--+ src/
    //       |
    //       o-- shader_vertex.glsl
    //       |
    //       o-- shader_fragment.glsl
    //
    GLuint vertex_shader_id = LoadShader_Vertex("../../src/shader_vertex.glsl");
    GLuint fragment_shader_id = LoadShader_Fragment("../../src/shader_fragment.glsl");

    // Deletamos o programa de GPU anterior, caso ele exista.
    if ( g_GpuProgramID != 0 )
        glDeleteProgram(g_GpuProgramID);

    // Criamos um programa de GPU utilizando os shaders carregados acima.
    g_GpuProgramID = CreateGpuProgram(vertex_shader_id, fragment_shader_id);

    // Buscamos o endereço das variáveis definidas dentro do Vertex Shader.
    // Utilizaremos estas variáveis para enviar dados para a placa de vídeo
    // (GPU)! Veja arquivo "shader_vertex.glsl" e "shader_fragment.glsl".
    g_model_uniform      = glGetUniformLocation(g_GpuProgramID, "model"); // Variável da matriz "model"
    g_view_uniform       = glGetUniformLocation(g_GpuProgramID, "view"); // Variável da matriz "view" em shader_vertex.glsl
    g_projection_uniform = glGetUniformLocation(g_GpuProgramID, "projection"); // Variável da matriz "projection" em shader_vertex.glsl
    g_object_id_uniform  = glGetUniformLocation(g_GpuProgramID, "object_id"); // Variável "object_id" em shader_fragment.glsl
    g_bbox_min_uniform   = glGetUniformLocation(g_GpuProgramID, "bbox_min");
    g_bbox_max_uniform   = glGetUniformLocation(g_GpuProgramID, "bbox_max");
    lanterna_ligada_uniform = glGetUniformLocation(g_GpuProgramID, "lanterna_ligada"); // Variável usada para ligar ou desligar a lanterna
    smoke_life_uniform = glGetUniformLocation(g_GpuProgramID, "smoke_life"); // Variável usada para definir a textura das partículas de fumaça
    nozzle_flash_uniform = glGetUniformLocation(g_GpuProgramID, "nozzle_flash"); // Variável usada para dizer se é para desenhar o flash do tiro da arma
    tronco_uniform = glGetUniformLocation(g_GpuProgramID, "tronco"); // Variável usada para dizer se é para desenhar o tronco ou as folhas da árvore
    parte_carro_uniform = glGetUniformLocation(g_GpuProgramID, "parte_carro"); // Variável usada para definir a textura de cada parte do carro
    tela_de_menu_uniform = glGetUniformLocation(g_GpuProgramID, "tela_de_menu"); // Variável usada para indicar quando está no menu

    // Variáveis em "shader_fragment.glsl" para acesso das imagens de textura
    glUseProgram(g_GpuProgramID);
    glUniform1i(glGetUniformLocation(g_GpuProgramID, "chao"), 0);
    glUniform1i(glGetUniformLocation(g_GpuProgramID, "ceu"), 1);
    glUniform1i(glGetUniformLocation(g_GpuProgramID, "lanterna"), 2);
    glUniform1i(glGetUniformLocation(g_GpuProgramID, "crosshair"), 3);
    glUniform1i(glGetUniformLocation(g_GpuProgramID, "chao_normal"), 4);
    glUniform1i(glGetUniformLocation(g_GpuProgramID, "skull_diff"), 5);
    glUniform1i(glGetUniformLocation(g_GpuProgramID, "skull_normal"), 6);
    glUniform1i(glGetUniformLocation(g_GpuProgramID, "smoke"), 7);
    glUniform1i(glGetUniformLocation(g_GpuProgramID, "bark"), 8);
    glUniform1i(glGetUniformLocation(g_GpuProgramID, "folhas"), 9);
    glUniform1i(glGetUniformLocation(g_GpuProgramID, "cabine_diff"), 10);
    glUniform1i(glGetUniformLocation(g_GpuProgramID, "cabine_normal"), 11);
    glUniform1i(glGetUniformLocation(g_GpuProgramID, "cabine_spec"), 12);
    glUniform1i(glGetUniformLocation(g_GpuProgramID, "car_BL1"), 13);
    glUniform1i(glGetUniformLocation(g_GpuProgramID, "car_BL2"), 14);
    glUniform1i(glGetUniformLocation(g_GpuProgramID, "car_GL"), 15);
    glUniform1i(glGetUniformLocation(g_GpuProgramID, "car_HL"), 16);
    glUniform1i(glGetUniformLocation(g_GpuProgramID, "car_BL"), 17);
    glUniform1i(glGetUniformLocation(g_GpuProgramID, "car_Plaque"), 18);
    glUniform1i(glGetUniformLocation(g_GpuProgramID, "car_Logo"), 19);
    glUniform1i(glGetUniformLocation(g_GpuProgramID, "car_Tire"), 20);
    glUniform1i(glGetUniformLocation(g_GpuProgramID, "tela_fim_de_jogo"), 21);

    glUseProgram(0);
}

// Função que pega a matriz M e guarda a mesma no topo da pilha
void PushMatrix(glm::mat4 M)
{
    g_MatrixStack.push(M);
}

// Função que remove a matriz atualmente no topo da pilha e armazena a mesma na variável M
void PopMatrix(glm::mat4& M)
{
    if ( g_MatrixStack.empty() )
    {
        M = Matrix_Identity();
    }
    else
    {
        M = g_MatrixStack.top();
        g_MatrixStack.pop();
    }
}

// Função que computa as normais de um ObjModel, caso elas não tenham sido
// especificadas dentro do arquivo ".obj"
void ComputeNormals(ObjModel* model)
{
    if ( !model->attrib.normals.empty() )
        return;

    // Primeiro computamos as normais para todos os TRIÂNGULOS.
    // Segundo, computamos as normais dos VÉRTICES através do método proposto
    // por Gouraud, onde a normal de cada vértice vai ser a média das normais de
    // todas as faces que compartilham este vértice.

    size_t num_vertices = model->attrib.vertices.size() / 3;

    std::vector<int> num_triangles_per_vertex(num_vertices, 0);
    std::vector<glm::vec4> vertex_normals(num_vertices, glm::vec4(0.0f,0.0f,0.0f,0.0f));

    for (size_t shape = 0; shape < model->shapes.size(); ++shape)
    {
        size_t num_triangles = model->shapes[shape].mesh.num_face_vertices.size();

        for (size_t triangle = 0; triangle < num_triangles; ++triangle)
        {
            assert(model->shapes[shape].mesh.num_face_vertices[triangle] == 3);

            glm::vec4  vertices[3];
            for (size_t vertex = 0; vertex < 3; ++vertex)
            {
                tinyobj::index_t idx = model->shapes[shape].mesh.indices[3*triangle + vertex];
                const float vx = model->attrib.vertices[3*idx.vertex_index + 0];
                const float vy = model->attrib.vertices[3*idx.vertex_index + 1];
                const float vz = model->attrib.vertices[3*idx.vertex_index + 2];
                vertices[vertex] = glm::vec4(vx,vy,vz,1.0);
            }

            const glm::vec4  a = vertices[0];
            const glm::vec4  b = vertices[1];
            const glm::vec4  c = vertices[2];

            const glm::vec4  n = crossproduct(b-a,c-a);

            for (size_t vertex = 0; vertex < 3; ++vertex)
            {
                tinyobj::index_t idx = model->shapes[shape].mesh.indices[3*triangle + vertex];
                num_triangles_per_vertex[idx.vertex_index] += 1;
                vertex_normals[idx.vertex_index] += n;
                model->shapes[shape].mesh.indices[3*triangle + vertex].normal_index = idx.vertex_index;
            }
        }
    }

    model->attrib.normals.resize( 3*num_vertices );

    for (size_t i = 0; i < vertex_normals.size(); ++i)
    {
        glm::vec4 n = vertex_normals[i] / (float)num_triangles_per_vertex[i];
        n /= norm(n);
        model->attrib.normals[3*i + 0] = n.x;
        model->attrib.normals[3*i + 1] = n.y;
        model->attrib.normals[3*i + 2] = n.z;
    }
}

// Constrói triângulos para futura renderização a partir de um ObjModel.
void BuildTrianglesAndAddToVirtualScene(ObjModel* model)
{
    GLuint vertex_array_object_id;
    glGenVertexArrays(1, &vertex_array_object_id);
    glBindVertexArray(vertex_array_object_id);

    std::vector<GLuint> indices;
    std::vector<float>  model_coefficients;
    std::vector<float>  normal_coefficients;
    std::vector<float>  texture_coefficients;

    for (size_t shape = 0; shape < model->shapes.size(); ++shape)
    {
        size_t first_index = indices.size();
        size_t num_triangles = model->shapes[shape].mesh.num_face_vertices.size();

        const float minval = std::numeric_limits<float>::min();
        const float maxval = std::numeric_limits<float>::max();

        glm::vec3 bbox_min = glm::vec3(maxval,maxval,maxval);
        glm::vec3 bbox_max = glm::vec3(minval,minval,minval);

        for (size_t triangle = 0; triangle < num_triangles; ++triangle)
        {
            assert(model->shapes[shape].mesh.num_face_vertices[triangle] == 3);

            for (size_t vertex = 0; vertex < 3; ++vertex)
            {
                tinyobj::index_t idx = model->shapes[shape].mesh.indices[3*triangle + vertex];

                indices.push_back(first_index + 3*triangle + vertex);

                const float vx = model->attrib.vertices[3*idx.vertex_index + 0];
                const float vy = model->attrib.vertices[3*idx.vertex_index + 1];
                const float vz = model->attrib.vertices[3*idx.vertex_index + 2];
                //printf("tri %d vert %d = (%.2f, %.2f, %.2f)\n", (int)triangle, (int)vertex, vx, vy, vz);
                model_coefficients.push_back( vx ); // X
                model_coefficients.push_back( vy ); // Y
                model_coefficients.push_back( vz ); // Z
                model_coefficients.push_back( 1.0f ); // W

                bbox_min.x = std::min(bbox_min.x, vx);
                bbox_min.y = std::min(bbox_min.y, vy);
                bbox_min.z = std::min(bbox_min.z, vz);
                bbox_max.x = std::max(bbox_max.x, vx);
                bbox_max.y = std::max(bbox_max.y, vy);
                bbox_max.z = std::max(bbox_max.z, vz);

                // Inspecionando o código da tinyobjloader, o aluno Bernardo
                // Sulzbach (2017/1) apontou que a maneira correta de testar se
                // existem normais e coordenadas de textura no ObjModel é
                // comparando se o índice retornado é -1. Fazemos isso abaixo.

                if ( idx.normal_index != -1 )
                {
                    const float nx = model->attrib.normals[3*idx.normal_index + 0];
                    const float ny = model->attrib.normals[3*idx.normal_index + 1];
                    const float nz = model->attrib.normals[3*idx.normal_index + 2];
                    normal_coefficients.push_back( nx ); // X
                    normal_coefficients.push_back( ny ); // Y
                    normal_coefficients.push_back( nz ); // Z
                    normal_coefficients.push_back( 0.0f ); // W
                }

                if ( idx.texcoord_index != -1 )
                {
                    const float u = model->attrib.texcoords[2*idx.texcoord_index + 0];
                    const float v = model->attrib.texcoords[2*idx.texcoord_index + 1];
                    texture_coefficients.push_back( u );
                    texture_coefficients.push_back( v );
                }
            }
        }

        size_t last_index = indices.size() - 1;

        SceneObject theobject;
        theobject.name           = model->shapes[shape].name;
        theobject.first_index    = first_index; // Primeiro índice
        theobject.num_indices    = last_index - first_index + 1; // Número de indices
        theobject.rendering_mode = GL_TRIANGLES;       // Índices correspondem ao tipo de rasterização GL_TRIANGLES.
        theobject.vertex_array_object_id = vertex_array_object_id;

        theobject.bbox_min = bbox_min;
        theobject.bbox_max = bbox_max;

        g_VirtualScene[model->shapes[shape].name] = theobject;
    }

    GLuint VBO_model_coefficients_id;
    glGenBuffers(1, &VBO_model_coefficients_id);
    glBindBuffer(GL_ARRAY_BUFFER, VBO_model_coefficients_id);
    glBufferData(GL_ARRAY_BUFFER, model_coefficients.size() * sizeof(float), NULL, GL_STATIC_DRAW);
    glBufferSubData(GL_ARRAY_BUFFER, 0, model_coefficients.size() * sizeof(float), model_coefficients.data());
    GLuint location = 0; // "(location = 0)" em "shader_vertex.glsl"
    GLint  number_of_dimensions = 4; // vec4 em "shader_vertex.glsl"
    glVertexAttribPointer(location, number_of_dimensions, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(location);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    if ( !normal_coefficients.empty() )
    {
        GLuint VBO_normal_coefficients_id;
        glGenBuffers(1, &VBO_normal_coefficients_id);
        glBindBuffer(GL_ARRAY_BUFFER, VBO_normal_coefficients_id);
        glBufferData(GL_ARRAY_BUFFER, normal_coefficients.size() * sizeof(float), NULL, GL_STATIC_DRAW);
        glBufferSubData(GL_ARRAY_BUFFER, 0, normal_coefficients.size() * sizeof(float), normal_coefficients.data());
        location = 1; // "(location = 1)" em "shader_vertex.glsl"
        number_of_dimensions = 4; // vec4 em "shader_vertex.glsl"
        glVertexAttribPointer(location, number_of_dimensions, GL_FLOAT, GL_FALSE, 0, 0);
        glEnableVertexAttribArray(location);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }

    if ( !texture_coefficients.empty() )
    {
        GLuint VBO_texture_coefficients_id;
        glGenBuffers(1, &VBO_texture_coefficients_id);
        glBindBuffer(GL_ARRAY_BUFFER, VBO_texture_coefficients_id);
        glBufferData(GL_ARRAY_BUFFER, texture_coefficients.size() * sizeof(float), NULL, GL_STATIC_DRAW);
        glBufferSubData(GL_ARRAY_BUFFER, 0, texture_coefficients.size() * sizeof(float), texture_coefficients.data());
        location = 2; // "(location = 1)" em "shader_vertex.glsl"
        number_of_dimensions = 2; // vec2 em "shader_vertex.glsl"
        glVertexAttribPointer(location, number_of_dimensions, GL_FLOAT, GL_FALSE, 0, 0);
        glEnableVertexAttribArray(location);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }

    GLuint indices_id;
    glGenBuffers(1, &indices_id);

    // "Ligamos" o buffer. Note que o tipo agora é GL_ELEMENT_ARRAY_BUFFER.
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indices_id);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(GLuint), NULL, GL_STATIC_DRAW);
    glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, indices.size() * sizeof(GLuint), indices.data());
    // glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0); // XXX Errado!
    //

    // "Desligamos" o VAO, evitando assim que operações posteriores venham a
    // alterar o mesmo. Isso evita bugs.
    glBindVertexArray(0);
}

// Carrega um Vertex Shader de um arquivo GLSL. Veja definição de LoadShader() abaixo.
GLuint LoadShader_Vertex(const char* filename)
{
    // Criamos um identificador (ID) para este shader, informando que o mesmo
    // será aplicado nos vértices.
    GLuint vertex_shader_id = glCreateShader(GL_VERTEX_SHADER);

    // Carregamos e compilamos o shader
    LoadShader(filename, vertex_shader_id);

    // Retorna o ID gerado acima
    return vertex_shader_id;
}

// Carrega um Fragment Shader de um arquivo GLSL . Veja definição de LoadShader() abaixo.
GLuint LoadShader_Fragment(const char* filename)
{
    // Criamos um identificador (ID) para este shader, informando que o mesmo
    // será aplicado nos fragmentos.
    GLuint fragment_shader_id = glCreateShader(GL_FRAGMENT_SHADER);

    // Carregamos e compilamos o shader
    LoadShader(filename, fragment_shader_id);

    // Retorna o ID gerado acima
    return fragment_shader_id;
}

// Função auxilar, utilizada pelas duas funções acima. Carrega código de GPU de
// um arquivo GLSL e faz sua compilação.
void LoadShader(const char* filename, GLuint shader_id)
{
    // Lemos o arquivo de texto indicado pela variável "filename"
    // e colocamos seu conteúdo em memória, apontado pela variável
    // "shader_string".
    std::ifstream file;
    try {
        file.exceptions(std::ifstream::failbit);
        file.open(filename);
    } catch ( std::exception& e ) {
        fprintf(stderr, "ERROR: Cannot open file \"%s\".\n", filename);
        std::exit(EXIT_FAILURE);
    }
    std::stringstream shader;
    shader << file.rdbuf();
    std::string str = shader.str();
    const GLchar* shader_string = str.c_str();
    const GLint   shader_string_length = static_cast<GLint>( str.length() );

    // Define o código do shader GLSL, contido na string "shader_string"
    glShaderSource(shader_id, 1, &shader_string, &shader_string_length);

    // Compila o código do shader GLSL (em tempo de execução)
    glCompileShader(shader_id);

    // Verificamos se ocorreu algum erro ou "warning" durante a compilação
    GLint compiled_ok;
    glGetShaderiv(shader_id, GL_COMPILE_STATUS, &compiled_ok);

    GLint log_length = 0;
    glGetShaderiv(shader_id, GL_INFO_LOG_LENGTH, &log_length);

    // Alocamos memória para guardar o log de compilação.
    // A chamada "new" em C++ é equivalente ao "malloc()" do C.
    GLchar* log = new GLchar[log_length];
    glGetShaderInfoLog(shader_id, log_length, &log_length, log);

    // Imprime no terminal qualquer erro ou "warning" de compilação
    if ( log_length != 0 )
    {
        std::string  output;

        if ( !compiled_ok )
        {
            output += "ERROR: OpenGL compilation of \"";
            output += filename;
            output += "\" failed.\n";
            output += "== Start of compilation log\n";
            output += log;
            output += "== End of compilation log\n";
        }
        else
        {
            output += "WARNING: OpenGL compilation of \"";
            output += filename;
            output += "\".\n";
            output += "== Start of compilation log\n";
            output += log;
            output += "== End of compilation log\n";
        }

        fprintf(stderr, "%s", output.c_str());
    }

    // A chamada "delete" em C++ é equivalente ao "free()" do C
    delete [] log;
}

// Esta função cria um programa de GPU, o qual contém obrigatoriamente um
// Vertex Shader e um Fragment Shader.
GLuint CreateGpuProgram(GLuint vertex_shader_id, GLuint fragment_shader_id)
{
    // Criamos um identificador (ID) para este programa de GPU
    GLuint program_id = glCreateProgram();

    // Definição dos dois shaders GLSL que devem ser executados pelo programa
    glAttachShader(program_id, vertex_shader_id);
    glAttachShader(program_id, fragment_shader_id);

    // Linkagem dos shaders acima ao programa
    glLinkProgram(program_id);

    // Verificamos se ocorreu algum erro durante a linkagem
    GLint linked_ok = GL_FALSE;
    glGetProgramiv(program_id, GL_LINK_STATUS, &linked_ok);

    // Imprime no terminal qualquer erro de linkagem
    if ( linked_ok == GL_FALSE )
    {
        GLint log_length = 0;
        glGetProgramiv(program_id, GL_INFO_LOG_LENGTH, &log_length);

        // Alocamos memória para guardar o log de compilação.
        // A chamada "new" em C++ é equivalente ao "malloc()" do C.
        GLchar* log = new GLchar[log_length];

        glGetProgramInfoLog(program_id, log_length, &log_length, log);

        std::string output;

        output += "ERROR: OpenGL linking of program failed.\n";
        output += "== Start of link log\n";
        output += log;
        output += "\n== End of link log\n";

        // A chamada "delete" em C++ é equivalente ao "free()" do C
        delete [] log;

        fprintf(stderr, "%s", output.c_str());
    }

    // Os "Shader Objects" podem ser marcados para deleção após serem linkados
    glDeleteShader(vertex_shader_id);
    glDeleteShader(fragment_shader_id);

    // Retornamos o ID gerado acima
    return program_id;
}

// Definição da função que será chamada sempre que a janela do sistema
// operacional for redimensionada, por consequência alterando o tamanho do
// "framebuffer" (região de memória onde são armazenados os pixels da imagem).
void FramebufferSizeCallback(GLFWwindow* window, int width, int height)
{
    // Indicamos que queremos renderizar em toda região do framebuffer. A
    // função "glViewport" define o mapeamento das "normalized device
    // coordinates" (NDC) para "pixel coordinates".  Essa é a operação de
    // "Screen Mapping" ou "Viewport Mapping" vista em aula ({+ViewportMapping2+}).
    glViewport(0, 0, width, height);

    // Atualizamos também a razão que define a proporção da janela (largura /
    // altura), a qual será utilizada na definição das matrizes de projeção,
    // tal que não ocorra distorções durante o processo de "Screen Mapping"
    // acima, quando NDC é mapeado para coordenadas de pixels. Veja slides 205-215 do documento Aula_09_Projecoes.pdf.
    //
    // O cast para float é necessário pois números inteiros são arredondados ao
    // serem divididos!
    g_ScreenRatio = (float)width / height;
}

// Variáveis globais que armazenam a última posição do cursor do mouse, para
// que possamos calcular quanto que o mouse se movimentou entre dois instantes
// de tempo. Utilizadas no callback CursorPosCallback() abaixo.

// Função callback chamada sempre que o usuário aperta algum dos botões do mouse
void MouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
{
    if (button == GLFW_MOUSE_BUTTON_LEFT)
    {
        if (action == GLFW_PRESS)
            g_LeftMouseButtonPressed = true;
        else if (action == GLFW_RELEASE)
            g_LeftMouseButtonPressed = false;
    }
}

void centerMouse(GLFWwindow* window, int screenWidth, int screenHeight) {
    glfwSetCursorPos(window, screenWidth / 2.0, screenHeight / 2.0);
    cursorCentered = true;
}

// Função callback chamada sempre que o usuário movimentar o cursor do mouse em
// cima da janela OpenGL.
void CursorPosCallback(GLFWwindow* window, double xpos, double ypos)
{
    if (cursorCentered) {
        cursorCentered = false;
        g_LastCursorPosX = xpos;
        g_LastCursorPosY = ypos;
        return;
    }

    // Deslocamento do cursor do mouse em x e y de coordenadas de tela!

    float dx = xpos - g_LastCursorPosX;
    float dy = ypos - g_LastCursorPosY;


    // Atualizamos parâmetros da câmera com os deslocamentos
    g_CameraTheta -= 0.003f*dx;
    g_CameraPhi   -= 0.003f*dy;

    // Em coordenadas esféricas, o ângulo phi deve ficar entre -pi/2 e +pi/2.
    float phimax = PI/2;
    float phimin = -phimax;

    if (g_CameraPhi > phimax)
        g_CameraPhi = phimax;

    if (g_CameraPhi < phimin)
        g_CameraPhi = phimin;

    // Atualizamos as variáveis globais para armazenar a posição atual do
    // cursor como sendo a última posição conhecida do cursor.
    g_LastCursorPosX = xpos;
    g_LastCursorPosY = ypos;

    centerMouse(window, 1200, 800);
}


// Função callback chamada sempre que o usuário movimenta a "rodinha" do mouse.
void ScrollCallback(GLFWwindow* window, double xoffset, double yoffset)
{
    // A FAZER
}

// Definição da função que será chamada sempre que o usuário pressionar alguma
// tecla do teclado. Veja http://www.glfw.org/docs/latest/input_guide.html#input_key
void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mod)
{
        if (key == GLFW_KEY_K)              // TESTE PARA TELA DE FIM DE JOGO, SERÁ ARRUMADO
    {
        if (action == GLFW_PRESS)
            tecla_K_pressionada = true;
        else if (action == GLFW_RELEASE)
            tecla_K_pressionada = false;
    }


        if (key == GLFW_KEY_W)
    {
        if (action == GLFW_PRESS)
            tecla_W_pressionada = true;
        else if (action == GLFW_RELEASE)
            tecla_W_pressionada = false;
    }

    if (key == GLFW_KEY_A)
    {
        if (action == GLFW_PRESS)
            tecla_A_pressionada = true;
        else if (action == GLFW_RELEASE)
            tecla_A_pressionada = false;
    }

    if (key == GLFW_KEY_S)
    {
        if (action == GLFW_PRESS)
            tecla_S_pressionada = true;
        else if (action == GLFW_RELEASE)
            tecla_S_pressionada = false;
    }

    if (key == GLFW_KEY_D)
    {
        if (action == GLFW_PRESS)
            tecla_D_pressionada = true;
        else if (action == GLFW_RELEASE)
            tecla_D_pressionada = false;
    }

    if (key == GLFW_KEY_E)
    {
        if (action == GLFW_PRESS)
            tecla_E_pressionada = true;
        else if (action == GLFW_RELEASE)
            tecla_E_pressionada = false;
    }

        if (key == GLFW_KEY_F)
    {
        if (action == GLFW_PRESS && tecla_F_pressionada == false)
            tecla_F_pressionada = true;
        else if (action == GLFW_PRESS && tecla_F_pressionada == true)
            tecla_F_pressionada = false;
    }

    if (key == GLFW_KEY_R)
    {
        if (action == GLFW_PRESS)
            tecla_R_pressionada = true;
        else if (action == GLFW_RELEASE)
            tecla_R_pressionada = false;
    }

    // Se o usuário pressionar SHIFT, a velocidade de movimento aumenta
    if (key == GLFW_KEY_LEFT_SHIFT)
    {
        if (action == GLFW_PRESS)
            tecla_SHIFT_pressionada = true;
        else if (action == GLFW_RELEASE)
            tecla_SHIFT_pressionada = false;
    }

    // Se o usuário pressionar SPACE, o personagem pula
    if (key == GLFW_KEY_SPACE)
    {
        if (action == GLFW_PRESS)
            tecla_SPACE_pressionada = true;
        else if (action == GLFW_RELEASE)
            tecla_SPACE_pressionada = false;
    }

    // Se o usuário pressionar a tecla ESC, fechamos a janela.
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, GL_TRUE);

    // Se o usuário apertar a tecla P, utilizamos projeção perspectiva.
    if (key == GLFW_KEY_P && action == GLFW_PRESS)
    {
        g_UsePerspectiveProjection = true;
    }

    // Se o usuário apertar a tecla O, utilizamos projeção ortográfica.
    if (key == GLFW_KEY_O && action == GLFW_PRESS)
    {
        g_UsePerspectiveProjection = false;
    }

    // Se o usuário apertar a tecla H, fazemos um "toggle" do texto informativo mostrado na tela.
    if (key == GLFW_KEY_H && action == GLFW_PRESS)
    {
        g_ShowInfoText = !g_ShowInfoText;
    }
}

// Definimos o callback para impressão de erros da GLFW no terminal
void ErrorCallback(int error, const char* description)
{
    fprintf(stderr, "ERROR: GLFW: %s\n", description);
}

// Esta função recebe um vértice com coordenadas de modelo p_model e passa o
// mesmo por todos os sistemas de coordenadas armazenados nas matrizes model,
// view, e projection; e escreve na tela as matrizes e pontos resultantes
// dessas transformações.
void TextRendering_ShowModelViewProjection(
    GLFWwindow* window,
    glm::mat4 projection,
    glm::mat4 view,
    glm::mat4 model,
    glm::vec4 p_model
)
{
    if ( !g_ShowInfoText )
        return;

    glm::vec4 p_world = model*p_model;
    glm::vec4 p_camera = view*p_world;
    glm::vec4 p_clip = projection*p_camera;
    glm::vec4 p_ndc = p_clip / p_clip.w;

    float pad = TextRendering_LineHeight(window);

    TextRendering_PrintString(window, " Model matrix             Model     In World Coords.", -1.0f, 1.0f-pad, 1.0f);
    TextRendering_PrintMatrixVectorProduct(window, model, p_model, -1.0f, 1.0f-2*pad, 1.0f);

    TextRendering_PrintString(window, "                                        |  ", -1.0f, 1.0f-6*pad, 1.0f);
    TextRendering_PrintString(window, "                            .-----------'  ", -1.0f, 1.0f-7*pad, 1.0f);
    TextRendering_PrintString(window, "                            V              ", -1.0f, 1.0f-8*pad, 1.0f);

    TextRendering_PrintString(window, " View matrix              World     In Camera Coords.", -1.0f, 1.0f-9*pad, 1.0f);
    TextRendering_PrintMatrixVectorProduct(window, view, p_world, -1.0f, 1.0f-10*pad, 1.0f);

    TextRendering_PrintString(window, "                                        |  ", -1.0f, 1.0f-14*pad, 1.0f);
    TextRendering_PrintString(window, "                            .-----------'  ", -1.0f, 1.0f-15*pad, 1.0f);
    TextRendering_PrintString(window, "                            V              ", -1.0f, 1.0f-16*pad, 1.0f);

    TextRendering_PrintString(window, " Projection matrix        Camera                    In NDC", -1.0f, 1.0f-17*pad, 1.0f);
    TextRendering_PrintMatrixVectorProductDivW(window, projection, p_camera, -1.0f, 1.0f-18*pad, 1.0f);

    int width, height;
    glfwGetFramebufferSize(window, &width, &height);

    glm::vec2 a = glm::vec2(-1, -1);
    glm::vec2 b = glm::vec2(+1, +1);
    glm::vec2 p = glm::vec2( 0,  0);
    glm::vec2 q = glm::vec2(width, height);

    glm::mat4 viewport_mapping = Matrix(
        (q.x - p.x)/(b.x-a.x), 0.0f, 0.0f, (b.x*p.x - a.x*q.x)/(b.x-a.x),
        0.0f, (q.y - p.y)/(b.y-a.y), 0.0f, (b.y*p.y - a.y*q.y)/(b.y-a.y),
        0.0f , 0.0f , 1.0f , 0.0f ,
        0.0f , 0.0f , 0.0f , 1.0f
    );

    TextRendering_PrintString(window, "                                                       |  ", -1.0f, 1.0f-22*pad, 1.0f);
    TextRendering_PrintString(window, "                            .--------------------------'  ", -1.0f, 1.0f-23*pad, 1.0f);
    TextRendering_PrintString(window, "                            V                           ", -1.0f, 1.0f-24*pad, 1.0f);

    TextRendering_PrintString(window, " Viewport matrix           NDC      In Pixel Coords.", -1.0f, 1.0f-25*pad, 1.0f);
    TextRendering_PrintMatrixVectorProductMoreDigits(window, viewport_mapping, p_ndc, -1.0f, 1.0f-26*pad, 1.0f);
}

// Escrevemos na tela qual matriz de projeção está sendo utilizada.
void TextRendering_ShowProjection(GLFWwindow* window)
{
    if ( !g_ShowInfoText )
        return;

    float lineheight = TextRendering_LineHeight(window);
    float charwidth = TextRendering_CharWidth(window);

    if ( g_UsePerspectiveProjection )
        TextRendering_PrintString(window, "Perspective", 1.0f-13*charwidth, -1.0f+2*lineheight/10, 1.0f);
    else
        TextRendering_PrintString(window, "Orthographic", 1.0f-13*charwidth, -1.0f+2*lineheight/10, 1.0f);
}

// Escrevemos na tela o número de quadros renderizados por segundo (frames per
// second).
void TextRendering_ShowFramesPerSecond(GLFWwindow* window)
{
    if ( !g_ShowInfoText )
        return;

    // Variáveis estáticas (static) mantém seus valores entre chamadas
    // subsequentes da função!
    static float old_seconds = (float)glfwGetTime();
    static int   ellapsed_frames = 0;
    static char  buffer[20] = "?? fps";
    static int   numchars = 7;

    ellapsed_frames += 1;

    // Recuperamos o número de segundos que passou desde a execução do programa
    float seconds = (float)glfwGetTime();

    // Número de segundos desde o último cálculo do fps
    float ellapsed_seconds = seconds - old_seconds;

    if ( ellapsed_seconds > 1.0f )
    {
        numchars = snprintf(buffer, 20, "%.2f fps", ellapsed_frames / ellapsed_seconds);

        old_seconds = seconds;
        ellapsed_frames = 0;
    }

    float lineheight = TextRendering_LineHeight(window);
    float charwidth = TextRendering_CharWidth(window);

    TextRendering_PrintString(window, buffer, 1.0f-(numchars + 1)*charwidth, 1.0f-lineheight, 1.0f);
}

// Escrevemos na tela o número de segundos passados desde o início.
void TextRendering_ShowSecondsEllapsed(GLFWwindow* window)
{
    if ( !g_ShowInfoText )
        return;

    static char  buffer[20] = "?? Seg";
    static int   numchars = 7;

    // Recuperamos o número de segundos que passou desde a execução do programa
    float seconds = (float)glfwGetTime();

    numchars = snprintf(buffer, 20, "%.1f Seg", seconds);

    float lineheight = TextRendering_LineHeight(window);
    float charwidth = TextRendering_CharWidth(window);

    TextRendering_PrintString(window, buffer, -0.85f-(numchars + 1)*charwidth, 1.0f-lineheight, 1.0f);
}

// Escrevemos na tela o número de segundos passados desde o início.
void TextRendering_ShowAMMO(GLFWwindow* window, int ammo)
{
    if ( !g_ShowInfoText )
        return;

    static int   numchars = 7;

    char buffer[20] = "AMMO: ?/6";

    numchars = snprintf(buffer, 20, "AMMO: %d/6", ammo);

    float lineheight = TextRendering_LineHeight(window);
    float charwidth = TextRendering_CharWidth(window);

    TextRendering_PrintString(window, buffer, -0.85f-(numchars + 1)*charwidth, -0.95f+lineheight, 5.0f);
}

// Escrevemos na tela a instrução para conserto do carro
void TextRendering_ShowCarTip(GLFWwindow* window, float estado_carro)
{
    if ( !g_ShowInfoText )
        return;

    char buffer[200];

    int numchars = snprintf(buffer, 200, "Pressione E para consertar o carro (%.1f/100)", estado_carro);

    float lineheight = TextRendering_LineHeight(window);
    float charwidth = TextRendering_CharWidth(window);

    TextRendering_PrintString(window, buffer, 0.0f-(numchars + 1)*charwidth, -0.2f+lineheight, 2.0f);
}

// escrevendo na tela o menu

void TextRendering_Menu(GLFWwindow* window, int mov_escrita){
    if ( !g_ShowInfoText )
        return;

    char buffer[200];

    int numchars = snprintf(buffer, 200, "Aperte [SPACE] pra comecar o jogo!");

    float lineheight = TextRendering_LineHeight(window);
    float charwidth = TextRendering_CharWidth(window);

    TextRendering_PrintString(window, buffer, -0.5f-(numchars + 1)*charwidth, 0.60*sin(float(mov_escrita)/200), 5.0f);
}

// Função para debugging: imprime no terminal todas informações de um modelo
// geométrico carregado de um arquivo ".obj".
// Veja: https://github.com/syoyo/tinyobjloader/blob/22883def8db9ef1f3ffb9b404318e7dd25fdbb51/loader_example.cc#L98
void PrintObjModelInfo(ObjModel* model)
{
  const tinyobj::attrib_t                & attrib    = model->attrib;
  const std::vector<tinyobj::shape_t>    & shapes    = model->shapes;
  const std::vector<tinyobj::material_t> & materials = model->materials;

  printf("# of vertices  : %d\n", (int)(attrib.vertices.size() / 3));
  printf("# of normals   : %d\n", (int)(attrib.normals.size() / 3));
  printf("# of texcoords : %d\n", (int)(attrib.texcoords.size() / 2));
  printf("# of shapes    : %d\n", (int)shapes.size());
  printf("# of materials : %d\n", (int)materials.size());

  for (size_t v = 0; v < attrib.vertices.size() / 3; v++) {
    printf("  v[%ld] = (%f, %f, %f)\n", static_cast<long>(v),
           static_cast<const double>(attrib.vertices[3 * v + 0]),
           static_cast<const double>(attrib.vertices[3 * v + 1]),
           static_cast<const double>(attrib.vertices[3 * v + 2]));
  }

  for (size_t v = 0; v < attrib.normals.size() / 3; v++) {
    printf("  n[%ld] = (%f, %f, %f)\n", static_cast<long>(v),
           static_cast<const double>(attrib.normals[3 * v + 0]),
           static_cast<const double>(attrib.normals[3 * v + 1]),
           static_cast<const double>(attrib.normals[3 * v + 2]));
  }

  for (size_t v = 0; v < attrib.texcoords.size() / 2; v++) {
    printf("  uv[%ld] = (%f, %f)\n", static_cast<long>(v),
           static_cast<const double>(attrib.texcoords[2 * v + 0]),
           static_cast<const double>(attrib.texcoords[2 * v + 1]));
  }

  // For each shape
  for (size_t i = 0; i < shapes.size(); i++) {
    printf("shape[%ld].name = %s\n", static_cast<long>(i),
           shapes[i].name.c_str());
    printf("Size of shape[%ld].indices: %lu\n", static_cast<long>(i),
           static_cast<unsigned long>(shapes[i].mesh.indices.size()));

    size_t index_offset = 0;

    assert(shapes[i].mesh.num_face_vertices.size() ==
           shapes[i].mesh.material_ids.size());

    printf("shape[%ld].num_faces: %lu\n", static_cast<long>(i),
           static_cast<unsigned long>(shapes[i].mesh.num_face_vertices.size()));

    // For each face
    for (size_t f = 0; f < shapes[i].mesh.num_face_vertices.size(); f++) {
      size_t fnum = shapes[i].mesh.num_face_vertices[f];

      printf("  face[%ld].fnum = %ld\n", static_cast<long>(f),
             static_cast<unsigned long>(fnum));

      // For each vertex in the face
      for (size_t v = 0; v < fnum; v++) {
        tinyobj::index_t idx = shapes[i].mesh.indices[index_offset + v];
        printf("    face[%ld].v[%ld].idx = %d/%d/%d\n", static_cast<long>(f),
               static_cast<long>(v), idx.vertex_index, idx.normal_index,
               idx.texcoord_index);
      }

      printf("  face[%ld].material_id = %d\n", static_cast<long>(f),
             shapes[i].mesh.material_ids[f]);

      index_offset += fnum;
    }

    printf("shape[%ld].num_tags: %lu\n", static_cast<long>(i),
           static_cast<unsigned long>(shapes[i].mesh.tags.size()));
    for (size_t t = 0; t < shapes[i].mesh.tags.size(); t++) {
      printf("  tag[%ld] = %s ", static_cast<long>(t),
             shapes[i].mesh.tags[t].name.c_str());
      printf(" ints: [");
      for (size_t j = 0; j < shapes[i].mesh.tags[t].intValues.size(); ++j) {
        printf("%ld", static_cast<long>(shapes[i].mesh.tags[t].intValues[j]));
        if (j < (shapes[i].mesh.tags[t].intValues.size() - 1)) {
          printf(", ");
        }
      }
      printf("]");

      printf(" floats: [");
      for (size_t j = 0; j < shapes[i].mesh.tags[t].floatValues.size(); ++j) {
        printf("%f", static_cast<const double>(
                         shapes[i].mesh.tags[t].floatValues[j]));
        if (j < (shapes[i].mesh.tags[t].floatValues.size() - 1)) {
          printf(", ");
        }
      }
      printf("]");

      printf(" strings: [");
      for (size_t j = 0; j < shapes[i].mesh.tags[t].stringValues.size(); ++j) {
        printf("%s", shapes[i].mesh.tags[t].stringValues[j].c_str());
        if (j < (shapes[i].mesh.tags[t].stringValues.size() - 1)) {
          printf(", ");
        }
      }
      printf("]");
      printf("\n");
    }
  }

  for (size_t i = 0; i < materials.size(); i++) {
    printf("material[%ld].name = %s\n", static_cast<long>(i),
           materials[i].name.c_str());
    printf("  material.Ka = (%f, %f ,%f)\n",
           static_cast<const double>(materials[i].ambient[0]),
           static_cast<const double>(materials[i].ambient[1]),
           static_cast<const double>(materials[i].ambient[2]));
    printf("  material.Kd = (%f, %f ,%f)\n",
           static_cast<const double>(materials[i].diffuse[0]),
           static_cast<const double>(materials[i].diffuse[1]),
           static_cast<const double>(materials[i].diffuse[2]));
    printf("  material.Ks = (%f, %f ,%f)\n",
           static_cast<const double>(materials[i].specular[0]),
           static_cast<const double>(materials[i].specular[1]),
           static_cast<const double>(materials[i].specular[2]));
    printf("  material.Tr = (%f, %f ,%f)\n",
           static_cast<const double>(materials[i].transmittance[0]),
           static_cast<const double>(materials[i].transmittance[1]),
           static_cast<const double>(materials[i].transmittance[2]));
    printf("  material.Ke = (%f, %f ,%f)\n",
           static_cast<const double>(materials[i].emission[0]),
           static_cast<const double>(materials[i].emission[1]),
           static_cast<const double>(materials[i].emission[2]));
    printf("  material.Ns = %f\n",
           static_cast<const double>(materials[i].shininess));
    printf("  material.Ni = %f\n", static_cast<const double>(materials[i].ior));
    printf("  material.dissolve = %f\n",
           static_cast<const double>(materials[i].dissolve));
    printf("  material.illum = %d\n", materials[i].illum);
    printf("  material.map_Ka = %s\n", materials[i].ambient_texname.c_str());
    printf("  material.map_Kd = %s\n", materials[i].diffuse_texname.c_str());
    printf("  material.map_Ks = %s\n", materials[i].specular_texname.c_str());
    printf("  material.map_Ns = %s\n",
           materials[i].specular_highlight_texname.c_str());
    printf("  material.map_bump = %s\n", materials[i].bump_texname.c_str());
    printf("  material.map_d = %s\n", materials[i].alpha_texname.c_str());
    printf("  material.disp = %s\n", materials[i].displacement_texname.c_str());
    printf("  <<PBR>>\n");
    printf("  material.Pr     = %f\n", materials[i].roughness);
    printf("  material.Pm     = %f\n", materials[i].metallic);
    printf("  material.Ps     = %f\n", materials[i].sheen);
    printf("  material.Pc     = %f\n", materials[i].clearcoat_thickness);
    printf("  material.Pcr    = %f\n", materials[i].clearcoat_thickness);
    printf("  material.aniso  = %f\n", materials[i].anisotropy);
    printf("  material.anisor = %f\n", materials[i].anisotropy_rotation);
    printf("  material.map_Ke = %s\n", materials[i].emissive_texname.c_str());
    printf("  material.map_Pr = %s\n", materials[i].roughness_texname.c_str());
    printf("  material.map_Pm = %s\n", materials[i].metallic_texname.c_str());
    printf("  material.map_Ps = %s\n", materials[i].sheen_texname.c_str());
    printf("  material.norm   = %s\n", materials[i].normal_texname.c_str());
    std::map<std::string, std::string>::const_iterator it(
        materials[i].unknown_parameter.begin());
    std::map<std::string, std::string>::const_iterator itEnd(
        materials[i].unknown_parameter.end());

    for (; it != itEnd; it++) {
      printf("  material.%s = %s\n", it->first.c_str(), it->second.c_str());
    }
    printf("\n");
  }
}

// set makeprg=cd\ ..\ &&\ make\ run\ >/dev/null
// vim: set spell spelllang=pt_br :

