#include <GL/glew.h>
#include <GL/glut.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#define _USE_MATH_DEFINES
#include <math.h>

struct Vertex {
    float x, y, z;
    float nx, ny, nz;
};

std::vector<Vertex> vertices;
std::vector<std::vector<int>> faces;
std::vector<Vertex> originalVertices;
std::vector<int> indices;

GLint lightPosLocation;
GLuint VBO, IBO;
GLuint shaderProgram;

Vertex currentTranslation = { 0.0f,0.0f,0.0f };

void computeFaceNormals(std::vector<Vertex>& vertices, const std::vector<std::vector<int>>& faces) {
    for (const auto& face : faces) {
        const Vertex& v0 = vertices[face[0]];
        const Vertex& v1 = vertices[face[1]];
        const Vertex& v2 = vertices[face[2]];

        float ux = v1.x - v0.x;
        float uy = v1.y - v0.y;
        float uz = v1.z - v0.z;

        float vx = v2.x - v0.x;
        float vy = v2.y - v0.y;
        float vz = v2.z - v0.z;

        float nx = uy * vz - uz * vy;
        float ny = uz * vx - ux * vz;
        float nz = ux * vy - uy * vx;

        float length = std::sqrt(nx * nx + ny * ny + nz * nz);
        if (length > 0) {
            nx /= length;
            ny /= length;
            nz /= length;
        }

        for (int i = 0; i < face.size(); ++i) {
            vertices[face[i]].nx += nx;
            vertices[face[i]].ny += ny;
            vertices[face[i]].nz += nz;
        }
    }
}

bool parseOFF(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Failed to open file: " << filename << std::endl;
        return false;
    }

    std::string line;
    if (!getline(file, line) || line != "OFF") {
        std::cerr << "Invalid OFF format: missing or incorrect header" << std::endl;
        return false;
    }

    int numVertices, numFaces, numEdges;
    if (!(file >> numVertices >> numFaces >> numEdges)) {
        std::cerr << "Failed to read OFF header" << std::endl;
        return false;
    }

    vertices.resize(numVertices);
    for (int i = 0; i < numVertices; ++i) {
        if (!(file >> vertices[i].x >> vertices[i].y >> vertices[i].z)) {
            std::cerr << "Failed to read vertex data" << std::endl;
            return false;
        }
    }

    faces.resize(numFaces);
    for (int i = 0; i < numFaces; ++i) {
        int numVerticesInFace;
        if (!(file >> numVerticesInFace) || numVerticesInFace < 3) {
            std::cerr << "Invalid number of vertices in face" << std::endl;
            return false;
        }
        faces[i].resize(numVerticesInFace);
        for (int j = 0; j < numVerticesInFace; ++j) {
            if (!(file >> faces[i][j]) || faces[i][j] < 0 || faces[i][j] >= numVertices) {
                std::cerr << "Invalid vertex index in face" << std::endl;
                return false;
            }
        }
    }
    originalVertices = vertices;
    file.close();
    return true;
}

//for TOON shading use VertexShader and FragmentShader
//for GOOCH shading use VertexShader2 and FragmentShader2
//for Phong shading use VertexShader and FragmentShader3
const char* vertexShader = R"(
    layout(location = 0) in vec4 position;
    layout(location = 1) in vec4 Normal;
    varying vec4 fragPos; 
    varying vec4 normal;

    void main()
    {
        normal = vec4(gl_NormalMatrix * Normal.xyz, 0.0);
        gl_Position = gl_ModelViewProjectionMatrix * position; 
        fragPos = position;
    }
)";

const char* fragmentShader = R"(
    uniform vec4 lightPos;
    varying vec4 normal;
    varying vec4 fragPos; 

    void main() {

        vec4 lightDir = normalize(lightPos - fragPos);

        float intensity = max(dot(normalize(normal), lightDir), 0.0);

        vec4 color;
        if (intensity > 0.95) color = vec4(1.0, 0.5, 0.5, 1.0);
        else if (intensity > 0.5) color = vec4(0.6, 0.3, 0.3, 1.0);
        else if (intensity > 0.25) color = vec4(0.4, 0.2, 0.2, 1.0);
        else color = vec4(0.2, 0.1, 0.1, 1.0);

        gl_FragColor = color;
    }
)";

