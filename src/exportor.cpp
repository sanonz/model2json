#include "exportor.h"

using namespace std;
using namespace rapidjson;


namespace Model2Json
{
    static void WriteTextureBinary(const char* filename, aiTexture* texture)
    {
        auto* data = reinterpret_cast<uint8_t*>(texture->pcData);
        fstream out(filename, ios::out | ios::binary);
        out.write((char*) data, texture->mWidth);
        out.close();
    }

    ComponentType getComponentType(unsigned int a)
    {
        if (a <= COMPONENT_TYPE_BYTE - 1 && a >= -COMPONENT_TYPE_BYTE) {
            return ComponentType_BYTE;
        }
        if (a <= COMPONENT_TYPE_SHORT - 1 && a >= -COMPONENT_TYPE_SHORT) {
            return ComponentType_SHORT;
        }

        return ComponentType_FLOAT;
    }

    ComponentType getComponentUnsignedType(unsigned int a)
    {
        if (a <= COMPONENT_TYPE_UNSIGNED_BYTE) {
            return ComponentType_UNSIGNED_BYTE;
        }
        if (a <= COMPONENT_TYPE_UNSIGNED_SHORT) {
            return ComponentType_UNSIGNED_SHORT;
        }
        if (a <= COMPONENT_TYPE_UNSIGNED_INT) {
            return ComponentType_UNSIGNED_INT;
        }
    }

    float round(float vertex)
    {
        return vertex;
    }

    int FindIndex(Value& object, float searchElement)
    {
        int index = -1;

        for(unsigned int i = 0; i < object.Size(); ++i)
        {
            if(object[i] == searchElement)
            {
                index = i;
                break;
            }
        }

        return index;
    }


    JsonExporter::JsonExporter()
        : mDoc()
        , mAl(mDoc.GetAllocator())
        , mScene(nullptr)
    {
        mDoc.SetObject();

        Value nodes;
        nodes.SetArray();

        Value vertices;
        vertices.SetArray();

        Value textures;
        textures.SetArray();

        Value materials;
        materials.SetArray();

        mDoc.AddMember("nodes", nodes, mAl);
        mDoc.AddMember("vertices", vertices, mAl);
        mDoc.AddMember("textures", textures, mAl);
        mDoc.AddMember("materials", materials, mAl);
    }

