//
// Created by Adrien BLANCHET on 22/06/2020.
//

#include "TabModOptions.h"

#include <HybridModManager.h>
#include <StateAlchemist/controller.h>

#include <note_cell.hpp>
#include <util.hpp>


using namespace brls::literals;

TabModOptions::TabModOptions(): brls::Box(brls::Axis::COLUMN) {
  Util::padContent(this);
  this->buildDisableAllMods();
}

void TabModOptions::buildDisableAllMods() {
  brls::NoteCell* disableAll = new brls::NoteCell();
  disableAll->setText("Disable all mods");
  disableAll->setNote(
    "Turn all mods off for this game and move any enabled files back into the mod storage folder. "\
    "This is useful if you want to delete or replace mods on the SD card."
  );

  disableAll->registerClickAction([](brls::View* view) {
    Util::buildConfirmDialog(
      "Disable all mods?",
      "Disabling all mods.",
      [](std::atomic<float>& progress) { HybridModManager::disableAllMods(controller.titleId, progress); }
    )->open();
    return true;
  });

  disableAll->updateActionHint(brls::BUTTON_A, "Disable Mods");

  this->addView(disableAll);
}

TabModOptions* TabModOptions::create() {
  return new TabModOptions();
}


