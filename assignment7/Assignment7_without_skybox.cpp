#include "assignment7/Assignment7.h"
#include "common/core.h"

std::shared_ptr<Camera> Assignment7::CreateCamera() const
{
    const glm::vec2 resolution = GetImageOutputResolution();
    std::shared_ptr<Camera> camera = std::make_shared<PerspectiveCamera>(resolution.x / resolution.y, 26.6f);
    camera->SetPosition(glm::vec3(5.f, -22.2f, 11.0f));
    camera->Rotate(glm::vec3(1.f, 0.f, 0.f), PI* 6.f/13.f);
    return camera;
}


// Assignment 7 Part 1 TODO: Change the '1' here.
// 0 -- Naive.
// 1 -- BVH.
// 2 -- Grid.
#define ACCELERATION_TYPE 1

void loadObj(float sp, float df, float reflec, std::shared_ptr<Scene> newScene, const char* texture_file, const char* object_file)
{
	std::shared_ptr<BlinnPhongMaterial> cubeMaterial = std::make_shared<BlinnPhongMaterial>();
	cubeMaterial->SetDiffuse(glm::vec3(df, df, df));
	cubeMaterial->SetSpecular(glm::vec3(sp, sp, sp), 40.f);
	cubeMaterial->SetReflectivity(reflec);
	cubeMaterial->SetTexture("diffuseTexture", TextureLoader::LoadTexture(texture_file));
	//cubeMaterial->SetTexture("specularTexture", TextureLoader::LoadTexture("genji_texture_c2.png"));
	// Objects
	std::vector<std::shared_ptr<aiMaterial>> loadedMaterials;
	std::vector<std::shared_ptr<MeshObject>> cubeObjects = MeshLoader::LoadMesh(object_file, &loadedMaterials);
	for (size_t i = 0; i < cubeObjects.size(); ++i) {
		std::shared_ptr<Material> materialCopy = cubeMaterial->Clone();
		materialCopy->LoadMaterialFromAssimp(loadedMaterials[i]);
		cubeObjects[i]->SetMaterial(materialCopy);

		std::shared_ptr<SceneObject> cubeSceneObject = std::make_shared<SceneObject>();
		cubeSceneObject->AddMeshObject(cubeObjects[i]);
		cubeSceneObject->Rotate(glm::vec3(1.f, 0.f, 0.f), PI / 2.f);

		cubeSceneObject->CreateAccelerationData(AccelerationTypes::BVH);
		cubeSceneObject->ConfigureAccelerationStructure([](AccelerationStructure* genericAccelerator) {
			BVHAcceleration* accelerator = dynamic_cast<BVHAcceleration*>(genericAccelerator);
			accelerator->SetMaximumChildren(2);
			accelerator->SetNodesOnLeaves(2);
		});

		cubeSceneObject->ConfigureChildMeshAccelerationStructure([](AccelerationStructure* genericAccelerator) {
			BVHAcceleration* accelerator = dynamic_cast<BVHAcceleration*>(genericAccelerator);
			accelerator->SetMaximumChildren(2);
			accelerator->SetNodesOnLeaves(2);
		});
		newScene->AddSceneObject(cubeSceneObject);
	}
}

void addLight(glm::vec3 pos, glm::vec3 color, std::shared_ptr<Scene> newScene)
{
	std::shared_ptr<Light> pointLight = std::make_shared<PointLight>();
	pointLight->SetPosition(pos);
	pointLight->SetLightColor(color);
	newScene->AddLight(pointLight);
}

std::shared_ptr<Scene> Assignment7::CreateScene() const
{
    std::shared_ptr<Scene> newScene = std::make_shared<Scene>();

    // Material
	float sp = 0.0f;
	float df = 1.0f;

	loadObj(sp, df, 0.0f, newScene, "final/genji_diff.png", "final/second_genji_model_modified.obj");
	//loadObj(sp, df, 1.0f, newScene, "genji_diff.png", "second_genji_model.obj");
	loadObj(1.0f, 0.2f, 1.0f, newScene, "final/metalic.jpg", "final/sword2_modified.obj");
    //loadObj(1.0f, 0.2f, 1.0f, newScene, "final/metalic.jpg", "final/sword3.obj");
	loadObj(sp, df * 2.0f / 3.0f, 0.0f, newScene, "final/room_g1_uv_painted.png", "final/room_g1.obj");
	loadObj(sp, df,  0.0f, newScene, "final/room_g2_uv_painted.png", "final/room_g2.obj");
    loadObj(sp, df,  0.0f, newScene, "final/floor_uv_painted.png", "final/floor.obj");
    // Lights
    //std::shared_ptr<Light> pointLight = std::make_shared<PointLight>();
    //pointLight->SetPosition(glm::vec3(0.0f, -5.0f, -10.0f));
    //pointLight->SetLightColor(glm::vec3(1.f, 1.f, 1.f));

#if ACCELERATION_TYPE == 0
    newScene->GenerateAccelerationData(AccelerationTypes::NONE);
#elif ACCELERATION_TYPE == 1
    newScene->GenerateAccelerationData(AccelerationTypes::BVH);
#else
    UniformGridAcceleration* accelerator = dynamic_cast<UniformGridAcceleration*>(newScene->GenerateAccelerationData(AccelerationTypes::UNIFORM_GRID));
    assert(accelerator);
    // Assignment 7 Part 2 TODO: Change the glm::ivec3(10, 10, 10) here.
    accelerator->SetSuggestedGridSize(glm::ivec3(10, 10, 10));
#endif    
    //newScene->AddLight(pointLight);


	addLight(glm::vec3(-3.0f, -1.0f, 15.0f), glm::vec3(1.f, 1.f, 1.f), newScene);
	addLight(glm::vec3(13.0f, -1.0f, 15.0f), glm::vec3(1.f, 1.f, 1.f), newScene);
	

    return newScene;

}
std::shared_ptr<ColorSampler> Assignment7::CreateSampler() const
{
    std::shared_ptr<JitterColorSampler> jitter = std::make_shared<JitterColorSampler>();
    jitter->SetGridSize(glm::ivec3(2, 2, 2));
    return jitter;
}

std::shared_ptr<class Renderer> Assignment7::CreateRenderer(std::shared_ptr<Scene> scene, std::shared_ptr<ColorSampler> sampler) const
{
    return std::make_shared<BackwardRenderer>(scene, sampler);
}

int Assignment7::GetSamplesPerPixel() const
{
    return 2;
}

bool Assignment7::NotifyNewPixelSample(glm::vec3 inputSampleColor, int sampleIndex)
{
    return true;
}

int Assignment7::GetMaxReflectionBounces() const
{
    return 1;
}

int Assignment7::GetMaxRefractionBounces() const
{
    return 0;
}

glm::vec2 Assignment7::GetImageOutputResolution() const
{
    return glm::vec2(720.f, 480.f);
    // return glm::vec2(640.f, 480.f);
    //return glm::vec2(960.f, 720.f);
}
