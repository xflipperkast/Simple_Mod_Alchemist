#include "HybridModManager.h"

#include <borealis/extern/nlohmann/json.hpp>

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <map>
#include <sstream>
#include <stdexcept>

namespace fs = std::filesystem;
using json = nlohmann::json;

namespace {
  const std::vector<std::string> LAYERS = {"romfs", "exefs"};
  const fs::path LOG_PATH = "/switch/Simple_Mod_alchemist/error.log";

  fs::path sdPath(const std::string& value) {
    if (value.rfind("sd:/", 0) == 0) return fs::path("/") / value.substr(4);
    if (!value.empty() && value[0] == '/') return fs::path(value);
    return fs::path("/") / value;
  }

  void logLine(const std::string& line) {
    try {
      fs::create_directories(LOG_PATH.parent_path());
      std::ofstream out(LOG_PATH, std::ios::app);
      out << line << '\n';
    } catch (...) {
    }
  }

  void logStep(const char* step, u64 titleId, const std::string& detail) {
    logLine(std::string(step) + " title_id=" + HybridModManager::titleIdString(titleId) + " " + detail);
  }

  void moveFile(const fs::path& source, const fs::path& target) {
    fs::create_directories(target.parent_path());
    if (fs::exists(target)) fs::remove(target);
    fs::rename(source, target);
  }

  json loadConfig() {
    if (!fs::exists(HybridModManager::CONFIG_PATH)) {
      logLine("config load: missing " + HybridModManager::CONFIG_PATH + ", using empty config");
      return json{{"games", json::object()}};
    }
    json cfg;
    try {
      std::ifstream in(HybridModManager::CONFIG_PATH);
      in >> cfg;
    } catch (...) {
      logLine("config load: failed to parse " + HybridModManager::CONFIG_PATH + ", using empty config");
      cfg = json{{"games", json::object()}};
    }
    if (!cfg.contains("games") || !cfg["games"].is_object()) cfg["games"] = json::object();
    return cfg;
  }

  void saveConfig(const json& cfg) {
    fs::create_directories(fs::path(HybridModManager::CONFIG_PATH).parent_path());
    std::ofstream(HybridModManager::CONFIG_PATH) << cfg.dump(2) << '\n';
    logLine("config save: " + HybridModManager::CONFIG_PATH);
  }

  bool startsWith(const std::string& value, const std::string& prefix) {
    return value.rfind(prefix, 0) == 0;
  }

  fs::path modsRoot(u64 titleId) {
    return sdPath("switch/Simple_Mod_alchemist/mods") / HybridModManager::titleIdString(titleId);
  }

  std::string idFromFolder(const std::string& folder, bool isPack) {
    auto value = startsWith(folder, "[PACK]_") ? folder.substr(7) : folder;
    std::string out = isPack ? "pack_" : "";
    for (auto c : value) out += std::isalnum(static_cast<unsigned char>(c)) ? static_cast<char>(std::tolower(c)) : '_';
    return out;
  }

  std::string nameFromFolder(const std::string& folder) {
    auto name = startsWith(folder, "[PACK]_") ? folder.substr(7) : folder;
    std::replace(name.begin(), name.end(), '_', ' ');
    return name;
  }

  std::string modFolder(const json& mod) {
    auto folder = mod.value("root_folder", "");
    if (folder.empty()) folder = mod.value("folder_name", "");
    return folder;
  }

  std::vector<std::string> layersIn(const fs::path& root) {
    std::vector<std::string> layers;
    for (const auto& layer : LAYERS) {
      if (fs::is_directory(root / layer)) layers.push_back(layer);
    }
    return layers;
  }

  std::vector<std::string> packLayers(const fs::path& sourceRoot, const fs::path& contents, const std::string& pack) {
    auto layers = layersIn(sourceRoot);
    for (const auto& layer : LAYERS) {
      if (fs::is_directory(contents / (layer + "_" + pack)) && std::find(layers.begin(), layers.end(), layer) == layers.end()) {
        layers.push_back(layer);
      }
    }
    return layers;
  }

