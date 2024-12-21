#include <fstream>
#include "Reader.h"
#include <iostream>
#include <cstring>
#include <sstream>
#include <vector>
#include <glm/glm.hpp>

using namespace std;

class Reader
{

public:
    Eye *eye;
    vec4 *ambientLight;
    vector<Plane *> *planes;
    vector<Surface *> *objects;
    vector<Light *> *lights;
    vector<SpotLight *> *spotlights;
    vector<Sphere *> *spheres;

    Reader()
    {
        this->eye = new Eye();
        this->ambientLight = new glm::vec4(0);
        this->lights = new vector<Light *>();
        this->spotlights = new vector<SpotLight *>();
        this->spheres = new vector<Sphere *>();
        this->planes = new vector<Plane *>();
        this->objects = new vector<Surface *>();
    };

    void parser(string fileName)
    {

        int object_tracker = 0, posindex = 0, intensity_index = 0;
        Light *light = nullptr;
        char type = 'a';
        float first_cord = 1, second_cord = 1, third_cord = 1, forth_cord;

        // handle input file
        ifstream inputFile(fileName);
        if (!inputFile)
        {
            cerr << "Error in opening file " << fileName << ": " << strerror(errno) << endl;
        }

        string line;
        while (getline(inputFile, line))
        {
            char firstChar = ' ';
            vector<double> numbers;
            istringstream iss(line);
            iss >> firstChar; // Extract the first character
            double number;
            while (iss >> number){
                numbers.push_back(number); // Store the number in the vector
            }

            // Output the extracted character and numbers (for debugging purpose)
            cout << "character:" << firstChar << endl;
            cout << "Numbers: ";
            for (int i=0; i < 4; i++){
                cout << numbers[i] << " ";
            }
            cout << endl;

            // parsed arguments
            first_cord = numbers[0];
            second_cord = numbers[1];
            third_cord = numbers[2];
            forth_cord = numbers[3];
            type = firstChar;

            switch (type){

            case 'a':
                this->ambientLight = new vec4(first_cord, second_cord, third_cord, forth_cord);
                break;

            case 'e':
                this->eye = new Eye(first_cord, second_cord, third_cord);
                break;


            case 'p':
                this->spotlights->at(posindex)->setPosition(first_cord, second_cord, third_cord);
                this->spotlights->at(posindex)->setAngle(forth_cord);
                (posindex)++;
                break;

            case 'd':
                if (forth_cord == 1){
                    light = new SpotLight(vec3(first_cord, second_cord, third_cord));
                    this->spotlights->push_back(const_cast<SpotLight *>(reinterpret_cast<const SpotLight *>(light)));
                }
                else
                    light = new DirectionalLight(vec3(first_cord, second_cord, third_cord));
                light->setDirection(first_cord, second_cord, third_cord);
                this->lights->push_back(light);

                break;

            case 'i':
                this->lights->at(intensity_index)->setIntensity(vec4(first_cord, second_cord, third_cord, forth_cord));
                (intensity_index)++;
                break;

            case 'c':
                this->objects->at(object_tracker)->setShininess(forth_cord);
                this->objects->at(object_tracker)->setColor(vec4(first_cord, second_cord, third_cord, forth_cord));
                (object_tracker)++;
                break;

            default:
                if (forth_cord < 0){ //plane
                    ObjectType plane_type = getType(type);
                    Plane *p = new Plane(first_cord, second_cord, third_cord, forth_cord, plane_type);
                    this->planes->push_back(p);
                    p = this->planes->at(this->planes->size() - 1);
                    this->objects->push_back(p);
                }
                else{ //sphere
                    ObjectType sphere_type = getType(type);
                    Sphere *s = new Sphere(first_cord, second_cord, third_cord, forth_cord, sphere_type);
                    s->setRadius(forth_cord);
                    this->spheres->push_back(s);
                    s = this->spheres->at(this->spheres->size() - 1);
                    this->objects->push_back(s);

                }
            }
        }
    }

    static ObjectType getType(char c){
        if(c == 't')
            return TRANSPARENT;
        if(c == 'o')
            return OBJ;
        if(c == 'r')
            return REFLECTIVE;
        return NOTHING;

    }

};
