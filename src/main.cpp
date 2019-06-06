#include <unistd.h>

#include "exportor.cpp"

using namespace std;
using namespace Model2Json;


string getPwd()
{
    // 获取当前目录
    char dir[100];
    getcwd(dir, 100);
    string folder = dir;
    string bin = "bin";
    int pos = folder.rfind(bin);
    folder.replace(pos, bin.size(), "");

    return folder;
}

int main() {
    string root = getPwd();
    string modelPath = "samples/Door.fbx";

    // string modelPath;
    // cout << "Model(" << modelPath << "):";
    // cin >> modelPath;

    JsonExporter exporter;
    exporter.ReadFile(root + modelPath);
    exporter.Save("model.json");

    return 0;
}
