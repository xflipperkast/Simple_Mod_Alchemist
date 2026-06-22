//
// Created by Nadrino on 03/09/2019.
//

#include <GameBrowser.h>
#include <HybridModManager.h>

#include <switch.h>

#include <StateAlchemist/controller.h>
#include <StateAlchemist/fs_manager.h>
#include <StateAlchemist/meta_manager.h>
#include <StateAlchemist/constants.h>

#include <algorithm>
#include <cstring>
#include <filesystem>
#include <set>

namespace fs = std::filesystem;

GameBrowser gameBrowser;

namespace {
  const std::string HYBRID_MODS_PATH = "/switch/Simple_Mod_alchemist/mods";

  bool startsWith(const std::string& value, const std::string& prefix) {
    return value.rfind(prefix, 0) == 0;
  }

  void addTitleId(std::set<u64>& titleIds, const std::string& folder) {
    if (MetaManager::hasTitleId(folder)) titleIds.insert(MetaManager::parseTitleId(folder));
  }
}

GameBrowser::GameBrowser(){}

// getters
const ConfigHandler &GameBrowser::getConfigHandler() const {
  return _configHandler_;
}
ConfigHandler &GameBrowser::getConfigHandler(){
  return _configHandler_;
}
std::vector<Game> &GameBrowser::getGameList(){
  return _gameList_;
}

std::optional<Game> GameBrowser::getGame(const u64 &titleId_) {
  for (auto& game : _gameList_) {
    if (game.titleId == titleId_) {
      return game;
    }
  }
  
  return std::nullopt;
}

std::string GameBrowser::getOrCreateGamePath(const std::string& titleId) {
  std::optional<Game> game = getGame(MetaManager::getNumericTitleId(titleId));

  if (game == std::nullopt) {
    std::string path = HYBRID_MODS_PATH + "/" + titleId;
    FsManager::createFolderIfNeeded(path);
    return path;
  }

  return game.value().path;
}

// Browse
void GameBrowser::selectGame(const Game& game) {
  controller.setTitleId(game.titleId);
  controller.setGamePath(game.path);
  HybridModManager::loadMods(game.titleId);
}

// protected
void GameBrowser::loadGames() {
  _gameList_.clear();

  std::set<u64> titleIds;
  
  if (fs::is_directory(HYBRID_MODS_PATH)) {
    for (const auto& entry : fs::directory_iterator(HYBRID_MODS_PATH)) {
      if (entry.is_directory()) addTitleId(titleIds, entry.path().filename().string());
    }
  }

  if (fs::is_directory(ATMOSPHERE_PATH)) {
    for (const auto& titleDir : fs::directory_iterator(ATMOSPHERE_PATH)) {
      if (!titleDir.is_directory()) continue;
      bool hasPack = false;
      for (const auto& entry : fs::directory_iterator(titleDir.path())) {
        if (!entry.is_directory()) continue;
        auto name = entry.path().filename().string();
        hasPack = startsWith(name, "romfs_[PACK]_") || startsWith(name, "exefs_[PACK]_") || name == "romfs" || name == "exefs";
        if (hasPack) break;
      }
      if (hasPack) addTitleId(titleIds, titleDir.path().filename().string());
    }
  }

  for (auto titleId : titleIds) {
      auto titleIdText = MetaManager::getHexTitleId(titleId);
      auto game = std::make_unique<Game>(titleId, titleIdText);
      game->path = HYBRID_MODS_PATH + "/" + titleIdText;

      // Load the icon for the game:
      u64 gameDataSize {};
      auto gameData = std::make_unique<NsApplicationControlData>();
      if (
        R_SUCCEEDED(
          nsGetApplicationControlData(
            NsApplicationControlSource_Storage,
            titleId,
            gameData.get(),
            sizeof(NsApplicationControlData),
            &gameDataSize
          )
        )
      ) {
        const auto iconSize = gameDataSize - sizeof(NacpStruct);
        game->icon.resize(iconSize);
        std::memcpy(game->icon.data(), gameData->icon, game->icon.size());
      }

      // Load the title of the game:
      NacpLanguageEntry* nameData;
      if (R_SUCCEEDED(nsGetApplicationDesiredLanguage(&gameData->nacp, &nameData)) && strlen(nameData->name) != 0) {
        game->name = nameData->name;

      }

      _gameList_.push_back(std::move(*game));
  }
}

