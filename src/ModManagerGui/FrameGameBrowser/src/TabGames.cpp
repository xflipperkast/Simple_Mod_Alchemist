//
// Created by Adrien BLANCHET on 21/06/2020.
//

#include "TabGames.h"
#include "FrameModBrowser.h"
#include "NoGames.h"

#include "GenericToolbox.Switch.h"
#include "GenericToolbox.Vector.h"

#include <Game.h>
#include <util.hpp>
#include <note_cell.hpp>

using namespace brls::literals;

TabGames::TabGames() {
  this->container = new brls::Box(brls::Axis::COLUMN);
  Util::padContent(this->container);
  this->addView(this->container);

  if (gameBrowser.getGameList().empty()) {
    this->addView(new NoGames());
  } else {
    this->load();
  }
}

void TabGames::load() {
  std::vector<Game> gameList = gameBrowser.getGameList();
  _gameItems_.reserve(gameList.size());

  for(auto& gameEntry : gameList) {
    _gameItems_.emplace_back();
    _gameItems_.back().title = gameEntry.name;
    _gameItems_.back().item = this->buildGameCell(gameEntry);
  }

  GenericToolbox::sortVector(_gameItems_, [](const GameItem& a_, const GameItem& b_){
    return GenericToolbox::toLowerCase(a_.title) < GenericToolbox::toLowerCase(b_.title);
  });
  
  // add to the view
  for (auto& game : _gameItems_) { this->container->addView(game.item); }
}

brls::IconCell* TabGames::buildGameCell(const Game& game) {
  brls::IconCell* item = new brls::IconCell();

  item->setText(game.name);
  if (game.icon.size() > 0) {
    item->setIconFromMem(game.icon.data(), 0x20000);
  }

  item->registerClickAction([game](brls::View* view) {
    gameBrowser.selectGame(game);

    FrameModBrowser* modsBrowser = new FrameModBrowser();
    brls::Application::pushActivity(modsBrowser);
    modsBrowser->initialize();
    return true;
  });

  item->updateActionHint(brls::BUTTON_A, "View Mods");

  return item;
}

brls::View* TabGames::create() { return new TabGames(); }
