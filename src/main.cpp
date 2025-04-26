#include <iostream>
#include <filesystem>
#include <fstream>
#include "os.hpp"
#include "mystl.hpp"
#include "miniz.h"

using std::cout; 
using std::endl;

bool create_jar_from_classes(const std::vector<std::string>& class_files);
std::string run_known_scala_class_file(std::string& name);
bool add_scala_runtime(const std::string& jar_path);
std::vector<uint8_t> load_file(const std::string& filepath);
std::vector<std::string> find_source_files(const std::filesystem::path& root);
void compile_kotlin_files(const my::vector<std::string>& files);
std::string try_run_scala_class_files(std::vector<std::string>& class_names);
std::string try_run_class_files(std::vector<std::string>& class_names);
std::string run_known_class_file(std::string& name);
bool already_compiled(const std::filesystem::path& root);
std::string infer_file_type(const std::filesystem::path& root);
void compile_files(const std::vector<std::string>& files, const std::string& filetype);
std::vector<std::string> get_class_names();
std::vector<std::string> get_class_files();

int main(int argc, char** argv) 
{
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
      result = filetype == ".scala" ? run_known_scala_class_file(arg) : run_known_class_file(arg); 
    }
    else result = filetype == ".scala" ? try_run_scala_class_files(class_names) : try_run_class_files(class_names);
    cout << result << endl;

    return 0;
}

std::string infer_file_type(const std::filesystem::path& root)
{
    std::vector<std::string> files;
    for (const auto& entry : std::filesystem::recursive_directory_iterator(root)) {
        if (!entry.is_regular_file()) continue;
        std::string ext = entry.path().extension();
        if (ext == ".scala" || ext == ".sc") return ".scala";
        else if (ext == ".java") return ".java";
        else if (ext == ".kt") return ".kt";
    }
    return "UNKNOWN";
}

std::vector<std::string> find_source_files(const std::filesystem::path& root) 
{
    std::vector<std::string> files;
    std::string filetype = infer_file_type(root);
    for (const auto& entry : std::filesystem::recursive_directory_iterator(root)) {
        if (!entry.is_regular_file()) continue;
        std::string ext = entry.path().extension();
        if (my::string(ext).contains(filetype)) {
            files.emplace_back(entry.path().string());
        }
    }
    return files;
}

std::vector<std::string> get_class_files()
{
    std::vector<std::string> result;
    for (const auto& entry : std::filesystem::recursive_directory_iterator("out")) {
        if (!entry.is_regular_file()) continue;
        std::string ext = entry.path().extension();
        if (ext == ".class") result.emplace_back(entry.path().string());
    }
    return result;
}

std::vector<std::string> get_class_names()
{
    std::vector<std::string> result;
    std::filesystem::path out_dir = "out";
    for (const auto& entry : std::filesystem::recursive_directory_iterator(out_dir)) {
        if (!entry.is_regular_file()) continue;
        if (entry.path().extension() != ".class") continue;
        std::filesystem::path relative_path = std::filesystem::relative(entry.path(), out_dir);
        std::string without_ext = relative_path.replace_extension("").string();
        for (auto& ch : without_ext) {
            if (ch == '/' || ch == '\\') ch = '.'; 
        }
        result.emplace_back(without_ext);
    }
    return result;
}

