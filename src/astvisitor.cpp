#include "astvisitor.h"
#include "generate_utils.h"
#include "types.h"

#include <algorithm>
#include <queue>

namespace {
/// A node in the type dependency tree
struct TypeDependencyNode final {
  /// The type for this node
  const clang::Type *type{nullptr};

  /// The type referencing this one
  TypeList parents;

  /// The list of types referenced by this node
  TypeList children;
};

/// This node contains the location and name for a given type
struct TypeInformation final {
  /// Type name
  std::string name;

  /// Type location
  SourceCodeLocation location;
};

/// A reference to a type node
using TypeDependencyNodeRef = std::shared_ptr<TypeDependencyNode>;

/// The type dependency tree
using TypeDependencyTree =
    std::unordered_map<const clang::Type *, TypeDependencyNodeRef>;

/// The type information map contains name and location for each type
/// we have found
using TypeInformationMap =
    std::unordered_map<const clang::Type *, TypeInformation>;

/// A map used to tie a function to its dependencies
using FunctionMap = std::unordered_map<clang::FunctionDecl *, TypeList>;
}  // namespace

/// Private class data
struct ASTVisitor::PrivateData final {
  // The AST context received from the ASTConsumer
  clang::ASTContext *ast_context{nullptr};

  /// The source manager, used to acquire source code locations
  clang::SourceManager *source_manager{nullptr};

  /// The name mangler received from the ASTConsumer
  clang::MangleContext *name_mangler{nullptr};

  /// The type dependency tree
  TypeDependencyTree type_dependency_tree;

  /// This variable map functions to their type dependencies
  FunctionMap function_map;

  /// The list of types that we have already enumerated
  TypeList enumerated_type_list;

  /// Name and location for each type we encountered
  TypeInformationMap type_info_map;

  /// The list of blacklisted functions
  BlacklistedFunctionList blacklisted_function_list;

  /// The list of whitelisted functions
  WhitelistedFunctionList whitelisted_function_list;
};

ASTVisitor::ASTVisitor() : d(new PrivateData) {}

bool ASTVisitor::isClassMethod(clang::FunctionDecl *decl) {
  auto method_decl = dynamic_cast<clang::CXXMethodDecl *>(decl);
  return (method_decl != nullptr);
}

clang::CXXRecordDecl *ASTVisitor::getClass(clang::FunctionDecl *decl) {
  auto cxx_record = dynamic_cast<clang::CXXMethodDecl *>(decl);
  assert(cxx_record != nullptr && "Not a class method!");

  return cxx_record->getParent();
}

ClassList ASTVisitor::collectClasses(clang::CXXRecordDecl *decl) {
  ClassList class_list = {decl};

  for (const auto &base_class_specifier : decl->bases()) {
    auto cxx_record_decl = base_class_specifier.getType()->getAsCXXRecordDecl();
    assert(cxx_record_decl != nullptr && "Invalid record decl");

    class_list.insert(cxx_record_decl);
  }

  for (const auto &class_decl : class_list) {
    TypeInformation class_type = {};
    class_type.location =
        getSourceCodeLocation(*d->ast_context, *d->source_manager, class_decl);
    class_type.name = class_decl->getNameAsString();
    d->type_info_map.insert({class_decl->getTypeForDecl(), class_type});
  }

  return class_list;
}

MethodList ASTVisitor::collectClassMethods(const ClassList &class_list) {
  MethodList method_list;

  for (const auto &cxx_record : class_list) {
    for (auto method : cxx_record->methods()) {
      method_list.insert(method);
    }
  }

  return method_list;
}

TypeList ASTVisitor::collectClassMemberTypes(const ClassList &class_list) {
  TypeList type_list;

  for (const auto &cxx_record : class_list) {
    for (auto field : cxx_record->fields()) {
      auto type_source_info = field->getTypeSourceInfo();
      if (type_source_info == nullptr) {
        continue;
      }

      auto type_loc = type_source_info->getTypeLoc();
      auto field_type = type_loc.getTypePtr();

      type_list.insert(field_type);

      TypeInformation type_info = {};
      type_info.location =
          getSourceCodeLocation(*d->ast_context, *d->source_manager, field);
      type_info.name = field->getType().getAsString();
      d->type_info_map.insert({field_type, type_info});
    }
  }

  return type_list;
}

