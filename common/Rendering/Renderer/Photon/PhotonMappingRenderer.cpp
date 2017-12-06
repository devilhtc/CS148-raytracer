#include "common/Rendering/Renderer/Photon/PhotonMappingRenderer.h"
#include "common/Scene/Scene.h"
#include "common/Sampling/ColorSampler.h"
#include "common/Scene/Lights/Light.h"
#include "common/Scene/Geometry/Primitives/Primitive.h"
#include "common/Scene/Geometry/Mesh/MeshObject.h"
#include "common/Rendering/Material/Material.h"
#include "common/Intersection/IntersectionState.h"
#include "common/Scene/SceneObject.h"
#include "common/Scene/Geometry/Mesh/MeshObject.h"
#include "common/Rendering/Material/Material.h"
#include "glm/gtx/component_wise.hpp"

#define VISUALIZE_PHOTON_MAPPING 1

PhotonMappingRenderer::PhotonMappingRenderer(std::shared_ptr<class Scene> scene, std::shared_ptr<class ColorSampler> sampler):
    BackwardRenderer(scene, sampler), 
    diffusePhotonNumber(1000000),
    maxPhotonBounces(1000)
{
    srand(static_cast<unsigned int>(time(NULL)));
}

void PhotonMappingRenderer::InitializeRenderer()
{
    // Generate Photon Maps
    GenericPhotonMapGeneration(diffuseMap, diffusePhotonNumber);
    diffuseMap.optimise();
}

void PhotonMappingRenderer::GenericPhotonMapGeneration(PhotonKdtree& photonMap, int totalPhotons)
{
    float totalLightIntensity = 0.f;
    size_t totalLights = storedScene->GetTotalLights();
    for (size_t i = 0; i < totalLights; ++i) {
        const Light* currentLight = storedScene->GetLightObject(i);
        if (!currentLight) {
            continue;
        }
        totalLightIntensity += glm::length(currentLight->GetLightColor());
    }

    // Shoot photons -- number of photons for light is proportional to the light's intensity relative to the total light intensity of the scene.
    for (size_t i = 0; i < totalLights; ++i) {
        const Light* currentLight = storedScene->GetLightObject(i);
        if (!currentLight) {
            continue;
        }

        const float proportion = glm::length(currentLight->GetLightColor()) / totalLightIntensity;
        const int totalPhotonsForLight = static_cast<const int>(proportion * totalPhotons);
        const glm::vec3 photonIntensity = currentLight->GetLightColor() / static_cast<float>(totalPhotonsForLight);
        for (int j = 0; j < totalPhotonsForLight; ++j) {
            Ray photonRay;
            std::vector<char> path;
            path.push_back('L');
            currentLight->GenerateRandomPhotonRay(photonRay);
            TracePhoton(photonMap, &photonRay, photonIntensity, path, 1.f, maxPhotonBounces);
        }
    }
}

// CUSTOM HELPER FUNCTIONS
// START
float max3 (glm::vec3 v) {
    return std::max (std::max (v.x, v.y), v.z);
}

float rand01() {
    return ((float)rand() ) / (((float) RAND_MAX) + 1.f);
}

glm::vec3 get_tangent(glm::vec3 normal) {
    glm::vec3 e = glm::vec3(1.f, 0.f, 0.f);
    if (glm::dot(normal, e) > 0.8f || glm::dot(normal, e) < -0.8f) {
        e = glm::vec3(0.f, 1.f, 0.f);
    }
    return glm::normalize( glm::cross(normal, e) );
}

glm::vec3 get_bitangent(glm::vec3 normal, glm::vec3 tangent) {
    return glm::normalize( glm::cross(normal, tangent) );
}

glm::vec3 get_absolute_bounce_direction(glm::vec3 r, glm::vec3 n, glm::vec3 t, glm::vec3 b) {
    glm::vec3 abd = r.x * t + r.y * b + r.z * n;
    return glm::normalize(abd);
}

// END

