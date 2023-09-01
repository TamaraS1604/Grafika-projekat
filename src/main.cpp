#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <learnopengl/filesystem.h>
#include <learnopengl/shader.h>
#include <learnopengl/camera.h>
#include "Model.h"

#include <iostream>
#include<vector>
#include<cstdlib>
#include<ctime>

class Tile{
public:
    int modelId,rotation,top,left,right,bottom;
    Tile(int modelId,int rotation,int top,int bottom,int left,int right){
        this->top=top;
        this->bottom=bottom;
        this->left=left;
        this->right=right;
        this->modelId=modelId;
        this->rotation=rotation;
    }
};

void framebuffer_size_callback(GLFWwindow *window, int width, int height);

void mouse_callback(GLFWwindow *window, double xpos, double ypos);

void scroll_callback(GLFWwindow *window, double xoffset, double yoffset);

void processInput(GLFWwindow *window);

void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods);

unsigned int loadCubemap(vector<std::string> faces);

// settings
const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;

// camera

float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;

// timing
float deltaTime = 0.0f;
float lastFrame = 0.0f;

struct PointLight {
    glm::vec3 position;
    glm::vec3 ambient;
    glm::vec3 diffuse;
    glm::vec3 specular;

    float constant;
    float linear;
    float quadratic;
};

struct ProgramState {
    glm::vec3 clearColor = glm::vec3(0);
    bool ImGuiEnabled = false;
    Camera camera;
    bool CameraMouseMovementUpdateEnabled = true;
    glm::vec3 backpackPosition = glm::vec3(0.0f);
    float backpackScale = 1.0f;
    PointLight pointLight;
    ProgramState()
            : camera(glm::vec3(0.0f, 0.0f, 3.0f)) {}

    void SaveToFile(std::string filename);

    void LoadFromFile(std::string filename);
};

void ProgramState::SaveToFile(std::string filename) {
    std::ofstream out(filename);
    out << clearColor.r << '\n'
        << clearColor.g << '\n'
        << clearColor.b << '\n'
        << ImGuiEnabled << '\n'
        << camera.Position.x << '\n'
        << camera.Position.y << '\n'
        << camera.Position.z << '\n'
        << camera.Front.x << '\n'
        << camera.Front.y << '\n'
        << camera.Front.z << '\n';
}

void ProgramState::LoadFromFile(std::string filename) {
    std::ifstream in(filename);
    if (in) {
        in >> clearColor.r
           >> clearColor.g
           >> clearColor.b
           >> ImGuiEnabled
           >> camera.Position.x
           >> camera.Position.y
           >> camera.Position.z
           >> camera.Front.x
           >> camera.Front.y
           >> camera.Front.z;
    }
}

ProgramState *programState;

void DrawImGui(ProgramState *programState);

