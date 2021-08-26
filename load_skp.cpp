#include "load_skp.h"
#include <cstdio>
#include <exception>
#include <map>
#include <GL/gl3w.h>
#include <glm/gtc/type_ptr.hpp>

#define CHECK(op) (checkError((op), __LINE__))

namespace diorama {

const int32_t NO_ID = -1;


struct PrimitiveBuilder {
    vector<glm::vec3> vertices, stqCoords, normals;
    vector<MeshIndex> indices;
};

SkpLoader::SkpLoader(string path, World &world, ShaderManager &shaders)
    : world(world)
    , shaders(shaders)
{
    printf("Loading from %s\n", path.c_str());
    SUInitialize();

    if (CHECK(SUModelCreateFromFile(&model, path.c_str())))
        throw std::exception("Couldn't create model");
}

SkpLoader::~SkpLoader()
{
    CHECK(SUModelRelease(&model));
    // TODO keep initialized?
    SUTerminate();
}

void SkpLoader::loadGlobal()
{
    glActiveTexture(GL_TEXTURE0);  // for creating textures

    size_t numMaterials;
    // get "All" materials to include Images (also Layers which are unused)
    CHECK(SUModelGetNumAllMaterials(model, &numMaterials));
    unique_ptr<SUMaterialRef[]> materials(new SUMaterialRef[numMaterials]);
    CHECK(SUModelGetAllMaterials(model, numMaterials,
        materials.get(), &numMaterials));
    for (int i = 0; i < numMaterials; i++) {
        int32_t id = getID(SUMaterialToEntity(materials[i]));
        loadedMaterials[id] = loadMaterial(materials[i]);
    }
    glBindTexture(GL_TEXTURE_2D, 0);

    // IDs seem to follow dependency order, so load definitions in order of ID
    std::map<int32_t, SUComponentDefinitionRef> defs;

    size_t numCompDefs;
    CHECK(SUModelGetNumComponentDefinitions(model, &numCompDefs));
    unique_ptr<SUComponentDefinitionRef[]> compDefs(
        new SUComponentDefinitionRef[numCompDefs]);
    CHECK(SUModelGetComponentDefinitions(model, numCompDefs,
        compDefs.get(), &numCompDefs));
    for (int i = 0; i < numCompDefs; i++)
        defs[getID(SUComponentDefinitionToEntity(compDefs[i]))] = compDefs[i];
    
    // groups can have shared definitions!
    // they are automatically made unique when edited.
    size_t numGroupDefs;
    CHECK(SUModelGetNumGroupDefinitions(model, &numGroupDefs));
    unique_ptr<SUComponentDefinitionRef[]> groupDefs(
        new SUComponentDefinitionRef[numGroupDefs]);
    CHECK(SUModelGetGroupDefinitions(model, numGroupDefs,
        groupDefs.get(), &numGroupDefs));
    for (int i = 0; i < numGroupDefs; i++)
        defs[getID(SUComponentDefinitionToEntity(groupDefs[i]))] = groupDefs[i];

    size_t numImageDefs;
    CHECK(SUModelGetNumImageDefinitions(model, &numImageDefs));
    unique_ptr<SUComponentDefinitionRef[]> imageDefs(
        new SUComponentDefinitionRef[numImageDefs]);
    CHECK(SUModelGetImageDefinitions(model, numImageDefs,
        imageDefs.get(), &numImageDefs));
    for (int i = 0; i < numImageDefs; i++) {
        SUComponentDefinitionRef imageDef = imageDefs[i];
        defs[getID(SUComponentDefinitionToEntity(imageDef))] = imageDef;
        // :(  see imageInstances
        size_t numInstances;
        CHECK(SUComponentDefinitionGetNumInstances(imageDef, &numInstances));
        unique_ptr<SUComponentInstanceRef[]> instances(
            new SUComponentInstanceRef[numInstances]);
        CHECK(SUComponentDefinitionGetInstances(imageDef, numInstances,
            instances.get(), &numInstances));
        for (int j = 0; j < numInstances; j++) {
            imageInstances[getID(SUComponentInstanceToEntity(instances[j]))]
                = instances[j];
        }
    }

    for (auto &defPair : defs) {
        Component * component = new Component;

        SUStringRef nameStr = createString();
        CHECK(SUComponentDefinitionGetName(defPair.second, &nameStr));
        component->name = convertStringAndRelease(nameStr);
        printf("Definition %d: %s\n",
            defPair.first, component->name.c_str());

        SUEntitiesRef entities = SU_INVALID;
        CHECK(SUComponentDefinitionGetEntities(defPair.second, &entities));
        // component definitions don't seem to use materials
        loadEntities(entities, *component);
        int32_t id = getID(SUComponentDefinitionToEntity(defPair.second));
        componentDefinitions[id] = unique_ptr<Component>(component);
    }

    glBindVertexArray(0);
}

Component * SkpLoader::loadRoot()
{
    Component *root = new Component;
    root->name = "root";
    SUEntitiesRef entities = SU_INVALID;
    CHECK(SUModelGetEntities(model, &entities));
    printf("Model:\n");
    loadEntities(entities, *root);
    glBindVertexArray(0);
    return root;
}

void SkpLoader::loadEntities(SUEntitiesRef entities, Component &component)
{
    component.mesh = loadMesh(entities);

    size_t numGroups;
    CHECK(SUEntitiesGetNumGroups(entities, &numGroups));
    unique_ptr<SUGroupRef[]> groups(new SUGroupRef[numGroups]);
    CHECK(SUEntitiesGetGroups(entities, numGroups, groups.get(), &numGroups));
    for (int i = 0; i < numGroups; i++) {
        SUComponentInstanceRef instance = SUGroupToComponentInstance(groups[i]);
        loadInstance(instance)->setParent(&component);
    }

    size_t numInstances;
    CHECK(SUEntitiesGetNumInstances(entities, &numInstances));
    unique_ptr<SUComponentInstanceRef[]> instances(
        new SUComponentInstanceRef[numInstances]);
    CHECK(SUEntitiesGetInstances(entities, numInstances,
        instances.get(), &numInstances));
    for (int i = 0; i < numInstances; i++) {
        SUComponentInstanceRef instance = instances[i];
        loadInstance(instance)->setParent(&component);
    }

    size_t numImages;
    CHECK(SUEntitiesGetNumImages(entities, &numImages));
    unique_ptr<SUImageRef[]> images(new SUImageRef[numImages]);
    CHECK(SUEntitiesGetImages(entities, numImages, images.get(), &numImages));
    for (int i = 0; i < numImages; i++) {
        // :(  see imageInstances
        int32_t imageID = getID(SUImageToEntity(images[i]));
        auto instIt = imageInstances.find(imageID);
        if (instIt == imageInstances.end()) {
            printf("  Image instance %d not found!\n", imageID);
            continue;
        }
        loadInstance(instIt->second)->setParent(&component);
    }
}

Component * SkpLoader::loadInstance(SUComponentInstanceRef instance)
{
    SUComponentDefinitionRef definition = SU_INVALID;
    CHECK(SUComponentInstanceGetDefinition(instance, &definition));
    int32_t definitionID = getID(SUComponentDefinitionToEntity(definition));
    auto defIt = componentDefinitions.find(definitionID);
    if (defIt == componentDefinitions.end()) {
        printf("  Definition %d not loaded!\n", definitionID);
        return new Component;
    }

    Component *component = defIt->second->cloneHierarchy();

    SUStringRef nameStr = createString();
    CHECK(SUComponentInstanceGetName(instance, &nameStr));
    string name = convertStringAndRelease(nameStr);
    printf("  Instance %s of %d\n", name.c_str(),
        definitionID);
    // instances with empty names use the name of their definition
    // this matches the behavior of Dynamic Components
    if (!name.empty())
        component->name = name;

    SUTransformation suTransform;
    CHECK(SUComponentInstanceGetTransform(instance, &suTransform));
    // both glm and sketchup use column-major order
    component->tLocalMut() = Transform(glm::make_mat4(suTransform.values));

    SUDrawingElementRef element = SUComponentInstanceToDrawingElement(instance);
    SUMaterialRef material = SU_INVALID;
    if (!SUDrawingElementGetMaterial(element, &material)) {
        int32_t materialID = getID(SUMaterialToEntity(material));
        auto matIt = loadedMaterials.find(materialID);
        if (matIt != loadedMaterials.end()) {
            component->material = matIt->second;
        } else {
            printf("    Material %d not loaded!\n", materialID);
        }
    }  // otherwise leave the component's current material

    return component;
}

Mesh * SkpLoader::loadMesh(SUEntitiesRef entities)
{
    size_t numFaces;
    CHECK(SUEntitiesGetNumFaces(entities, &numFaces));
    if (numFaces == 0)
        return nullptr;
    unique_ptr<SUFaceRef[]> faces(new SUFaceRef[numFaces]);
    CHECK(SUEntitiesGetFaces(entities, numFaces, faces.get(), &numFaces));

    Mesh * mesh = new Mesh;
    world.addResource(mesh);

    // maps material ID to builder
    unordered_map<int32_t, PrimitiveBuilder> materialPrimitives;
    mesh->collision.emplace_back();
    CollisionPrimitive &collision = mesh->collision.back();

    for (int i = 0; i < numFaces; i++) {
        SUMeshHelperRef helper = SU_INVALID;
        CHECK(SUMeshHelperCreate(&helper, faces[i]));

        size_t numVertices;
        CHECK(SUMeshHelperGetNumVertices(helper, &numVertices));

        unique_ptr<SUPoint3D[]> suVertices(new SUPoint3D[numVertices]);
        CHECK(SUMeshHelperGetVertices(helper, numVertices,
            suVertices.get(), &numVertices));
        unique_ptr<SUPoint3D[]> suSTQCoords(new SUPoint3D[numVertices]);
        CHECK(SUMeshHelperGetFrontSTQCoords(helper, numVertices,
            suSTQCoords.get(), &numVertices));
        unique_ptr<SUVector3D[]> suNormals(new SUVector3D[numVertices]);
        CHECK(SUMeshHelperGetNormals(helper, numVertices,  
            suNormals.get(), &numVertices));

        size_t numTriangles;
        CHECK(SUMeshHelperGetNumTriangles(helper, &numTriangles));
        size_t numIndices = numTriangles * 3;
        unique_ptr<size_t[]> suIndices(new size_t[numIndices]);
        CHECK(SUMeshHelperGetVertexIndices(
            helper, numIndices, suIndices.get(), &numIndices));

        CHECK(SUMeshHelperRelease(&helper));


        SUMaterialRef material = SU_INVALID;
        int32_t materialID;
        if (!SUFaceGetFrontMaterial(faces[i], &material))
            materialID = getID(SUMaterialToEntity(material));
        else
            materialID = NO_ID;
        
        // constructs if doesn't exist
        PrimitiveBuilder &build = materialPrimitives[materialID];
        MeshIndex renderOffset = build.vertices.size();
        convertVec3Array(suVertices.get(), numVertices, build.vertices);
        convertVec3Array(suSTQCoords.get(), numVertices, build.stqCoords);
        convertVec3Array(suNormals.get(), numVertices, build.normals);

        MeshIndex collisionOffset = collision.vertices.size();
        convertVec3Array(suVertices.get(), numVertices, collision.vertices);

        for (int i = 0; i < numIndices; i++) {
            build.indices.push_back((MeshIndex)suIndices[i] + renderOffset);
            collision.indices.push_back(
                (MeshIndex)suIndices[i] + collisionOffset);
        }
    }  // for each face

    for (auto &primPair : materialPrimitives) {
        int32_t materialID = primPair.first;
        PrimitiveBuilder &build = primPair.second;

        mesh->render.emplace_back();
        RenderPrimitive &primitive = mesh->render.back();

        if (materialID != NO_ID)
        {
            auto matIt = loadedMaterials.find(materialID);
            if (matIt != loadedMaterials.end()) {
                primitive.material = matIt->second;
            } else {
                printf("  Material %d not loaded!\n", materialID);
            }
        }

        glBindVertexArray(primitive.vertexArray);

        size_t vertexBufferSize = build.vertices.size() * sizeof(glm::vec3);
        glBindBuffer(GL_ARRAY_BUFFER,
                     primitive.attribBuffers[RenderPrimitive::ATTRIB_POSITION]);
        glBufferData(GL_ARRAY_BUFFER, vertexBufferSize,
                     &build.vertices[0], GL_STATIC_DRAW);
        glVertexAttribPointer(RenderPrimitive::ATTRIB_POSITION, 3, GL_FLOAT,
                              GL_FALSE, 0, (void *)0);

        glBindBuffer(GL_ARRAY_BUFFER,
                     primitive.attribBuffers[RenderPrimitive::ATTRIB_NORMAL]);
        glBufferData(GL_ARRAY_BUFFER, vertexBufferSize,
                     &build.normals[0], GL_STATIC_DRAW);
        glVertexAttribPointer(RenderPrimitive::ATTRIB_NORMAL, 3, GL_FLOAT,
                              GL_FALSE, 0, (void *)0);

        glBindBuffer(GL_ARRAY_BUFFER,
                     primitive.attribBuffers[RenderPrimitive::ATTRIB_STQ]);
        glBufferData(GL_ARRAY_BUFFER, vertexBufferSize,
                     &build.stqCoords[0], GL_STATIC_DRAW);
        glVertexAttribPointer(RenderPrimitive::ATTRIB_STQ, 3, GL_FLOAT,
                              GL_FALSE, 0, (void *)0);

        for (int i = 0; i < RenderPrimitive::ATTRIB_MAX; i++)
            glEnableVertexAttribArray(i);

        // binding is stored in VAO
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, primitive.elementBuffer);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                     build.indices.size() * sizeof(MeshIndex),
                     &build.indices[0], GL_STATIC_DRAW);
        primitive.numIndices = build.indices.size();
    }  // for each material primitive

    printf("  %zu primitives\n", mesh->render.size());
    return mesh;
}

