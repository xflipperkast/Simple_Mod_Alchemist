#pragma once

#include "borealis.hpp"

class TabHybridMods : public brls::Box {
  public:
    explicit TabHybridMods(bool showModpacks);
    static brls::View* createMods();
    static brls::View* createModpacks();

  private:
    void reload();
    bool showModpacks;
};
