#pragma once
#include <string>
#include <vector>
#include <set>
#include <regex>
#include "parser.h"

namespace includecpp {
namespace generator {

class TypeResolver {
public:
    struct ParsedType {
        std::string base_name;
        bool is_template = false;
        std::vector<ParsedType> template_args;
        bool is_const = false;
        bool is_reference = false;
        bool is_pointer = false;

        std::string to_cpp_string() const;
        std::string to_python_hint() const;
    };

    static ParsedType parse_type(const std::string& type_str);

    static std::string to_python_hint(const ParsedType& type);
    static std::string to_pybind11_signature(const ParsedType& type);
    static std::string strip_qualifiers(const std::string& type);
    static std::string normalize_type(const std::string& type);

    static bool is_container_type(const std::string& type);
    static bool is_primitive_type(const std::string& type);
    static bool is_numeric_type(const std::string& type);
    static bool requires_custom_converter(const ParsedType& type);

private:
    static std::vector<std::string> split_template_args(const std::string& args);
    static std::string extract_template_content(const std::string& type_str);
    static std::string to_lower(const std::string& str);
};

class ContainerBindings {
public:

    static std::string generate_vector_binding(const std::string& element_type,
                                               const std::string& module_var);

    static std::string generate_map_binding(const std::string& key_type,
                                            const std::string& value_type,
                                            const std::string& module_var);

    static std::string generate_nested_container_binding(const TypeMetadata& type,
                                                         const std::string& module_var);

    static std::set<std::string> find_used_containers(const ModuleDescriptor& module);

private:
    static std::string sanitize_type_name(const std::string& type);
};

}}
