#include "sourcecodeparser.h"
#include "translationunithandler.h"

#include <fstream>
#include <sstream>

#include <clang/AST/ASTContext.h>
#include <clang/AST/PrettyPrinter.h>
#include <clang/AST/RecursiveASTVisitor.h>
#include <clang/Basic/TargetInfo.h>
#include <clang/Frontend/CompilerInstance.h>
#include <clang/Frontend/TextDiagnosticPrinter.h>
#include <clang/Lex/DirectoryLookup.h>
#include <clang/Lex/PreprocessorOptions.h>
#include <clang/Parse/ParseAST.h>

namespace trailofbits {
struct SourceCodeParser::PrivateData final {
  clang::CompilerInstance compiler;
};

SourceCodeParser::SourceCodeParser(
    const std::vector<std::string> &additional_include_dirs)
    : d(new PrivateData) {
  auto &header_search_options = d->compiler.getHeaderSearchOpts();
  for (const auto &path : additional_include_dirs) {
    header_search_options.AddPath(
        path, clang::frontend::IncludeDirGroup::Angled, false, true);
  }

  clang::LangOptions language_options;
  language_options.CPlusPlus = 1;
  language_options.GNUMode = 1;
  language_options.RTTI = 1;
  language_options.Bool = 1;
  language_options.CXXExceptions = 1;

  auto &invocation = d->compiler.getInvocation();
  invocation.setLangDefaults(language_options, clang::InputKind::CXX,
                             llvm::Triple(llvm::sys::getDefaultTargetTriple()),
                             d->compiler.getPreprocessorOpts(),
                             clang::LangStandard::lang_cxx98);

  std::shared_ptr<clang::TargetOptions> target_options =
      std::make_shared<clang::TargetOptions>();
  target_options->Triple = llvm::sys::getDefaultTargetTriple();

  d->compiler.createDiagnostics();
  clang::TargetInfo *target_information = clang::TargetInfo::CreateTargetInfo(
      d->compiler.getDiagnostics(), target_options);
  d->compiler.setTarget(target_information);

  d->compiler.createFileManager();
  d->compiler.createSourceManager(d->compiler.getFileManager());

  d->compiler.createPreprocessor(clang::TU_Complete);
  d->compiler.getPreprocessorOpts().UsePredefines = false;

  d->compiler.setASTConsumer(
      llvm::make_unique<TranslationUnitHandler>(d->compiler));
  d->compiler.createASTContext();
}

SourceCodeParser::~SourceCodeParser() {}

SourceCodeParser::Status SourceCodeParser::processFile(
    const std::string &path) const {
  std::string buffer;

  {
    std::ifstream input_file;
    input_file.open(path, std::ifstream::in | std::ifstream::binary);
    if (!input_file) return Status(false, StatusCode::OpenFailed);

    std::stringstream input_file_stream;
    input_file_stream << input_file.rdbuf();

    buffer = input_file_stream.str();
  }

  return processBuffer(buffer);
}

SourceCodeParser::Status SourceCodeParser::processBuffer(
    const std::string &buffer) const {
  clang::SourceManager &source_manager = d->compiler.getSourceManager();
  source_manager.clearIDTables();

  clang::FileID file_id =
      source_manager.createFileID(llvm::MemoryBuffer::getMemBuffer(
          llvm::StringRef(buffer), llvm::StringRef("main.cpp")));
  source_manager.setMainFileID(file_id);

  std::string clang_output_buffer;
  llvm::raw_string_ostream clang_output_stream(clang_output_buffer);

  clang::DiagnosticsEngine &diagnostics_engine = d->compiler.getDiagnostics();
  clang::TextDiagnosticPrinter diagnostic_consumer(
      clang_output_stream, &diagnostics_engine.getDiagnosticOptions());
  diagnostics_engine.setClient(&diagnostic_consumer, false);

  clang::Preprocessor &preprocessor = d->compiler.getPreprocessor();

  diagnostic_consumer.BeginSourceFile(d->compiler.getLangOpts(), &preprocessor);
  clang::ParseAST(preprocessor, &d->compiler.getASTConsumer(),
                  d->compiler.getASTContext());
  diagnostic_consumer.EndSourceFile();

  if (diagnostic_consumer.getNumErrors() != 0) {
    return Status(false, StatusCode::ParsingError, clang_output_buffer);
  }

  return Status(true, StatusCode::SucceededWithWarnings, clang_output_buffer);
}

SRCPARSER_PUBLICSYMBOL ISourceCodeParser::Status CreateSourceCodeParser(
    std::unique_ptr<ISourceCodeParser> &obj,
    const std::vector<std::string> &additional_include_dirs) {
  obj = nullptr;

  try {
    ISourceCodeParser *ptr = new SourceCodeParser(additional_include_dirs);
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
