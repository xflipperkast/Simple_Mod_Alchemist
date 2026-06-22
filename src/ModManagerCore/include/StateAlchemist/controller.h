#pragma once

#include <switch.h>

#include <string>

class Controller {
  public:
    u64 titleId{};
    std::string gamePath;
    std::string group;
    std::string source;

    Controller();
    ~Controller();

    void setTitleId(const u64& titleId);
    void setGamePath(const std::string& path);
};

extern Controller controller;
