#include "exportor.hpp"

using namespace std;
using namespace rapidjson;


namespace Model2Json
{

    static void ParserTextureName(const char* str, string& filename)
    {
        filename = str;
        int pos = filename.rfind("\\");
        if (pos == -1)
        {
            pos = filename.rfind("/");
        }
        filename.replace(0, pos + 1, "");
    }

    static void WriteTextureBinary(aiTexture* texture)
    {
        string filename;
        ParserTextureName(texture->mFilename.C_Str(), filename);
        auto* data = reinterpret_cast<uint8_t*>(texture->pcData);
        fstream out(filename, ios::out | ios::binary);
        out.write((char*) &data, texture->mWidth);
        out.close();
    }


    JsonExporter::JsonExporter(string filename)
        : filename(filename + ".json")
        , mBin(filename + ".bin", ios::out | ios::binary | ios::ate)
        , mDoc()
        , mAl(mDoc.GetAllocator())
        , mScene(nullptr)
    {
        mDoc.SetObject();

        Value nodes;
        nodes.SetArray();

        Value textures;
        textures.SetArray();

        Value materials;
        materials.SetArray();

        mDoc.AddMember("nodes", nodes, mAl);
        mDoc.AddMember("textures", textures, mAl);
        mDoc.AddMember("materials", materials, mAl);
    }

