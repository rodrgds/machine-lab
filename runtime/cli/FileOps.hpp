#ifndef MACHINE_LAB_CLI_FILE_OPS_HPP
#define MACHINE_LAB_CLI_FILE_OPS_HPP

#include <filesystem>
#include <string>

namespace lcom::cli {

bool copyIfExists(const std::filesystem::path &from,
                  const std::filesystem::path &to,
                  std::string &error);
bool copyTreeIfExists(const std::filesystem::path &from,
                      const std::filesystem::path &to,
                      std::string &error);
void makeExecutable(const std::filesystem::path &path);
std::string shellQuote(const std::filesystem::path &path);
bool writeTextFile(const std::filesystem::path &path,
                   const std::string &text,
                   bool force,
                   std::string &error);

} // namespace lcom::cli

#endif
