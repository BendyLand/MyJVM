#include "utils.hpp"

using std::cout; 
using std::endl;

int main(int argc, char** argv) 
{
    if (!std::filesystem::exists(".languages")) {
        cout << "Generating JVM runtimes..." << endl;
        restore_languages_directory();
        cout << "Runtimes generated successfully!" << endl;
    }
    std::vector<std::string> files; 
    if (argc > 1) files = find_source_files(argv[1]); 
    else {
        std::cerr << "Usage: myjvm <project_path> [main_class]" << endl;
        exit(1);
    }
    std::string filetype = infer_file_type(".");
    if (!already_compiled(".")) compile_files(files, filetype);
    std::vector<std::string> class_names = get_class_names();
    std::string result;
    if (argc > 2) {
      std::string arg = std::string(argv[2]);
      result = run_known_class_file(arg, filetype); 
    }
    else result = try_run_class_files(class_names, filetype);
    cout << result << endl;

    return 0;
}
