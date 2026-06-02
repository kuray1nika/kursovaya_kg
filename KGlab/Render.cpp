#include "Render.h"
#include "GUItextRectangle.h"
#include "MyShaders.h"
#include "ObjLoader.h"
#include "Texture.h"
#define DEG2RAD 0.0174532925f

#include <windows.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <cmath>

#include <vector>
#include <string>
#include <iomanip>
#include <sstream>


#include "debout.h"

// Внутренняя логика "движка"
#include "MyOGL.h"
extern OpenGL gl;
#include "Light.h"
Light light;
#include "Camera.h"
Camera camera;


bool texturing = true;
bool lightning = true;
bool alpha = false;

std::vector<ObjModel> rabbit_frames;
int total_rabbit_frames = 30;

// Переключение режимов освещения, текстурирования, альфа-наложения
void switchModes(OpenGL* sender, KeyEventArg arg)
{
    // Конвертируем код клавиши в букву
    auto key = LOWORD(MapVirtualKeyA(arg.key, MAPVK_VK_TO_CHAR));

    switch (key)
    {
    case 'L':
        lightning = !lightning;
        break;
    case 'T':
        texturing = !texturing;
        break;
    case 'A':
        alpha = !alpha;
        break;
    }
}

// Умножение матриц c[M1][N1] = a[M1][N1] * b[M2][N2]
template <typename T, int M1, int N1, int M2, int N2> void MatrixMultiply(const T* a, const T* b, T* c)
{
    for (int i = 0; i < M1; ++i)
    {
        for (int j = 0; j < N2; ++j)
        {
            c[i * N2 + j] = T(0);
            for (int k = 0; k < N1; ++k)
            {
                c[i * N2 + j] += a[i * N1 + k] * b[k * N2 + j];
            }
        }
    }
}

// Текстовый прямоугольник в верхнем правом углу.
// OGL не предоставляет возможности для хранения текста;
// внутри этого класса создается картинка с текстом (через GDI),
// в виде текстуры накладывается на прямоугольник и рисуется на экране.
// Это самый простой, но очень неэффективный способ написать что-либо на экране.
GuiTextRectangle text;

// ID для текстуры
GLuint texId;

ObjModel f;

Shader cassini_sh;
Shader phong_sh;
Shader vb_sh;
Shader simple_texture_sh;

Texture stankin_tex, vb_tex, monkey_tex;
Texture diffuse_tex, normal_tex, specular_tex;
Shader normalmap_sh;

// Кусты
ObjModel bush, bush2, bush3, tree, pump;

// Текстуры первого куста
Texture bush_diffuse, bush_normal, bush_specular, bush_translucency;

// Текстуры второго куста
Texture bush2_diffuse, bush2_normal, bush2_specular, bush2_translucency;

// Текстуры третьего куста
Texture bush3_diffuse, bush3_normal, bush3_specular, bush3_translucency;

// Деревья
Texture tree1_diffuse, tree1_normal, tree1_specular, tree1_translucency;
Texture pump_diffuse, pump_normal, pump_specular, pump_translucency;
Shader bush_sh;
// === УПРАВЛЯЕМЫЙ ЗАЯЦ ===
ObjModel rabbit;
Texture rabbit_skin;
Texture rabbit_fluffiness, rabbit_normal;

ObjModel flower, clover;
Texture flower_tex, clover_tex;
float flower_x = 1.5f, flower_y = 1.5f;
float clover_x = -1.5f, clover_y = -1.5f;
int clover_count = 0;

struct Plant {
    float x, y;
    ObjModel* model;
    Texture* tex;
    bool active;
};

Plant flower_inst, clover_inst;

const float obstacles[][2] = {
    {-1.7f, -1.7f},  // bush
    {0.0f, -0.5f},   // bush2
    {-2.0f, 2.0f},   // bush3
    {1.3f, 1.3f},    // tree
    {1.7f, -1.7f}    // pump
};
const int num_obstacles = 5;

