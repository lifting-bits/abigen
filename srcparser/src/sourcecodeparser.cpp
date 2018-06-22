#include "sourcecodeparser.h"
#include "asttypecollector.h"
#include "llvm_compatibility.h"

#include <fstream>
#include <sstream>
#include <vector>

#include <clang/Basic/TargetInfo.h>
#include <clang/Basic/TargetOptions.h>
#include <clang/Frontend/TextDiagnosticPrinter.h>
#include <clang/Lex/Preprocessor.h>
#include <clang/Lex/PreprocessorOptions.h>
#include <clang/Parse/ParseAST.h>

namespace trailofbits {
struct SourceCodeParser::PrivateData final {};

SourceCodeParser::SourceCodeParser() : d(new PrivateData) {}

SourceCodeParser::~SourceCodeParser() {}

SourceCodeParser::Status SourceCodeParser::processFile(
    std::vector<FunctionType> &function_type_list,
    std::vector<std::string> &overloaded_functions_blacklisted,
    const std::string &path, const SourceCodeParserSettings &settings) const {
  std::string buffer;

  {
    std::ifstream input_file;
    input_file.open(path, std::ifstream::in | std::ifstream::binary);
    if (!input_file) return Status(false, StatusCode::OpenFailed);

    std::stringstream input_file_stream;
    input_file_stream << input_file.rdbuf();

    buffer = input_file_stream.str();
  }

  return processBuffer(function_type_list, overloaded_functions_blacklisted,
                       buffer, settings);
}

SourceCodeParser::Status SourceCodeParser::processBuffer(
    std::vector<FunctionType> &function_type_list,
    std::vector<std::string> &overloaded_functions_blacklisted,
    const std::string &buffer, const SourceCodeParserSettings &settings) const {
  function_type_list.clear();
  overloaded_functions_blacklisted.clear();

  std::unique_ptr<clang::CompilerInstance> compiler;
  auto status = createCompilerInstance(compiler, settings);
  if (!status.succeeded()) {
    return status;
  }

  compiler->setASTConsumer(llvm::make_unique<ASTTypeCollector>(
      function_type_list, overloaded_functions_blacklisted));
  compiler->createASTContext();

  clang::SourceManager &source_manager = compiler->getSourceManager();

  clang::FileID file_id =
      source_manager.createFileID(llvm::MemoryBuffer::getMemBuffer(
          llvm::StringRef(buffer), llvm::StringRef("main.cpp")));

  source_manager.setMainFileID(file_id);

  std::string clang_output_buffer;
  llvm::raw_string_ostream clang_output_stream(clang_output_buffer);

  clang::DiagnosticsEngine &diagnostics_engine = compiler->getDiagnostics();

  clang::TextDiagnosticPrinter diagnostic_consumer(
      clang_output_stream, &diagnostics_engine.getDiagnosticOptions());

  diagnostics_engine.setClient(&diagnostic_consumer, false);

  clang::Preprocessor &preprocessor = compiler->getPreprocessor();

  diagnostic_consumer.BeginSourceFile(compiler->getLangOpts(), &preprocessor);

  clang::ParseAST(preprocessor, &compiler->getASTConsumer(),
                  compiler->getASTContext());

  diagnostic_consumer.EndSourceFile();

  if (diagnostic_consumer.getNumErrors() != 0) {
    return Status(false, StatusCode::ParsingError, clang_output_buffer);
  }

  return Status(true, StatusCode::SucceededWithWarnings, clang_output_buffer);
}

SourceCodeParser::Status SourceCodeParser::createCompilerInstance(
    std::unique_ptr<clang::CompilerInstance> &compiler,
    const SourceCodeParserSettings &settings) const {
  compiler = nullptr;

  std::unique_ptr<clang::CompilerInstance> obj;
  try {
    obj = std::make_unique<clang::CompilerInstance>();
  } catch (const std::bad_alloc &) {
    return Status(false, StatusCode::MemoryAllocationFailure);
  }

  auto &header_search_options = obj->getHeaderSearchOpts();
  header_search_options.UseBuiltinIncludes = 0;
  header_search_options.UseStandardSystemIncludes = 0;
  header_search_options.UseStandardCXXIncludes = 0;
  header_search_options.ResourceDir = settings.resource_dir;

  for (const auto &path : settings.cxx_system) {
    header_search_options.AddPath(
        path, clang::frontend::IncludeDirGroup::System, false, false);
  }

  for (const auto &path : settings.c_system) {
    header_search_options.AddPath(
        path, clang::frontend::IncludeDirGroup::ExternCSystem, false, false);
  }

  for (const auto &path : settings.additional_include_folders) {
    header_search_options.AddPath(
        path, clang::frontend::IncludeDirGroup::System, false, false);
  }

  clang::LangOptions &language_options = obj->getLangOpts();
  clang::InputKind input_kind;
  clang::LangStandard::Kind language_standard;

  if (settings.cpp) {
    language_options.CPlusPlus = 1;
    language_options.RTTI = 1;
    language_options.CXXExceptions = 1;

    input_kind = kClangFrontendInputKindCxx;

    switch (settings.standard) {
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
        return Status(false, StatusCode::InvalidLanguageStandard);
    }

  } else {
    language_options.CPlusPlus = 0;
    language_options.RTTI = 0;
    language_options.CXXExceptions = 0;

    input_kind = kClangFrontendInputKindC;

    switch (settings.standard) {
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
        return Status(false, StatusCode::InvalidLanguageStandard);
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

  compiler = std::move(obj);
  obj = nullptr;

  return Status(true);
}

SRCPARSER_PUBLICSYMBOL ISourceCodeParser::Status CreateSourceCodeParser(
    std::unique_ptr<ISourceCodeParser> &obj) {
  obj = nullptr;

  try {
    ISourceCodeParser *ptr = new SourceCodeParser();
    obj.reset(ptr);

    return ISourceCodeParser::Status(true);

  } catch (const std::bad_alloc &) {
    return ISourceCodeParser::Status(
        false, ISourceCodeParser::StatusCode::MemoryAllocationFailure);

  } catch (const ISourceCodeParser::Status &status) {
    return status;
  }
}
}  // namespace trailofbits