  json syncConfig(u64 titleId) {
    logStep("syncConfig:start", titleId, "root=" + modsRoot(titleId).string());
    auto cfg = loadConfig();
    auto key = HybridModManager::titleIdString(titleId);
    auto root = modsRoot(titleId);
    fs::create_directories(root);

    auto& game = cfg["games"][key];
    if (!game.is_object()) game = json::object();
    game["game_name"] = game.value("game_name", key);

    std::map<std::string, json> oldMods;
    if (game.contains("mods") && game["mods"].is_array()) {
      for (auto& mod : game["mods"]) oldMods[modFolder(mod)] = mod;
    }

    std::vector<std::string> folders;
    auto addFolder = [&](const std::string& folder) {
      if (std::find(folders.begin(), folders.end(), folder) == folders.end()) folders.push_back(folder);
    };
    for (const auto& entry : fs::directory_iterator(root)) {
      if (entry.is_directory()) addFolder(entry.path().filename().string());
    }
    auto contents = sdPath("atmosphere/contents") / key;
    logStep("syncConfig:scan", titleId, "contents=" + contents.string() + " root_folders=" + std::to_string(folders.size()));
    if (fs::is_directory(contents)) {
      for (const auto& entry : fs::directory_iterator(contents)) {
        if (!entry.is_directory()) continue;
        auto name = entry.path().filename().string();
        for (const auto& layer : LAYERS) {
          auto prefix = layer + "_";
          if (startsWith(name, prefix) && startsWith(name.substr(prefix.size()), "[PACK]_")) addFolder(name.substr(prefix.size()));
        }
      }
    }
    std::sort(folders.begin(), folders.end());

    bool hasLiveLayer = std::any_of(LAYERS.begin(), LAYERS.end(), [&](const std::string& layer) {
      return fs::exists(contents / layer);
    });
    auto active = game.value("active_modpack_folder", game.value("active_modpack", ""));
    if (!active.empty() && std::find(folders.begin(), folders.end(), active) == folders.end() && !hasLiveLayer) active.clear();
    if (active.empty()) {
      for (const auto& folder : folders) {
        auto old = oldMods.find(folder);
        if (startsWith(folder, "[PACK]_") && old != oldMods.end() && old->second.value("is_enabled", false)) {
          active = folder;
          break;
        }
      }
    }
    if (active.empty() && hasLiveLayer) {
      for (const auto& folder : folders) {
        if (!startsWith(folder, "[PACK]_")) continue;
        bool stagedMissing = std::any_of(LAYERS.begin(), LAYERS.end(), [&](const std::string& layer) {
          return fs::exists(contents / layer) && !fs::exists(contents / (layer + "_" + folder));
        });
        if (stagedMissing) {
          active = folder;
          break;
        }
      }
    }
    if (!active.empty() && startsWith(active, "[PACK]_")) addFolder(active);
    std::sort(folders.begin(), folders.end());

    game["mods"] = json::array();
    for (const auto& folder : folders) {
      bool isPack = startsWith(folder, "[PACK]_");
      auto old = oldMods.find(folder);
      json mod = old == oldMods.end() ? json::object() : old->second;
      mod["id"] = mod.value("id", idFromFolder(folder, isPack));
      mod["name"] = nameFromFolder(folder);
      mod["is_modpack"] = isPack;
      mod["is_enabled"] = isPack ? folder == active : mod.value("is_enabled", false);
      mod["root_folder"] = folder;
      mod.erase("folder_name");
      mod.erase("storage_path");
      game["mods"].push_back(mod);
      logStep("syncConfig:mod", titleId, "folder=" + folder + " id=" + mod["id"].get<std::string>() + " pack=" + std::string(isPack ? "true" : "false") + " enabled=" + std::string(mod["is_enabled"].get<bool>() ? "true" : "false"));
    }
    game["active_modpack_folder"] = active;
    game.erase("active_modpack");
    logStep("syncConfig:done", titleId, "active=" + active + " mods=" + std::to_string(game["mods"].size()));
    saveConfig(cfg);
    return cfg;
  }

  json& gameConfig(json& cfg, u64 titleId) {
    return cfg["games"][HybridModManager::titleIdString(titleId)];
  }

  fs::path singleRoot(u64 titleId, const json& mod) {
    return modsRoot(titleId) / modFolder(mod);
  }

