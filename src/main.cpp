#include <iostream>
#include <filesystem>

using std::cout; 
using std::endl;

int main() 
{
  cout << "Hello myjvm!" << endl;



  return 0;
}

std::vector<std::string> find_source_files(const std::filesystem::path& root) 
{
    std::vector<std::string> files;
    for (const auto& entry : std::filesystem::recursive_directory_iterator(root)) {
        if (!entry.is_regular_file()) continue;
        std::string ext = entry.path().extension();
        if (ext == ".scala" || ext == ".sc" || ext == ".java" || ext == ".kt") {
            files.emplace_back(entry.path().string());
        }
    }
    return files;
}

