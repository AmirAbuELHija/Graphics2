#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <Debugger.h>
#include <VertexBuffer.h>
#include <VertexBufferLayout.h>
#include <IndexBuffer.h>
#include <VertexArray.h>
#include <Shader.h>
#include <Texture.h>
#include <Camera.h>

#include <iostream>
#include "Reader.cpp"
/* Window size */
const unsigned int width = 800;
const unsigned int height = 800;
const float PI = 3.14159265;
/* Shape vertices coordinates with positions, colors, and corrected texCoords */
float vertices[] = {
    // positions   // texCoords (flipped Y-axis)
    -1.0f,  1.0f,  0.0f, 0.0f, // top-left
    -1.0f, -1.0f,  0.0f, 1.0f, // bottom-left
    1.0f, -1.0f,  1.0f, 1.0f, // bottom-right
    1.0f,  1.0f,  1.0f, 0.0f  // top-right
};

/* Indices for vertices order */
unsigned int indices[] = { // first and second triangle
    0, 1, 2,
    0, 2, 3  
};

void init_ray(Surface* closestObject, Ray reflectedRay){
    closestObject = new Plane(0.0, 0.0, 0.0, 0.0, NOTHING);
    reflectedRay.setHitPoint(reflectedRay.getRayOrigin() + reflectedRay.getRayDirection());
    reflectedRay.setSceneObject(closestObject);
}

float calcA(Ray ray){
    vec3 direction = ray.getRayDirection();
    float result = dot(direction, direction);
    return result;
}

float calcB(vec3 oc, Ray ray){
    vec3 direction = ray.getRayDirection();
    float result = dot(oc, direction);
    return 2.0f * result;
}

float calcC(vec3 oc, Sphere* sph){
    float result = dot(oc, oc);
    float multi = sph->getRadius() * sph->getRadius();
    return result - multi;
}
Ray UpdateRay(int j, int i, Surface* ob, bool update, Ray reflectedRay, Reader* scene) {

    float width = 2.0f / 800.0f;
    float height = 2.0f / 800.0f;

    if (!update) {
        vec3 pixelCenter(-1 + width / 2, 1 - height / 2, 0);
        vec3 exactPixel = pixelCenter + vec3(j * width, -1 * (i * height), 0);
        vec3 eyeVec = scene->eye->getCoordinates();
        vec3 rayDirection = normalize(exactPixel - eyeVec);

        reflectedRay.setRayDirection(rayDirection);
        reflectedRay.setRayOrigin(eyeVec);
    }

    // update the ray
    Surface* closestObject = NULL;
    init_ray(closestObject, reflectedRay);
    float nearest_obj = INFINITY;

    for (int i = 0; i < scene->objects->size(); i++) {
        float t = 0.0;
        Surface* currentObject = scene->objects->at(i);
        if (currentObject != ob) {

            if (currentObject->getObjectClass() == SPHERE) {       // sphere          
                vec3 oc = reflectedRay.getRayOrigin() - currentObject->getPosition();
                float a, b, c;
                a = calcA(reflectedRay);
                b = calcB(oc, reflectedRay);
                c = calcC(oc, (Sphere*)currentObject);

                float quad_delta = b * b - 4 * a * c; // Discriminant of the quadratic equation

                if (quad_delta >= 0) {
                    float quad_ans1 = (-b - sqrt(quad_delta)) / (2.0f * a);
                    float quad_ans2 = (-b + sqrt(quad_delta)) / (2.0f * a);

                    if (quad_ans1 < 0 && quad_ans2 < 0) {
                        t = -1.0f; // no intersection

                    }
                    float first;
                    if(quad_ans1 >= 0)
                        first = quad_ans1;
                    else
                        first = quad_ans2;

                    if (first <= 0.0001f) {
                        float second = (quad_ans1 >= 0 && quad_ans2 >= 0) ? glm::max(quad_ans1, quad_ans2) : -1.0f;
                        t = second;

                    }
                    else {
                        t = first;

                    }
                }
                else {
                    t = -1.0f;
                }
            }

            else { // plane
                float denominator = glm::dot(reflectedRay.getRayDirection(), currentObject->getPosition());

                if (abs(denominator) < 0.0001f) {
                    t = -1.0f; // No intersection

                }
                // intersection equation
                t = -(glm::dot(reflectedRay.getRayOrigin(), currentObject->getPosition()) + ((Plane*)currentObject)->getD()) / denominator;

                if (t < 0.0f) {
                    t = -1.0f; // No intersection
                }
            }
            if ((t >= 0) && t < nearest_obj) {
                closestObject = currentObject;
                nearest_obj = t;
                reflectedRay.setSceneObject(closestObject);
                reflectedRay.setHitPoint(reflectedRay.getRayOrigin() + reflectedRay.getRayDirection() * nearest_obj);
            }
        }
    }

    return reflectedRay;
}