    void JsonExporter::ReadFile(string &path)
    {
        unsigned int flags = aiProcess_Triangulate | aiProcess_JoinIdenticalVertices | aiProcess_FlipUVs;
        Assimp::Importer import;
        const aiScene* scene = import.ReadFile(path, flags);

        if (!scene || scene->mFlags == AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
        {
            cout << "ERROR::ASSIMP::" << import.GetErrorString () << endl;
        } else {
            mScene = scene;

            WriteNodes();
            WriteTextures();
            WriteMaterials();
        }
    }

    void JsonExporter::WriteNodes()  {
        WriteNode(-1, mScene->mRootNode);
    }

    void JsonExporter::WriteNode(int parentIndex, aiNode* node) {
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

        if (parentIndex == -1) {
            Value nullType;
            nullType.SetNull();

            object.AddMember("parent", nullType, mAl);
        } else {
            object.AddMember("parent", parentIndex, mAl);
        }

        object.AddMember("children", children, mAl);

        for(int i = 0; i < node->mNumMeshes; i++)
        {
            aiMesh* mesh = mScene->mMeshes[node->mMeshes[i]];
            WriteVertex(object, mesh);
        }

        SizeType index = nodes.Size();
        nodes.PushBack(object, mAl);

        for(int i = 0; i < node->mNumChildren; i++)
        {
            WriteNode(int(index), node->mChildren[i]);
        }

        if (parentIndex != -1)
        {
            nodes[parentIndex]["children"].PushBack(index, mAl);
        }
    }

    void JsonExporter::WriteVertex(Value& object, aiMesh* mesh)
    {
        Value vertex;
        vertex.SetObject();

        Value index;
        index.SetObject();

        Value position;
        position.SetObject();

        WriteVertices(index, mesh->mNumFaces, mesh->mFaces);
        vertex.AddMember("index", index, mAl);

        WriteVertices(position, mesh->mNumVertices, mesh->mVertices);
        vertex.AddMember("position", position, mAl);

        if (mesh->HasNormals())
        {
            Value normal;
            normal.SetObject();
            WriteVertices(normal, mesh->mNumVertices, mesh->mNormals);
            vertex.AddMember("normal", normal, mAl);
        }

        if (mesh->HasVertexColors(0))
        {
            Value color;
            color.SetObject();
            WriteVertices(color, mesh->mNumVertices, mesh->mColors[0]);
            vertex.AddMember("color", color, mAl);
        }

        if (mesh->HasTextureCoords(0))
        {
            Value uv;
            uv.SetObject();
            WriteVertices(uv, mesh->mNumVertices, mesh->mTextureCoords[0]);
            vertex.AddMember("uv", uv, mAl);
        }

        object.AddMember("material", mesh->mMaterialIndex, mAl);
        object.AddMember("vertex", vertex, mAl);
    }

    void JsonExporter::WriteVertices(Value& object, unsigned& count, aiFace* list)
    {
        int offset = mSize, length = 0;

        for(int i = 0; i < count; ++i)
        {
            aiFace face = list[i];

            for(int j = 0; j < face.mNumIndices; ++j)
            {
                unsigned short point = face.mIndices[j];
                int size = sizeof(point);
                mBin.write((const char*) &point, size);
                length += size;
            }
        }

        mSize += length;
        WriteVertexMeta(object, ComponentType_UNSIGNED_SHORT, 1, offset, length);
    }

    void JsonExporter::WriteVertices(Value& object, unsigned& count, aiVector3D* list)
    {
        int offset = mSize;
        int size = sizeof(float);
        int length = size * count * 3;

        for(int i = 0; i < count; ++i)
        {
            aiVector3D vertex = list[i];

            mBin.write((const char*) &vertex.x, size);
            mBin.write((const char*) &vertex.y, size);
            mBin.write((const char*) &vertex.z, size);
        }

        mSize += length;
        WriteVertexMeta(object, ComponentType_FLOAT, 3, offset, length);
    }

    void JsonExporter::WriteVertices(Value& object, unsigned& count, aiColor4D* list)
    {
        int offset = mSize;
        int size = sizeof(int);
        int length = size * count * 4;

        for(int i = 0; i < count; ++i)
        {
            aiColor4D color = list[i];

            mBin.write((const char*) &color.r, size);
            mBin.write((const char*) &color.g, size);
            mBin.write((const char*) &color.b, size);
            mBin.write((const char*) &color.a, size);
        }

        mSize += length;
        WriteVertexMeta(object, ComponentType_UNSIGNED_INT, 4, offset, length);
    }

    void JsonExporter::WriteVertexMeta(Value& object, ComponentType type, int stride, int& offset, int& length)
    {
        object.AddMember("type"  , type  , mAl);
        object.AddMember("stride", stride, mAl);
        object.AddMember("offset", offset, mAl);
        object.AddMember("length", length, mAl);
    }

    void JsonExporter::WriteTextures()
    {
        for(int i = 0; i < mScene->mNumTextures; ++i)
        {
            aiTexture* texture = mScene->mTextures[i];
            WriteTextureBinary(texture);
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

            string map;
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
                ParserTextureName(s.data, map);
                diffuseMap.SetString(map.c_str(), map.size(), mAl);
                data.AddMember("diffuseMap", diffuseMap, mAl);
            }
            if(AI_SUCCESS == m->Get(AI_MATKEY_TEXTURE_AMBIENT(0), s))
            {
                Value ambientMap;
                ParserTextureName(s.data, map);
                ambientMap.SetString(map.c_str(), map.size(), mAl);
                data.AddMember("diffuseMap", ambientMap, mAl);
            }
            if(AI_SUCCESS == m->Get(AI_MATKEY_TEXTURE_SPECULAR(0), s))
            {
                Value specularMap;
                ParserTextureName(s.data, map);
                specularMap.SetString(map.c_str(), map.size(), mAl);
                data.AddMember("diffuseMap", specularMap, mAl);
            }
            if(AI_SUCCESS == m->Get(AI_MATKEY_TEXTURE_SHININESS(0), s))
            {
                Value shininessMap;
                ParserTextureName(s.data, map);
                shininessMap.SetString(map.c_str(), map.size(), mAl);
                data.AddMember("diffuseMap", shininessMap, mAl);
            }
            if(AI_SUCCESS == m->Get(AI_MATKEY_TEXTURE_OPACITY(0), s))
            {
                Value opacityMap;
                ParserTextureName(s.data, map);
                opacityMap.SetString(map.c_str(), map.size(), mAl);
                data.AddMember("diffuseMap", opacityMap, mAl);
            }
            if(AI_SUCCESS == m->Get(AI_MATKEY_TEXTURE_HEIGHT(0), s))
            {
                Value opacityMap;
                ParserTextureName(s.data, map);
                opacityMap.SetString(map.c_str(), map.size(), mAl);
                data.AddMember("diffuseMap", opacityMap, mAl);
            }
            if(AI_SUCCESS == m->Get(AI_MATKEY_TEXTURE_NORMALS(0), s))
            {
                Value opacityMap;
                ParserTextureName(s.data, map);
                opacityMap.SetString(map.c_str(), map.size(), mAl);
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

    void JsonExporter::Save()
    {
        StringBuffer buffer;
        // Writer<StringBuffer> writer(buffer);
        PrettyWriter<StringBuffer> writer(buffer);
        writer.SetIndent(' ', 2);
        mDoc.Accept(writer);
        string json = buffer.GetString();
        cout << json << endl;

        fstream outBin(filename.c_str(), ios::out | ios::ate);
        outBin.write(json.c_str(), json.size());
        outBin.close();
    }

    JsonExporter::~JsonExporter()
    {

    }
}