// Состояния зайца
enum RabbitState { ALIVE, DYING, RESPAWNING };
RabbitState rabbit_state = ALIVE;
float death_timer = 0.0f;
float death_rotation = 0.0f;
float respawn_timer = 0.0f;
float respawn_start_x = 0.0f, respawn_start_y = 0.0f;
float respawn_target_x = 0.0f, respawn_target_y = 0.0f;

// Позиция и состояние
float rabbit_x = 0.0f;
float rabbit_z = 0.0f;
float rabbit_y = 0.0f;     // Для прыжка
float rabbit_angle = 0.0f; // Куда смотрит
float rabbit_hop = 0.0f;   // Таймер прыжка
bool rabbit_moving = false;
// Прыжок зайца
float rabbit_jump_timer = 0.0f;
float rabbit_jump_duration = 2.0f;  // 2 секунды на прыжок
float rabbit_start_x = 0.0f;
float rabbit_start_y = 0.0f;
float rabbit_target_x = 0.0f;
float rabbit_target_y = 0.0f;
bool rabbit_is_jumping = false;
Shader rabbit_sh;

bool isSafePosition(float x, float y, float min_distance = 0.8f) {
    // Проверка границ поля
    if (x < -2.3f || x > 2.3f || y < -2.3f || y > 2.3f)
        return false;

    // Проверка расстояния до препятствий
    for (int i = 0; i < num_obstacles; i++) {
        float dist = sqrtf(pow(x - obstacles[i][0], 2) + pow(y - obstacles[i][1], 2));
        if (dist < min_distance)
            return false;
    }

    // Проверка расстояния до стартовой позиции зайца (0, 0)
    float dist_to_start = sqrtf(x * x + y * y);
    if (dist_to_start < 0.8f)
        return false;

    return true;
}

void spawnPlant(Plant& plant, const Plant& otherPlant) {
    int attempts = 0;
    do {
        plant.x = ((rand() % 40) - 20) / 10.0f;  // От -2.0 до 2.0
        plant.y = ((rand() % 40) - 20) / 10.0f;
        attempts++;
    } while (
        (!isSafePosition(plant.x, plant.y)) ||  // Проверка препятствий
        (attempts < 100 &&
            sqrtf(pow(plant.x - otherPlant.x, 2) + pow(plant.y - otherPlant.y, 2)) < 1.0f)
        );
    plant.active = true;
}

void handleRabbitInput(OpenGL* sender, KeyEventArg arg);