vec3 get_Normal(vec3 hit_point, Surface* obj) {
    if (obj->getObjectClass() == SPHERE) {
        return normalize(hit_point - ((Sphere*)obj)->getPosition());
    }
    else
        return normalize(vec3(obj->getCoordinates()));
}

float calc_defuse(vec3 N, Ray ray, Light* light) { //defuse

    if (ray.getSceneObject()->getObjectClass() == SPHERE) {
        vec3 light_Direction = normalize(light->direction);
        if (light->type == SPOTLIGHT) {
            vec3 SpotRay = normalize(ray.getHitPoint() - ((SpotLight*)light)->getPosition());
            float cos = dot(SpotRay, light_Direction);
            if (cos >= ((SpotLight*)light)->getAngle()){

                light_Direction = SpotRay;
                cos = dot(N, -light_Direction);
                return glm::max(cos, 0.0f);;
            }
            else {
                return 0.0f;
            }
        }
        else { // DIRECTIONAL
            float cos = dot(N, -light_Direction);
            return glm::max(cos, 0.0f);;
        }
    }
    else { // PLANE
        vec3 light_Direction = -normalize(light->direction);
        if (light->type == SPOTLIGHT) {
            vec3 SpotRay = normalize(ray.getHitPoint() - ((SpotLight*)light)->getPosition());
            float cos = dot(SpotRay, -light_Direction);
            if (cos >= ((SpotLight*)light)->getAngle()){
                light_Direction = -SpotRay;
                cos = dot(N, -light_Direction);
                return glm::max(cos, 0.0f);;
            }
            else {
                return 0.0f;
            }
        }
        else { // DIRECTIONAL
            float cos = dot(N, -light_Direction);
            return glm::max(cos, 0.0f);
        }
    }
}

