#pragma once

#include <switch.h>

#include <atomic>
#include <string>
#include <vector>

struct HybridModSummary {
  std::string id;
  std::string name;
  bool isModpack{};
  bool isEnabled{};
  std::string folderName;
};

namespace HybridModManager {
  const std::string CONFIG_PATH = "/switch/Simple_Mod_alchemist/config.json";

  std::string titleIdString(u64 titleId);
  std::vector<HybridModSummary> loadMods(u64 titleId);
  void setSingleModEnabled(u64 titleId, const std::string& modId, bool enabled, std::atomic<float>& progress);
  void applyModpack(u64 titleId, const std::string& modId, std::atomic<float>& progress);
  void disableActiveModpack(u64 titleId, std::atomic<float>& progress);
}