// Выполняется один раз перед первым рендером
void initRender()
{   normalmap_sh.VshaderFileName = "shaders/normalmap.vert";
    normalmap_sh.FshaderFileName = "shaders/normalmap.frag";
    normalmap_sh.LoadShaderFromFile();
    normalmap_sh.Compile();

    bush_sh.VshaderFileName = "shaders/bush.vert";
    bush_sh.FshaderFileName = "shaders/bush.frag";
    bush_sh.LoadShaderFromFile();
    bush_sh.Compile();

    diffuse_tex.LoadTexture("textures/floor_diffuse.tga");
    normal_tex.LoadTexture("textures/floor_normal.tga");
    specular_tex.LoadTexture("textures/floor_specular.tga");

    // ПЕРВЫЙ КУСТ
    bush_diffuse.LoadTexture("textures/diffuse.tga");
    bush_normal.LoadTexture("textures/normal1.tga");
    bush_specular.LoadTexture("textures/specular1.tga");
    bush_translucency.LoadTexture("textures/translucency.tga");
    bush.LoadModel("models/bush_01.obj");

    // ВТОРОЙ КУСТ
    bush2_diffuse.LoadTexture("textures/bush2_diffuse.tga");
    bush2_normal.LoadTexture("textures/bush2_normal.tga");
    bush2_specular.LoadTexture("textures/bush2_specular.tga");
    bush2_translucency.LoadTexture("textures/bush2_translucency.tga");
    bush2.LoadModel("models/bush_02.obj");

    // ТРЕТИЙ КУСТ
    bush3_diffuse.LoadTexture("textures/bush3_diffuse.tga");
    bush3_normal.LoadTexture("textures/bush3_normal.tga");
    bush3_specular.LoadTexture("textures/bush3_specular.tga");
    bush3_translucency.LoadTexture("textures/bush3_translucency.tga");
    bush3.LoadModel("models/bush_03.obj");

    tree1_diffuse.LoadTexture("textures/diffuse_none.tga");
    tree1_normal.LoadTexture("textures/normal_none.tga");
    tree1_specular.LoadTexture("textures/specular_none.tga");
    tree.LoadModel("models/snow_pine_tree.obj");

    pump_diffuse.LoadTexture("textures/pump_diffuse.tga");
    pump_normal.LoadTexture("textures/pump_normal.tga");
    pump_specular.LoadTexture("textures/pump_specular.tga");
    pump_translucency.LoadTexture("textures/pump_translucency.tga");
    pump.LoadModel("models/pumpkin_plant.obj");

    // Шейдер зайца
    rabbit_sh.VshaderFileName = "shaders/rabbit.vert";
    rabbit_sh.FshaderFileName = "shaders/rabbit.frag";
    rabbit_sh.LoadShaderFromFile();
    rabbit_sh.Compile();

    // Шейдер для цветка и клевера
    simple_texture_sh.VshaderFileName = "shaders/simple_texture.vert";
    simple_texture_sh.FshaderFileName = "shaders/simple_texture.frag";
    simple_texture_sh.LoadShaderFromFile();
    simple_texture_sh.Compile();

    rabbit_skin.LoadTexture("textures/rabbit-skinn.png");
    rabbit_fluffiness.LoadTexture("textures/Fur-skin.png");
    rabbit_normal.LoadTexture("textures/rabbit_NORM.png");

    flower_tex.LoadTexture("textures/HP3_flower.tga");
    clover_tex.LoadTexture("textures/HP3_bush02.tga");
    flower.LoadModel("models/HP3_flower4.obj");
    clover.LoadModel("models/HP3_bush022.obj");

    // Выделяем память под 30 моделей
    rabbit_frames.resize(total_rabbit_frames);

    for (int i = 0; i < total_rabbit_frames; i++) {
        std::stringstream ss;
        ss << "models/rabbit2" << std::setfill('0') << std::setw(4) << (i + 1) << ".obj";

        std::string filename = ss.str();

        rabbit_frames[i].LoadModel(filename.c_str());
    }

    flower_inst.model = &flower;
    flower_inst.tex = &flower_tex;
    flower_inst.x = 1.5f;
    flower_inst.y = 1.5f;
    flower_inst.active = true;

    clover_inst.model = &clover;
    clover_inst.tex = &clover_tex;
    clover_inst.x = -1.5f;
    clover_inst.y = -1.5f;
    clover_inst.active = true;

    //==============НАСТРОЙКА ТЕКСТУР================
    // 4 байта на хранение пикселя
    glPixelStorei(GL_UNPACK_ALIGNMENT, 4);

    //================НАСТРОЙКА КАМЕРЫ======================
    camera.caclulateCameraPos();

    // привязываем камеру к событиям "движка"
    gl.WheelEvent.reaction(&camera, &Camera::Zoom);
    gl.MouseMovieEvent.reaction(&camera, &Camera::MouseMovie);
    gl.MouseLeaveEvent.reaction(&camera, &Camera::MouseLeave);
    gl.MouseLdownEvent.reaction(&camera, &Camera::MouseStartDrag);
    gl.MouseLupEvent.reaction(&camera, &Camera::MouseStopDrag);
    //==============НАСТРОЙКА СВЕТА===========================
    // Привязываем свет к событиям "движка"
    gl.MouseMovieEvent.reaction(&light, &Light::MoveLight);
    gl.KeyDownEvent.reaction(&light, &Light::StartDrug);
    gl.KeyUpEvent.reaction(&light, &Light::StopDrug);
    //========================================================
    //====================Прочее==============================
    gl.KeyDownEvent.reaction(switchModes);
    gl.KeyDownEvent.reaction(handleRabbitInput);
    text.setSize(512, 180);
    //========================================================

    camera.setPosition(2, 1.5, 1.5);
}

float view_matrix[16];
double full_time = 0;
int location = 0;

