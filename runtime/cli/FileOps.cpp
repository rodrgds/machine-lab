#include "FileOps.hpp"

#include <fstream>

namespace lcom::cli {

bool copyIfExists(const std::filesystem::path &from,
                  const std::filesystem::path &to,
                  std::string &error) {
  std::error_code ec;
  if (!std::filesystem::exists(from, ec)) return true;
  std::filesystem::create_directories(to.parent_path(), ec);
  if (ec) {
    error = ec.message();
    return false;
  }
  std::filesystem::copy_file(from, to, std::filesystem::copy_options::overwrite_existing, ec);
  if (ec) {
    error = ec.message();
    return false;
  }
  return true;
}

bool copyTreeIfExists(const std::filesystem::path &from,
                      const std::filesystem::path &to,
                      std::string &error) {
  std::error_code ec;
  if (!std::filesystem::exists(from, ec)) return true;
  std::filesystem::create_directories(to, ec);
  if (ec) {
    error = ec.message();
    return false;
  }
  std::filesystem::copy(from, to,
                        std::filesystem::copy_options::recursive |
                            std::filesystem::copy_options::overwrite_existing,
                        ec);
  if (ec) {
    error = ec.message();
    return false;
  }
  return true;
}

bool makeExecutable(const std::filesystem::path &path, std::string &error) {
  std::error_code ec;
  std::filesystem::permissions(path,
                               std::filesystem::perms::owner_exec |
                                   std::filesystem::perms::group_exec |
                                   std::filesystem::perms::others_exec,
                               std::filesystem::perm_options::add,
                               ec);
  if (ec) {
    error = "could not make " + path.string() + " executable: " + ec.message();
    return false;
  }
  return true;
}

std::string shellQuote(const std::filesystem::path &path) {
  std::string s = path.string();
  std::string out = "'";
  for (char c : s) {
    if (c == '\'') {
      out += "'\\''";
    } else {
      out.push_back(c);
    }
  }
  out.push_back('\'');
  return out;
}

bool writeTextFile(const std::filesystem::path &path,
                   const std::string &text,
                   bool force,
                   std::string &error) {
  namespace fs = std::filesystem;
  std::error_code ec;
  if (!force && fs::exists(path, ec)) return true;
  fs::create_directories(path.parent_path(), ec);
  if (ec) {
    error = ec.message();
    return false;
  }
  std::ofstream out(path);
  if (!out.is_open()) {
    error = "could not open " + path.string();
    return false;
  }
  out << text;
  out.flush();
  if (!out) {
    error = "could not write " + path.string();
    return false;
  }
  return true;
}

} // namespace lcom::cli