TypeList ASTVisitor::collectRecordMemberTypes(clang::RecordDecl *decl) {
  TypeList type_list;

  for (auto field : decl->fields()) {
    auto type_source_info = field->getTypeSourceInfo();
    if (type_source_info == nullptr) {
      continue;
    }

    auto type_loc = type_source_info->getTypeLoc();
    auto field_type = type_loc.getTypePtr();

    type_list.insert(field_type);

    TypeInformation type_info = {};
    type_info.location =
        getSourceCodeLocation(*d->ast_context, *d->source_manager, field);
    type_info.name = field->getType().getAsString();
    d->type_info_map.insert({field_type, type_info});
  }

  return type_list;
}

TypeList ASTVisitor::collectFunctionParameterTypes(clang::FunctionDecl *decl) {
  TypeList type_list;

  for (const auto &param : decl->parameters()) {
    auto type_source_info = param->getTypeSourceInfo();
    if (type_source_info == nullptr) {
      continue;
    }

    auto type_loc = type_source_info->getTypeLoc();
    auto type = type_loc.getTypePtr();
    if (type == nullptr) {
      throw std::logic_error("Failed to acquire the Type ptr");
    }

    type_list.insert(type);

    TypeInformation type_info = {};
    type_info.location =
        getSourceCodeLocation(*d->ast_context, *d->source_manager, param);

    type_info.name = param->getType().getAsString();
    d->type_info_map.insert({type, type_info});
  }

  return type_list;
}

std::string ASTVisitor::getMangledFunctionName(
    clang::FunctionDecl *function_declaration) {
  std::string function_name;

  if (d->name_mangler->shouldMangleCXXName(function_declaration)) {
    std::string buffer;
    llvm::raw_string_ostream stream(buffer);

    d->name_mangler->mangleName(function_declaration, stream);
    function_name = stream.str();
  } else {
    function_name = function_declaration->getName().str();
  }

  return function_name;
}

std::string ASTVisitor::getFriendlyFunctionName(
    clang::FunctionDecl *function_declaration) {
  std::string class_name;
  if (isClassMethod(function_declaration)) {
    auto class_record = getClass(function_declaration);
    class_name = class_record->getNameAsString();
  }

  if (!function_declaration->getDeclName().isIdentifier()) {
    if (dynamic_cast<clang::CXXConstructorDecl *>(function_declaration) !=
        nullptr) {
      return class_name + " constructor";

    } else if (dynamic_cast<clang::CXXDestructorDecl *>(function_declaration) !=
               nullptr) {
      return class_name + " destructor";

    } else {
      return "<Missing friendly name>";
    }

  } else {
    auto name = function_declaration->getName().str();

    if (!class_name.empty()) {
      return class_name + "::" + name;
    } else {
      return name;
    }
  }
}

bool ASTVisitor::getTypeInformation(std::string &type_name,
                                    SourceCodeLocation &type_location,
                                    const clang::Type *type) {
  type_name.clear();
  type_location = {};

  auto it = d->type_info_map.find(type);
  if (it == d->type_info_map.end()) {
    return false;
  }

  const auto &type_info = it->second;
  type_location = type_info.location;
  type_name = type_info.name;

  return true;
}

ASTVisitor::Status ASTVisitor::create(IASTVisitorRef &ref) {
  ref.reset();

  try {
    auto ptr = new ASTVisitor();
    ref.reset(ptr);
    return Status(true);

  } catch (const std::bad_alloc &) {
    return Status(false, StatusCode::MemoryAllocationFailure);

  } catch (const Status &status) {
    return status;
  }
}

ASTVisitor::~ASTVisitor() {}