// Отрисовка сетки 5×5 из травяных плиток
void DrawGrassGrid(float tileSize = 1.0f, int gridSize = 5)
{
    normalmap_sh.UseShader();

    int loc;
    loc = glGetUniformLocationARB(normalmap_sh.program, "light_pos_v");
    glUniform3fARB(loc, light.x(), light.y(), light.z());
    loc = glGetUniformLocationARB(normalmap_sh.program, "view_pos");
    glUniform3fARB(loc, camera.x(), camera.y(), camera.z());

    glActiveTexture(GL_TEXTURE0);
    diffuse_tex.Bind();
    glActiveTexture(GL_TEXTURE1);
    normal_tex.Bind();
    glActiveTexture(GL_TEXTURE2);
    specular_tex.Bind();

    loc = glGetUniformLocationARB(normalmap_sh.program, "diffuse_tex");
    glUniform1iARB(loc, 0);
    loc = glGetUniformLocationARB(normalmap_sh.program, "normal_tex");
    glUniform1iARB(loc, 1);
    loc = glGetUniformLocationARB(normalmap_sh.program, "specular_tex");
    glUniform1iARB(loc, 2);

    float halfSize = (gridSize * tileSize) / 2.0f;

    for (int i = 0; i < gridSize; ++i)
    {
        for (int j = 0; j < gridSize; ++j)
        {
            float x0 = -halfSize + j * tileSize;
            float z0 = -halfSize + i * tileSize;
            float x1 = x0 + tileSize;
            float z1 = z0 + tileSize;

            float u0 = (float)j / gridSize;
            float u1 = (float)(j + 1) / gridSize;
            float v0 = (float)i / gridSize;
            float v1 = (float)(i + 1) / gridSize;

            glBegin(GL_QUADS);
            glNormal3f(0, 1, 0);
            glTexCoord2f(u1, v1); glVertex3f(x1, 0.0f, z1);  // верхний правый
            glTexCoord2f(u1, v0); glVertex3f(x1, 0.0f, z0);  // нижний правый
            glTexCoord2f(u0, v0); glVertex3f(x0, 0.0f, z0);  // нижний левый
            glTexCoord2f(u0, v1); glVertex3f(x0, 0.0f, z1);  // верхний левый
            glEnd();
        }
    }

    glActiveTexture(GL_TEXTURE0);
}

// Универсальная отрисовка куста с прозрачностью
void DrawBush(ObjModel& model,
    Texture& diffTex, Texture& normTex, Texture& specTex, Texture& transTex,
    float x, float y, float z,
    float scale = 0.01f,
    float rotateX = 90.0f)
{
    bush_sh.UseShader();

    // Униформы
    int loc;
    loc = glGetUniformLocationARB(bush_sh.program, "light_pos_v");
    glUniform3fARB(loc, light.x(), light.y(), light.z());
    loc = glGetUniformLocationARB(bush_sh.program, "view_pos");
    glUniform3fARB(loc, camera.x(), camera.y(), camera.z());

    // Текстуры
    glActiveTexture(GL_TEXTURE0); diffTex.Bind();
    glActiveTexture(GL_TEXTURE1); normTex.Bind();
    glActiveTexture(GL_TEXTURE2); specTex.Bind();
    glActiveTexture(GL_TEXTURE3);
    transTex.Bind();

    loc = glGetUniformLocationARB(bush_sh.program, "diffuse_tex"); glUniform1iARB(loc, 0);
    loc = glGetUniformLocationARB(bush_sh.program, "normal_tex");  glUniform1iARB(loc, 1);
    loc = glGetUniformLocationARB(bush_sh.program, "specular_tex"); glUniform1iARB(loc, 2);
    loc = glGetUniformLocationARB(bush_sh.program, "translucency_tex");
    glUniform1iARB(loc, 3);
    // Прозрачность
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_ALPHA_TEST);
    glAlphaFunc(GL_GREATER, 0.1);

    // Позиция и трансформации
    glPushMatrix();
    glTranslated(x, y, z);
    glRotated(rotateX, 1, 0, 0);
    glScaled(scale, scale, scale);
    model.Draw();
    glPopMatrix();

    // Сброс
    glDisable(GL_ALPHA_TEST);
    glDisable(GL_BLEND);
    glActiveTexture(GL_TEXTURE0);
}