float calc_specular(vec3 V, Ray ray, Light* light) { //specular
    if (ray.getSceneObject()->getObjectClass() == SPHERE) { // sphere
        vec3 light_Direction = normalize(light->direction);
        vec3 normal_Sphere = get_Normal(ray.getHitPoint(), ray.getSceneObject());

        if (light->type == SPOTLIGHT) { // SPOTLIGHT
            vec3 Spot_Ray = normalize(ray.getHitPoint() - ((SpotLight*)light)->getPosition());
            float cos = dot(Spot_Ray, light_Direction);
            if (cos >= ((SpotLight*)light)->getAngle()) {
                light_Direction = Spot_Ray;
                vec3 reflected_Equation = light_Direction - 2.0f * normal_Sphere * dot(light_Direction, normal_Sphere);
                float cos = dot(V, reflected_Equation);
                cos = glm::max(0.0f, cos);
                return pow(cos, ray.getSceneObject()->getShininess());
            }
            else {
                return 0.0f;
            }
        }
        else { // DIRECTIONAL
            vec3 reflected_Equation = light_Direction - 2.0f * normal_Sphere * dot(light_Direction, normal_Sphere);
            float cos = dot(V, reflected_Equation);
            cos = glm::max(0.0f, cos);
            return pow(cos, ray.getSceneObject()->getShininess());
        }
    }

    else { // plane
        vec3 light_Direction = normalize(light->direction);
        vec3 normal_Plane = get_Normal(ray.getHitPoint(), ray.getSceneObject());
        if (light->type == DIRECTIONAL) {
            vec3 reflected_Equation = light_Direction - 2.0f * normal_Plane * dot(light_Direction, normal_Plane);
            float cos = dot(V, reflected_Equation);
            cos = glm::max(0.0f, cos);
            return pow(cos, ray.getSceneObject()->getShininess());
        }
        else {
            vec3 SpotRay = normalize(ray.getHitPoint() - ((SpotLight*)light)->getPosition());
            float cos = dot(SpotRay, light_Direction);
            if (cos < ((SpotLight*)light)->getAngle()) {
                return 0.0f;
            }
            else {
                light_Direction = SpotRay;
                vec3 reflected_Equation = light_Direction - 2.0f * normal_Plane * dot(light_Direction, normal_Plane);
                float cos = dot(V, reflected_Equation);
                cos = glm::max(0.0f, cos);
                return pow(cos, ray.getSceneObject()->getShininess());
            }
        }
    }
}

float calc_shadow(Ray ray, Light* light, Reader* scene) { //shadow

    vec3 light_Direction = glm::normalize(light->direction);
    float closest_obj = INFINITY;

    if (light->type == SPOTLIGHT) {
        vec3 SpotRay = glm::normalize(ray.getHitPoint() - ((SpotLight*)light)->getPosition());
        float cos = dot(SpotRay, light_Direction);

        if (cos >= ((SpotLight*)light)->getAngle()) {
            light_Direction = SpotRay;
            vec3 position = ((SpotLight*)light)->getPosition();
            vec3 hit_point = ray.getHitPoint();
            closest_obj = glm::length(position - hit_point);
        }
        else {
            return 0.0f;
        }
    }

    for (int i = 0; i < scene->objects->size(); i++) {
        Surface* currentObject = scene->objects->at(i);

        if (currentObject != ray.getSceneObject()) {
            Ray ray_oppo = Ray(-light_Direction, ray.getHitPoint());
            float temp = 0.0;

            if (currentObject != ray.getSceneObject()) {

                if (currentObject->getObjectClass() == PLANE) {
                    float denominator = glm::dot(ray_oppo.getRayDirection(), currentObject->getPosition());

                    if (abs(denominator) < 0.0001f) {
                        temp = -1.0f; // No intersection

                    }
                    // intersection equation
                    temp = -(glm::dot(ray_oppo.getRayOrigin(), currentObject->getPosition()) + ((Plane*)currentObject)->getD()) / denominator;

                    if (temp < 0.0f) {
                        temp = -1.0f; // No intersection

                    }

                }

                else {
                    vec3 oc = ray_oppo.getRayOrigin() - currentObject->getPosition();
                    float a, b, c;
                    a = calcA(ray_oppo);
                    b = calcB(oc, ray_oppo);
                    c = calcC(oc, (Sphere*)currentObject);

                    float quad_delta = b * b - 4 * a * c; // Discriminant of the quadratic equation

                    if (quad_delta < 0) {
                        temp = -1.0f;

                    }
                    else {
                        float quad_ans1 = (-b - sqrt(quad_delta)) / (2.0f * a);
                        float quad_ans2 = (-b + sqrt(quad_delta)) / (2.0f * a);

                        if (quad_ans1 < 0 && quad_ans2 < 0) {
                            temp = -1.0f; // no intersection

                        }
                        float first;
                        if(quad_ans1 >= 0)
                            first = quad_ans1;
                        else
                            first = quad_ans2;

                        if (first <= 0.0001f) {
                            float second = (quad_ans1 >= 0 && quad_ans2 >= 0) ? glm::max(quad_ans1, quad_ans2) : -1.0f;
                            temp = second;

                        }
                        else {
                            temp = first;

                        }
                    }
                }
            }

            if ((temp > 0) && (temp < closest_obj)) {
                return 0.0;
            }

        }
    }

    return 1.0;
}

