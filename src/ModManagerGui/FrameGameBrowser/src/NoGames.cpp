#include "NoGames.h"


NoGames::NoGames(): brls::Box(brls::Axis::COLUMN) {
  this->inflateFromXMLRes("xml/FrameGameBrowser/no_games.xml");

  this->topNote->setFocusable(false);
  this->bottomNote->setFocusable(false);
}