void handleRabbitInput(OpenGL* sender, KeyEventArg arg)
{
    float jump_distance = 0.5f;
    float turn = 4.0f;
    auto key = LOWORD(MapVirtualKeyA(arg.key, MAPVK_VK_TO_CHAR));

    if ((key == 'W' || key == VK_UP) && !rabbit_is_jumping) {
        // Начинаем прыжок
        rabbit_start_x = rabbit_x;
        rabbit_start_y = rabbit_y;

        // Цель прыжка
        rabbit_target_x = rabbit_x + sinf(rabbit_angle * DEG2RAD) * jump_distance;
        rabbit_target_y = rabbit_y + cosf(rabbit_angle * DEG2RAD) * jump_distance;

        // Границы поля
        rabbit_target_x = max(-2.4f, min(2.4f, rabbit_target_x));
        rabbit_target_y = max(-2.4f, min(2.4f, rabbit_target_y));

        // ПРОВЕРКА СТОЛКНОВЕНИЯ С ЦВЕТКОМ
        if (flower_inst.active) {
            float dist_to_flower = sqrtf(pow(rabbit_target_x - flower_inst.x, 2) +
                pow(rabbit_target_y - flower_inst.y, 2));
            if (dist_to_flower < 0.3f) {
                rabbit_state = DYING;
                death_timer = 0.0f;
                death_rotation = 0.0f;
                return;
            }
        }

        // ПРОВЕРКА СБОРА КЛЕВЕРА
        if (clover_inst.active) {
            float dist_to_clover = sqrtf(pow(rabbit_target_x - clover_inst.x, 2) +
                pow(rabbit_target_y - clover_inst.y, 2));
            if (dist_to_clover < 0.3f) {
                clover_count++;
                spawnPlant(clover_inst, flower_inst);
            }
        }

        rabbit_is_jumping = true;
        rabbit_jump_timer = rabbit_jump_duration;
        rabbit_moving = true;
    }
    else if (key == 'S' || key == VK_DOWN) {
        rabbit_angle += 180.0f;
        if (rabbit_angle > 360.0f) rabbit_angle -= 360.0f;
    }
    else if (key == 'A' || key == VK_LEFT) {
        rabbit_angle -= turn;
    }
    else if (key == 'D' || key == VK_RIGHT) {
        rabbit_angle += turn;
    }

    // Нормализуем угол
    if (rabbit_angle > 360.0f) rabbit_angle -= 360.0f;
    if (rabbit_angle < 0.0f)   rabbit_angle += 360.0f;
}