const char* vertexShader2 = R"(
    layout(location = 0) in vec4 position;
    layout(location = 1) in vec4 Normal;
    varying vec4 fragPos; 
    varying vec4 normal;
    varying float NdotL;
    varying vec3 ReflectVec;
    varying vec3 ViewVec;
    uniform vec4 lightPos;

    void main()
    {
        vec3 ecPos    = (gl_ModelViewMatrix * position).xyz; 
        normal = vec4(gl_NormalMatrix * Normal.xyz, 0.0);
        vec3 lightVec = normalize((lightPos.xyz - ecPos));
        ReflectVec    = normalize(reflect(-lightVec, normalize(normal.xyz)));
        ViewVec       = normalize(-ecPos);
        NdotL         = (dot(lightVec, normalize(normal.xyz)) + 1.0) * 0.5;
        gl_Position = gl_ModelViewProjectionMatrix * position; 
        gl_FrontColor = vec4(vec3(0.75), 1.0);
        gl_BackColor  = vec4(0.0);

        fragPos = position;
    }
)";

const char* fragmentShader2 = R"(
    uniform vec4 lightPos;
    varying vec4 normal;
    varying vec4 fragPos;
    varying float NdotL;
    varying vec3 ReflectVec;
    varying vec3 ViewVec;


    const vec3 SurfaceColor = vec3(0.75, 0.75, 0.75);
    const vec3 WarmColor = vec3(0.1, 0.4, 0.8);
    const vec3 CoolColor = vec3(0.6, 0.0, 0.0);
    const float DiffuseWarm = 0.45;
    const float DiffuseCool = 0.045;

    void main() {
        vec3 kcool    = min(CoolColor + DiffuseCool * vec3(gl_Color), vec3(1.0));
        vec3 kwarm    = min(WarmColor + DiffuseWarm * vec3(gl_Color), vec3(1.0));
        vec3 kfinal   = mix(kcool, kwarm, NdotL) * gl_Color.a;
        vec3 nreflect = normalize(ReflectVec);
        vec3 nview    = normalize(ViewVec);
        float spec    = max(dot(nreflect, nview), 0.0);
        spec          = pow(spec, 32.0);

        gl_FragColor = vec4(min(kfinal + spec, vec3(1.0)), 1.0);
    }
)";


const char* fragmentShader3 = R"(
    uniform vec4 lightPos;
    varying vec4 normal;
    varying vec4 fragPos;

    const vec4 ambientMaterial = vec4(0.2, 0.2, 0.2, 1.0);
    const vec4 diffuseMaterial = vec4(0.8, 0.8, 0.8, 1.0);
    const vec4 specularMaterial = vec4(1.0, 1.0, 1.0, 1.0);
    const float shininess = 32.0;

    void main() {
        vec3 norm = normalize(normal.xyz);
        vec3 lightv = normalize(lightPos.xyz - fragPos.xyz);
        vec3 viewv = -normalize(fragPos.xyz);
        vec3 halfv = normalize(lightv + viewv);

        vec4 diffuse = max(0.0, dot(lightv, norm)) * diffuseMaterial * lightPos.w;

        vec4 ambient = ambientMaterial * lightPos.w;

        vec4 specular = vec4(0.0, 0.0, 0.0, 1.0);
        if (dot(lightv, viewv) > 0.0) {
            specular = pow(max(0.0, dot(norm, halfv)), shininess) * specularMaterial * lightPos.w;
        }

        vec3 color = clamp(vec3(ambient + diffuse + specular), 0.0, 1.0);

        gl_FragColor = vec4(color, 1.0);
    }
)";


static GLuint CompileShader(GLuint type, const std::string& source) {
    GLuint id = glCreateShader(type);
    const char* src = source.c_str();
    glShaderSource(id, 1, &src, nullptr);
    glCompileShader(id);
    
    int result;
    glGetShaderiv(id, GL_COMPILE_STATUS, &result);

    if (result == GL_FALSE) {
        int length;
        glGetShaderiv(id, GL_INFO_LOG_LENGTH, &length);
        char* message = (char*)alloca(length * sizeof(char));
        glGetShaderInfoLog(id, length, &length, message);
        std::cout << "Failed to compile " <<
            (type == GL_VERTEX_SHADER ? "vertex " : "fragment ")
            << "shader!" << std::endl;
        std::cout << message << std::endl;
        glDeleteShader(id);

        return 0;
    }

    return id;
}

static GLuint CreateShader(const std::string& vertexShader, const std::string& fragmentShader) {
    GLuint program = glCreateProgram();
    GLuint vs = CompileShader(GL_VERTEX_SHADER, vertexShader);
    GLuint fs = CompileShader(GL_FRAGMENT_SHADER, fragmentShader);

    glAttachShader(program, vs);
    glAttachShader(program, fs);
    glLinkProgram(program);
    glValidateProgram(program);

    glDeleteShader(vs);
    glDeleteShader(fs);

    return program;
}

