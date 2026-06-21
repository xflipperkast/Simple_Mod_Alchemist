#include "TabHybridMods.h"

#include <HybridModManager.h>
#include <StateAlchemist/controller.h>
#include <loading_dialog.hpp>
#include <note_cell.hpp>
#include <util.hpp>

#include <exception>
#include <thread>

namespace {
  const char* emptyText(bool showModpacks) {
    return showModpacks ? "No modpacks found" : "No mods found";
  }

  const char* emptyNote(bool showModpacks) {
    return showModpacks
      ? "Use staged folders like romfs_[PACK]_Name in /atmosphere/contents/<title_id>/."
      : "Add folders to /switch/Simple_Mod_alchemist/mods/<title_id>/.";
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
    auto* cell = new brls::NoteCell();
    cell->setText(mod.name + (mod.isEnabled ? " (Enabled)" : " (Disabled)"));
    cell->setNote(mod.folderName);
    cell->registerClickAction([this, id = mod.id, isModpack = mod.isModpack, enabled = mod.isEnabled](brls::View*) {
      auto* loading = LoadingDialog::build();
      loading->setAction("Applying mods, please wait...");
      loading->open();

      std::thread([this, loading, id, isModpack, enabled]() {
        std::string error;
        try {
          if (isModpack && enabled) HybridModManager::disableActiveModpack(controller.titleId, loading->getAtomicProgress());
          else if (isModpack) HybridModManager::applyModpack(controller.titleId, id, loading->getAtomicProgress());
          else HybridModManager::setSingleModEnabled(controller.titleId, id, !enabled, loading->getAtomicProgress());
        } catch (const std::exception& e) {
          error = e.what();
        }
        loading->close([this, error]() {
          if (!error.empty()) brls::Application::notify("Apply failed: " + error);
          this->reload();
        });
      }).detach();
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
