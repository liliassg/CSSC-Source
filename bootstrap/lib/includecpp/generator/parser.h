#pragma once
#include <string>
#include <vector>
#include <map>
#include <set>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <filesystem>

struct ParameterInfo {
    std::string name;
    std::string type;
    std::string default_value;
    bool is_const = false;
    bool is_reference = false;
    bool is_pointer = false;
};

struct FunctionBinding {
    std::string module_name;
    std::string function_name;
    std::vector<std::pair<std::string, std::string>> params;
    std::string documentation;

    std::string return_type = "void";
    std::vector<ParameterInfo> parameters;
    bool is_const = false;
    bool is_static = false;
    bool is_inline = false;
    std::string full_signature;

    bool is_template = false;
    std::vector<std::string> template_types;
};

struct MethodSignature {
    std::string name;
    std::string return_type;
    std::vector<ParameterInfo> parameters;
    std::vector<std::string> param_types;
    bool is_const = false;
    bool is_static = false;
    bool is_virtual = false;
    bool is_override = false;
    std::string documentation;
};

struct FieldInfo {
    std::string name;
    std::string type;
    bool is_static = false;
    bool is_const = false;
    bool is_array = false;
    int array_size = 0;
    std::string documentation;
};

struct ConstructorInfo {
    std::vector<std::string> param_types;
};

struct ClassBinding {
    std::string module_name;
    std::string class_name;
    std::vector<std::pair<std::string, std::string>> params;
    std::vector<std::string> methods;
    std::vector<std::string> fields;
    std::vector<ConstructorInfo> constructors;
    bool auto_bind_all;
    std::string documentation;
    std::map<std::string, std::string> method_docs;

    std::vector<MethodSignature> method_signatures;
    std::vector<FieldInfo> field_infos;
};

struct VariableBinding {
    std::string module_name;
    std::string variable_name;
    std::string documentation;
};

struct EnumBinding {
    std::string module_name;
    std::string enum_name;
    std::vector<std::string> values;
    bool export_values = true;
    bool is_class_enum = false;
    std::string documentation;
};

struct StructBinding {
    std::string module_name;
    std::string struct_name;
    std::vector<FieldInfo> fields;
    std::vector<std::string> template_types;
    bool is_template = false;
    std::string documentation;

    std::vector<ConstructorInfo> constructors;
    std::vector<MethodSignature> method_signatures;
    std::vector<std::string> methods;

    std::string get_full_name() const {
        return struct_name;
    }

    std::string get_template_suffix(const std::string& type) const {
        return "_" + type;
    }
};

struct TypeMetadata {
    enum TypeCategory {
        PRIMITIVE,
        STRUCT_TYPE,
        VECTOR_TYPE,
        MAP_TYPE,
        CLASS_TYPE,
        UNKNOWN
    };

    TypeCategory category = UNKNOWN;
    std::string base_type;
    std::string full_signature;
    std::vector<TypeMetadata> template_args;
    bool is_const = false;
    bool is_reference = false;
    bool is_pointer = false;

    std::string to_python_type_hint() const;
    std::string to_pybind11_type() const;
    std::string to_cpp_type() const;
    bool is_container() const {
        return category == VECTOR_TYPE || category == MAP_TYPE;
    }
    bool requires_custom_converter() const {
        return is_container() || category == STRUCT_TYPE;
    }
};

struct ModuleDependency {
    std::string target_module;
    std::vector<std::string> required_types;
    bool is_optional = false;
};

struct ModuleDescriptor {
    std::string module_name;
    std::string source_path;
    std::string header_path;
    bool has_header;
    bool expose_all;

    std::vector<FunctionBinding> functions;
    std::vector<ClassBinding> classes;
    std::vector<StructBinding> structs;
    std::vector<VariableBinding> variables;
    std::vector<EnumBinding> enums;

    std::vector<ModuleDependency> dependencies;
    std::vector<std::string> additional_sources;

    std::string version;
    std::map<std::string, std::string> metadata;
};

class TypeRegistry {
public:

    void register_struct(const std::string& module, const StructBinding& s);
    void register_class(const std::string& module, const ClassBinding& c);
    void register_dependency(const std::string& from_module, const ModuleDependency& dep);

    TypeMetadata resolve_type(const std::string& type_string) const;
    bool is_struct(const std::string& type_name) const;
    bool is_class(const std::string& type_name) const;
    bool type_exists(const std::string& type_name) const;

    std::vector<std::string> get_dependencies(const std::string& module) const;
    std::vector<std::string> get_dependency_order() const;
    bool has_circular_dependency() const;
    std::vector<std::string> get_circular_path() const;

    std::string get_module_for_type(const std::string& type_name) const;
    std::vector<std::string> get_all_modules() const;

private:
    std::map<std::string, StructBinding> structs_;
    std::map<std::string, ClassBinding> classes_;
    std::map<std::string, std::string> type_to_module_;
    std::map<std::string, std::vector<ModuleDependency>> dependencies_;

    std::vector<std::string> topological_sort() const;
    bool has_cycle_dfs(const std::string& module,
                       std::set<std::string>& visited,
                       std::set<std::string>& rec_stack,
                       std::vector<std::string>& path) const;
};

class API
{
public:
    static int main(int argc, char* argv[]);
    static std::vector<ModuleDescriptor> parse_all_cp_files(const std::string& plugins_dir);
    static ModuleDescriptor parse_cp_file(const std::string& filepath);
    static std::string generate_pybind11_code(const std::vector<ModuleDescriptor>& modules);
    static std::string generate_class_bindings(const ClassBinding& cls, const ModuleDescriptor& mod);
    static bool validate_bindings_code(const std::string& code);
    static void validate_module_name(const std::string& name, int line_num);
    static void validate_namespace_includecpp(const std::string& source_path, const std::string& module_name);
    static bool write_files(const std::vector<ModuleDescriptor>& modules,
                           const std::string& bindings_path,
                           const std::string& sources_path);
    static std::string compute_file_hash(const std::string& filepath);
    static std::string generate_registry_json(const std::vector<ModuleDescriptor>& modules, const std::string& plugins_dir);

    static std::vector<FunctionBinding> parse_cpp_function_signatures(const std::string& cpp_file_path);
    static std::vector<ParameterInfo> parse_parameter_list(const std::string& params_str);

private:
    static std::string trim(const std::string& str);
    static std::vector<std::string> split(const std::string& str, char delimiter);
    static std::string extract_between(const std::string& str, char open, char close);
    static std::string safe_extract_between(const std::string& str,
                                           size_t start_pos,
                                           size_t end_pos,
                                           const std::string& context);
    static bool starts_with(const std::string& str, const std::string& prefix);
    static std::string normalize_path(const std::string& path);
    static std::string replace_all(std::string str, const std::string& from, const std::string& to);
    static std::string extract_doc_string(const std::string& str);
    static std::map<std::string, std::string> parse_doc_statements(const std::string& content);
};