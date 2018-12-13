#include "generate_utils.h"
#include "std_filesystem.h"

#include <clang/AST/Decl.h>
#include <clang/AST/Mangle.h>

SourceCodeLocation getSourceCodeLocation(clang::ASTContext &ast_context,
                                         clang::SourceManager &source_manager,
                                         const clang::Decl *declaration) {
  auto start_location = declaration->getLocStart();
  auto full_start_location = ast_context.getFullLoc(start_location);

#if LLVM_MAJOR_VERSION > 5
  static_cast<void>(source_manager);
  const auto file_entry = full_start_location.getFileEntry();
#else
  auto file_id = full_start_location.getFileID();
  const auto file_entry = source_manager.getFileEntryForID(file_id);
#endif

  SourceCodeLocation output;
  if (file_entry != nullptr) {
    output.file_path = file_entry->getName();
  }

  if (output.file_path.empty()) {
    // This is the file we generated in memory for clang
    output.file_path = "main.cpp";
  }

  output.line = full_start_location.getSpellingLineNumber();
  output.column = full_start_location.getSpellingColumnNumber();

  return output;
}

bool createCompilerInstance(CompilerInstanceRef &compiler,
                            ProfileManagerRef &profile_manager,
                            const LanguageManager &language_manager,
                            const CommandLineOptions &cmdline_options) {
  CompilerInstanceSettings compiler_settings;
  auto prof_mgr_status = profile_manager->get(compiler_settings.profile,
                                              cmdline_options.profile_name);
  if (!prof_mgr_status.succeeded()) {
    std::cerr << prof_mgr_status.toString() << "\n";
    return false;
  }

  if (!language_manager.parseLanguageDefinition(
          compiler_settings.language, compiler_settings.language_standard,
          cmdline_options.language)) {
    std::cerr << "Invalid language definition\n";
    return false;
  }

  compiler_settings.enable_gnu_extensions =
      cmdline_options.enable_gnu_extensions;

  compiler_settings.use_visual_cxx_mangling =
      cmdline_options.use_visual_cxx_mangling;

  compiler_settings.additional_include_folders = cmdline_options.header_folders;

  auto compiler_status = CompilerInstance::create(compiler, compiler_settings);
  if (!compiler_status.succeeded()) {
    std::cerr << compiler_status.toString() << "\n";
    return false;
  }

  return true;
}

bool enumerateIncludeFiles(std::vector<HeaderDescriptor> &header_files,
                           const std::string &header_folder) {
  const static StringList valid_extensions = {".h", ".hh", ".hp", ".hpp",
                                              ".hxx"};

  auto root_header_folder = stdfs::absolute(header_folder);

  try {
    stdfs::recursive_directory_iterator it(root_header_folder);
    for (const auto &directory_entry : it) {
      if (!stdfs::is_regular_file(directory_entry.path())) {
        continue;
      }

      auto path = directory_entry.path();

      const auto &ext = path.extension().string();
      if (ext.empty() ||
          std::find(valid_extensions.begin(), valid_extensions.end(), ext) ==
              valid_extensions.end()) {
        continue;
      }

      HeaderDescriptor header_desc = {};
      header_desc.name = path.filename();

      for (auto parent_path = path.parent_path();
           !parent_path.empty() && parent_path != parent_path.root_path();
           parent_path = parent_path.parent_path()) {
        if (parent_path == root_header_folder) {
          break;
        }

        auto current_relative_path =
            parent_path.filename() /
            (header_desc.possible_prefixes.empty()
                 ? ""
                 : header_desc.possible_prefixes.back());
        header_desc.possible_prefixes.push_back(current_relative_path.string());
      }

      header_files.push_back(header_desc);
    }

    return true;

  } catch (...) {
    return false;
  }
}

bool enumerateIncludeFiles(std::vector<HeaderDescriptor> &header_files,
                           const StringList &header_folders) {
  header_files = {};

  for (const auto &folder : header_folders) {
    if (!enumerateIncludeFiles(header_files, folder)) {
      std::cerr << "Failed to enumerate the include files in the following "
                   "directory: "
                << folder << "\n";

      return false;
    }
  }

  return true;
}

StringList generateIncludeDirectives(
    const HeaderDescriptor &header_descriptor) {
  StringList result = {header_descriptor.name};

  for (const auto &prefix : header_descriptor.possible_prefixes) {
    auto include_directive = stdfs::path(prefix) / header_descriptor.name;
    result.emplace_back(include_directive);
  }

  return result;
}

std::string generateSourceBuffer(const StringList &include_list,
                                 const StringList &base_includes) {
  std::stringstream buffer;
  for (const auto &include : base_includes) {
    buffer << "#include <" << include << ">\n";
  }

  for (const auto &include : include_list) {
    buffer << "#include <" << include << ">\n";
  }

  return buffer.str();
}