  std::vector<json*> enabledSingleMods(json& game) {
    std::vector<json*> mods;
    for (auto& mod : game["mods"]) {
      if (!mod.value("is_modpack", false) && mod.value("is_enabled", false)) {
        mods.push_back(&mod);
      }
    }
    return mods;
  }

  bool safeRelativePath(const fs::path& path) {
    if (path.is_absolute()) return false;
    return std::none_of(path.begin(), path.end(), [](const fs::path& part) { return part == ".."; });
  }

  std::vector<fs::path> sourceFiles(const fs::path& root) {
    std::vector<fs::path> files;
    for (const auto& layer : layersIn(root)) {
      auto source = root / layer;
      for (const auto& entry : fs::recursive_directory_iterator(source)) {
        if (entry.is_regular_file()) files.push_back(fs::path(layer) / fs::relative(entry.path(), source));
      }
    }
    return files;
  }

  std::vector<fs::path> movedFiles(const json& mod) {
    std::vector<fs::path> files;
    if (!mod.contains("enabled_files") || !mod["enabled_files"].is_array()) return files;
    for (const auto& value : mod["enabled_files"]) {
      if (!value.is_string()) continue;
      fs::path file(value.get<std::string>());
      if (safeRelativePath(file)) files.push_back(file);
    }
    return files;
  }

  void pruneEmptyLayers(const fs::path& contents) {
    for (const auto& layer : LAYERS) {
      auto root = contents / layer;
      if (!fs::exists(root)) continue;
      std::vector<fs::path> dirs;
      for (const auto& entry : fs::recursive_directory_iterator(root)) {
        if (entry.is_directory()) dirs.push_back(entry.path());
      }
      for (auto it = dirs.rbegin(); it != dirs.rend(); ++it) {
        if (fs::exists(*it) && fs::is_empty(*it)) fs::remove(*it);
      }
      if (fs::exists(root) && fs::is_empty(root)) fs::remove(root);
    }
  }

  void removeSingleFiles(const fs::path& contents, const std::vector<json*>& singleMods, u64 titleId) {
    logLine("removeSingleFiles: contents=" + contents.string() + " mods=" + std::to_string(singleMods.size()));
    std::vector<fs::path> dirs;
    for (auto* mod : singleMods) {
      auto root = singleRoot(titleId, *mod);
      auto files = movedFiles(*mod);
      if (files.empty()) files = sourceFiles(root);
      for (const auto& file : files) {
        auto target = contents / file;
        auto source = root / file;
        if (fs::exists(target)) {
          logLine("removeSingleFiles: move " + target.string() + " -> " + source.string());
          moveFile(target, source);
          dirs.push_back(target.parent_path());
        }
        auto backup = target;
        backup += ".bak";
        if (fs::exists(backup)) {
          logLine("removeSingleFiles: restore " + backup.string() + " -> " + target.string());
          fs::rename(backup, target);
        }
      }
      (*mod)["enabled_files"] = json::array();
    }
    for (auto it = dirs.rbegin(); it != dirs.rend(); ++it) {
      if (fs::exists(*it) && fs::is_empty(*it)) fs::remove(*it);
    }
    pruneEmptyLayers(contents);
  }

  int countFiles(const std::vector<json*>& singleMods, u64 titleId) {
    int count = 0;
    for (const auto* mod : singleMods) {
      count += sourceFiles(singleRoot(titleId, *mod)).size();
    }
    return count;
  }

  fs::path stagedLayer(const fs::path& contents, const std::string& layer, const std::string& pack) {
    return contents / (layer + "_" + pack);
  }

  void ensureLayerStaged(const fs::path& contents, const fs::path& packRoot, const std::string& pack, const std::string& layer) {
    auto staged = stagedLayer(contents, layer, pack);
    if (fs::exists(staged)) {
      logLine("ensureLayerStaged: already staged " + staged.string());
      return;
    }
    if (!fs::exists(packRoot / layer)) throw std::runtime_error("Selected modpack layer is missing.");
    logLine("ensureLayerStaged: copy " + (packRoot / layer).string() + " -> " + staged.string());
    fs::copy(packRoot / layer, staged, fs::copy_options::recursive);
  }

