#ifndef MACHINE_LAB_CLI_LAB_CATALOG_HPP
#define MACHINE_LAB_CLI_LAB_CATALOG_HPP

#include <string>
#include <vector>

namespace lcom::cli {

struct StudentLabSpec {
  std::string id;
  std::string dir;
  std::string title;
  std::vector<std::string> aliases;
  std::vector<std::string> sources;
  std::vector<std::string> functions;
};

const std::vector<StudentLabSpec> &studentLabSpecs();
const StudentLabSpec *studentLabSpec(const std::string &name);

} // namespace lcom::cli

#endif