// calc Snell Law
Ray calc_Snell_Law(Ray ray, glm::vec3 N, glm::vec3 rayDirection, float snellFrac) {
    vec3 normal_surface = get_Normal(-ray.getHitPoint(), ray.getSceneObject());
    float cos_a = dot(normal_surface, -ray.getRayDirection());
    float theta_a = acos(cos_a) * (180.0f / PI);

    // change in transparent sphere
    float snell_Fraction = (1.0f / 1.5f);

    float sin_a = sin(theta_a * (PI / 180.0f));
    float sin_b = snell_Fraction * sin_a;
    float theta_b = asin(sin_b) * (180.0f / PI);
    float cos_b = cos(theta_b * (PI / 180.0f));

    vec3 calc_snellMulti1 = (snellFrac * cos_a - cos_b) * N;
    vec3 calc_snellMulti2 = snellFrac * (-ray.getRayDirection());
    vec3 direc_ray = calc_snellMulti1 - calc_snellMulti2;
    Ray new_Ray(direc_ray, ray.getHitPoint());
    return new_Ray;
}

vec4 GetPixelColor(int pixelX, int pixelY, Ray currentRay, int recursionDepth, Reader* scene) {
    vec3 finalColor(0, 0, 0);
    vec3 emittedLight(0, 0, 0);
    vec3 specularComponent(0, 0, 0); 
    vec3 accumulatedLight(0, 0, 0);
    vec3 ambientReflectance(0, 0, 0);
    vec3 ambientLight(0, 0, 0);
    vec3 reflectiveComponent(0, 0, 0);
    vec3 reflectedLight(0, 0, 0);
    vec3 diffuseComponent(0, 0, 0); 
    if (currentRay.getSceneObject()->getType() == OBJ) { // Handle OBJ type
        ambientReflectance = currentRay.getSceneObject()->getColor(currentRay.getHitPoint());
        ambientLight = vec3(scene->ambientLight->r, scene->ambientLight->g, scene->ambientLight->b);

        for (int lightIndex = 0; lightIndex < scene->lights->size(); ++lightIndex) {
            vec3 specularReflectance(0.7f, 0.7f, 0.7f);
            vec3 diffuseReflectance = currentRay.getSceneObject()->getColor(currentRay.getHitPoint()) * scene->lights->at(lightIndex)->getIntensity();
            specularReflectance *= scene->lights->at(lightIndex)->getIntensity();

            vec3 normal = get_Normal(currentRay.getHitPoint(), currentRay.getSceneObject());
            vec3 viewDirection = normalize(currentRay.getRayOrigin() - currentRay.getHitPoint());

            diffuseComponent = diffuseReflectance * calc_defuse(normal, currentRay, scene->lights->at(lightIndex));
            specularComponent = specularReflectance * calc_specular(viewDirection, currentRay, scene->lights->at(lightIndex));

            float lightVisibility = calc_shadow(currentRay, scene->lights->at(lightIndex), scene);

            accumulatedLight += (diffuseComponent + specularComponent) * lightVisibility;
        }
    }

    finalColor = emittedLight + (ambientReflectance * ambientLight) + accumulatedLight + (reflectiveComponent * reflectedLight);

    if (currentRay.getSceneObject()->getType() == REFLECTIVE) { // Handle reflective type
        if (recursionDepth == 5) {
            return vec4(0.f, 0.f, 0.f, 0.f);
        }

        vec3 reflectionDirection = currentRay.getRayDirection() - 2.0f * get_Normal(currentRay.getHitPoint(), currentRay.getSceneObject()) * dot(currentRay.getRayDirection(), get_Normal(currentRay.getHitPoint(), currentRay.getSceneObject()));
        Ray reflectedRay(reflectionDirection, currentRay.getHitPoint());
        reflectedRay = UpdateRay(pixelX, pixelY, currentRay.getSceneObject(), true, reflectedRay, scene);

        if (reflectedRay.getSceneObject()->getType() == NOTHING) {
            return vec4(0.f, 0.f, 0.f, 0.f);
        }

        vec4 reflectedColor = GetPixelColor(pixelX, pixelY, reflectedRay, recursionDepth + 1, scene);
        finalColor = vec3(reflectedColor.r, reflectedColor.g, reflectedColor.b);
    }

    if (currentRay.getSceneObject()->getType() == TRANSPARENT) { // Handle transparent type
        if (recursionDepth == 5) {
            return vec4(0.f, 0.f, 0.f, 0.f);
        }

        vec3 surfaceNormal = get_Normal(currentRay.getHitPoint(), currentRay.getSceneObject());
        float refractionRatio = (0.5f / 1.5f); // tran ratio
        Ray refractedRay = calc_Snell_Law(currentRay, surfaceNormal, currentRay.getRayDirection(), refractionRatio);

        Plane* blackPlane = new Plane(0.0, 0.0, 0.0, 0.0, NOTHING);
        refractedRay = UpdateRay(pixelX, pixelY, blackPlane, true, refractedRay, scene);

        Surface* currentObject = currentRay.getSceneObject();
        float intersectionDistance = 0.0f;

        if (currentObject->getObjectClass() == PLANE) {
            float denominator = glm::dot(currentRay.getRayDirection(), currentObject->getPosition());

            if (abs(denominator) < 0.0001f) {
                intersectionDistance = -1.0f;
            }

            intersectionDistance = -(glm::dot(currentRay.getRayOrigin(), currentObject->getPosition()) + ((Plane*)currentObject)->getD()) / denominator;

            if (intersectionDistance < 0.0f) {
                intersectionDistance = -1.0f;
            }
        } else {
            vec3 rayOriginOffset = currentRay.getRayOrigin() - currentObject->getPosition();
            float a = calcA(currentRay);
            float b = calcB(rayOriginOffset, currentRay);
            float c = calcC(rayOriginOffset, (Sphere*)currentObject);

            float discriminant = b * b - 4 * a * c;

            if (discriminant < 0) {
                intersectionDistance = -1.0f;
            } else {
                float quad_ans1 = (-b - sqrt(discriminant)) / (2.0f * a);
                float quad_ans2 = (-b + sqrt(discriminant)) / (2.0f * a);

                if (quad_ans1 < 0 && quad_ans2 < 0) {
                    intersectionDistance = -1.0f;
                }

                float closest_Hit;
                if(quad_ans1 >= 0)
                    closest_Hit = quad_ans1;
                else
                    closest_Hit = quad_ans2;

                if (closest_Hit <= 0.0001f) {
                    float furtherHit = (quad_ans1 >= 0 && quad_ans2 >= 0) ? glm::max(quad_ans1, quad_ans2) : -1.0f;
                    intersectionDistance = furtherHit;
                } else {
                    intersectionDistance = closest_Hit;
                }
            }
        }

        vec3 secondaryHitPoint = refractedRay.getRayOrigin() + refractedRay.getRayDirection() * intersectionDistance;
        surfaceNormal = get_Normal(secondaryHitPoint, refractedRay.getSceneObject());
        Ray transmittedRay = calc_Snell_Law(refractedRay, surfaceNormal, refractedRay.getRayDirection(), refractionRatio);

        transmittedRay = UpdateRay(pixelX, pixelY, refractedRay.getSceneObject(), true, refractedRay, scene);

        if (transmittedRay.getSceneObject()->getType() == NOTHING) {
            return vec4(0.f, 0.f, 0.f, 0.f);
        }

        vec4 transmittedColor = GetPixelColor(pixelX, pixelY, transmittedRay, recursionDepth + 1, scene);
        finalColor = vec3(transmittedColor.r, transmittedColor.g, transmittedColor.b);
    }

    finalColor = min(finalColor, vec3(1.0, 1.0, 1.0));
    finalColor = max(finalColor, vec3(0.0, 0.0, 0.0));
    return vec4(finalColor.r, finalColor.g, finalColor.b, 1.0);
}