  void activateLayer(const fs::path& contents, const std::string& layer, const std::string& pack) {
    auto live = contents / layer;
    if (!fs::exists(live)) {
      auto staged = stagedLayer(contents, layer, pack);
      logLine("activateLayer: rename " + staged.string() + " -> " + live.string());
      fs::rename(staged, live);
    }
  }

  void detachCurrentPack(const fs::path& contents, const std::string& currentPack) {
    if (currentPack.empty()) {
      for (const auto& layer : LAYERS) {
        if (fs::exists(contents / layer)) throw std::runtime_error("active_modpack_folder is empty.");
      }
      return;
    }
    for (const auto& layer : LAYERS) {
      auto live = contents / layer;
      if (!fs::exists(live)) continue;
      auto staged = stagedLayer(contents, layer, currentPack);
      if (fs::exists(staged)) throw std::runtime_error("Current modpack staging folder already exists.");
      logLine("detachCurrentPack: rename " + live.string() + " -> " + staged.string());
      fs::rename(live, staged);
    }
  }

  void moveSingles(const fs::path& contents, const std::vector<json*>& singleMods, u64 titleId, std::atomic<float>& progress) {
    int done = 0;
    int total = countFiles(singleMods, titleId);
    logLine("moveSingles: mods=" + std::to_string(singleMods.size()) + " files=" + std::to_string(total) + " contents=" + contents.string());
    for (auto* mod : singleMods) {
      auto root = singleRoot(titleId, *mod);
      (*mod)["enabled_files"] = json::array();
      for (const auto& file : sourceFiles(root)) {
        auto source = root / file;
        auto target = contents / file;
        if (fs::exists(target)) {
          auto backup = target;
          backup += ".bak";
          if (!fs::exists(backup)) {
            logLine("moveSingles: backup " + target.string() + " -> " + backup.string());
            moveFile(target, backup);
          }
        }
        logLine("moveSingles: move " + source.string() + " -> " + target.string());
        moveFile(source, target);
        (*mod)["enabled_files"].push_back(file.generic_string());
        progress.store(total == 0 ? 1.0f : static_cast<float>(++done) / total);
      }
    }
  }

  json::iterator findMod(json& mods, const std::string& modId, bool isModpack) {
    return std::find_if(mods.begin(), mods.end(), [&](const json& mod) {
      return mod.value("is_modpack", false) == isModpack && mod.value("id", "") == modId;
    });
  }
}

std::string HybridModManager::titleIdString(u64 titleId) {
  std::ostringstream out;
  out << std::hex << std::nouppercase << std::setw(16) << std::setfill('0') << titleId;
  return out.str();
}

std::vector<HybridModSummary> HybridModManager::loadMods(u64 titleId) {
  auto cfg = syncConfig(titleId);
  auto key = titleIdString(titleId);
  if (!cfg["games"].contains(key) || !cfg["games"][key].contains("mods")) return {};

  std::vector<HybridModSummary> mods;
  for (auto& mod : cfg["games"][key]["mods"]) {
    mods.push_back({
      mod.value("id", ""),
      mod.value("name", mod.value("id", "")),
      mod.value("is_modpack", false),
      mod.value("is_enabled", false),
      mod.value("root_folder", "")
    });
  }
  return mods;
}

void HybridModManager::applyModpack(u64 titleId, const std::string& modId, std::atomic<float>& progress) {
  logStep("applyModpack:start", titleId, "mod_id=" + modId);
  auto cfg = syncConfig(titleId);
  auto& game = gameConfig(cfg, titleId);
  auto& mods = game["mods"];
  auto selected = findMod(mods, modId, true);
  if (selected == mods.end()) throw std::runtime_error("Selected modpack was not found.");

  auto title = titleIdString(titleId);
  auto contents = sdPath("atmosphere/contents") / title;
  auto sourceRoot = modsRoot(titleId);
  auto currentPack = game.value("active_modpack_folder", "");
  auto targetPack = selected->value("root_folder", "");
  if (targetPack.empty()) throw std::runtime_error("Selected modpack has no root_folder.");

  auto singles = enabledSingleMods(game);
  auto targetRoot = sourceRoot / targetPack;
  auto targetLayers = packLayers(targetRoot, contents, targetPack);
  logStep("applyModpack:plan", titleId, "current=" + currentPack + " target=" + targetPack + " singles=" + std::to_string(singles.size()) + " layers=" + std::to_string(targetLayers.size()));
  if (targetLayers.empty()) throw std::runtime_error("Selected modpack has no romfs/exefs folder.");

  fs::create_directories(contents);
  removeSingleFiles(contents, singles, titleId);

  bool switching = currentPack != targetPack;
  if (switching) {
    for (const auto& layer : targetLayers) ensureLayerStaged(contents, targetRoot, targetPack, layer);
    detachCurrentPack(contents, currentPack);
  } else {
    for (const auto& layer : targetLayers) {
      if (!fs::exists(contents / layer)) ensureLayerStaged(contents, targetRoot, targetPack, layer);
    }
  }
  for (const auto& layer : targetLayers) activateLayer(contents, layer, targetPack);

  moveSingles(contents, singles, titleId, progress);

  game["active_modpack_folder"] = targetPack;
  for (auto& mod : mods) {
    if (mod.value("is_modpack", false)) {
      mod["is_enabled"] = mod.value("id", "") == modId;
    }
  }
  saveConfig(cfg);
  progress.store(1.0f);
  logStep("applyModpack:done", titleId, "mod_id=" + modId);
}

