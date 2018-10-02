#include "generate_utils.h"
#include "std_filesystem.h"

#include <clang/AST/Decl.h>
#include <clang/AST/Mangle.h>

namespace {
SourceCodeLocation getSourceCodeLocation(clang::ASTContext &ast_context,
                                         clang::SourceManager &source_manager,
                                         clang::Decl *declaration) {
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
}  // namespace

bool functionReceivesFunctionPointer(const clang::FunctionDecl *func_decl) {
  // Start by collecting the types used in the function
  std::set<const clang::Type *> queue;

  for (const auto &param : func_decl->parameters()) {
    auto type_source_info = param->getTypeSourceInfo();
    if (type_source_info == nullptr) {
      continue;
    }

    auto type_loc = type_source_info->getTypeLoc();
    auto type = type_loc.getTypePtr();
    if (type == nullptr) {
      throw std::logic_error("Failed to acquire the Type ptr");
    }

    queue.insert(type);
  }

  // Recurse into each type, and only halt if we find a function pointer
  std::set<const clang::Type *> processed;
  std::set<const clang::Type *> skipped;

  while (true) {
    std::set<const clang::Type *> pending;

    for (auto queue_it = queue.begin(); queue_it != queue.end();) {
      const auto &type = *queue_it;

      if (processed.count(type) > 0) {
        queue_it = queue.erase(queue_it);

      } else if (type->isBuiltinType()) {
        processed.insert(type);
        queue_it = queue.erase(queue_it);

      } else if (type->isPointerType()) {
        auto pointee_type = type->getPointeeType().getTypePtr();
        pending.insert(pointee_type);
        processed.insert(type);

        queue_it = queue.erase(queue_it);

      } else if (type->isFunctionProtoType()) {
        return true;

      } else if (type->isFunctionNoProtoType()) {
        return true;

      } else if (type->isFunctionType()) {
        return true;

      } else if (type->isFunctionPointerType()) {
        return true;

      } else if (type->isRecordType()) {
        auto record_type = type->getAs<clang::RecordType>();
        auto declaration = record_type->getDecl();

        for (const auto &field : declaration->fields()) {
          auto type_source_info = field->getTypeSourceInfo();
          if (type_source_info == nullptr) {
            continue;
          }

          auto type_loc = type_source_info->getTypeLoc();
          auto field_type = type_loc.getTypePtr();
          pending.insert(field_type);
        }

        processed.insert(type);

        queue_it = queue.erase(queue_it);

      } else if (type->getArrayElementTypeNoTypeQual() != nullptr) {
        auto element_type = type->getArrayElementTypeNoTypeQual();

        pending.insert(element_type);
        processed.insert(type);

        queue_it = queue.erase(queue_it);

      } else if (type->getAs<clang::TypedefType>()) {
        auto typedef_type = type->getAs<clang::TypedefType>();
        auto underlying_type =
            typedef_type->getDecl()->getUnderlyingType().getTypePtr();

        pending.insert(underlying_type);
        processed.insert(underlying_type);

        queue_it = queue.erase(queue_it);

      } else {
        skipped.insert(type);
        queue_it = queue.erase(queue_it);
      }
    }

    if (pending.empty()) {
      break;
    }

    for (const auto &new_type : pending) {
      queue.insert(new_type);
    }

    pending.clear();
  }

  auto debug_env_var = getenv("ABIGEN_DEBUG");
  if (debug_env_var != nullptr && !skipped.empty()) {
    std::cerr << "Skipped types:\n\n";
    for (const auto &type : skipped) {
      type->dump();
      std::cerr << "\n";
    }

    std::cerr << "\n";
  }

  return false;
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

bool astFunctionCallback(clang::Decl *declaration,
                         clang::ASTContext &ast_context,
                         clang::SourceManager &source_manager,
                         void *user_defined,
                         clang::MangleContext *name_mangler) {
  auto &state = *static_cast<ABILibraryState *>(user_defined);

  // Helper used to blacklist functions
  auto L_blacklistFunction = [&state, &ast_context, &source_manager](
                                 const std::string &function_name,
                                 clang::Decl *declaration,
                                 BlacklistedFunction::Reason reason) -> void {
    BlacklistedFunction function;
    function.name = function_name;
    function.reason = reason;

    function.location =
        getSourceCodeLocation(ast_context, source_manager, declaration);

    state.blacklisted_function_list.insert({function_name, function});
  };

  // Skip constructors and destructors
  if (dynamic_cast<clang::CXXConstructorDecl *>(declaration) != nullptr ||
      dynamic_cast<clang::CXXDestructorDecl *>(declaration) != nullptr) {
    return true;
  }

  auto function_declaration = dynamic_cast<clang::FunctionDecl *>(declaration);

  std::string function_name;
  if (name_mangler->shouldMangleCXXName(function_declaration)) {
    std::string buffer;
    llvm::raw_string_ostream test(buffer);

    name_mangler->mangleName(function_declaration, test);
    function_name = test.str();
  } else {
    function_name = function_declaration->getName().str();
  }

  // Skip the function if we already blacklisted it
  if (state.blacklisted_function_list.find(function_name) !=
      state.blacklisted_function_list.end()) {
    return true;
  }

  // Blacklist the function if we already added it once in the whitelist
  auto whitelisted_func_it =
      state.whitelisted_function_list.find(function_name);

  if (whitelisted_func_it != state.whitelisted_function_list.end()) {
    state.whitelisted_function_list.erase(whitelisted_func_it);
    L_blacklistFunction(function_name, declaration,
                        BlacklistedFunction::Reason::DuplicateName);
    return true;
  }

  // Blacklist variadic functions
  if (function_declaration->isVariadic()) {
    L_blacklistFunction(function_name, declaration,
                        BlacklistedFunction::Reason::Variadic);
    return true;
  }

  // Finally, blacklist anything that receives a function pointer
  if (functionReceivesFunctionPointer(function_declaration)) {
    L_blacklistFunction(function_name, declaration,
                        BlacklistedFunction::Reason::FunctionPointer);
    return true;
  }

  WhitelistedFunction function_descriptor;
  function_descriptor.name = function_name;
  function_descriptor.location =
      getSourceCodeLocation(ast_context, source_manager, declaration);

  state.whitelisted_function_list.insert({function_name, function_descriptor});

  return true;
}