#include "generate_utils.h"
#include "std_filesystem.h"

#include <clang/AST/Decl.h>
#include <clang/AST/Mangle.h>
#include <clang/Lex/Preprocessor.h>
#include <clang/Lex/PreprocessorOptions.h>

namespace {
#if LLVM_MAJOR_VERSION <= 4
const auto kClangFrontendInputKindCxx = clang::IK_CXX;
const auto kClangFrontendInputKindC = clang::IK_C;
#else
const auto kClangFrontendInputKindCxx = clang::InputKind::CXX;
const auto kClangFrontendInputKindC = clang::InputKind::C;
#endif

/// The ASTConsumer is used to instantiate the ASTVisitor for each translation
/// unit
class ASTConsumer final : public clang::ASTConsumer {
  /// The source manager, used to obtain the location (SourceLocation) of each
  /// declaration
  clang::SourceManager &source_manager;

  /// The user ASTVisitor
  IASTVisitorRef ast_visitor;

  /// The mangler used for C++ symbols
  std::unique_ptr<clang::MangleContext> name_mangler;

 public:
  ASTConsumer(clang::SourceManager &source_manager, IASTVisitorRef ast_visitor,
              std::unique_ptr<clang::MangleContext> name_mangler)
      : source_manager(source_manager),
        ast_visitor(ast_visitor),
        name_mangler(std::move(name_mangler)) {}

  virtual ~ASTConsumer() override = default;

  virtual void HandleTranslationUnit(clang::ASTContext &ast_context) override {
    if (!ast_visitor) {
      return;
    }

    ast_visitor->initialize(&ast_context, &source_manager, name_mangler.get());
    ast_visitor->TraverseDecl(ast_context.getTranslationUnitDecl());
    ast_visitor->finalize();
  }
};
}  // namespace

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

CompilerInstance::Status createClangCompilerInstance(
    std::unique_ptr<clang::CompilerInstance> &compiler,
    const CompilerInstanceSettings &settings, IASTVisitorRef ast_visitor) {
  compiler.reset();

  std::unique_ptr<clang::CompilerInstance> obj;

  try {
    obj = std::make_unique<clang::CompilerInstance>();
  } catch (const std::bad_alloc &) {
    return CompilerInstance::Status(
        false, CompilerInstance::StatusCode::MemoryAllocationFailure);
  }

  auto &header_search_options = obj->getHeaderSearchOpts();
  header_search_options.UseBuiltinIncludes = 0;
  header_search_options.UseStandardSystemIncludes = 0;
  header_search_options.UseStandardCXXIncludes = 0;
  header_search_options.ResourceDir = settings.profile.resource_dir;

  if (settings.language != Language::C && settings.language != Language::CXX) {
    return CompilerInstance::Status(
        false, CompilerInstance::StatusCode::InvalidLanguage);
  }

  stdfs::path profile_root(settings.profile.root_path);

  auto path_list_it = settings.profile.internal_isystem.find(settings.language);
  if (path_list_it != settings.profile.internal_isystem.end()) {
    const auto &path_list = path_list_it->second;

    for (const auto &path : path_list) {
      std::string absolute_path = (profile_root / path).string();
      header_search_options.AddPath(absolute_path,
                                    clang::frontend::IncludeDirGroup::System,
                                    false, false);
    }
  }

  path_list_it =
      settings.profile.internal_externc_isystem.find(settings.language);
  if (path_list_it != settings.profile.internal_externc_isystem.end()) {
    const auto &path_list = path_list_it->second;

    for (const auto &path : path_list) {
      std::string absolute_path = (profile_root / path).string();
      header_search_options.AddPath(
          absolute_path, clang::frontend::IncludeDirGroup::ExternCSystem, false,
          false);
    }
  }

  for (const auto &path : settings.additional_include_folders) {
    try {
      auto absolute_path = stdfs::absolute(path);

      header_search_options.AddPath(absolute_path.string(),
                                    clang::frontend::IncludeDirGroup::System,
                                    false, false);
    } catch (...) {
      std::cerr << "Failed to acquire the absolute path for the following "
                   "include folder: " +
                       path;
    }
  }

  clang::LangOptions &language_options = obj->getLangOpts();
  clang::InputKind input_kind;
  clang::LangStandard::Kind language_standard;

  if (settings.language == Language::CXX) {
    language_options.CPlusPlus = 1;
    language_options.RTTI = 1;
    language_options.CXXExceptions = 1;

    input_kind = kClangFrontendInputKindCxx;

    switch (settings.language_standard) {
      case 98:
        language_standard = clang::LangStandard::lang_cxx98;
        break;

      case 11:
        language_standard = clang::LangStandard::lang_cxx11;
        break;

      case 14:
        language_standard = clang::LangStandard::lang_cxx14;
        break;

      default:
        return CompilerInstance::Status(
            false, CompilerInstance::StatusCode::InvalidLanguageStandard);
    }

  } else {
    language_options.CPlusPlus = 0;
    language_options.RTTI = 0;
    language_options.CXXExceptions = 0;

    input_kind = kClangFrontendInputKindC;

    switch (settings.language_standard) {
      case 89:
        language_standard = clang::LangStandard::lang_c89;
        break;

      case 94:
        language_standard = clang::LangStandard::lang_c94;
        break;

      case 99:
        language_standard = clang::LangStandard::lang_c99;
        break;

      case 11:
        language_standard = clang::LangStandard::lang_c11;
        break;

      default:
        return CompilerInstance::Status(
            false, CompilerInstance::StatusCode::InvalidLanguageStandard);
    }
  }

  language_options.GNUMode = settings.enable_gnu_extensions ? 1 : 0;
  language_options.GNUKeywords = 1;
  language_options.Bool = 1;

  auto &invocation = obj->getInvocation();
  invocation.setLangDefaults(language_options, input_kind,
                             llvm::Triple(llvm::sys::getDefaultTargetTriple()),
                             obj->getPreprocessorOpts(), language_standard);

  std::shared_ptr<clang::TargetOptions> target_options =
      std::make_shared<clang::TargetOptions>();

  target_options->Triple = llvm::sys::getDefaultTargetTriple();

  obj->createDiagnostics();

  clang::TargetInfo *target_information = clang::TargetInfo::CreateTargetInfo(
      obj->getDiagnostics(), target_options);

  obj->setTarget(target_information);

  obj->createFileManager();
  obj->createSourceManager(obj->getFileManager());

  obj->createPreprocessor(clang::TU_Complete);
  obj->getPreprocessorOpts().UsePredefines = false;

  auto &preprocessor = obj->getPreprocessor();
  preprocessor.getBuiltinInfo().initializeBuiltins(
      preprocessor.getIdentifierTable(), language_options);

  auto &source_manager = obj->getSourceManager();

  obj->createASTContext();

  std::unique_ptr<clang::MangleContext> name_mangler;
  if (settings.use_visual_cxx_mangling) {
    name_mangler.reset(clang::MicrosoftMangleContext::create(
        obj->getASTContext(), obj->getDiagnostics()));
  } else {
    name_mangler.reset(clang::ItaniumMangleContext::create(
        obj->getASTContext(), obj->getDiagnostics()));
  }

  if (!name_mangler) {
    return CompilerInstance::Status(
        false, CompilerInstance::StatusCode::MemoryAllocationFailure);
  }

  obj->setASTConsumer(llvm::make_unique<ASTConsumer>(
      source_manager, ast_visitor, std::move(name_mangler)));

  name_mangler.release();

  compiler = std::move(obj);
  obj.release();

  return CompilerInstance::Status(true);
}
