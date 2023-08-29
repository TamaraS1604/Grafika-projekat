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
#include <learnopengl/model.h>

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

void drawWater(Shader waterShader,PointLight pointLight,Model water){
    waterShader.use();

    //        pointLight.position = glm::vec3(10.0 * cos(currentFrame), 0.0f, 10.0 * sin(currentFrame));
    waterShader.setVec3("pointLight.position", pointLight.position);
    waterShader.setVec3("pointLight.ambient", pointLight.ambient);
    waterShader.setVec3("pointLight.diffuse", pointLight.diffuse);
    waterShader.setVec3("pointLight.specular", pointLight.specular);
    waterShader.setFloat("pointLight.constant", pointLight.constant);
    waterShader.setFloat("pointLight.linear", pointLight.linear);
    waterShader.setFloat("pointLight.quadratic", pointLight.quadratic);
    waterShader.setVec3("viewPosition", programState->camera.Position);
    waterShader.setFloat("material.shininess", 32.0f);

    glm::mat4 projection = glm::perspective(glm::radians(programState->camera.Zoom),
                                  (float) SCR_WIDTH / (float) SCR_HEIGHT, 0.1f, 100.0f);
    glm::mat4 view = programState->camera.GetViewMatrix();
    waterShader.setMat4("projection", projection);
    waterShader.setMat4("view", view);

    glm::mat4 model=glm::scale(glm::rotate(glm::translate(glm::mat4(1.0),glm::vec3(0,-1,0)),0.0f,glm::vec3(0,1,0)),glm::vec3(100,1,100));
    waterShader.setMat4("model",model);
    water.Draw(waterShader);

}
#define N 20
void drawMap(Shader ourShader,PointLight pointLight,int n,Model *models,int map[N][N],std::vector<Tile> tiles){
    ourShader.use();
    //        pointLight.position = glm::vec3(10.0 * cos(currentFrame), 0.0f, 10.0 * sin(currentFrame));
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
    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model,
                           programState->backpackPosition); // translate it down so it's at the center of the scene

    for(int i=0;i<n;i++){
        for(int j=0;j<n;j++){
            if(map[i][j]==-1)continue;
            model=
                glm::rotate(glm::translate(glm::mat4(1.0),glm::vec3(i,0,j)),tiles[map[i][j]].rotation*1.57079633f,glm::vec3(0,1,0));
            ourShader.setMat4("model",model);
            models[tiles[map[i][j]].modelId].Draw(ourShader);
        }
    }
//    for(int i=0;i<8;i++){
//        model=
//            glm::rotate(glm::translate(glm::mat4(1.0),glm::vec3(i,0,0)),0.0f,glm::vec3(0,1,0));
//        ourShader.setMat4("model",model);
//        models[i].Draw(ourShader);
//    }
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
        if(top!=2&&tiles[i].bottom!=top)odgovara=0;
        if(bottom!=2&&tiles[i].top!=bottom)odgovara=0;
        if(left!=2&&tiles[i].right!=left)odgovara=0;
        if(right!=2&&tiles[i].left!=right)odgovara=0;
        if(odgovara==1)availableTiles.push_back(i);
    }

    return availableTiles;
//    return std::vector<int>();

}

int backTracking(int mat[N][N],std::vector<Tile> tiles,int x,int y){
    if(x>=N||y>=N)
        return 1;
    if(mat[x][y]!=-1)
        return backTracking(mat,tiles,(x+1)%N,y+((x+1)/N));
    std::vector<int> mogucnosti=odrediMogucnosti(mat, tiles, x, y);
    //std::cout<<mogucnosti.size();

        for(int i=0;i<mogucnosti.size();i++){
            int j=rand()%(mogucnosti.size());
            int p=mogucnosti[i];
            mogucnosti[i]=mogucnosti[j];
            mogucnosti[j]=p;
        }
        for(int i=0;i<mogucnosti.size();i++){
//      for(int i=mogucnosti.size()-1;i>=0;i--){
            mat[x][y]=mogucnosti[i];

            if(backTracking(mat,tiles,(x+1)%N,y+((x+1)/N))){
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
//            map[i][j]=rand()%6;
            map[i][j]=-1;
        }
    }
    map[0][0]=7;
    map[4][5]=7;

    map[7][7]=7;

    map[11][11]=7;
    //std::cout<<"\npocinje da generise mapu\n";
    backTracking(map,tiles,0,0);
    for(int i=0;i<N;i++){
        for(int j=0;j<N;j++){
            std::cout<<map[i][j]<<"   ";
        }
        std::cout<<"\n";
    }
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
    GLFWwindow *window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "LearnOpenGL", NULL, NULL);
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
    stbi_set_flip_vertically_on_load(true);

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

    // build and compile shaders
    // -------------------------
    Shader ourShader("resources/shaders/2.model_lighting.vs", "resources/shaders/2.model_lighting.fs");
    Shader waterShader("resources/shaders/water.vs", "resources/shaders/water.fs");

    // load models
    // -----------
    Model ourModel("resources/objects/dungeon/model/obj/wall.obj");
    ourModel.SetShaderTextureNamePrefix("material.");

    Model models[]={
        Model("resources/objects/dungeon/model/obj/dirt.obj"),
        Model("resources/objects/dungeon/model/obj/stairs.obj"),
        Model("resources/objects/dungeon/model/obj/barrel.obj"),
        Model("resources/objects/dungeon/model/obj/wall-narrow.obj"),
        Model("resources/objects/dungeon/model/obj/wall-opening.obj"),
        Model("resources/objects/dungeon/model/obj/wall-half.obj"),
        Model("resources/objects/dungeon/model/obj/floor.obj"),
    };

    std::vector<Tile> tiles({
        //dirt
        Tile(0,0,1,1,1,1),
        //stairs
        Tile(1,0,1,0,0,0),
        Tile(1,1,0,0,1,0),
        Tile(1,2,0,1,0,0),
        Tile(1,3,0,0,0,1),
        //barrel
        Tile(2,0,0,0,0,0),
        //wall-Narrow
        Tile(3,0,1,1,0,0),
        Tile(3,1,0,0,1,1),
        //wall-Opening
        Tile(4,0,1,1,0,0),
        Tile(4,1,0,0,1,1),
        //wall-half
        Tile(5,0,1,0,1,1),
        Tile(5,1,1,1,1,0),
        Tile(5,2,0,1,1,1),
        Tile(5,3,1,1,0,1),
        Tile(6,0,0,0,0,0),
        Tile(6,0,0,0,0,0),
        Tile(6,0,0,0,0,0),
        Tile(6,0,0,0,0,0),
        Tile(6,0,0,0,0,0),
        Tile(6,0,0,0,0,0),
    });



    int map[N][N];
    generateMap(map,tiles);

    Model water("resources/objects/water/water.obj");

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


        drawWater(waterShader,pointLight,water);

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
}
