#ifndef PARSER_H
#define PARSER_H

#include <vector>
#include <glm/glm.hpp>
#include <string>
#pragma once

using namespace glm;

enum ObjectType{
    OBJ,
    REFLECTIVE,
    TRANSPARENT,
    NOTHING
};
enum ObjectClass{
    PLANE,
    SPHERE
};
enum LightType{
    DIRECTIONAL,
    SPOTLIGHT
};

struct Surface{

protected:
    ObjectType type;
    ObjectClass objectClass;
    vec3 position_cord = vec3(0, 0, 0);
    vec4 coordinates = vec4(0, 0, 0, 0);
    vec3 color = vec3(0, 0, 0);
    float shine = 0;

public:
    virtual vec3 getColor(vec3 hit) = 0;
    void setColor(vec4 color){
        this->color = vec3(color.r, color.g, color.b);
        this->shine = color.w;
    }
    void setShininess(float shine){
        this->shine = shine;
    }
    ObjectClass getObjectClass(){
        return this->objectClass;
    }
    vec3 getPosition(){
        return position_cord;
    }
    float getShininess(){
        return this->shine;
    }
    vec4 getCoordinates(){
        return this->coordinates;
    }
    ObjectType getType(){
        return this->type;
    }
    
};

struct Sphere : Surface{

private:
    double radius;

public:
    Sphere(double x, double y, double z, double r, ObjectType type){
        this->coordinates = vec4(x, y, z, r);
        this->type = type;
        this->objectClass = SPHERE;
        this->position_cord = vec3(x, y, z);
        this->radius = r;
    };

    float getRadius(){
        return this->radius;
    }
    void setRadius(double r){
        this->radius = r;
    }
    vec3 getPosition(){
        return this->position_cord;
    }
    vec3 getColor(vec3 hit){
        return this->color;
    }
};

struct Plane : Surface{

    Plane(double a, double b, double c, double d, ObjectType type){
        this->coordinates = vec4(a, b, c, d);
        this->position_cord = vec3(a, b, c);
        this->type = type;
        this->objectClass = PLANE;
    };

    vec3 getPosition(){
        return this->position_cord;
    }
    float getD(){
        return this->coordinates.w;
    }
    vec3 getColor(vec3 hit){

        // Checkerboard pattern
        float scaling = 0.5f;
        float chessboard = 0;

        if (hit.x < 0){
            chessboard += floor((0.5 - hit.x) / scaling);
        }
        else{
            chessboard += floor(hit.x / scaling);
        }

        if (hit.y < 0){
            chessboard += floor((0.5 - hit.y) / scaling);
        }
        else
            chessboard += floor(hit.y / scaling);
        

        chessboard = (chessboard * 0.5) - int(chessboard * 0.5);
        chessboard *= 2;

        if (chessboard > 0.5)
            return 0.5f * this->color;
        
        return this->color;
    }
};

struct Ray
{

private:
    vec3 direction;
    vec3 origin;
    vec3 hit;
    Surface *sceneObject;

public:
    Ray(vec3 direction, vec3 origin)
    {
        this->direction = direction;
        this->origin = origin;
        this->hit = origin + direction;
        this->sceneObject = new Plane(0.0, 0.0, 0.0, 0.0, NOTHING);
        ;
    }

    vec3 getRayDirection()
    {
        return this->direction;
    }
    vec3 getRayOrigin()
    {
        return this->origin;
    }
    vec3 getHitPoint()
    {
        return this->hit;
    }
    Surface *getSceneObject()
    {
        return this->sceneObject;
    }
    void setRayDirection(vec3 vec)
    {
        this->direction = vec;
    }

    void setRayOrigin(vec3 vec)
    {
        this->origin = vec;
    }
    void setHitPoint(vec3 vec)
    {
        this->hit = vec;
    }
    void setSceneObject(Surface *obj)
    {
        this->sceneObject = obj;
    }
};

struct Eye
{
    vec3 coordinates;

    Eye(double x, double y, double z)
    {
        coordinates = vec3(x, y, z);
    };
    Eye()
    {
        coordinates = vec3(0, 0, 0);
    };
    vec3 getCoordinates()
    {
        return this->coordinates;
    }
};

struct Light
{
    vec3 direction;
    vec3 intensity;
    float shine = 0;
    LightType type;

    void setDirection(float x, float y, float z)
    {
        this->direction = vec3(x, y, z);
    }

    void setIntensity(vec4 intensity)
    {
        this->intensity = vec3(intensity.r, intensity.g, intensity.b);
        this->shine = intensity.w;
    }

    vec3 getIntensity()
    {
        return this->intensity;
    }
};

struct DirectionalLight : Light
{
    DirectionalLight(vec3 direction)
    {
        this->type = DIRECTIONAL;
        this->direction = direction;
    }
};

struct SpotLight : Light
{
    vec3 position_cord;
    float w = 0;

    SpotLight(vec3 direction)
    {
        this->type = SPOTLIGHT;
        this->direction = direction;
        w = 0;
        position_cord = vec3(0, 0, 0);
    }

    void setPosition(float x, float y, float z)
    {
        this->position_cord = vec3(x, y, z);
    }
    void setAngle(float w)
    {
        this->w = w;
    }
    float getAngle()
    {
        return w;
    }
    vec3 getPosition()
    {
        return this->position_cord;
    }
};

#endif