Material * SkpLoader::loadMaterial(SUMaterialRef suMaterial)
{
    SUStringRef nameStr = createString();
    CHECK(SUMaterialGetName(suMaterial, &nameStr));
    string name = convertStringAndRelease(nameStr);
    printf("Material %d: %s\n", getID(SUMaterialToEntity(suMaterial)),
        name.c_str());

    Material * material = new Material;
    world.addResource(material);

    CHECK(SUMaterialIsDrawnTransparent(suMaterial, &material->transparent));
    if (material->transparent)
        printf("  Material is transparent\n");

    SUMaterialType type;
    CHECK(SUMaterialGetType(suMaterial, &type));

    if (type == SUMaterialType_Colored) {
        SUColor color;
        CHECK(SUMaterialGetColor(suMaterial, &color));
        material->shader = &shaders.coloredProg;
        material->color = colorToVec(color);
        material->texture = &Texture::NO_TEXTURE;
        printf("  Solid color %f %f %f %f\n",
            material->color.r, material->color.g, material->color.b,
            material->color.a);
    } else {
        SUTextureRef texture = SU_INVALID;
        CHECK(SUMaterialGetTexture(suMaterial, &texture));
        material->texture = loadTexture(texture);

        size_t width, height;  // ignored
        double sScale, tScale;
        CHECK(SUTextureGetDimensions(texture, &width, &height,
            &sScale, &tScale));
        material->scale = glm::vec2(sScale, tScale);

        double alpha;
        CHECK(SUMaterialGetOpacity(suMaterial, &alpha));

        if (type == SUMaterialType_ColorizedTexture) {
            SUMaterialColorizeType type;
            CHECK(SUMaterialGetColorizeType(suMaterial, &type));
            if (type == SUMaterialColorizeType_Tint) {
                printf("  Colorize (tint)\n");
                material->shader = &shaders.tintedTextureProg;
            } else if (type == SUMaterialColorizeType_Shift) {
                printf("  Colorize (shift)\n");
                material->shader = &shaders.shiftedTextureProg;
            }

            double hue, sat, light;
            CHECK(SUMaterialGetColorizeDeltas(suMaterial, &hue, &sat, &light));
            material->color = glm::vec4(hue, sat, light, alpha);
        } else {
            material->shader = &shaders.texturedProg;
            material->color = glm::vec4(1,1,1,alpha);
        }
    }

    return material;
}


