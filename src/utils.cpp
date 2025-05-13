#include <archive.h>
#include <algorithm>
#include <archive_entry.h>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <zstd.h> 
#include "os.hpp"
#include "utils.hpp"

using std::cout;
using std::endl;

extern const unsigned char _binary__languages_tar_zst_start[];
extern const unsigned char _binary__languages_tar_zst_end[];

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
    std::filesystem::path cwd = std::filesystem::current_path();
    std::string prefix = cwd.string() + "/.languages/scala-compiler-jars/";
    std::string scala_cp = 
        prefix + "scala3-compiler_3-3.3.1.jar:" +
        prefix + "scala3-library_3-3.3.1.jar:" +
        prefix + "scala3-interfaces-3.3.1.jar:" +
        prefix + "scala-library-2.13.12.jar:" +
        prefix + "tasty-core_3-3.3.1.jar:" +
        prefix + "scala-asm-9.5.0-scala-1.jar:" +
        prefix + "util-interface-1.3.0.jar:" +
        prefix + "protobuf-java-3.7.0.jar:" +
        prefix + "jline-reader-3.19.0.jar:" +
        prefix + "jline-terminal-3.19.0.jar:" +
        prefix + "jline-terminal-jna-3.19.0.jar:" +
        prefix + "jna-5.3.1.jar";
    std::string java_path = cwd.string() + "/.languages/jvm-runtime-standard/bin/java";
    std::vector<std::string> class_cmd = {
        java_path,
        "-Dscala.usejavacp=true",
        "-cp", scala_cp,
        "dotty.tools.dotc.Main",
        "-sourcepath", ".",
        "-d", "out/"
    };
    for (const auto& file : files) class_cmd.push_back(file); 
    std::pair<int, std::string> res = OS::run_command(class_cmd);
    if (res.first != 0) {
        std::cerr << "Compilation failed: " << res.second << endl;
        return;
    }
    std::vector<std::string> class_files = get_class_files();
    if (!add_scala_runtime(".languages/scala-compiler-jars/scala3-library_3-3.3.1.jar")) {
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
    std::string command = ".languages/kotlin-compiler/kotlinc/bin/kotlinc -include-runtime -d out/all_files.jar " + file_str;
    std::pair<int, std::string> res = OS::run_command(command);
    if (res.first != 0) {
        std::cerr << "Compilation failed: " << res.second << endl;
        return;
    }
    cout << "Files compiled successfully!" << endl;
}