void PhotonMappingRenderer::TracePhoton(PhotonKdtree& photonMap, Ray* photonRay, glm::vec3 lightIntensity, std::vector<char>& path, float currentIOR, int remainingBounces)
{
    /*
     * Assignment 8 TODO: Trace a photon into the scene and make it bounce.
     *    
     *    How to insert a 'Photon' struct into the photon map.
     *        Photon myPhoton;
     *        ... set photon properties ...
     *        photonMap.insert(myPhoton);
     */
    
    //std::printf("TracePhoton is called with %d remaining bounces\n", remainingBounces);
    if (remainingBounces < 0) { return; }
    
    assert(photonRay);
    IntersectionState state(0, 0);
    state.currentIOR = currentIOR;
    
    // find out whether it intersects with the scene and where
    bool existsIntersection = storedScene->Trace(photonRay, &state);
    if ( !existsIntersection ) { return; }
    const glm::vec3 intersectionPoint = state.intersectionRay.GetRayPosition(state.intersectionT);
    
    // store the photon if we have to
    // std::printf("The path vector has size %d \n", (int)path.size());
    if (((int) path.size()) > 1) {
        Photon myPhoton;
        myPhoton.position = intersectionPoint;
        myPhoton.intensity = lightIntensity;

        photonMap.insert(myPhoton);
    }
    
    // add to path so that in the next recursive call the photon should be stored
    path.push_back('B');
    
    // get material
    const MeshObject* hitMeshObject = state.intersectedPrimitive->GetParentMeshObject();
    const Material* hitMaterial = hitMeshObject->GetMaterial();
    
    // determine if bounced or absorbed
    glm::vec3 diffuseReflection = hitMaterial->GetBaseDiffuseReflection();
    float p_r = max3(diffuseReflection);
    float rand_p_r = rand01();
    // std::printf("Then random number is %f and p_r is %f \n", rand_p_r, p_r);
    
    if (rand_p_r > p_r) { return; }
    
    // now it should be bounced and TracePhoton should be called recursively
    
    // 1st step, generate a random direction in hemisphere (relative)
    float u1 = rand01();
    float u2 = rand01();
    
    float bounce_r = pow(u1, 0.5f);
    float bounce_theta = 2.f * PI * u2;
    
    float bounce_x = bounce_r * cos(bounce_theta);
    float bounce_y = bounce_r * sin(bounce_theta);
    float bounce_z = pow(1.f - u1, 0.5f);
    
    glm::vec3 relative_bounce_direction = glm::vec3(bounce_x,bounce_y,bounce_z);
    relative_bounce_direction = glm::normalize(relative_bounce_direction);
    
    // 2nd step, get the normal, tangent and bitangent of the bouncing surface
    glm::vec3 surface_normal = glm::normalize( state.ComputeNormal() );
    glm::vec3 surface_tangent = get_tangent(surface_normal);
    glm::vec3 surface_bitangent = get_bitangent(surface_normal, surface_tangent);
    
    // 3rd step, get the absolute bounce direction by essentially a matrix multiplication
    glm::vec3 absolute_bounce_direction  =  get_absolute_bounce_direction(relative_bounce_direction, surface_normal, surface_tangent, surface_bitangent);
    
    
    // update ray parameters (after bouncing)
    
    // apply offset (so that it won't self-intersect)
    glm::vec3 newRayPosition = intersectionPoint + LARGE_EPSILON * absolute_bounce_direction;
    
    // update
    photonRay->SetRayDirection(absolute_bounce_direction);
    photonRay->SetRayPosition(newRayPosition);
    
    // recursive call
    TracePhoton(photonMap, photonRay, lightIntensity, path, currentIOR, remainingBounces - 1);
}

glm::vec3 PhotonMappingRenderer::ComputeSampleColor(const struct IntersectionState& intersection, const class Ray& fromCameraRay) const
{
    glm::vec3 finalRenderColor = BackwardRenderer::ComputeSampleColor(intersection, fromCameraRay);
#if VISUALIZE_PHOTON_MAPPING
    Photon intersectionVirtualPhoton;
    intersectionVirtualPhoton.position = intersection.intersectionRay.GetRayPosition(intersection.intersectionT);

    std::vector<Photon> foundPhotons;
    diffuseMap.find_within_range(intersectionVirtualPhoton, 0.003f, std::back_inserter(foundPhotons));
    if (!foundPhotons.empty()) {
        finalRenderColor += glm::vec3(1.f, 0.f, 0.f);
    }
#endif
    return finalRenderColor;
}

void PhotonMappingRenderer::SetNumberOfDiffusePhotons(int diffuse)
{
    diffusePhotonNumber = diffuse;
}