void drawWater(Shader waterShader,PointLight pointLight,Model water, Texture waterTexture, Texture normalTexture){
    waterShader.use();

    glBindTexture(GL_TEXTURE_2D,waterTexture.id);
    glBindTexture(GL_TEXTURE_2D,normalTexture.id);

    waterShader.setVec3("pointLight.position", pointLight.position);
    waterShader.setVec3("pointLight.ambient", pointLight.ambient);
    waterShader.setVec3("pointLight.diffuse", pointLight.diffuse);
    waterShader.setVec3("pointLight.specular", pointLight.specular);
    waterShader.setFloat("pointLight.constant", pointLight.constant);
    waterShader.setFloat("pointLight.linear", pointLight.linear);
    waterShader.setFloat("pointLight.quadratic", pointLight.quadratic);
    waterShader.setVec3("viewPosition", programState->camera.Position);

    waterShader.setFloat("material.shininess", 32.0f);

    glEnable(GL_CULL_FACE);
    glm::mat4 projection = glm::perspective(glm::radians(programState->camera.Zoom),
                                  (float) SCR_WIDTH / (float) SCR_HEIGHT, 0.1f, 100.0f);
    glm::mat4 view = programState->camera.GetViewMatrix();
    waterShader.setMat4("projection", projection);
    waterShader.setMat4("view", view);

    glm::mat4 model=glm::scale(glm::rotate(glm::translate(glm::mat4(1.0),glm::vec3(0,-0.001,0)),0.0f,glm::vec3(0,1,0)),glm::vec3(100,1,100));
    waterShader.setMat4("model",model);
    water.Draw(waterShader);
    glDisable(GL_CULL_FACE);



}
#define N 20
#define NUMBER_OF_TILES 16
std::vector<Tile>* tiles_g;
void drawMap(Shader ourShader,PointLight pointLight,int n,Model *models,int map[N][N],std::vector<Tile> tiles){
    ourShader.use();
    ourShader.setVec3("pointLight.position", pointLight.position);
    ourShader.setVec3("pointLight.ambient", pointLight.ambient);
    ourShader.setVec3("pointLight.diffuse", pointLight.diffuse);
    ourShader.setVec3("pointLight.specular", pointLight.specular);
    ourShader.setFloat("pointLight.constant", pointLight.constant);
    ourShader.setFloat("pointLight.linear", pointLight.linear);
    ourShader.setFloat("pointLight.quadratic", pointLight.quadratic);
    ourShader.setVec3("viewPosition", programState->camera.Position);
    ourShader.setFloat("material.shininess", 32.0f);
    // view/projection transformations
    glm::mat4 projection = glm::perspective(glm::radians(programState->camera.Zoom),
                                            (float) SCR_WIDTH / (float) SCR_HEIGHT, 0.1f, 100.0f);
    glm::mat4 view = programState->camera.GetViewMatrix();
    ourShader.setMat4("projection", projection);
    ourShader.setMat4("view", view);

    // render the loaded model
    glEnable(GL_CULL_FACE);
    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model,
                           programState->backpackPosition); // translate it down so it's at the center of the scene

    for(int i=0;i<n;i++){
        for(int j=0;j<n;j++){
            float x=i*2;
            float y=0;
            float z=j*2;
            float pi_half=1.57079633f;
            if(map[i][j]==-1)continue;
            model=
                glm::rotate(glm::translate(glm::mat4(1.0),glm::vec3(x,y,z)),tiles[map[i][j]].rotation*pi_half,glm::vec3(0,1,0));
            ourShader.setMat4("model",model);
            models[tiles[map[i][j]].modelId].Draw(ourShader);
            models[2].Draw(ourShader);
            if(i<N-1){
                int left=tiles[map[i][j]].right;
                int right=tiles[map[i+1][j]].left;
                int top=0,bottom=0;
                for(int k=0;k<tiles.size();k++){
                    if(tiles[k].left==left&&tiles[k].right==right&&tiles[k].top==top&&tiles[k].bottom==bottom){
                        model=
                            glm::rotate(glm::translate(glm::mat4(1.0),glm::vec3(x+1,y,z)),tiles[k].rotation*pi_half,glm::vec3(0,1,0));
                        ourShader.setMat4("model",model);
                        models[tiles[k].modelId].Draw(ourShader);
                        models[2].Draw(ourShader);
                        break;
                    }
                }
            }
            if(j<N-1){
                int top=tiles[map[i][j]].bottom;
                int bottom=tiles[map[i][j+1]].top;
                int left=0,right=0;
                for(int k=0;k<tiles.size();k++){
                    if(tiles[k].left==left&&tiles[k].right==right&&tiles[k].top==top&&tiles[k].bottom==bottom){
                        model=
                            glm::rotate(glm::translate(glm::mat4(1.0),glm::vec3(x,y,z+1)),tiles[k].rotation*pi_half,glm::vec3(0,1,0));
                        ourShader.setMat4("model",model);
                        models[tiles[k].modelId].Draw(ourShader);
                        models[2].Draw(ourShader);
                        break;
                    }
                }
            }
            if(j<N-1&&i<N-1){
                model=
                    glm::rotate(glm::translate(glm::mat4(1.0),glm::vec3(x+1,y,z+1)),0.0f,glm::vec3(0,1,0));
                ourShader.setMat4("model",model);
                models[2].Draw(ourShader);
            }
        }
    }

    glDisable(GL_CULL_FACE);
}

