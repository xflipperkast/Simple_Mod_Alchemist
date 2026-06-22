#include "StateAlchemist/controller.h"

#include "StateAlchemist/fs_manager.h"
#include "StateAlchemist/meta_manager.h"

Controller controller;

Controller::Controller() {
  MetaManager::tryResult(fsOpenSdCardFileSystem(&FsManager::sdSystem));
}

Controller::~Controller() {
  fsFsClose(&FsManager::sdSystem);
}

void Controller::setTitleId(const u64& titleId_) {
  titleId = titleId_;
}

void Controller::setGamePath(const std::string& path) {
  gamePath = path;
}
