#pragma once
#include "common.h"

#include <SketchUpAPI/sketchup.h>
#include "mesh.h"
#include "material.h"
#include "component.h"
#include "world.h"

namespace diorama {

class SkpLoader
{
public:
    SkpLoader(string path, World *world, const ShaderManager &shaders);
    ~SkpLoader();

    // call before loading anything else
    void loadGlobal();
    Component * loadRoot();

private:
    // return null for no mesh
    void loadEntities(SUEntitiesRef entities, Component *component);
    Component * loadInstance(SUComponentInstanceRef instance);
    Mesh * loadMesh(SUEntitiesRef entities);
    Material * loadMaterial(SUMaterialRef suMaterial);
    Texture * loadTexture(SUTextureRef suTexture);

    // utils
    int32_t getID(SUEntityRef entity);
    SUStringRef createString();
    string convertStringAndRelease(SUStringRef *suStr);
    glm::vec4 colorToVec(SUColor color);
    SUResult checkError(SUResult result, int line);

    // convert double to float
    template<typename T>
    void convertVec3Array(T *suVectors, size_t count,
                          vector<glm::vec3> &outVectors)
    {
        for (int i = 0; i < count; i++) {
            T &vec = suVectors[i];
            outVectors.push_back(glm::vec3(
                (float)vec.x, (float)vec.y, (float)vec.z));
        }
    }

    SUModelRef model = SU_INVALID;
    World *world;
    const ShaderManager &shaders;

    // maps file name to texture
    // file name seems to be the only way to identify shared ImageReps, but this
    // can cause conflicts
    unordered_map<string, Texture *> loadedTextures;
    // maps SU material ID to material (not persistent ID!)
    unordered_map<int32_t, Material *> loadedMaterials;
    // maps SU definition ID to component hierarchy
    unordered_map<int32_t, unique_ptr<Component>> componentDefinitions;
    // maps image instance ID to ComponentInstance
    // because we can't cast Image to ComponentInstance for some reason :(
    unordered_map<int32_t, SUComponentInstanceRef> imageInstances;
};

}  // namespace
