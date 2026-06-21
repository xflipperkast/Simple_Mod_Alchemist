#pragma once

#include <switch.h>

#include <vector>
#include <string>
#include <atomic>

class Controller {
  public:
    u64 titleId; // The current Game's Title ID
    std::string gamePath;
    std::string group;
    std::string source;

    void setTitleId(const u64& titleId);
    void setGamePath(const std::string& path);

    bool doesGameHaveFolder();

    // NOTE: vectors returned from functions in controller
    // are implied to be sorted alphabetically unless stated otherwise

    /*
     * Load all groups from the game folder
     * 
     * @param sort Whether to sort the list of names alphabetically or not
     *             Can take considerable performance when in nested loops, so sometimes it's good to skip if not needed
     */
    std::vector<std::string> loadGroups(bool sort);

    /*
     * Load all source options within the specified group
     * 
     * @param sort Whether to sort the list of names alphabetically or not
     *             Can take considerable performance when in nested loops, so sometimes it's good to skip if not needed
     */
    std::vector<std::string> loadSources(bool sort);

    /*
     * Load all mod options that could be activated for the moddable source in the group
     * 
     * @param sort Whether to sort the list of names alphabetically or not
     *             Can take considerable performance when in nested loops, so sometimes it's good to skip if not needed
     */
    std::vector<std::string> loadMods(bool sort);

    /*
     * Gets the mod currently activated for the moddable source in the group
     *
     * Returns an empty string if no mod is active and vanilla files are being used
     */
    std::string getActiveMod(const std::string& source);

    /*
     * Activates the specified mod, moving all its files into the atmosphere folder for the game
     */
    void activateMod(const std::string& mod);

    /**
     * Deactivates the currently active mod, restoring the moddable source to its vanilla state
     */
    void deactivateMod();

    /**
     * @param progress Scale of 0.0-1.0 of the method's current progress.
     *                 Updated while the method runs.
     */
    void deactivateAll(std::atomic<float>& progress);

    /**
     * Gets Mod Alchemist's game directory:
     */
    std::string getGamePath();

    /**
     * Gets the file path for the specified group
     */
    std::string getGroupPath();

    Controller();

    /**
     * Unmount SD card when destroyed 
     */
    ~Controller();

  private:

    /**
     * Returns all files belonging to a mod from the atmosphere active mods folder to their original location
     * 
     * Essentially the same as deactivating the mod, except this can't be used with the default mod option.
     */
    void returnFiles(const std::string& mod);

    /**
     * Gets the file path for the specified source within the group
     */
    std::string getSourcePath();

    /**
     * Get the file path for the specified mod within the moddable source
     */
    std::string getModPath(const std::string& mod);

    /**
     * Gets the game's path that's stored within Atmosphere's directory
     */
    std::string getAtmospherePath();

    /**
     * Gets the file path for the list of moved files for the specified mod
     * 
     * The file should only exist if the mod is currently active
     */
    std::string getMovedFilesListFilePath(const std::string& mod);
};

extern Controller controller;
