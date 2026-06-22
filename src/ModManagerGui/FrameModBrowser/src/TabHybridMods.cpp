#include "TabHybridMods.h"

#include <HybridModManager.h>
#include <StateAlchemist/controller.h>
#include <loading_dialog.hpp>
#include <note_cell.hpp>
#include <util.hpp>

#include <borealis/core/thread.hpp>
#include <borealis/views/cells/cell_detail.hpp>

#include <algorithm>
#include <cctype>
#include <exception>
#include <filesystem>
#include <fstream>
#include <thread>

namespace {
  const char* LOG_PATH = "/switch/Simple_Mod_alchemist/error.log";

  const char* emptyText(bool showModpacks) {
    return showModpacks ? "No modpacks found" : "No mods found";
  }

  const char* emptyNote(bool showModpacks) {
    return showModpacks
      ? "Use staged folders like romfs_[PACK]_Name in /atmosphere/contents/<title_id>/."
      : "Add folders to /switch/Simple_Mod_alchemist/mods/<title_id>/.";
  }

  std::string displayName(std::string name) {
    if (name.rfind("[PACK]_", 0) == 0) name.erase(0, 7);
    std::replace(name.begin(), name.end(), '_', ' ');
    std::transform(name.begin(), name.end(), name.begin(), [](unsigned char c) { return static_cast<char>(std::toupper(c)); });
    return name;
  }

  const char* actionName(bool isModpack, bool enabled) {
    if (isModpack) return enabled ? "disable_modpack" : "apply_modpack";
    return enabled ? "disable_mod" : "enable_mod";
  }

  void logUiStep(const std::string& line) {
    try {
      std::filesystem::create_directories("/switch/Simple_Mod_alchemist");
      std::ofstream out(LOG_PATH, std::ios::app);
      out << line << '\n';
    } catch (...) {
    }
  }

  void resetUiLog() {
    try {
      std::filesystem::create_directories("/switch/Simple_Mod_alchemist");
      std::ofstream(LOG_PATH, std::ios::trunc);
    } catch (...) {
    }
  }

  void logApplyError(const char* action, u64 titleId, const std::string& id, const std::string& error) {
    logUiStep("ui action=" + std::string(action) + " title_id=" + HybridModManager::titleIdString(titleId) + " mod_id=" + id + " error=" + error);
  }
}

TabHybridMods::TabHybridMods(bool showModpacks): brls::Box(brls::Axis::COLUMN), showModpacks(showModpacks) {
  Util::padContent(this);
  this->reload();
}

void TabHybridMods::reload() {
  this->clearViews();

  auto mods = HybridModManager::loadMods(controller.titleId);
  bool hasVisibleMods = false;
  for (const auto& mod : mods) {
    if (mod.isModpack == this->showModpacks) hasVisibleMods = true;
  }
  if (!hasVisibleMods) {
    auto* note = new brls::NoteCell();
    note->setText(emptyText(this->showModpacks));
    note->setNote(emptyNote(this->showModpacks));
    note->setFocusable(false);
    this->addView(note);
    return;
  }

  for (const auto& mod : mods) {
    if (mod.isModpack != this->showModpacks) continue;
    auto* cell = new brls::DetailCell();
    cell->setText(displayName(mod.name));
    cell->setDetailText(mod.isEnabled ? "Enabled" : "Disabled");
    cell->setDetailTextColor(brls::Application::getTheme()[mod.isEnabled ? "brls/list/listItem_value_color" : "brls/text_disabled"]);
    cell->registerClickAction([this, id = mod.id, isModpack = mod.isModpack, enabled = mod.isEnabled](brls::View*) {
      resetUiLog();
      logUiStep("ui reset log title_id=" + HybridModManager::titleIdString(controller.titleId));
      logUiStep("ui click begin title_id=" + HybridModManager::titleIdString(controller.titleId) + " mod_id=" + id + " pack=" + std::string(isModpack ? "true" : "false") + " enabled=" + std::string(enabled ? "true" : "false"));
      auto* loading = LoadingDialog::build();
      logUiStep("ui loading built title_id=" + HybridModManager::titleIdString(controller.titleId));
      loading->setAction("Applying mods, please wait...");
      logUiStep("ui loading action set title_id=" + HybridModManager::titleIdString(controller.titleId));
      loading->open();
      logUiStep("ui loading open returned title_id=" + HybridModManager::titleIdString(controller.titleId));
      std::string error;
      auto action = actionName(isModpack, enabled);
      logUiStep("ui sync action begin title_id=" + HybridModManager::titleIdString(controller.titleId) + " action=" + action + " mod_id=" + id);
      try {
        if (isModpack && enabled) HybridModManager::disableActiveModpack(controller.titleId, loading->getAtomicProgress());
        else if (isModpack) HybridModManager::applyModpack(controller.titleId, id, loading->getAtomicProgress());
        else HybridModManager::setSingleModEnabled(controller.titleId, id, !enabled, loading->getAtomicProgress());
      } catch (const std::exception& e) {
        error = e.what();
        logApplyError(action, controller.titleId, id, error);
      }
      logUiStep("ui sync action end title_id=" + HybridModManager::titleIdString(controller.titleId) + " action=" + action + " mod_id=" + id + " error=" + (error.empty() ? "none" : error));
      auto finish = [this, error]() {
        logUiStep("ui finish start title_id=" + HybridModManager::titleIdString(controller.titleId) + " error=" + (error.empty() ? "none" : error));
        if (!error.empty()) brls::Application::notify("Apply failed: " + error);
        this->reload();
        brls::Application::giveFocus(this->getDefaultFocus());
        logUiStep("ui finish end title_id=" + HybridModManager::titleIdString(controller.titleId));
      };
      logUiStep("ui close request title_id=" + HybridModManager::titleIdString(controller.titleId));
      loading->close(finish);
      logUiStep("ui close returned title_id=" + HybridModManager::titleIdString(controller.titleId));
      logUiStep("ui click end title_id=" + HybridModManager::titleIdString(controller.titleId) + " mod_id=" + id);
      return true;
    });
    cell->updateActionHint(brls::BUTTON_A, mod.isEnabled ? "Disable" : (mod.isModpack ? "Apply" : "Enable"));
    this->addView(cell);
  }
}

brls::View* TabHybridMods::createMods() {
  return new TabHybridMods(false);
}

brls::View* TabHybridMods::createModpacks() {
  return new TabHybridMods(true);
}