void initScene() {
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glEnable(GL_DEPTH_TEST);

    glGenBuffers(1, &VBO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);

    computeFaceNormals(vertices, faces);

    std::vector<float> vertexData;
    for (const auto& vertex : vertices) {
        vertexData.push_back(vertex.x);
        vertexData.push_back(vertex.y);
        vertexData.push_back(vertex.z);
        vertexData.push_back(vertex.nx);
        vertexData.push_back(vertex.ny);
        vertexData.push_back(vertex.nz);
    }
    glBufferData(GL_ARRAY_BUFFER, vertexData.size() * sizeof(float), vertexData.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), 0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float))); // Normal attribute
    glEnableVertexAttribArray(1);

    glGenBuffers(1, &IBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, IBO);

    indices.clear();
    for (const auto& face : faces) {
        for (const auto& index : face) {
            indices.push_back(index);
        }
    }
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(int), indices.data(), GL_STATIC_DRAW);

    std::string shaderChoice;
    std::cout << "Which shader do you want to use? (1: Toon Shader, 2: Gooch Shader, 3: Phong Shader):";
    std::cin >> shaderChoice;

    const char* vertexShade;
    const char* fragmentShade;

    if (shaderChoice == "1") {
        vertexShade = vertexShader;
        fragmentShade = fragmentShader;
    }
    else if (shaderChoice == "2") {
        vertexShade = vertexShader2;
        fragmentShade = fragmentShader2;
    }
    else if (shaderChoice == "3") {
        vertexShade = vertexShader;
        fragmentShade = fragmentShader3;
    }
    else {
        std::cerr << "Invalid shader choice. Using Toon shader." << std::endl;
        vertexShade = vertexShader;
        fragmentShade = fragmentShader;
    }

    shaderProgram = CreateShader(vertexShade, fragmentShade);
    
    lightPosLocation = glGetUniformLocation(shaderProgram, "lightPos");
    if (lightPosLocation == -1) {
        std::cerr << "Failed to find uniform location: lightPos" << std::endl;
        return;
    }
}

void renderScene() {

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glUseProgram(shaderProgram);

    GLfloat lightPos[] = { -3.0f, -3.0f, -3.0f, 1.0f };
    glUniform4fv(lightPosLocation, 1, lightPos);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, IBO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);

    glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);


    glUseProgram(0);

    glutSwapBuffers();
    glutPostRedisplay();
}


void rotateOverArbitrary(Vertex P1, Vertex P2, float angle) {
    Vertex axis = { P2.x - P1.x, P2.y - P1.y, P2.z - P1.z };
    float magnitude = sqrt(axis.x * axis.x + axis.y * axis.y + axis.z * axis.z);
    axis.x /= magnitude;
    axis.y /= magnitude;
    axis.z /= magnitude;

    glTranslatef(-P1.x, -P1.y, -P1.z);

    float d = sqrt(axis.y * axis.y + axis.z * axis.z);
    float e = sqrt(axis.x * axis.x + axis.y * axis.y + axis.z * axis.z);

    if (d == 0) {
        glRotatef(-angle, 1, 0, 0);
    }
    else
    {
        float cos_alpha = axis.z / d;
        float sin_alpha = axis.y / d;
        float cos_beta = d / e;
        float sin_beta = axis.x / e;

        glRotatef(((-acosf(cos_alpha)) * 180.0 / M_PI), 1, 0, 0);
        glRotatef(((-asinf(sin_beta)) * 180.0 / M_PI), 0, 1, 0);

        glRotatef(-angle, 0, 0, 1);

        glRotatef(((asinf(sin_beta)) * 180.0 / M_PI), 0, 1, 0);
        glRotatef(((acosf(cos_alpha)) * 180.0 / M_PI), 1, 0, 0);
    }

    glTranslatef(P1.x, P1.y, P1.z);
}