void ASTVisitor::initialize(clang::ASTContext *ast_context,
                            clang::SourceManager *source_manager,
                            clang::MangleContext *name_mangler) {
  d->ast_context = ast_context;
  d->source_manager = source_manager;
  d->name_mangler = name_mangler;

  d->type_dependency_tree.clear();
  d->function_map.clear();
  d->enumerated_type_list.clear();
  d->type_info_map.clear();
  d->blacklisted_function_list.clear();
  d->whitelisted_function_list.clear();
}

void ASTVisitor::enumerateTypeDependencies(const TypeList &root_type_list) {
  for (const auto &type : root_type_list) {
    enumerateTypeDependencies(type);
  }
}

void ASTVisitor::enumerateTypeDependencies(const clang::Type *root_type) {
  std::queue<const clang::Type *> queue;
  queue.push(root_type);

  while (!queue.empty()) {
    // Get the next type from the queue
    auto current_type = queue.front();
    queue.pop();

    // Skip this type if we already know about it
    if (d->enumerated_type_list.count(current_type) != 0) {
      continue;
    }

    d->enumerated_type_list.insert(current_type);

    // Get (or create a new one) the node for this type
    TypeDependencyNodeRef current_node_ref;

    auto type_tree_it = d->type_dependency_tree.find(current_type);
    if (type_tree_it == d->type_dependency_tree.end()) {
      current_node_ref = std::make_shared<TypeDependencyNode>();
      current_node_ref->type = current_type;
      d->type_dependency_tree.insert({current_type, current_node_ref});
    } else {
      current_node_ref = type_tree_it->second;
    }

    // Expand the type we have
    std::unordered_set<const clang::Type *> current_type_children = {};

    if (current_type->isPointerType()) {
      // Pointers: get the type they are pointing to
      auto pointee_type = current_type->getPointeeType().getTypePtr();
      current_type_children.insert(pointee_type);

    } else if (current_type->isRecordType()) {
      // Structures (Records): Enumerate the member types and the methods
      TypeList referenced_types;

      if (current_type->getAsCXXRecordDecl() != nullptr) {
        auto cxx_record_decl = current_type->getAsCXXRecordDecl();

        auto class_list = collectClasses(cxx_record_decl);
        auto method_list = collectClassMethods(class_list);

        referenced_types = collectClassMemberTypes(class_list);
        for (const auto &method : method_list) {
          auto parameter_type_list = collectFunctionParameterTypes(method);

          for (const auto &param_type : parameter_type_list) {
            referenced_types.insert(param_type);
          }
        }
      } else {
        auto record_decl = current_type->getAsCXXRecordDecl();
        referenced_types = collectRecordMemberTypes(record_decl);
      }

      for (const auto &child_type : referenced_types) {
        current_type_children.insert(child_type);
      }

    } else if (current_type->getArrayElementTypeNoTypeQual() != nullptr) {
      // Arrays: the the base element type
      auto array_element_type = current_type->getArrayElementTypeNoTypeQual();
      current_type_children.insert(array_element_type);

    } else if (current_type->getAs<clang::TypedefType>() != nullptr) {
      // Type definitions (either with `typedef` or `using`): get the underlying
      // type
      auto typedef_type = current_type->getAs<clang::TypedefType>();

      auto underlying_type =
          typedef_type->getDecl()->getUnderlyingType().getTypePtr();

      current_type_children.insert(underlying_type);

    } else if (!current_type->isCanonicalUnqualified()) {
      auto qual_type = current_type->getCanonicalTypeUnqualified();
      current_type_children.insert(qual_type.getTypePtr());
    }

    // Append the children type we found to the current type; add the child type
    // to the queue only if it is new
    for (const auto &child_type : current_type_children) {
      current_node_ref->children.insert(child_type);

      TypeDependencyNodeRef child_node_ref;
      type_tree_it = d->type_dependency_tree.find(child_type);
      if (type_tree_it != d->type_dependency_tree.end()) {
        child_node_ref = type_tree_it->second;
      } else {
        child_node_ref = std::make_shared<TypeDependencyNode>();
        child_node_ref->type = child_type;

        d->type_dependency_tree.insert({child_type, child_node_ref});

        queue.push(child_type);
      }

      child_node_ref->parents.insert(current_type);
    }
  }
}