void Render(double delta_time)
{
    full_time += delta_time;

    // Настройка камеры и света
    if (gl.isKeyPressed('F')) // если нажата F - свет из камеры
    {
        light.SetPosition(camera.x(), camera.y(), camera.z());
    }
    camera.SetUpCamera();
    // Забираем матрицу MODELVIEW сразу после установки камеры,
    // так как в ней отсутствуют трансформации glRotate
    glGetFloatv(GL_MODELVIEW_MATRIX, view_matrix);

    light.SetUpLight();

    // Рисуем оси
    gl.DrawAxes();

    glBindTexture(GL_TEXTURE_2D, 0);

    // Включаем нормализацию нормалей
    // чтобы glScaled не влияли на них.

    glEnable(GL_NORMALIZE);
    glDisable(GL_LIGHTING);
    glDisable(GL_TEXTURE_2D);
    glDisable(GL_BLEND);

    // Переключаем режимы (см void switchModes(OpenGL *sender, KeyEventArg arg))
    if (lightning)
        glEnable(GL_LIGHTING);
    if (texturing)
    {
        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, 0); // Сбрасываем текущую текстуру
    }

    if (alpha)
    {
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    }

    //=============НАСТРОЙКА МАТЕРИАЛА==============

    // Настройка материала, все что рисуется ниже будет иметь этот материал.
    // Массивы с настройками материала
    float amb[] = {0.2, 0.2, 0.1, 1.};
    float dif[] = {0.4, 0.65, 0.5, 1.};
    float spec[] = {0.9, 0.8, 0.3, 1.};
    float sh = 0.2f * 256;

    // Фоновая
    glMaterialfv(GL_FRONT, GL_AMBIENT, amb);
    // Дифузная
    glMaterialfv(GL_FRONT, GL_DIFFUSE, dif);
    // Зеркальная
    glMaterialfv(GL_FRONT, GL_SPECULAR, spec);
    // Размер блика
    glMaterialf(GL_FRONT, GL_SHININESS, sh);

    // Сглаживание освещения
    glShadeModel(GL_SMOOTH); // закраска по Гуро
                             //(GL_SMOOTH - плоская закраска)

    //============ РИСОВАТЬ ТУТ ==============

    normalmap_sh.UseShader();

    // Передаём позицию камеры и света в шейдер
    location = glGetUniformLocationARB(normalmap_sh.program, "light_pos_v");
    glUniform3fARB(location, light.x(), light.y(), light.z());

    location = glGetUniformLocationARB(normalmap_sh.program, "view_pos");
    glUniform3fARB(location, camera.x(), camera.y(), camera.z());

    // Привязываем текстуры к разным текстурным юнитам
    glActiveTexture(GL_TEXTURE0);
    diffuse_tex.Bind();
    glActiveTexture(GL_TEXTURE1);
    normal_tex.Bind();

    // Передаём номера юнитов в шейдер
    location = glGetUniformLocationARB(normalmap_sh.program, "diffuse_tex");
    glUniform1iARB(location, 0);  // TEXTURE0
    location = glGetUniformLocationARB(normalmap_sh.program, "normal_tex");
    glUniform1iARB(location, 1);

    // === ПЛИТКА 5×5 ИЗ ТРАВЫ ===
    glPushMatrix();
    glTranslated(0, 0, 0);
    glRotated(90, 1, 0, 0);
    DrawGrassGrid(1.0f, 5);
    glPopMatrix();


