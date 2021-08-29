#include "load_skp.h"
#include <exception>
#include <map>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/string_cast.hpp>

#define CHECK(op) (checkError((op), __LINE__))

namespace diorama {

const int32_t NO_ID = -1;


struct PrimitiveBuilder {
    vector<glm::vec3> vertices, stqCoords, normals;
    vector<MeshIndex> indices;
};

SkpLoader::SkpLoader(string path, World *world, const ShaderManager *shaders)
    : world(world)
    , shaders(shaders)
{
    cout << "Loading from " <<path<< "\n";
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
        component->name = convertStringAndRelease(&nameStr);
        cout << "Definition " <<defPair.first<< ": " <<component->name<< "\n";

        SUEntitiesRef entities = SU_INVALID;
        CHECK(SUComponentDefinitionGetEntities(defPair.second, &entities));
        // component definitions don't seem to use materials
        loadEntities(entities, component);
        int32_t id = getID(SUComponentDefinitionToEntity(defPair.second));
        componentDefinitions[id] = unique_ptr<Component>(component);
    }
}

Component * SkpLoader::loadRoot()
{
    Component *root = new Component;
    root->name = "root";
    SUEntitiesRef entities = SU_INVALID;
    CHECK(SUModelGetEntities(model, &entities));
    cout << "Model:\n";
    loadEntities(entities, root);
    return root;
}

void SkpLoader::loadEntities(SUEntitiesRef entities, Component *component)
{
    component->mesh = loadMesh(entities);

    size_t numGroups;
    CHECK(SUEntitiesGetNumGroups(entities, &numGroups));
    unique_ptr<SUGroupRef[]> groups(new SUGroupRef[numGroups]);
    CHECK(SUEntitiesGetGroups(entities, numGroups, groups.get(), &numGroups));
    for (int i = 0; i < numGroups; i++) {
        SUComponentInstanceRef instance = SUGroupToComponentInstance(groups[i]);
        loadInstance(instance)->setParent(component);
    }

    size_t numInstances;
    CHECK(SUEntitiesGetNumInstances(entities, &numInstances));
    unique_ptr<SUComponentInstanceRef[]> instances(
        new SUComponentInstanceRef[numInstances]);
    CHECK(SUEntitiesGetInstances(entities, numInstances,
        instances.get(), &numInstances));
    for (int i = 0; i < numInstances; i++) {
        SUComponentInstanceRef instance = instances[i];
        loadInstance(instance)->setParent(component);
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
            cout << "  Image instance " <<imageID<< " not found!\n";
            continue;
        }
        loadInstance(instIt->second)->setParent(component);
    }
}

Component * SkpLoader::loadInstance(SUComponentInstanceRef instance)
{
    SUComponentDefinitionRef definition = SU_INVALID;
    CHECK(SUComponentInstanceGetDefinition(instance, &definition));
    int32_t definitionID = getID(SUComponentDefinitionToEntity(definition));
    auto defIt = componentDefinitions.find(definitionID);
    if (defIt == componentDefinitions.end()) {
        cout << "  Definition " <<definitionID<< " not loaded!\n";
        return new Component;
    }

    Component *component = defIt->second->cloneHierarchy();

    SUStringRef nameStr = createString();
    CHECK(SUComponentInstanceGetName(instance, &nameStr));
    string name = convertStringAndRelease(&nameStr);
    cout << "  Instance " <<name<< " of " <<definitionID<< "\n";
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
            cout << "    Material " <<materialID<< " not loaded!\n";
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
    world->addResource(mesh);

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
                cout << "  Material " <<materialID<< " not loaded!\n";
            }
        }

        size_t vertexBufferSize = build.vertices.size() * sizeof(glm::vec3);
        primitive.setAttribData(RenderPrimitive::ATTRIB_POSITION,
            vertexBufferSize, 3, GLDataType::Float, &build.vertices[0]);
        primitive.setAttribData(RenderPrimitive::ATTRIB_NORMAL,
            vertexBufferSize, 3, GLDataType::Float, &build.normals[0]);
        primitive.setAttribData(RenderPrimitive::ATTRIB_STQ,
            vertexBufferSize, 3, GLDataType::Float, &build.stqCoords[0]);
        primitive.setIndices(build.indices.size(), &build.indices[0]);
    }

    cout << "  " <<mesh->render.size()<< " primitives\n";
    return mesh;
}