Texture * SkpLoader::loadTexture(SUTextureRef suTexture)
{
    SUStringRef fileNameStr = createString();
    CHECK(SUTextureGetFileName(suTexture, &fileNameStr));
    string fileName = convertStringAndRelease(fileNameStr);

    auto texIt = loadedTextures.find(fileName);
    if (texIt != loadedTextures.end()) {
        printf("  Already loaded %s\n", fileName.c_str());
        return texIt->second;
    }
    printf("  Texture %s\n", fileName.c_str());
    
    SUImageRepRef image = SU_INVALID;
    CHECK(SUImageRepCreate(&image));  // uncolorized
    CHECK(SUTextureGetImageRep(suTexture, &image));

    size_t width, height;
    CHECK(SUImageRepGetPixelDimensions(image, &width, &height));

    unique_ptr<SUColor[]> colors(new SUColor[width * height]);
    // this is actually much faster than SUImageRepConvertTo32BitsPerPixel
    CHECK(SUImageRepGetDataAsColors(image, colors.get()));

    CHECK(SUImageRepRelease(&image));

    Texture * texture(new Texture);
    world.addResource(texture);

    glBindTexture(GL_TEXTURE_2D, texture->glTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA,
                 width, height, 0,
                 GL_RGBA, GL_UNSIGNED_BYTE, colors.get());
    glGenerateMipmap(GL_TEXTURE_2D);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
                    GL_LINEAR_MIPMAP_LINEAR);  // trilinear

    loadedTextures[fileName] = texture;
    return texture;
}


