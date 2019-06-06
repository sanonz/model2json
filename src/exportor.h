#include <math.h>
#include <fstream>
#include <iostream>
#include <string.h>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include <rapidjson/document.h>
#include <rapidjson/prettywriter.h>

// see https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/TypedArray#TypedArray_objects
#define COMPONENT_TYPE_BYTE pow(2, 7)
#define COMPONENT_TYPE_UNSIGNED_BYTE pow(2, 8) - 1
#define COMPONENT_TYPE_SHORT pow(2, 15)
#define COMPONENT_TYPE_UNSIGNED_SHORT pow(2, 16) - 1
#define COMPONENT_TYPE_UNSIGNED_INT pow(2, 32) - 1


using namespace std;
using namespace rapidjson;


namespace Model2Json
{

    enum ComponentType
    {
        ComponentType_BYTE = 5120,
        ComponentType_UNSIGNED_BYTE = 5121,
        ComponentType_SHORT = 5122,
        ComponentType_UNSIGNED_SHORT = 5123,
        ComponentType_UNSIGNED_INT = 5125,
        ComponentType_FLOAT = 5126
    };


    class JsonExporter
    {
    private:

    protected:
        Document mDoc;
        Document::AllocatorType& mAl;

        const aiScene* mScene;


        void WriteNodes();
        void WriteNode(int parentIndex, aiNode* node);
        void WriteNodeVertices(Value& object, aiNode* node);
        void WriteNodeVertex(Value& object, unsigned int stride, ComponentType type, Value& data);
        void WriteNodeGroup(Value& object, unsigned int start, unsigned int count, unsigned int materialIndex);
        void WriteMeshIndices(Value& object, unsigned int count, aiFace* faces, unsigned int start);
        void WriteMeshVertices(Value& object, unsigned int count, aiVector3D* vectors);
        void WriteMeshVertices(Value& object, unsigned int count, aiColor4D* colors);
        void WriteTextures();
        void WriteMaterials();
        void WriteAnimations();

        Value MakeValue(const aiVector3D* v);
        Value MakeValue(const aiQuaternion* q);
        Value MakeValue(aiColor4D* c);

    public:
        explicit JsonExporter();
        ~JsonExporter();

        void ReadFile(const string& path);
        void Save(const string& path);

    };

}