void compile_java_files(const my::vector<std::string>& files)
{
    cout << "Compiling Java files..." << endl;
    std::string file_str = files.join(" ");
    std::string class_cmd = ".languages/jvm-runtime-standard/bin/javac -d out/ " + file_str;
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

std::string run_known_class_file(std::string& name, const std::string& filetype)
{
    if (filetype == ".java") return run_known_java_class_file(name);
    else if (filetype == ".kt") return run_kotlin_jar();
    else if (filetype == ".scala") return run_known_scala_class_file(name);
    else return "Unknown file type provided.";
}

std::string try_run_class_files(std::vector<std::string>& class_names, const std::string& filetype)
{
    if (filetype == ".java") return try_run_java_class_files(class_names);
    else if (filetype == ".kt") return run_kotlin_jar();
    else if (filetype == ".scala") return try_run_scala_class_files(class_names);
    else return "Unknown file type provided.";
}

std::string run_known_java_class_file(std::string& name)
{
    cout << "Running: '" << name << "'" << endl;
    std::string command = ".languages/jvm-runtime-standard/bin/java -cp out/all_files.jar " + name;
    std::pair<int, std::string> res = OS::run_command(command);
    if (res.first == 0) return res.second;
    else return "Unable to run provided entrypoint.";
}

std::string try_run_java_class_files(std::vector<std::string>& class_names)
{
    cout << "Attempting:" << endl;
    for (std::string& name : class_names) {
        if (my::string(name).contains("$") || name.starts_with("scala.")) continue;
        std::string result = run_known_java_class_file(name);
        if (result != "Unable to run provided entrypoint.") return result; 
    }    
    return "No valid entrypoints detected.";
}

std::string run_known_scala_class_file(std::string& name)
{
    cout << "Running: '" << name << "'" << endl;
    std::string scala_cp = 
        ".languages/scala-compiler-jars/scala3-compiler_3-3.3.1.jar:"
        ".languages/scala-compiler-jars/scala3-library_3-3.3.1.jar:"
        ".languages/scala-compiler-jars/scala3-interfaces-3.3.1.jar:"
        ".languages/scala-compiler-jars/scala-library-2.13.12.jar:"
        ".languages/scala-compiler-jars/tasty-core_3-3.3.1.jar:"
        ".languages/scala-compiler-jars/scala-asm-9.5.0-scala-1.jar:"
        ".languages/scala-compiler-jars/util-interface-1.3.0.jar:"
        ".languages/scala-compiler-jars/protobuf-java-3.7.0.jar:"
        ".languages/scala-compiler-jars/jline-reader-3.19.0.jar:"
        ".languages/scala-compiler-jars/jline-terminal-3.19.0.jar:"
        ".languages/scala-compiler-jars/jline-terminal-jna-3.19.0.jar:"
        ".languages/scala-compiler-jars/jna-5.3.1.jar";
    std::string command = 
        ".languages/jvm-runtime-standard/bin/java "
        "-Dscala.usejavacp=true "
        "-cp out/all_files.jar:" + scala_cp + " " + name;
    std::pair<int, std::string> res = OS::run_command(command);
    if (res.first == 0) return res.second;
    else return "Unable to run provided Scala entrypoint.";
}

std::string try_run_scala_class_files(std::vector<std::string>& class_names)
{
    cout << "Attempting:" << endl;
    for (std::string& name : class_names) {
        if (my::string(name).contains("$") || name.starts_with("scala.")) continue;
        std::string result = run_known_scala_class_file(name);
        if (result != "Unable to run provided Scala entrypoint.") {
            return result;
        }
    }
    return "No valid Scala entrypoints detected.";
}

std::string run_kotlin_jar()
{
    cout << "Running Kotlin project." << endl;;
    std::string java_path = ".languages/jvm-runtime-standard/bin/java";
    std::string command = java_path + " -jar out/all_files.jar";
    std::pair<int, std::string> res = OS::run_command(command);
    if (res.first == 0) return res.second;
    else return "Unable to run Kotlin jar.";
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
    if (!file) {
        std::cerr << "Failed to open " << filepath << endl;
        exit(1);
    }
    return {std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>()};
}

bool create_jar_from_classes(const std::vector<std::string>& class_files) 
{
    std::filesystem::path output_jar = "out/all_files.jar";
    struct archive* a = archive_write_new();
    archive_write_set_format_zip(a);
    if (archive_write_open_filename(a, output_jar.c_str()) != ARCHIVE_OK) {
        std::cerr << "Failed to open archive: " << archive_error_string(a) << std::endl;
        archive_write_free(a);
        return false;
    }
    for (const auto& filepath_str : class_files) {
        std::filesystem::path filepath(filepath_str);
        std::filesystem::path relative_path = std::filesystem::relative(filepath, "out");
        std::ifstream file(filepath, std::ios::binary);
        if (!file) {
            std::cerr << "Failed to read file: " << filepath << endl;
            continue;
        }
        std::vector<char> buffer((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
        struct archive_entry* entry = archive_entry_new();
        archive_entry_set_pathname(entry, relative_path.string().c_str());
        archive_entry_set_size(entry, buffer.size());
        archive_entry_set_filetype(entry, AE_IFREG);
        archive_entry_set_perm(entry, 0644);
        archive_write_header(a, entry);
        archive_write_data(a, buffer.data(), buffer.size());
        archive_entry_free(entry);
    }
    archive_write_close(a);
    archive_write_free(a);
    cout << "Successfully created " << output_jar << "!" << endl;
    return true;
}

bool add_scala_runtime(const std::string& jar_path)
{
    struct archive* a = archive_read_new();
    if (!a) {
        std::cerr << "Failed to create archive object!" << endl;
        return false;
    }
    archive_read_support_format_zip(a);
    if (archive_read_open_filename(a, jar_path.c_str(), 10240) != ARCHIVE_OK) {
        std::cerr << "Failed to open JAR: " << jar_path << " Error: " << archive_error_string(a) << std::endl;
        archive_read_free(a);
        return false;
    }
    struct archive_entry* entry;
    while (archive_read_next_header(a, &entry) == ARCHIVE_OK) {
        const char* entry_name = archive_entry_pathname(entry);
        if (!entry_name) {
            std::cerr << "Encountered file with no name, skipping..." << endl;
            archive_read_data_skip(a);
            continue;
        }
        // Skip directories
        if (archive_entry_filetype(entry) == AE_IFDIR) {
            archive_read_data_skip(a);
            continue;
        }
        std::filesystem::path output_path = std::filesystem::path("out") / entry_name;
        std::filesystem::create_directories(output_path.parent_path());
        std::ofstream out_file(output_path, std::ios::binary);
        if (!out_file) {
            std::cerr << "Failed to open output file for writing: " << output_path << std::endl;
            archive_read_data_skip(a);
            continue;
        }
        const void* buffer;
        size_t size;
        int64_t offset;
        while (archive_read_data_block(a, &buffer, &size, &offset) == ARCHIVE_OK) {
            out_file.write(reinterpret_cast<const char*>(buffer), size);
        }
        out_file.close();
    }
    archive_read_close(a);
    archive_read_free(a);
    cout << "Successfully extracted " << jar_path << " into out/ directory." << endl;
    return true;
}

void write_embedded_archive_to_disk(const std::string& path)
{
    std::ofstream ofs(path, std::ios::binary);
    if (!ofs) {
        std::cerr << "Unable to open file: " << path << endl;
        exit(1);
    }
    size_t archive_size = _binary__languages_tar_zst_end - _binary__languages_tar_zst_start;
    ofs.write(reinterpret_cast<const char*>(_binary__languages_tar_zst_start), archive_size);
}

void decompress_zstd_file(const std::string& input_path, const std::string& output_path)
{
    std::ifstream ifs(input_path, std::ios::binary);
    if (!ifs) {
        std::cerr << "Failed to open input file for decompression." << endl;
        exit(1);
    }
    std::ofstream ofs(output_path, std::ios::binary);
    if (!ofs) {
        std::cerr << "Failed to open output file for decompression." << endl;
        exit(1);
    }
    const size_t CHUNK_SIZE = 16384;
    std::vector<char> in_buf(CHUNK_SIZE);
    std::vector<char> out_buf(CHUNK_SIZE);
    ZSTD_DStream* dstream = ZSTD_createDStream();
    if (!dstream) {
        std::cerr << "Failed to create ZSTD_DStream." << endl;
        exit(1);
    }
    ZSTD_initDStream(dstream);
    while (ifs) {
        ifs.read(in_buf.data(), CHUNK_SIZE);
        size_t bytes_read = ifs.gcount();
        ZSTD_inBuffer input = { in_buf.data(), bytes_read, 0 };
        while (input.pos < input.size) {
            ZSTD_outBuffer output = { out_buf.data(), out_buf.size(), 0 };
            size_t ret = ZSTD_decompressStream(dstream, &output, &input);
            if (ZSTD_isError(ret)) {
                throw std::runtime_error("Decompression error: " + std::string(ZSTD_getErrorName(ret)));
            }
            ofs.write(out_buf.data(), output.pos);
        }
    }
    ZSTD_freeDStream(dstream);
}

void extract_tar(const std::string& tar_path, const std::string& output_dir) 
{
    struct archive* a = archive_read_new();
    archive_read_support_format_tar(a);
    archive_read_support_filter_all(a); // In case it's compressed
    if (archive_read_open_filename(a, tar_path.c_str(), 10240) != ARCHIVE_OK) {
        throw std::runtime_error("Failed to open tar file: " + tar_path);
    }
    archive_entry* entry;
    while (archive_read_next_header(a, &entry) == ARCHIVE_OK) {
        std::filesystem::path full_path = std::filesystem::path(output_dir) / archive_entry_pathname(entry);
        auto type = archive_entry_filetype(entry);
        if (type == AE_IFDIR) {
            std::filesystem::create_directories(full_path);
        } 
        else if (type == AE_IFREG) {
            std::filesystem::create_directories(full_path.parent_path());
            std::ofstream out(full_path, std::ios::binary);
            if (!out) {
                throw std::runtime_error("Failed to create output file: " + full_path.string());
            }
            const void* buff;
            size_t size;
            la_int64_t offset;
            while (archive_read_data_block(a, &buff, &size, &offset) == ARCHIVE_OK) {
                out.write(reinterpret_cast<const char*>(buff), size);
            }
        } 
    }

    archive_read_free(a);
}

void restore_languages_directory()
{
    write_embedded_archive_to_disk("temp_archive.zst");
    decompress_zstd_file("temp_archive.zst", "temp_archive.tar");
    extract_tar("temp_archive.tar", ".");
    std::filesystem::remove("temp_archive.zst");
    std::filesystem::remove("temp_archive.tar");
    std::filesystem::permissions(".languages/jvm-runtime-standard/bin/java",
                             std::filesystem::perms::owner_exec |
                             std::filesystem::perms::group_exec |
                             std::filesystem::perms::others_exec,
                             std::filesystem::perm_options::add);

    std::filesystem::permissions(".languages/jvm-runtime-standard/bin/javac",
                             std::filesystem::perms::owner_exec |
                             std::filesystem::perms::group_exec |
                             std::filesystem::perms::others_exec,
                             std::filesystem::perm_options::add);
    std::filesystem::permissions(".languages/kotlin-compiler/kotlinc/bin/kotlinc", 
                             std::filesystem::perms::owner_exec |
                             std::filesystem::perms::group_exec |
                             std::filesystem::perms::others_exec, 
                             std::filesystem::perm_options::add);
}

bool any_env_prefix_set(const std::string& target) 
{
    return std::any_of(target.begin(), target.end(), [&](auto){
        static std::string prefix;
        prefix += target[prefix.size()];
        return std::getenv(prefix.c_str()) != nullptr;
    });
}

