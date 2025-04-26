#pragma once

#include <filesystem>
#include "mystl.hpp"

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
void write_embedded_archive_to_disk(const std::string& path);
void decompress_zstd_file(const std::string& input_path, const std::string& output_path);
void extract_tar(const std::string& tar_path, const std::string& dest_dir);
void restore_languages_directory();

