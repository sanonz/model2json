#include <unistd.h>

#include "exportor.cpp"

using namespace std;
using namespace Model2Json;


string getpwd()
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
    string defaultModel = "samples/Door.fbx";
    string path = getpwd();
    path.append(defaultModel);

    // string path;
    // cout << "Model(" << defaultModel << "):";
    // cin >> path;

    string filename = "model";
    JsonExporter exporter(filename);
    exporter.ReadFile(path);
    exporter.Save();

    return 0;
}