std::vector<int> odrediMogucnosti(int mat[N][N],std::vector<Tile> tiles,int x,int y){
    std::vector<int> availableTiles=std::vector<int>();
    int top=2,bottom=2,left=2,right=2;
    if(x+1<N && mat[x+1][y]!=-1){
        right=tiles[mat[x+1][y]].left;
    }
    if(x-1>=0 && mat[x-1][y]!=-1){
        left=tiles[mat[x-1][y]].right;
    }
    if(y+1<N && mat[x][y+1]!=-1){
        bottom=tiles[mat[x][y+1]].top;
    }
    if(y-1>=0 && mat[x][y-1]!=-1){
        top=tiles[mat[x][y-1]].bottom;
    }
    for(int i=0;i<tiles.size();i++){
        int odgovara=1;
        if((top!=2)&&(tiles[i].top!=top))odgovara=0;
        if((bottom!=2)&&(tiles[i].bottom!=bottom))odgovara=0;
        if((left!=2)&&(tiles[i].left!=left))odgovara=0;
        if((right!=2)&&(tiles[i].right!=right))odgovara=0;
        if(odgovara==1)availableTiles.push_back(i);
    }
    return availableTiles;
}

int backTracking(int mat[N][N],std::vector<Tile> tiles,int x,int y){
    if(x>=N||y>=N)
        return 1;
    if(mat[x][y]!=-1)
        return backTracking(mat,tiles,(x+1)%N,y+((x+1)/N));
    std::vector<int> mogucnosti=odrediMogucnosti(mat, tiles, x, y);
    for(int i=0;i<mogucnosti.size();i++){
        int j=rand()%(mogucnosti.size());
        int p=mogucnosti[i];
        mogucnosti[i]=mogucnosti[j];
        mogucnosti[j]=p;
    }
    for(int i=0;i<mogucnosti.size();i++){
        mat[x][y]=mogucnosti[i];

        if(backTracking(mat,tiles,(x+1)%N,y+((x+1)/N))==1){
            return 1;
        }
        mat[x][y]=-1;
    }
    return 0;
}

void generateMap(int map[N][N],std::vector<Tile> tiles){
    srand(time(NULL));
    for(int i=0;i<N;i++){
        for(int j=0;j<N;j++){
            map[i][j]=-1;
        }
    }
    backTracking(map,tiles,0,0);
//    for(int i=0;i<N;i++){
//        for(int j=0;j<N;j++){
//            std::cout<<map[i][j]<<"   ";
//        }
//        std::cout<<"\n";
//    }
}