int32_t SkpLoader::getID(SUEntityRef entity)
{
    int32_t id = 0;
    CHECK(SUEntityGetID(entity, &id));
    return id;
}

SUStringRef SkpLoader::createString()
{
    SUStringRef str = SU_INVALID;
    CHECK(SUStringCreate(&str));
    return str;
}

string SkpLoader::convertStringAndRelease(SUStringRef &suStr)
{
    size_t len = 0;
    CHECK(SUStringGetUTF8Length(suStr, &len));
    unique_ptr<char[]> utf8(new char[len + 1]);
    CHECK(SUStringGetUTF8(suStr, len + 1, utf8.get(), &len));
    CHECK(SUStringRelease(&suStr));
    string stdStr(utf8.get());
    return stdStr;
}

glm::vec4 SkpLoader::colorToVec(SUColor color)
{
    return glm::vec4(color.red / 255.0,
                     color.green / 255.0,
                     color.blue / 255.0,
                     color.alpha / 255.0);
}

SUResult SkpLoader::checkError(SUResult result, int line)
{
    if (!result)
        return result;
    printf("SU Error line %d: ", line);
    switch (result) {
        case SU_ERROR_NULL_POINTER_INPUT:
            printf("Null pointer input\n");     break;
        case SU_ERROR_INVALID_INPUT:
            printf("Invalid input\n");          break;
        case SU_ERROR_NULL_POINTER_OUTPUT:
            printf("Null pointer output\n");    break;
        case SU_ERROR_INVALID_OUTPUT:
            printf("Invalid output\n");         break;
        case SU_ERROR_OVERWRITE_VALID:
            printf("Overwrite valid\n");        break;
        case SU_ERROR_GENERIC:
            printf("Generic\n");                break;
        case SU_ERROR_SERIALIZATION:
            printf("Serialization\n");          break;
        case SU_ERROR_OUT_OF_RANGE:
            printf("Out of range\n");           break;
        case SU_ERROR_NO_DATA:
            printf("No data\n");                break;
        case SU_ERROR_INSUFFICIENT_SIZE:
            printf("Insufficient size\n");      break;
        case SU_ERROR_UNKNOWN_EXCEPTION:
            printf("Unknown exception\n");      break;
        case SU_ERROR_MODEL_INVALID:
            printf("Invalid\n");                break;
        case SU_ERROR_MODEL_VERSION:
            printf("Version\n");                break;
        case SU_ERROR_LAYER_LOCKED:
            printf("Locked\n");                 break;
        case SU_ERROR_DUPLICATE:
            printf("Duplicate\n");              break;
        case SU_ERROR_PARTIAL_SUCCESS:
            printf("Partial success\n");        break;
        case SU_ERROR_UNSUPPORTED:
            printf("Unsupported\n");            break;
        case SU_ERROR_INVALID_ARGUMENT:
            printf("Invalid argument\n");       break;
        case SU_ERROR_ENTITY_LOCKED:
            printf("Locked\n");                 break;
        case SU_ERROR_INVALID_OPERATION:
            printf("Invalid operation\n");      break;
        default:
            printf("Unknown\n");                break;
    }
    return result;
}

}  // namespace