void compile_scala_files(const my::vector<std::string>& files)
{
    cout << "Compiling Scala files..." << endl;
    std::string file_str = files.join(" ");
    std::string scala_cp = 
        "languages/scala-compiler-jars/scala3-compiler_3-3.3.1.jar:"
        "languages/scala-compiler-jars/scala3-library_3-3.3.1.jar:"
        "languages/scala-compiler-jars/scala3-interfaces-3.3.1.jar:"
        "languages/scala-compiler-jars/scala-library-2.13.12.jar:"
        "languages/scala-compiler-jars/tasty-core_3-3.3.1.jar:"
        "languages/scala-compiler-jars/scala-asm-9.5.0-scala-1.jar:"
        "languages/scala-compiler-jars/util-interface-1.3.0.jar:"
        "languages/scala-compiler-jars/protobuf-java-3.7.0.jar:"
        "languages/scala-compiler-jars/jline-reader-3.19.0.jar:"
        "languages/scala-compiler-jars/jline-terminal-3.19.0.jar:"
        "languages/scala-compiler-jars/jline-terminal-jna-3.19.0.jar:"
        "languages/scala-compiler-jars/jna-5.3.1.jar";
    std::string class_cmd = 
        "languages/jvm-runtime-standard/bin/java "  
        "-Dscala.usejavacp=true "
        "-cp \"" + scala_cp + "\" "
        "dotty.tools.dotc.Main "
        "-d out/ " + file_str;
    std::pair<int, std::string> res = OS::run_command(class_cmd);
    if (res.first != 0) {
        cout << "Compilation failed: " << res.second << endl;
        return;
    }
    std::vector<std::string> class_files = get_class_files();
    if (!add_scala_runtime("languages/scala-compiler-jars/scala3-library_3-3.3.1.jar")) {
        std::cerr << "Unable to add Scala3 runtime." << endl;
        exit(1);
    }
    bool success = create_jar_from_classes(class_files);
    if (success) cout << "Files compiled successfully!" << endl;
    else cout << "Unable to archive files to a jar.";
}

void compile_kotlin_files(const my::vector<std::string>& files)
{
    cout << "Compiling Kotlin files..." << endl;
    std::string file_str = files.join(" ");
    std::string command = "languages/kotlin-compiler/kotlinc/bin/kotlinc -include-runtime -d out/all.jar " + file_str;
    std::pair<int, std::string> res = OS::run_command(command);
    if (res.first != 0) {
        cout << "Compilation failed: " << res.second << endl;
        return;
    }
    cout << "Files compiled successfully!" << endl;
}

void compile_java_files(const my::vector<std::string>& files)
{
    cout << "Compiling Java files..." << endl;
    std::string file_str = files.join(" ");
    std::string class_cmd = "languages/jvm-runtime-standard/bin/javac -d out/ " + file_str;
    std::pair<int, std::string> res = OS::run_command(class_cmd);
    if (res.first != 0) {
        cout << "Compilation failed: " << res.second << endl;
        return;
    }
    std::vector<std::string> class_files = get_class_files();
    bool success = create_jar_from_classes(class_files);
    if (success) cout << "Files compiled successfully!" << endl;
    else cout << "Unable to archive files to a jar.";
}

std::string try_run_class_files(std::vector<std::string>& class_names)
{
    cout << "Attempting to run:" << endl;
    for (std::string& name : class_names) {
        if (my::string(name).contains("$") || name.starts_with("scala.")) continue;
        cout << " - '" << name << "'" << endl;
        std::string command = "languages/jvm-runtime-standard/bin/java -cp out/all_files.jar " + name;
        std::pair<int, std::string> res = OS::run_command(command);
        if (res.first == 0) return res.second;
    }    
    return "No valid entrypoints detected.";
}

std::string run_known_class_file(std::string& name)
{
    cout << "Running: '" << name << "'" << endl;
    std::string command = "languages/jvm-runtime-standard/bin/java -cp out/all_files.jar " + name;
    std::pair<int, std::string> res = OS::run_command(command);
    if (res.first == 0) return res.second;
    else return "Unable to run provided entrypoint.";
}

std::string run_known_scala_class_file(std::string& name)
{
    cout << "Running Scala entrypoint: '" << name << "'" << endl;
    std::string scala_cp = 
        "languages/scala-compiler-jars/scala3-compiler_3-3.3.1.jar:"
        "languages/scala-compiler-jars/scala3-library_3-3.3.1.jar:"
        "languages/scala-compiler-jars/scala3-interfaces-3.3.1.jar:"
        "languages/scala-compiler-jars/scala-library-2.13.12.jar:"
        "languages/scala-compiler-jars/tasty-core_3-3.3.1.jar:"
        "languages/scala-compiler-jars/scala-asm-9.5.0-scala-1.jar:"
        "languages/scala-compiler-jars/util-interface-1.3.0.jar:"
        "languages/scala-compiler-jars/protobuf-java-3.7.0.jar:"
        "languages/scala-compiler-jars/jline-reader-3.19.0.jar:"
        "languages/scala-compiler-jars/jline-terminal-3.19.0.jar:"
        "languages/scala-compiler-jars/jline-terminal-jna-3.19.0.jar:"
        "languages/scala-compiler-jars/jna-5.3.1.jar";
    std::string command = 
        "languages/jvm-runtime-standard/bin/java "
        "-Dscala.usejavacp=true "
        "-cp out/all_files.jar:" + scala_cp + " " + name;
    std::pair<int, std::string> res = OS::run_command(command);
    if (res.first == 0) return res.second;
    else return "Unable to run provided Scala entrypoint.";
}