Material * SkpLoader::loadMaterial(SUMaterialRef suMaterial)
{
    SUStringRef nameStr = createString();
    CHECK(SUMaterialGetName(suMaterial, &nameStr));
    string name = convertStringAndRelease(&nameStr);
    cout << "Material " <<getID(SUMaterialToEntity(suMaterial))<< ": "
        <<name<< "\n";

    Material * material = new Material;
    world->addResource(material);

    bool transparent;
    CHECK(SUMaterialIsDrawnTransparent(suMaterial, &transparent));
    if (transparent)
        cout << "  Material is transparent\n";
    material->order = transparent ? RenderOrder::Transparent
        : RenderOrder::Opaque;

    SUMaterialType type;
    CHECK(SUMaterialGetType(suMaterial, &type));

    if (type == SUMaterialType_Colored) {
        SUColor color;
        CHECK(SUMaterialGetColor(suMaterial, &color));
        material->shader = &shaders->coloredProg;
        material->color = colorToVec(color);
        material->texture = &Texture::NO_TEXTURE;
        cout << "  Solid color " <<glm::to_string(material->color)<< "\n";
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
                cout << "  Colorize (tint)\n";
                material->shader = &shaders->tintedTextureProg;
            } else if (type == SUMaterialColorizeType_Shift) {
                cout << "  Colorize (shift)\n";
                material->shader = &shaders->shiftedTextureProg;
            }

            double hue, sat, light;
            CHECK(SUMaterialGetColorizeDeltas(suMaterial, &hue, &sat, &light));
            material->color = glm::vec4(hue, sat, light, alpha);
        } else {
            material->shader = &shaders->texturedProg;
            material->color = glm::vec4(1,1,1,alpha);
        }
    }

    return material;
}


Texture * SkpLoader::loadTexture(SUTextureRef suTexture)
{
    SUStringRef fileNameStr = createString();
    CHECK(SUTextureGetFileName(suTexture, &fileNameStr));
    string fileName = convertStringAndRelease(&fileNameStr);

    auto texIt = loadedTextures.find(fileName);
    if (texIt != loadedTextures.end()) {
        cout << "  Already loaded " <<fileName<< "\n";
        return texIt->second;
    }
    cout << "  Texture " <<fileName<< "\n";

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
    world->addResource(texture);
    texture->setImage(width, height, GLTextureFormat::Rgba,
                      GLDataType::UnsignedByte, colors.get());

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

string SkpLoader::convertStringAndRelease(SUStringRef *suStr)
{
    size_t len = 0;
    CHECK(SUStringGetUTF8Length(*suStr, &len));
    unique_ptr<char[]> utf8(new char[len + 1]);
    CHECK(SUStringGetUTF8(*suStr, len + 1, utf8.get(), &len));
    CHECK(SUStringRelease(suStr));
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
    cout << "SU Error line %d: ", line;
    switch (result) {
        case SU_ERROR_NULL_POINTER_INPUT:
            cout << "Null pointer input\n";     break;
        case SU_ERROR_INVALID_INPUT:
            cout << "Invalid input\n";          break;
        case SU_ERROR_NULL_POINTER_OUTPUT:
            cout << "Null pointer output\n";    break;
        case SU_ERROR_INVALID_OUTPUT:
            cout << "Invalid output\n";         break;
        case SU_ERROR_OVERWRITE_VALID:
            cout << "Overwrite valid\n";        break;
        case SU_ERROR_GENERIC:
            cout << "Generic\n";                break;
        case SU_ERROR_SERIALIZATION:
            cout << "Serialization\n";          break;
        case SU_ERROR_OUT_OF_RANGE:
            cout << "Out of range\n";           break;
        case SU_ERROR_NO_DATA:
            cout << "No data\n";                break;
        case SU_ERROR_INSUFFICIENT_SIZE:
            cout << "Insufficient size\n";      break;
        case SU_ERROR_UNKNOWN_EXCEPTION:
            cout << "Unknown exception\n";      break;
        case SU_ERROR_MODEL_INVALID:
            cout << "Invalid\n";                break;
        case SU_ERROR_MODEL_VERSION:
            cout << "Version\n";                break;
        case SU_ERROR_LAYER_LOCKED:
            cout << "Locked\n";                 break;
        case SU_ERROR_DUPLICATE:
            cout << "Duplicate\n";              break;
        case SU_ERROR_PARTIAL_SUCCESS:
            cout << "Partial success\n";        break;
        case SU_ERROR_UNSUPPORTED:
            cout << "Unsupported\n";            break;
        case SU_ERROR_INVALID_ARGUMENT:
            cout << "Invalid argument\n";       break;
        case SU_ERROR_ENTITY_LOCKED:
            cout << "Locked\n";                 break;
        case SU_ERROR_INVALID_OPERATION:
            cout << "Invalid operation\n";      break;
        default:
            cout << "Unknown\n";                break;
    }
    return result;
}

}  // namespace
