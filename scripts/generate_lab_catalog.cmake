if(NOT DEFINED INPUT OR NOT DEFINED OUTPUT)
  message(FATAL_ERROR "INPUT and OUTPUT are required")
endif()

file(READ "${INPUT}" catalog_json)

function(cpp_escape input output)
  string(REPLACE "\\" "\\\\" escaped "${input}")
  string(REPLACE "\"" "\\\"" escaped "${escaped}")
  string(REPLACE "\n" "\\n" escaped "${escaped}")
  set(${output} "${escaped}" PARENT_SCOPE)
endfunction()

function(json_array output json lab field filenames_only)
  string(JSON count LENGTH "${json}" "${lab}" "${field}")
  set(values "")
  if(count GREATER 0)
    math(EXPR last "${count} - 1")
    foreach(index RANGE 0 ${last})
      string(JSON value GET "${json}" "${lab}" "${field}" ${index})
      if(filenames_only)
        get_filename_component(value "${value}" NAME)
      endif()
      cpp_escape("${value}" value)
      if(NOT values STREQUAL "")
        string(APPEND values ", ")
      endif()
      string(APPEND values "\"${value}\"")
    endforeach()
  endif()
  set(${output} "${values}" PARENT_SCOPE)
endfunction()

set(generated "// Generated from course/labs/function-requests.json. Do not edit.\n")
string(APPEND generated "#include \"cli/LabCatalog.hpp\"\n\n")
string(APPEND generated "namespace lcom::cli {\n\n")
string(APPEND generated "const std::vector<StudentLabSpec> &studentLabSpecs() {\n")
string(APPEND generated "  static const std::vector<StudentLabSpec> specs = {\n")

string(JSON lab_count LENGTH "${catalog_json}")
math(EXPR last_lab "${lab_count} - 1")
foreach(index RANGE 0 ${last_lab})
  string(JSON id MEMBER "${catalog_json}" ${index})
  string(JSON device GET "${catalog_json}" "${id}" device)
  string(JSON title GET "${catalog_json}" "${id}" title)
  cpp_escape("${id}" id_cpp)
  cpp_escape("${device}" device_cpp)
  cpp_escape("${title}" title_cpp)
  json_array(aliases "${catalog_json}" "${id}" aliases FALSE)
  json_array(sources "${catalog_json}" "${id}" student_sources TRUE)
  json_array(functions "${catalog_json}" "${id}" functions FALSE)
  string(APPEND generated
    "      {\"${id_cpp}\", \"${device_cpp}\", \"${title_cpp}\", {${aliases}},\n"
    "       {${sources}},\n"
    "       {${functions}}},\n")
endforeach()

string(APPEND generated "  };\n  return specs;\n}\n\n")
string(APPEND generated
  "const StudentLabSpec *studentLabSpec(const std::string &name) {\n"
  "  for (const StudentLabSpec &spec : studentLabSpecs()) {\n"
  "    if (name == spec.id || name == spec.dir) return &spec;\n"
  "    for (const std::string &alias : spec.aliases) {\n"
  "      if (name == alias) return &spec;\n"
  "    }\n"
  "  }\n"
  "  return nullptr;\n"
  "}\n\n} // namespace lcom::cli\n")

file(WRITE "${OUTPUT}" "${generated}")
