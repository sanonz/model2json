#include <fstream>
#include <iostream>
#include <string.h>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include <rapidjson/document.h>
#include <rapidjson/prettywriter.h>


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
        int mSize = 0;

        string filename;
        fstream mBin;
        Document mDoc;
        Document::AllocatorType& mAl;

        const aiScene* mScene;


        void WriteNodes();
        void WriteNode(int parentIndex, aiNode* node);
        void WriteVertex(Value& object, aiMesh* mesh);
        void WriteVertices(Value& object, unsigned& count, aiFace    * list);
        void WriteVertices(Value& object, unsigned& count, aiVector3D* list);
        void WriteVertices(Value& object, unsigned& count, aiColor4D * list);
        void WriteVertexMeta(Value& object, ComponentType type, int stride, int& offset, int& length);
        void WriteTextures();
        void WriteMaterials();
        void WriteAnimations();

        Value MakeValue(const aiVector3D* v);
        Value MakeValue(const aiQuaternion* q);
        Value MakeValue(aiColor4D* c);

    public:
        JsonExporter(string filename);
        ~JsonExporter();

        void ReadFile(string& path);
        void Save();

    };

}