void reflectOverArbitrary(Vertex planePoint, Vertex planeNormal) {

    glTranslatef(-planePoint.x, -planePoint.y, -planePoint.z);

    float d = sqrt(planeNormal.y * planeNormal.y + planeNormal.z * planeNormal.z);
    float e = sqrt(planeNormal.x * planeNormal.x + planeNormal.y * planeNormal.y + planeNormal.z * planeNormal.z);

    if (d == 0) {
        glScalef(-1.0f, 1.0f, 1.0f);   
    }
    else {
        float cos_alpha = planeNormal.z / d;
        float sin_alpha = planeNormal.y / d;
        float cos_beta = d / e;
        float sin_beta = planeNormal.x / e;

        glRotatef(((-acosf(cos_alpha)) * 180.0 / M_PI), 1, 0, 0);
        glRotatef(((-asinf(sin_beta)) * 180.0 / M_PI), 0, 1, 0);

        glScalef(1.0f, 1.0f, -1.0f);

        glRotatef(((asinf(sin_beta)) * 180.0 / M_PI), 0, 1, 0);
        glRotatef(((acosf(cos_alpha)) * 180.0 / M_PI), 1, 0, 0);
    }
    glTranslatef(planePoint.x, planePoint.y, planePoint.z);
}

void translateModel(float x, float y, float z) {
    currentTranslation.x += x;
    currentTranslation.y += y;
    currentTranslation.z += z;
    glTranslatef(x, y, z);
}

void processNormalKeys(unsigned char key, int x, int y) {
    float shX, shY, shZ;
    float sx, sy, sz;
    float angle;
    float tx, ty, tz;
    Vertex P1, P2, n;

    switch (key) {
    case 't':
        printf("Enter translation values (tx ty tz): ");
        scanf_s("%f %f %f", &tx, &ty, &tz);
        glPushMatrix();
        translateModel(tx, ty, tz);
        break;
    case 's':
        printf("Enter scaling factors (sx sy sz): ");
        scanf_s("%f %f %f", &sx, &sy, &sz);
        glPushMatrix();
        glScalef(sx, sy, sz);
        break;
    case 'q'://shear in X Direstion
        printf("Enter shearing factors (shY shZ) for shearing in X-Direction: ");
        scanf_s("%f %f", &shY, &shZ);
        glPushMatrix();
        glScalef(1.0, shY, shZ); // Scale along y-axis
        glRotatef(-45.0, 0.0, 0.0, 1.0);
        break;
    case 'w'://shear in Y Direstion
        printf("Enter shearing factors (shX shZ) for shearing in Y-Direction: ");
        scanf_s("%f %f", &shX, &shZ);
        glPushMatrix();
        glScalef(shX, 1.0, shZ); // Scale along y-axis
        glRotatef(-45.0, 0.0, 0.0, 1.0); // Rotate counterclockwise by 45 degrees
        break;
    case 'e'://shear in Z Direstion
        printf("Enter shearing factors (shX shY) for shearing in Z-Direction: ");
        scanf_s("%f %f", &shX, &shY);
        glPushMatrix();
        glScalef(shX, shY, 1.0); // Scale along z-axis
        glRotatef(-45.0, 0.0, 0.0, 1.0);
        break;
    case 'r':
        printf("Enter coordinates of two points (x1 y1 z1 x2 y2 z2) defining the axis: "); //rotation around arbitrary and parallel axis
        scanf_s("%f %f %f %f %f %f", &P1.x, &P1.y, &P1.z, &P2.x, &P2.y, &P2.z);
        printf("Enter rotation angle: ");
        scanf_s("%f", &angle);
        glPushMatrix();
        rotateOverArbitrary(P1, P2, angle);
        break;
    case 'u':
        printf("Enter coordinates of a point and a normal (x1 y1 z1 nx ny nz) defining the plane: ");
        scanf_s("%f %f %f %f %f %f", &P1.x, &P1.y, &P1.z, &n.x, &n.y, &n.z);
        glPushMatrix();
        reflectOverArbitrary(P1, n);
        break;
    case 'a':
        glPopMatrix();
        break;
    default:
        break;
    }

    glutPostRedisplay();
}

void cleanup() {
    glDeleteBuffers(1, &IBO);
    glDeleteBuffers(1, &VBO);
    glDeleteProgram(shaderProgram);
}

int main(int argc, char** argv) {

    if (!parseOFF("off/6.off")) {
     return -1;
    }

    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH);
    glutInitWindowPosition(200, 100);
    glutInitWindowSize(500, 500);
    glutCreateWindow("Assignment 2 GPU");

    GLenum err = glewInit();
    if (err != GLEW_OK) {
        std::cerr << "GLEW initialization failed: " << glewGetErrorString(err) << std::endl;
        return -1;
    }

    glutDisplayFunc(renderScene);
    initScene();
    glutKeyboardFunc(processNormalKeys);
    glutMainLoop();
    cleanup();

    return 0;
}