unsigned char* rendering(Reader* scene) {
    auto* image = new unsigned char[width * height * 4];

    for (int i = 0; i < width; i++) {
        for (int j = 0; j < height; j++) {
            Plane* black_Plane = new Plane(0.0, 0.0, 0.0, 0.0, NOTHING);
            Ray init_ray(vec3(0, 0, 0), vec3(0, 0, 0));
            Ray ray = UpdateRay(j, i, black_Plane, false, init_ray, scene);
            vec4 color = GetPixelColor(j, i, ray, 0, scene);

            image[(j + width * i) * 4] = (unsigned char)(color.r * 255);
            image[(j + width * i) * 4 + 1] = (unsigned char)(color.g * 255);
            image[(j + width * i) * 4 + 2] = (unsigned char)(color.b * 255);
            image[(j + width * i) * 4 + 3] = (unsigned char)(color.a * 255);
        }
    }
    return image;
}



// helper function
void setup_openGL(GLuint VBO, GLuint VAO, GLuint EBO){
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);
}

void display_Image(unsigned char* data) {
    GLFWwindow* window;

    /*init */
    if (!glfwInit()) {
        return;
    }

    /* Set OpenGL to Version 3.3.0 */
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    /* Create a windowed mode window */
    window = glfwCreateWindow(width, height, "Ray Traced Image", NULL, NULL);
    if (!window) {
        glfwTerminate();
        return;
    }

    /* Make the window's context current */
    glfwMakeContextCurrent(window);

    /* Load GLAD so it configures OpenGL */
    if (!gladLoadGL()) {
        std::cerr << "Failed to initialize GLAD" << std::endl;
        return;
    }

    /* OpenGL Setup */
    GLuint texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);

    /* Upload the image data to the GPU */
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // Setup OpenGL objects
    GLuint VBO, VAO, EBO;
    setup_openGL(VBO, VAO, EBO);

    // Texture coordinate attribute
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);

    // Position attribute
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    // fragment shader source
    const char* frag_Shader_Source = R"(
        #version 330 core
        out vec4 FragColor;
        in vec2 TexCoord;
        uniform sampler2D screenTexture;
        void main() {
            FragColor = texture(screenTexture, TexCoord);
        }
    )";


    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &frag_Shader_Source, NULL);
    glCompileShader(fragmentShader);

    // vertex Shader Source
    const char* ver_Shader_Source = R"(
        #version 330 core
        layout (location = 0) in vec2 aPos;
        layout (location = 1) in vec2 aTexCoord;
        out vec2 TexCoord;
        void main() {
            gl_Position = vec4(aPos.x, aPos.y, 0.0, 1.0);
            TexCoord = aTexCoord;
        }
    )";

    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &ver_Shader_Source, NULL);
    glCompileShader(vertexShader);

    // Attach shader
    GLuint shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);

    // clean up
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    // rendering loop
    while (!glfwWindowShouldClose(window)) {
        glClear(GL_COLOR_BUFFER_BIT);

        glUseProgram(shaderProgram);
        glBindTexture(GL_TEXTURE_2D, texture);
        glBindVertexArray(VAO);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // Clean up
    glDeleteBuffers(1, &EBO);
    glDeleteBuffers(1, &VBO);
    glDeleteVertexArrays(1, &VAO);
    glDeleteProgram(shaderProgram);

    glfwTerminate();
}

int main(int argc, char* argv[]) {
    Reader* r = new Reader();
    r->parser("res/Scenes/scene1.txt");
    unsigned char* image = rendering(r);
    display_Image(image);


    delete[] image;
    return 0;
}