// Куст 1
    DrawBush(bush, bush_diffuse, bush_normal, bush_specular, bush_translucency,
        -1.7f, -1.7f, 0.0f,  // позиция: угол
        0.015f, 90.0f);        // масштаб, поворот

    // Куст 2
    DrawBush(bush2, bush2_diffuse, bush2_normal, bush2_specular, bush2_translucency,
        0.0f, -0.5f, 0.0f,
        0.015f, 90.0f);

    // Куст 3
    DrawBush(bush3, bush3_diffuse, bush3_normal, bush3_specular, bush3_translucency,
        -2.0f, 2.0f, 0.0f,
        0.015f, 90.0f);

    // Дерево
    DrawBush(tree, tree1_diffuse, tree1_normal, tree1_specular, tree1_translucency,
        1.3f, 1.3f, 0.0f,
        0.03f, 90.0f);

    // Тыква
    DrawBush(pump, pump_diffuse, pump_normal, pump_specular, pump_translucency,
        1.7f, -1.7f, 0.0f,
        0.02f, 90.0f);

    // === ОТРИСОВКА ЦВЕТКА ===
    simple_texture_sh.UseShader();
    glEnable(GL_TEXTURE_2D);
    glActiveTexture(GL_TEXTURE0);
    flower_inst.tex->Bind();

    int loc = glGetUniformLocationARB(simple_texture_sh.program, "tex");
    glUniform1iARB(loc, 0);

    glDisable(GL_LIGHTING);
    glColor3f(1.0f, 1.0f, 1.0f);

    glPushMatrix();
    glTranslated(flower_inst.x, flower_inst.y, 0.0f);
    glRotated(90, 1, 0, 0);
    glScaled(0.02f, 0.02f, 0.02f);
    flower_inst.model->Draw();
    glPopMatrix();

    // === ОТРИСОВКА КЛЕВЕРА ===
    simple_texture_sh.UseShader();
    glEnable(GL_TEXTURE_2D);
    glActiveTexture(GL_TEXTURE0);
    clover_inst.tex->Bind();

    loc = glGetUniformLocationARB(simple_texture_sh.program, "tex");
    glUniform1iARB(loc, 0);

    glDisable(GL_LIGHTING);
    glColor3f(1.0f, 1.0f, 1.0f);

    glPushMatrix();
    glTranslated(clover_inst.x, clover_inst.y, 0.0f);
    glRotated(90, 1, 0, 0);
    glScaled(0.02f, 0.02f, 0.02f);
    clover_inst.model->Draw();
    glPopMatrix();

    // === ЛОГИКА СМЕРТИ ЗАЙЦА ===
    if (rabbit_state == DYING) {
        death_timer += delta_time;

        if (death_timer < 1.0f) {
            death_rotation = 90.0f * (death_timer / 1.0f);
        }
        else if (death_timer < 2.0f) {
            death_rotation = 90.0f + 90.0f * ((death_timer - 1.0f) / 1.0f);
        }
        else {
            spawnPlant(flower_inst, clover_inst);

            rabbit_state = RESPAWNING;
            respawn_timer = 0.0f;
            respawn_start_x = rabbit_x;
            respawn_start_y = rabbit_y;
            respawn_target_x = 0.0f;
            respawn_target_y = 0.0f;
        }
    }
    // === ЛОГИКА ВОСКРЕШЕНИЯ ===
    if (rabbit_state == RESPAWNING) {
        respawn_timer += delta_time;

        if (respawn_timer < 3.0f) {
            float t = respawn_timer / 3.0f;

            // Плавное движение к стартовой позиции
            rabbit_x = respawn_start_x + (respawn_target_x - respawn_start_x) * t;
            rabbit_y = respawn_start_y + (respawn_target_y - respawn_start_y) * t;

            // Подпрыгивание по дуге
            float arc_height = sinf(t * 3.14159f) * 0.5f;

            // Переворот через спину
            float flip_rotation = t * 360.0f;

            // Полупрозрачность
            float alpha = t;

            Shader::DontUseShaders();
            glDisable(GL_LIGHTING);
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

            glEnable(GL_TEXTURE_2D);
            glActiveTexture(GL_TEXTURE0);
            rabbit_skin.Bind();

            glColor4f(1.0f, 1.0f, 1.0f, alpha);

            glPushMatrix();
            glTranslated(rabbit_x, rabbit_y, arc_height);
            glRotated(-rabbit_angle, 0, 0, 1);
            glRotated(90, 1, 0, 0);
            glRotated(180 + flip_rotation, 0, 1, 0);
            glRotated(death_rotation, 1, 0, 0);
            glScaled(0.03f, 0.03f, 0.03f);

            rabbit_frames[0].Draw();
            glPopMatrix();

            glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
            glDisable(GL_BLEND);
            glEnable(GL_LIGHTING);
        }
        else {
            rabbit_state = ALIVE;
            rabbit_x = 0.0f;
            rabbit_y = 0.0f;
            rabbit_angle = 0.0f;
            clover_count = 0;
        }
    }
     
     
     
    if (rabbit_state == ALIVE) {
    // ОТРИСОВКА ЗАЙЦА С ТЕКСТУРОЙ
        rabbit_sh.UseShader();

        int loc = glGetUniformLocationARB(rabbit_sh.program, "light_pos_v");
        glUniform3fARB(loc, light.x(), light.y(), light.z());
        loc = glGetUniformLocationARB(rabbit_sh.program, "view_pos");
        glUniform3fARB(loc, camera.x(), camera.y(), camera.z());

        glActiveTexture(GL_TEXTURE0);
        rabbit_skin.Bind();
        loc = glGetUniformLocationARB(rabbit_sh.program, "diffuse_tex");
        glUniform1iARB(loc, 0);


    
    glDisable(GL_LIGHTING);
    glColor3f(1.0f, 1.0f, 1.0f);

    // === Анимация кадров ===
    int current_frame = 0;
    if (rabbit_is_jumping) {
        rabbit_jump_timer -= delta_time;
        if (rabbit_jump_timer <= 0.0f) {
            rabbit_x = rabbit_target_x;
            rabbit_y = rabbit_target_y;
            rabbit_is_jumping = false;
            rabbit_moving = false;
            current_frame = 0;
        } else {
            float jump_elapsed = rabbit_jump_duration - rabbit_jump_timer;
            float ease = (jump_elapsed / rabbit_jump_duration);
            ease = ease * ease * (3.0f - 2.0f * ease);
            rabbit_x = rabbit_start_x + (rabbit_target_x - rabbit_start_x) * ease;
            rabbit_y = rabbit_start_y + (rabbit_target_y - rabbit_start_y) * ease;
            current_frame = int(jump_elapsed * 15.0f) % total_rabbit_frames;
        }
    } else {
        current_frame = 0;
    }
    
    // === Трансформации и отрисовка ===
    glPushMatrix();
    glTranslated(rabbit_x, rabbit_y, 0.0f);
    glRotated(-rabbit_angle, 0.0f, 0.0f, 1.0f);
    glRotated(90.0f, 1.0f, 0.0f, 0.0f);
    glRotated(180.0f, 0.0f, 1.0f, 0.0f);
    glScaled(0.03f, 0.03f, 0.03f);
    
    rabbit_frames[current_frame].Draw();
    glPopMatrix();
    
    glEnable(GL_LIGHTING);
    glActiveTexture(GL_TEXTURE0);
}

    //===============================================

    // Сбрасываем все трансформации
    glLoadIdentity();
    camera.SetUpCamera();
    Shader::DontUseShaders();
    // Рисуем источник света
    light.DrawLightGizmo();

    //================Сообщение в верхнем левом углу=======================
    glActiveTexture(GL_TEXTURE0);
    // Переключаемся на матрицу проекции
    glMatrixMode(GL_PROJECTION);
    // Сохраняем текущую матрицу проекции с перспективным преобразованием
    glPushMatrix();
    // Загружаем единичную матрицу в матрицу проекции
    glLoadIdentity();

    // Устанавливаем матрицу параллельной проекции
    glOrtho(0, gl.getWidth() - 1, 0, gl.getHeight() - 1, 0, 1);

    // Переключаемся на матрицу MODELVIEW
    glMatrixMode(GL_MODELVIEW);
    // Сохраняем матрицу
    glPushMatrix();
    // Сбрасываем все трансформации и настройки камеры загрузкой единичной матрицы
    glLoadIdentity();

    // Нарисованное тут находится в 2D системе координат
    // Нижний левый угол окна - точка (0,0)
    // Верхний правый угол (ширина_окна - 1, высота_окна - 1)

    std::wstringstream ss;
    ss << std::fixed << std::setprecision(3)
       << L"F - переместить свет в позицию камеры\n"
       << L"G - двигать свет по горизонтали\n"
       << L"G+ЛКМ - двигать свет по вертикали\n"
       << L"Координаты света: (" << std::setw(7) << light.x() << "," << std::setw(7) << light.y() << "," << std::setw(7)
       << light.z() << ")\n"
       << L"Координаты камеры: (" << std::setw(7) << camera.x() << "," << std::setw(7) << camera.y() << ","
       << std::setw(7) << camera.z() << ")\n"
       << L"Параметры камеры: R=" << std::setw(7) << camera.distance() << ", fi1=" << std::setw(7) << camera.fi1()
       << ", fi2=" << std::setw(7) << camera.fi2() << '\n'
       << L"delta_time: " << std::setprecision(5) << delta_time << '\n'
       << L"full_time: " << std::setprecision(2) << full_time << std::endl;

    text.setPosition(10, gl.getHeight() - 10 - 180);
    text.setText(ss.str().c_str());
    text.Draw();

    // ОТОБРАЖЕНИЕ СЧЁТЧИКА
    std::wstringstream clover_ss;
    clover_ss << L"Клеверов: " << clover_count;

    text.setPosition(10, 10);
    text.setText(clover_ss.str().c_str());
    text.Draw();

    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
}