void clear(){
    glClearColor(0.0f,0.0f,0.0f,1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

int main() {
    // glfw: initialize and configure
    // ------------------------------
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    // glfw window creation
    // --------------------
    GLFWwindow *window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Tamara's project", NULL, NULL);
    if (window == NULL) {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);
    glfwSetKeyCallback(window, key_callback);
    // tell GLFW to capture our mouse
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // glad: load all OpenGL function pointers
    // ---------------------------------------
    if (!gladLoadGLLoader((GLADloadproc) glfwGetProcAddress)) {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    // tell stb_image.h to flip loaded texture's on the y-axis (before loading model).
    //stbi_set_flip_vertically_on_load(true);

    programState = new ProgramState;
    //programState->LoadFromFile("resources/program_state.txt");
    if (programState->ImGuiEnabled) {
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    }
    // Init Imgui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    (void) io;



    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330 core");

    // configure global opengl state
    // -----------------------------
    glEnable(GL_DEPTH_TEST);

    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // build and compile shaders
    // -------------------------
    Shader ourShader("resources/shaders/2.model_lighting.vs", "resources/shaders/2.model_lighting.fs");
    Shader waterShader("resources/shaders/water.vs", "resources/shaders/water.fs");
    Shader skyboxShader("resources/shaders/skybox.vs", "resources/shaders/skybox.fs");

    float skyboxVertices[] = {
        -1.0f,  1.0f, -1.0f,
        -1.0f, -1.0f, -1.0f,
        1.0f, -1.0f, -1.0f,
        1.0f, -1.0f, -1.0f,
        1.0f,  1.0f, -1.0f,
        -1.0f,  1.0f, -1.0f,

        -1.0f, -1.0f,  1.0f,
        -1.0f, -1.0f, -1.0f,
        -1.0f,  1.0f, -1.0f,
        -1.0f,  1.0f, -1.0f,
        -1.0f,  1.0f,  1.0f,
        -1.0f, -1.0f,  1.0f,

        1.0f, -1.0f, -1.0f,
        1.0f, -1.0f,  1.0f,
        1.0f,  1.0f,  1.0f,
        1.0f,  1.0f,  1.0f,
        1.0f,  1.0f, -1.0f,
        1.0f, -1.0f, -1.0f,

        -1.0f, -1.0f,  1.0f,
        -1.0f,  1.0f,  1.0f,
        1.0f,  1.0f,  1.0f,
        1.0f,  1.0f,  1.0f,
        1.0f, -1.0f,  1.0f,
        -1.0f, -1.0f,  1.0f,

        -1.0f,  1.0f, -1.0f,
        1.0f,  1.0f, -1.0f,
        1.0f,  1.0f,  1.0f,
        1.0f,  1.0f,  1.0f,
        -1.0f,  1.0f,  1.0f,
        -1.0f,  1.0f, -1.0f,

        -1.0f, -1.0f, -1.0f,
        -1.0f, -1.0f,  1.0f,
        1.0f, -1.0f, -1.0f,
        1.0f, -1.0f, -1.0f,
        -1.0f, -1.0f,  1.0f,
        1.0f, -1.0f,  1.0f
    };
    unsigned int skyboxVAO, skyboxVBO;
    glGenVertexArrays(1, &skyboxVAO);
    glGenBuffers(1, &skyboxVBO);
    glBindVertexArray(skyboxVAO);
    glBindBuffer(GL_ARRAY_BUFFER, skyboxVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), &skyboxVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);

    vector<std::string> faces
        {
            FileSystem::getPath("resources/textures/skybox/right.png"),
            FileSystem::getPath("resources/textures/skybox/left.png"),
            FileSystem::getPath("resources/textures/skybox/top.png"),
            FileSystem::getPath("resources/textures/skybox/bottom.png"),
            FileSystem::getPath("resources/textures/skybox/front.png"),
            FileSystem::getPath("resources/textures/skybox/back.png")
        };
    unsigned int cubemapTexture = loadCubemap(faces);

    skyboxShader.use();
    skyboxShader.setInt("skybox", 0);

    //tile models
    Model models[]={
        Model("resources/objects/dungeon/model/obj/dirt.obj"),
        Model("resources/objects/dungeon/model/obj/stairs.obj"),
        Model("resources/objects/dungeon/model/obj/floor.obj"),
        Model("resources/objects/dungeon/model/obj/wall-narrow.obj"),
        Model("resources/objects/dungeon/model/obj/wall-opening.obj"),
        Model("resources/objects/dungeon/model/obj/wall-half.obj"),
        Model("resources/objects/dungeon/model/obj/barrel.obj"),
        Model("resources/objects/dungeon/model/obj/floor-detail.obj"),
        Model("resources/objects/dungeon/model/obj/rocks.obj"),
        Model("resources/objects/dungeon/model/obj/wall.obj"),
        Model("resources/objects/dungeon/model/obj/column.obj"),
        Model("resources/objects/dungeon/model/obj/stones.obj")
        //Model("resources/objects/dungeon/model/obj/character-human.obj"),
       // Model("resources/objects/dungeon/model/obj/character-orc.obj")

    };
    //tiles
    std::vector<Tile> tiles({
        //dirt
        Tile(0,0,1,1,1,1),
        //stairs
        Tile(1,0,1,0,0,0),
        Tile(1,3,0,0,0,1),
        Tile(1,2,0,1,0,0),
        Tile(1,1,0,0,1,0),
        //floor
        Tile(2,0,0,0,0,0),
        //wall-Narrow
        Tile(3,1,1,1,0,0),
        Tile(3,0,0,0,1,1),
        //wall-Opening
        Tile(4,1,1,1,0,0),
        Tile(4,0,0,0,1,1),
        //wall-half
        Tile(5,0,1,0,1,1),
        Tile(5,1,1,1,1,0),
        Tile(5,2,0,1,1,1),
        Tile(5,3,1,1,0,1),
        //barrel
        Tile(6,0,0,0,0,0),
        Tile(6,0,0,0,0,0),
        //floor detail
        Tile(7,0,0,0,0,0),
        Tile(7,0,0,0,0,0),
        //stone
        Tile(8,0,0,0,0,0),
        Tile(8,0,0,0,0,0),
        //wall
        Tile(9,0,0,0,0,0),
        //Tile(9,0,0,0,0,0),
        //column
        Tile(10,0,0,0,0,0),
        //stones
        Tile(11,0,0,0,0,0)
        //human
        //Tile(10,0,0,0,0,0),
        //orc
        //Tile(10,0,0,0,0,0)

    });

    tiles_g=&tiles;

    int map[N][N];
    generateMap(map,tiles);
    for(int i=1;i<N-1;i++){
        for(int j=1;j<N-1;j++){
            if(tiles[map[i][j]].left!=tiles[map[i-1][j]].right)std::cout<<"greska\n";
            if(tiles[map[i][j]].right!=tiles[map[i+1][j]].left)std::cout<<"greska\n";
            if(tiles[map[i][j]].top!=tiles[map[i][j-1]].bottom)std::cout<<"greska\n";
            if(tiles[map[i][j]].bottom!=tiles[map[i][j+1]].top)std::cout<<"greska\n";
        }
    }

    Model water("resources/objects/water/water.obj");

    char waterTextureFileName[]="water1.jpg";
    Texture waterTexture;
    waterTexture.id = TextureFromFile(waterTextureFileName, "resources/textures");
    waterTexture.path = waterTextureFileName;
    if(waterTexture.id==0)std::cout<<"error: nece da se ucita water diffuse";
    glBindTexture(GL_TEXTURE_2D,waterTexture.id);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    char normalTextureFileName[]="normal1.jpg";
    Texture normalTexture;
    normalTexture.id = TextureFromFile(normalTextureFileName, "resources/textures");
    normalTexture.path = normalTextureFileName;
    if(normalTexture.id==0)std::cout<<"error: nece da se ucita water normal";
    glBindTexture(GL_TEXTURE_2D,waterTexture.id);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);


    PointLight& pointLight = programState->pointLight;
    pointLight.position = glm::vec3(8.0f,8.0f,8.0f);
    pointLight.ambient = glm::vec3(0.1, 0.1, 0.1);
    pointLight.diffuse = glm::vec3(1.0,1.0,1.0);
    pointLight.specular = glm::vec3(1.0, 1.0, 1.0);

    pointLight.constant = 1.0f;
    pointLight.linear = 0.0f;
    pointLight.quadratic = 0.0f;


    while (!glfwWindowShouldClose(window)) {
        // per-frame time logic
        // --------------------
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        processInput(window);

        clear();

        drawMap(ourShader,pointLight,N,models,map,tiles);


        drawWater(waterShader,pointLight,water,waterTexture,normalTexture);


        //skybox projection

        glm::mat4 projection = glm::perspective(glm::radians(programState->camera.Zoom),
                                                (float) SCR_WIDTH / (float) SCR_HEIGHT, 0.1f, 100.0f);
        glm::mat4 view = programState->camera.GetViewMatrix();
        //ourShader.setMat4("projection", projection);
        //ourShader.setMat4("view", view);

        glDepthFunc(GL_LEQUAL);
        skyboxShader.use();
        view = glm::mat4(glm::mat3(programState->camera.GetViewMatrix()));
        skyboxShader.setMat4("view", view);
        skyboxShader.setMat4("projection", projection);
        glBindVertexArray(skyboxVAO);
        glActiveTexture(GL_TEXTURE0);
        glDrawArrays(GL_TRIANGLES, 0, 36);
        glBindVertexArray(0);
        glDepthFunc(GL_LESS);

//        if (programState->ImGuiEnabled)
//            DrawImGui(programState);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    programState->SaveToFile("resources/program_state.txt");
    delete programState;
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glDeleteVertexArrays(1, &skyboxVAO);
    glDeleteBuffers(1, &skyboxVBO);
    // glfw: terminate, clearing all previously allocated GLFW resources.
    // ------------------------------------------------------------------
    glfwTerminate();
    return 0;
}

// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
// ---------------------------------------------------------------------------------------------------------
void processInput(GLFWwindow *window) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        programState->camera.ProcessKeyboard(FORWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        programState->camera.ProcessKeyboard(BACKWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        programState->camera.ProcessKeyboard(LEFT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        programState->camera.ProcessKeyboard(RIGHT, deltaTime);
}

// glfw: whenever the window size changed (by OS or user resize) this callback function executes
// ---------------------------------------------------------------------------------------------
void framebuffer_size_callback(GLFWwindow *window, int width, int height) {
    // make sure the viewport matches the new window dimensions; note that width and
    // height will be significantly larger than specified on retina displays.
    glViewport(0, 0, width, height);
}

// glfw: whenever the mouse moves, this callback is called
// -------------------------------------------------------
void mouse_callback(GLFWwindow *window, double xpos, double ypos) {
    if (firstMouse) {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos; // reversed since y-coordinates go from bottom to top

    lastX = xpos;
    lastY = ypos;

    if (programState->CameraMouseMovementUpdateEnabled)
        programState->camera.ProcessMouseMovement(xoffset, yoffset);
}

// glfw: whenever the mouse scroll wheel scrolls, this callback is called
// ----------------------------------------------------------------------
void scroll_callback(GLFWwindow *window, double xoffset, double yoffset) {
    programState->camera.ProcessMouseScroll(yoffset);
}

void DrawImGui(ProgramState *programState) {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();


    {
        static float f = 0.0f;
        ImGui::Begin("Hello window");
        ImGui::Text("Hello text");
        ImGui::SliderFloat("Float slider", &f, 0.0, 1.0);
        ImGui::ColorEdit3("Background color", (float *) &programState->clearColor);
        ImGui::DragFloat3("Backpack position", (float*)&programState->backpackPosition);
        ImGui::DragFloat("Backpack scale", &programState->backpackScale, 0.05, 0.1, 4.0);

        ImGui::DragFloat("pointLight.constant", &programState->pointLight.constant, 0.05, 0.0, 1.0);
        ImGui::DragFloat("pointLight.linear", &programState->pointLight.linear, 0.05, 0.0, 1.0);
        ImGui::DragFloat("pointLight.quadratic", &programState->pointLight.quadratic, 0.05, 0.0, 1.0);
        ImGui::End();
    }

    {
        ImGui::Begin("Camera info");
        const Camera& c = programState->camera;
        ImGui::Text("Camera position: (%f, %f, %f)", c.Position.x, c.Position.y, c.Position.z);
        ImGui::Text("(Yaw, Pitch): (%f, %f)", c.Yaw, c.Pitch);
        ImGui::Text("Camera front: (%f, %f, %f)", c.Front.x, c.Front.y, c.Front.z);
        ImGui::Checkbox("Camera mouse update", &programState->CameraMouseMovementUpdateEnabled);
        ImGui::End();
    }

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods) {
    if (key == GLFW_KEY_F1 && action == GLFW_PRESS) {
        programState->ImGuiEnabled = !programState->ImGuiEnabled;
        if (programState->ImGuiEnabled) {
            programState->CameraMouseMovementUpdateEnabled = false;
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        } else {
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        }
    }
    if(key==GLFW_KEY_LEFT && action==GLFW_PRESS){
        for(int i=1;i<=4;i++){
            int rotation=(*tiles_g)[i].rotation;
            rotation=(rotation+1)%4;
            (*tiles_g)[i].rotation=rotation;
        }
        std::cout<<"press\n";
    }
}
unsigned int loadCubemap(vector<std::string> faces)
{
    unsigned int textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);

    int width, height, nrChannels;
    for (unsigned int i = 0; i < faces.size(); i++)
    {
        unsigned char *data = stbi_load(faces[i].c_str(), &width, &height, &nrChannels, 0);
        if (data)
        {
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
            stbi_image_free(data);
        }
        else
        {
            std::cout << "Cubemap texture failed to load at path: " << faces[i] << std::endl;
            stbi_image_free(data);
        }
    }
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    return textureID;
}