bool ASTVisitor::VisitFunctionDecl(clang::FunctionDecl *declaration) {
  // Gather all the referenced types
  TypeList referenced_types;

  if (isClassMethod(declaration)) {
    auto class_list = collectClasses(getClass(declaration));

    referenced_types = collectClassMemberTypes(class_list);
    auto method_list = collectClassMethods(class_list);

    for (const auto &method : method_list) {
      auto parameter_type_list = collectFunctionParameterTypes(method);

      referenced_types.reserve(referenced_types.size() +
                               parameter_type_list.size());

      std::move(parameter_type_list.begin(), parameter_type_list.end(),
                std::inserter(referenced_types, referenced_types.begin()));
    }

  } else {
    referenced_types = collectFunctionParameterTypes(declaration);
  }

  // Build the type dependency tree
  enumerateTypeDependencies(referenced_types);

  // Save this function (or method) along with the first level of
  // type dependencies
  d->function_map.insert({declaration, referenced_types});

  return true;
}

void ASTVisitor::finalize() {
  // clang-format off
  auto L_isFunction = [](const clang::Type *type) -> bool {
    return (
      type->isFunctionNoProtoType() ||
      type->isFunctionPointerType() ||
      type->isFunctionProtoType() ||
      type->isFunctionType()
    );
  };
  // clang-format on

  std::unordered_set<const clang::Type *> blacklisted_type_list;

  for (const auto &p : d->type_dependency_tree) {
    const auto &type = p.first;
    const auto &type_node_ref = p.second;

    // If we already blacklisted this type, skip it
    if (blacklisted_type_list.count(type) != 0) {
      continue;
    }

    // Test whether this type has any child type that should be
    // blacklisted
    std::queue<const clang::Type *> propagation_queue;

    for (const auto &child_type : type_node_ref->children) {
      if (L_isFunction(child_type)) {
        propagation_queue.push(child_type);
      }
    }

    // If this type is bannable or depends on bannable child types, then also
    // add the current type to the propagation queue
    if (!propagation_queue.empty()) {
      propagation_queue.push(type);
    }

    // Add this type if it's blacklistable and we didn't add it already
    if (L_isFunction(type) && propagation_queue.front() != type) {
      propagation_queue.push(type);
    }

    // Blacklist the types we collected, and also propagate the status upward
    TypeList propagated_types;

    while (!propagation_queue.empty()) {
      auto current_type = propagation_queue.front();
      propagation_queue.pop();

      if (propagated_types.count(current_type) != 0) {
        continue;
      }

      propagated_types.insert(current_type);

      blacklisted_type_list.insert(current_type);

      for (const auto &parent_type :
           d->type_dependency_tree.at(current_type)->parents) {
        blacklisted_type_list.insert(parent_type);
        propagation_queue.push(parent_type);

        auto it = d->type_dependency_tree.find(parent_type);
        if (it != d->type_dependency_tree.end()) {
          for (const auto &next_parent : it->second->parents) {
            propagation_queue.push(next_parent);
          }
        }
      }
    }
  }

  // Find duplicated functions
  std::unordered_map<std::string, std::vector<clang::FunctionDecl *>>
      name_to_function_map;

  for (const auto &p : d->function_map) {
    const auto &function_decl = p.first;
    auto function_name = getMangledFunctionName(function_decl);

    auto it = name_to_function_map.find(function_name);
    if (it != name_to_function_map.end()) {
      auto &vec = it->second;
      vec.push_back(function_decl);
    } else {
      name_to_function_map.insert({function_name, {function_decl}});
    }
  }

  for (const auto &p : name_to_function_map) {
    const auto &mangled_function_name = p.first;
    const auto &function_decl_list = p.second;

    if (function_decl_list.size() == 1U) {
      continue;
    }

    auto first_function_decl = function_decl_list.front();
    auto friendly_function_name = getFriendlyFunctionName(first_function_decl);

    auto first_function_location = getSourceCodeLocation(
        *d->ast_context, *d->source_manager, first_function_decl);

    BlacklistedFunction func = {};
    func.location = first_function_location;
    func.mangled_name = mangled_function_name;
    func.friendly_name = friendly_function_name;
    func.reason = BlacklistedFunction::Reason::DuplicateName;

    BlacklistedFunction::DuplicateFunctionLocations locations = {};

    d->function_map.erase(first_function_decl);

    for (auto func_decl_list_it = function_decl_list.begin() + 1;
         func_decl_list_it != function_decl_list.end(); func_decl_list_it++) {
      const auto &next_func_decl = *func_decl_list_it;
      d->function_map.erase(next_func_decl);

      auto next_func_location = getSourceCodeLocation(
          *d->ast_context, *d->source_manager, next_func_decl);
      locations.push_back(next_func_location);
    }

    func.reason_data = locations;
    d->blacklisted_function_list.push_back(func);
  }

  // Filter the remaining functions
  for (const auto &p : d->function_map) {
    const auto &function_decl = p.first;
    const auto &type_dependencies = p.second;

    auto mangled_function_name = getMangledFunctionName(function_decl);
    auto friendly_function_name = getFriendlyFunctionName(function_decl);

    auto function_location = getSourceCodeLocation(
        *d->ast_context, *d->source_manager, function_decl);

    // Search for bad types (function pointers)
    TypeList bad_type_list = {};
    for (const auto &type_dependency : type_dependencies) {
      if (blacklisted_type_list.count(type_dependency) != 0) {
        bad_type_list.insert(type_dependency);
      }
    }

    // List all the types that are related to the function pointer we found
    if (!bad_type_list.empty()) {
      TypeList visited_types;
      TypeList bad_type_queue = bad_type_list;

      while (!bad_type_queue.empty()) {
        TypeList pending_types = {};

        for (const auto &bad_type : bad_type_queue) {
          if (visited_types.count(bad_type) != 0U) {
            continue;
          }

          visited_types.insert(bad_type);

          const auto &type_node_ref = d->type_dependency_tree.at(bad_type);
          for (const auto &child_type : type_node_ref->children) {
            if (blacklisted_type_list.count(child_type) == 0U) {
              continue;
            }

            pending_types.insert(child_type);
          }
        }

        bad_type_queue = pending_types;
      }

      bad_type_list = visited_types;

      BlacklistedFunction func = {};
      func.location = function_location;
      func.friendly_name = friendly_function_name;
      func.mangled_name = mangled_function_name;
      func.reason = BlacklistedFunction::Reason::FunctionPointer;

      BlacklistedFunction::FunctionPointerLocations bad_type_locs = {};

      for (const auto &bad_type : bad_type_list) {
        std::string type_name;
        SourceCodeLocation type_location;
        if (!getTypeInformation(type_name, type_location, bad_type)) {
          continue;
        } else {
          bad_type_locs.push_back(std::make_pair(type_location, type_name));
        }
      }

      if (bad_type_locs.empty()) {
        std::cerr << "Failed to determine the type locations.\n";
      }

      func.reason_data = bad_type_locs;

      d->blacklisted_function_list.push_back(func);
      continue;
    }

    if (function_decl->isVariadic()) {
      BlacklistedFunction func = {};
      func.location = function_location;
      func.friendly_name = friendly_function_name;
      func.mangled_name = mangled_function_name;
      func.reason = BlacklistedFunction::Reason::Variadic;

      d->blacklisted_function_list.push_back(func);
      continue;
    }

    if (function_decl->isTemplated()) {
      BlacklistedFunction func = {};
      func.location = function_location;
      func.friendly_name = friendly_function_name;
      func.mangled_name = mangled_function_name;
      func.reason = BlacklistedFunction::Reason::Templated;

      d->blacklisted_function_list.push_back(func);
      continue;
    }

    WhitelistedFunction func = {};
    func.location = function_location;
    func.friendly_name = friendly_function_name;
    func.mangled_name = mangled_function_name;

    d->whitelisted_function_list.push_back(func);
  }
}

BlacklistedFunctionList ASTVisitor::blacklistedFunctions() const {
  return d->blacklisted_function_list;
}

WhitelistedFunctionList ASTVisitor::whitelistedFunctions() const {
  return d->whitelisted_function_list;
}
