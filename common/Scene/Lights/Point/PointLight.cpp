#include "common/Scene/Lights/Point/PointLight.h"


void PointLight::ComputeSampleRays(std::vector<Ray>& output, glm::vec3 origin, glm::vec3 normal) const
{
    origin += normal * LARGE_EPSILON;
    const glm::vec3 lightPosition = glm::vec3(GetPosition());
    const glm::vec3 rayDirection = glm::normalize(lightPosition - origin);
    const float distanceToOrigin = glm::distance(origin, lightPosition);
    output.emplace_back(origin, rayDirection, distanceToOrigin);
}

float PointLight::ComputeLightAttenuation(glm::vec3 origin) const
{
    return 1.f;
}

float rand01l() {
    return ((float)rand() ) / (((float) RAND_MAX) + 1.f);
}

void PointLight::GenerateRandomPhotonRay(Ray& ray) const
{
    // Assignment 8 TODO: Fill in the random point light samples here.
    
    float x, y, z;
    
    do {
        x = ( rand01l() - 0.5f ) * 2.f;
        y = ( rand01l() - 0.5f ) * 2.f;
        z = ( rand01l() - 0.5f ) * 2.f;
    } while ( x*x + y*y + z*z > 1.f );
    
    ray.SetRayDirection( glm::normalize( glm::vec3(x,y,z) ) );
    ray.SetRayPosition( glm::vec3( PointLight::GetPosition() ) );
}