void HybridModManager::setSingleModEnabled(u64 titleId, const std::string& modId, bool enabled, std::atomic<float>& progress) {
  logStep("setSingleModEnabled:start", titleId, "mod_id=" + modId + " enabled=" + std::string(enabled ? "true" : "false"));
  auto cfg = syncConfig(titleId);
  auto& game = gameConfig(cfg, titleId);
  auto& mods = game["mods"];
  auto selected = findMod(mods, modId, false);
  if (selected == mods.end()) throw std::runtime_error("Selected mod was not found.");

  auto contents = sdPath("atmosphere/contents") / titleIdString(titleId);
  std::vector<json*> single{&(*selected)};
  fs::create_directories(contents);

  if (enabled) {
    moveSingles(contents, single, titleId, progress);
  } else {
    removeSingleFiles(contents, single, titleId);
  }

  (*selected)["is_enabled"] = enabled;
  saveConfig(cfg);
  progress.store(1.0f);
  logStep("setSingleModEnabled:done", titleId, "mod_id=" + modId + " enabled=" + std::string(enabled ? "true" : "false"));
}

void HybridModManager::disableActiveModpack(u64 titleId, std::atomic<float>& progress) {
  logStep("disableActiveModpack:start", titleId, "");
  auto cfg = syncConfig(titleId);
  auto& game = gameConfig(cfg, titleId);
  auto& mods = game["mods"];
  auto currentPack = game.value("active_modpack_folder", "");
  if (currentPack.empty()) return;

  auto contents = sdPath("atmosphere/contents") / titleIdString(titleId);
  auto singles = enabledSingleMods(game);
  removeSingleFiles(contents, singles, titleId);
  detachCurrentPack(contents, currentPack);
  moveSingles(contents, singles, titleId, progress);

  game["active_modpack_folder"] = "";
  for (auto& mod : mods) {
    if (mod.value("is_modpack", false)) mod["is_enabled"] = false;
  }
  saveConfig(cfg);
  progress.store(1.0f);
  logStep("disableActiveModpack:done", titleId, "current=" + currentPack);
}

void HybridModManager::disableAllMods(u64 titleId, std::atomic<float>& progress) {
  logStep("disableAllMods:start", titleId, "");
  auto cfg = syncConfig(titleId);
  auto& game = gameConfig(cfg, titleId);
  auto& mods = game["mods"];
  auto currentPack = game.value("active_modpack_folder", "");
  auto contents = sdPath("atmosphere/contents") / titleIdString(titleId);
  auto singles = enabledSingleMods(game);

  fs::create_directories(contents);
  if (!singles.empty()) {
    removeSingleFiles(contents, singles, titleId);
  }
  if (!currentPack.empty()) {
    detachCurrentPack(contents, currentPack);
  }

  game["active_modpack_folder"] = "";
  for (auto& mod : mods) {
    mod["is_enabled"] = false;
    if (!mod.value("is_modpack", false)) {
      mod["enabled_files"] = json::array();
    }
  }

  saveConfig(cfg);
  progress.store(1.0f);
  logStep("disableAllMods:done", titleId, "");
}
