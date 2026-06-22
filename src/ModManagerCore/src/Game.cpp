#include <Game.h>

Game::Game(u64 titleId_, std::string name_): titleId(titleId_), name(name_) {
    path = "/switch/Simple_Mod_alchemist/mods/" + name_;
}