std::string try_run_scala_class_files(std::vector<std::string>& class_names)
{
    cout << "Attempting to run Scala classes:" << endl;
    for (std::string& name : class_names) {
        if (my::string(name).contains("$") || name.starts_with("scala.")) continue;
        cout << " - '" << name << "'" << endl;
        std::string result = run_known_scala_class_file(name);
        if (result != "Unable to run provided Scala entrypoint.") {
            return result;
        }
    }
    return "No valid Scala entrypoints detected.";
}

bool already_compiled(const std::filesystem::path& root)
{
    std::filesystem::path output = root / "out" / "all_files.jar";
    if (std::filesystem::exists(output)) return true;
    return false;
}

void compile_files(const std::vector<std::string>& files, const std::string& filetype)
{
    if (filetype == ".kt") compile_kotlin_files(files);
    else if (filetype == ".scala") compile_scala_files(files);
    else if (filetype == ".java") compile_java_files(files);
    else cout << "Unknown extension. No files compiled." << endl;
}

std::vector<uint8_t> load_file(const std::string& filepath) 
{
    std::ifstream file(filepath, std::ios::binary);
    if (!file) throw std::runtime_error("Failed to open " + filepath);
    return {std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>()};
}

bool create_jar_from_classes(const std::vector<std::string>& class_files) 
{
    mz_zip_archive zip_archive;
    memset(&zip_archive, 0, sizeof(zip_archive));
    // Create a new archive file
    if (!mz_zip_writer_init_file(&zip_archive, "out/all_files.jar", 0)) {
        std::cerr << "Failed to initialize zip archive!" << std::endl;
        return false;
    }
    for (const auto& filepath : class_files) {
        std::vector<uint8_t> file_data = load_file(filepath);
        std::string base_filename = filepath.substr(filepath.find_last_of("/\\") + 1);
        if (!mz_zip_writer_add_mem(&zip_archive, base_filename.c_str(), file_data.data(), file_data.size(), MZ_BEST_COMPRESSION)) {
            std::cerr << "Failed to add file to archive: " << filepath << std::endl;
            mz_zip_writer_end(&zip_archive);
            return false;
        }
    }
    mz_zip_writer_finalize_archive(&zip_archive);
    mz_zip_writer_end(&zip_archive);
    cout << "Successfully created out/all_files.jar!" << endl;
    return true;
}

bool add_scala_runtime(const std::string& jar_path)
{
    mz_zip_archive zip;
    memset(&zip, 0, sizeof(zip));
    if (!mz_zip_reader_init_file(&zip, jar_path.c_str(), 0)) {
        std::cerr << "Failed to open JAR: " << jar_path << endl;
        return false;
    }
    int file_count = (int)mz_zip_reader_get_num_files(&zip);
    for (int i = 0; i < file_count; ++i) {
        mz_zip_archive_file_stat file_stat;
        if (!mz_zip_reader_file_stat(&zip, i, &file_stat)) {
            std::cerr << "Failed to get file stat for index: " << i << endl;
            continue;
        }
        if (mz_zip_reader_is_file_a_directory(&zip, i)) {
            continue;
        }
        size_t size = 0;
        void* p = mz_zip_reader_extract_to_heap(&zip, i, &size, 0);
        if (!p) {
            std::cerr << "Failed to extract file: " << file_stat.m_filename << endl;
            continue;
        }
        std::filesystem::path output_path = std::filesystem::path("out") / file_stat.m_filename;
        std::filesystem::create_directories(output_path.parent_path());
        std::ofstream out_file(output_path, std::ios::binary);
        if (!out_file) {
            std::cerr << "Failed to write extracted file: " << output_path << endl;
            mz_free(p);
            continue;
        }
        out_file.write(reinterpret_cast<const char*>(p), size);
        out_file.close();
        mz_free(p);
    }
    mz_zip_reader_end(&zip);
    std::cout << "Successfully extracted " << jar_path << " into out/ directory." << endl;
    return true;
}