    void JsonExporter::ReadFile(const string &path)
    {
        unsigned int flags = aiProcess_Triangulate | aiProcess_JoinIdenticalVertices | aiProcess_FlipUVs;
        Assimp::Importer import;
        // import.SetPropertyBool(AI_CONFIG_IMPORT_FBX_PRESERVE_PIVOTS, false);
        const aiScene* scene = import.ReadFile(path, flags);

        if(!scene || scene->mFlags == AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
        {
            cout << "ERROR::ASSIMP::" << import.GetErrorString () << endl;
        } else {
            mScene = scene;

            WriteNodes();
            WriteTextures();
            WriteMaterials();
        }
    }

    void JsonExporter::WriteNodes()
    {
        WriteNode(-1, mScene->mRootNode);
    }

    void JsonExporter::WriteNode(int parentIndex, aiNode* node)
    {
        Value& nodes = mDoc["nodes"];

        Value object;
        object.SetObject();

        Value children;
        children.SetArray();

        aiVector3D scaling;
        aiVector3D position;
        aiQuaternion rotation;
        node->mTransformation.Decompose(scaling, rotation, position);

        Value name;
        name.SetString(node->mName.C_Str(), node->mName.length, mAl);

        object.AddMember("name", name, mAl);
        object.AddMember("scale", MakeValue(&scaling), mAl);
        object.AddMember("position", MakeValue(&position), mAl);
        object.AddMember("rotation", MakeValue(&rotation), mAl);

        if(parentIndex == -1)
        {
            object.AddMember("parent", kNullType, mAl);
        } else {
            object.AddMember("parent", parentIndex, mAl);
        }

        object.AddMember("children", children, mAl);

        WriteNodeVertices(object, node);

        SizeType index = nodes.Size();
        nodes.PushBack(object, mAl);

        if(parentIndex != -1)
        {
            nodes[parentIndex]["children"].PushBack(index, mAl);
        }

        for(int i = 0; i < node->mNumChildren; ++i)
        {
            WriteNode(int(index), node->mChildren[i]);
        }
    }

    void JsonExporter::WriteNodeVertices(Value& object, aiNode* node)
    {
        if(node->mNumMeshes > 0)
        {
            Value& vertices = mDoc["vertices"];

            Value vertex;
            vertex.SetObject();

            Value index;
            index.SetObject();
            Value indexData;
            indexData.SetArray();

            Value position;
            position.SetObject();
            Value positionData;
            positionData.SetArray();

            Value uv;
            uv.SetObject();
            Value uvData;
            uvData.SetArray();

            Value normal;
            normal.SetObject();
            Value normalData;
            normalData.SetArray();

            Value color;
            color.SetObject();
            Value colorData;
            colorData.SetArray();

            Value materials;
            materials.SetArray();

            Value groups;
            groups.SetArray();

            for(unsigned int i = 0; i < node->mNumMeshes; ++i)
            {
                unsigned int offset = positionData.Size() / 3;
                aiMesh* mesh = mScene->mMeshes[node->mMeshes[i]];

                if(node->mNumMeshes > 1)
                {
                    WriteNodeGroup(groups, indexData.Size(), mesh->mNumFaces * 3, i);
                }

                WriteMeshIndices(indexData, mesh->mNumFaces, mesh->mFaces, offset);
                WriteMeshVertices(positionData, mesh->mNumVertices, mesh->mVertices);

                if(mesh->HasNormals())
                {
                    WriteMeshVertices(normalData, mesh->mNumVertices, mesh->mNormals);
                }

                if(mesh->HasTextureCoords(0))
                {
                    for(unsigned int j = 0; j < mesh->mNumVertices; ++j)
                    {
                        uvData.PushBack(round(mesh->mTextureCoords[0][j].x), mAl);
                        uvData.PushBack(round(mesh->mTextureCoords[0][j].y), mAl);
                    }
                }

                if(mesh->HasVertexColors(0))
                {
                    WriteMeshVertices(colorData, mesh->mNumVertices, mesh->mColors[0]);
                }

                materials.PushBack(mesh->mMaterialIndex, mAl);
            }

            object.AddMember("vertex", vertices.Size(), mAl);

            ComponentType indexType = getComponentUnsignedType(indexData.Size());
            WriteNodeVertex(index, 1, indexType, indexData);
            vertex.AddMember("index", index, mAl);

            WriteNodeVertex(position, 3, ComponentType_FLOAT, positionData);
            vertex.AddMember("position", position, mAl);

            if(!normalData.Empty())
            {
                WriteNodeVertex(normal, 3, ComponentType_FLOAT, normalData);
                vertex.AddMember("normal", normal, mAl);
            }

            if(!uvData.Empty())
            {
                WriteNodeVertex(uv, 2, ComponentType_FLOAT, uvData);
                vertex.AddMember("uv", uv, mAl);
            }

            if(!colorData.Empty())
            {
                WriteNodeVertex(color, 4, ComponentType_UNSIGNED_BYTE, colorData);
                vertex.AddMember("color", color, mAl);
            }

            if(node->mNumMeshes > 1)
            {
                object.AddMember("groups", groups, mAl);
            }

            vertices.PushBack(vertex, mAl);
            object.AddMember("materials", materials, mAl);
        }
    }

    void JsonExporter::WriteNodeVertex(Value& object, unsigned int stride, ComponentType type, Value& data)
    {
        object.AddMember("stride", stride, mAl);
        object.AddMember("type", type, mAl);
        object.AddMember("data", data, mAl);
    }

    void JsonExporter::WriteNodeGroup(Value& object, unsigned int start, unsigned int count, unsigned int materialIndex)
    {
        Value group;
        group.SetArray();

        group.PushBack(start, mAl);
        group.PushBack(count, mAl);
        group.PushBack(materialIndex, mAl);

        object.PushBack(group, mAl);
    }

    void JsonExporter::WriteMeshIndices(Value& object, unsigned int count, aiFace* faces, unsigned int offset)
    {
        for(unsigned int i = 0; i < count; ++i)
        {
            aiFace& face = faces[i];

            for(unsigned int j = 0; j < face.mNumIndices; ++j)
            {
                object.PushBack(face.mIndices[j] + offset, mAl);
            }
        }
    }

    void JsonExporter::WriteMeshVertices(Value& object, unsigned int count, aiVector3D* vectors)
    {
        for(unsigned int i = 0; i < count; ++i)
        {
            object.PushBack(round(vectors[i].x), mAl);
            object.PushBack(round(vectors[i].y), mAl);
            object.PushBack(round(vectors[i].z), mAl);
        }
    }

    void JsonExporter::WriteMeshVertices(Value& object, unsigned int count, aiColor4D* colors)
    {
        for(unsigned int i = 0; i < count; ++i)
        {
            object.PushBack(colors[i].r, mAl);
            object.PushBack(colors[i].g, mAl);
            object.PushBack(colors[i].b, mAl);
            object.PushBack(colors[i].a, mAl);
        }
    }

    void JsonExporter::WriteTextures()
    {
        Value& textures = mDoc["textures"];

        for(int i = 0; i < mScene->mNumTextures; ++i)
        {
            aiTexture* tex = mScene->mTextures[i];

            const char* filename = mScene->GetShortFilename(tex->mFilename.C_Str());

            Value texture;
            texture.SetObject();

            Value file;
            file.SetObject();

            Value data;
            data.SetObject();

            file.AddMember("width", tex->mWidth, mAl);
            file.AddMember("height", tex->mHeight, mAl);
            file.AddMember("size", tex->mWidth, mAl);
            file.AddMember("url", StringRef(filename), mAl);

            data.AddMember("flipY", kTrueType, mAl);
            data.AddMember("minFilter", 1008, mAl);
            data.AddMember("magFilter", 1006, mAl);

            texture.AddMember("name", StringRef(filename), mAl);
            texture.AddMember("file", file, mAl);
            texture.AddMember("data", data, mAl);

            textures.PushBack(texture, mAl);

            WriteTextureBinary(filename, tex);
        }
    }

    void JsonExporter::WriteMaterials()
    {
        Value& materials = mDoc["materials"];

        for(int i = 0; i < mScene->mNumMaterials; ++i)
        {
            Value material;
            material.SetObject();

            Value data;
            data.SetObject();

            aiMaterial* m = mScene->mMaterials[i];

            aiString s;
            if(AI_SUCCESS == m->Get(AI_MATKEY_NAME, s))
            {
                Value name;
                name.SetString(s.C_Str(), s.length, mAl);
                material.AddMember("name", name, mAl);
            }
            if(AI_SUCCESS == m->Get(AI_MATKEY_TEXTURE_DIFFUSE(0), s))
            {
                Value diffuseMap;
                diffuseMap.SetString(mScene->GetShortFilename(s.data), mAl);
                data.AddMember("diffuseMap", diffuseMap, mAl);
            }
            if(AI_SUCCESS == m->Get(AI_MATKEY_TEXTURE_AMBIENT(0), s))
            {
                Value ambientMap;
                ambientMap.SetString(mScene->GetShortFilename(s.data), mAl);
                data.AddMember("diffuseMap", ambientMap, mAl);
            }
            if(AI_SUCCESS == m->Get(AI_MATKEY_TEXTURE_SPECULAR(0), s))
            {
                Value specularMap;
                specularMap.SetString(mScene->GetShortFilename(s.data), mAl);
                data.AddMember("diffuseMap", specularMap, mAl);
            }
            if(AI_SUCCESS == m->Get(AI_MATKEY_TEXTURE_SHININESS(0), s))
            {
                Value shininessMap;
                shininessMap.SetString(mScene->GetShortFilename(s.data), mAl);
                data.AddMember("diffuseMap", shininessMap, mAl);
            }
            if(AI_SUCCESS == m->Get(AI_MATKEY_TEXTURE_OPACITY(0), s))
            {
                Value opacityMap;
                opacityMap.SetString(mScene->GetShortFilename(s.data), mAl);
                data.AddMember("diffuseMap", opacityMap, mAl);
            }
            if(AI_SUCCESS == m->Get(AI_MATKEY_TEXTURE_HEIGHT(0), s))
            {
                Value opacityMap;
                opacityMap.SetString(mScene->GetShortFilename(s.data), mAl);
                data.AddMember("diffuseMap", opacityMap, mAl);
            }
            if(AI_SUCCESS == m->Get(AI_MATKEY_TEXTURE_NORMALS(0), s))
            {
                Value opacityMap;
                opacityMap.SetString(mScene->GetShortFilename(s.data), mAl);
                data.AddMember("diffuseMap", opacityMap, mAl);
            }

            aiColor4D c;
            if(AI_SUCCESS == m->Get(AI_MATKEY_COLOR_DIFFUSE, c))
            {
                data.AddMember("diffuse", MakeValue(&c), mAl);
            }
            if(AI_SUCCESS == m->Get(AI_MATKEY_COLOR_AMBIENT, c))
            {
                data.AddMember("ambient", MakeValue(&c), mAl);
            }
            if(AI_SUCCESS == m->Get(AI_MATKEY_COLOR_SPECULAR, c))
            {
                data.AddMember("specular", MakeValue(&c), mAl);
            }
            if(AI_SUCCESS == m->Get(AI_MATKEY_COLOR_EMISSIVE, c))
            {
                data.AddMember("emissive", MakeValue(&c), mAl);
            }
            if(AI_SUCCESS == m->Get(AI_MATKEY_COLOR_TRANSPARENT, c))
            {
                data.AddMember("transparent", MakeValue(&c), mAl);
            }

            ai_real o;
            if(AI_SUCCESS == m->Get(AI_MATKEY_OPACITY, o))
            {
                data.AddMember("opacity", o, mAl);
            }
            if(AI_SUCCESS == m->Get(AI_MATKEY_REFRACTI, o))
            {
                data.AddMember("refraction", o, mAl);
            }
            if(AI_SUCCESS == m->Get(AI_MATKEY_SHININESS, o))
            {
                data.AddMember("shininess", o, mAl);
            }

            material.AddMember("data", data, mAl);
            materials.PushBack(material, mAl);
        }
    }

    void JsonExporter::WriteAnimations()
    {
        for(int i = 0; i < mScene->mNumAnimations; ++i)
        {
            aiAnimation* animation = mScene->mAnimations[i];
        }
    }

    Value JsonExporter::MakeValue(const aiVector3D* v)
    {
        Value array;
        array.SetArray();

        array.PushBack(v->x, mAl);
        array.PushBack(v->y, mAl);
        array.PushBack(v->z, mAl);

        return array;
    }

    Value JsonExporter::MakeValue(const aiQuaternion* q)
    {
        Value array;
        array.SetArray();

        array.PushBack(q->x, mAl);
        array.PushBack(q->y, mAl);
        array.PushBack(q->z, mAl);
        array.PushBack(q->w, mAl);

        return array;
    }

    Value JsonExporter::MakeValue(aiColor4D* c)
    {
        Value color;
        color.SetArray();

        color.PushBack(c->r, mAl);
        color.PushBack(c->g, mAl);
        color.PushBack(c->b, mAl);

        return color;
    }

    void JsonExporter::Save(const string& path)
    {
        StringBuffer buffer;
        Writer<StringBuffer> writer(buffer);
        writer.SetMaxDecimalPlaces(6);
        // PrettyWriter<StringBuffer> writer(buffer);
        // writer.SetIndent(' ', 2);
        mDoc.Accept(writer);
        string json = buffer.GetString();

        fstream outBin(path.c_str(), ios::out | ios::ate);
        outBin.write(json.c_str(), json.size());
        outBin.close();
    }

    JsonExporter::~JsonExporter()
    {

    }
}
