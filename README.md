# nx-modloader-gx

nx-modloader-gx is a Nintendo Switch homebrew app for switching game mods and modpacks from the SD card.

## Features

- Per-game mod folders under `switch/Simple_Mod_alchemist/mods/<title_id>/`
- Single mods and modpacks in the same UI
- Files are moved, not copied
- Conflict-safe file handling

## Folder Layout

Create a folder for each game title ID here:

`switch/Simple_Mod_alchemist/mods/<title_id>/`

Inside that game folder:

- Single mod: `<mod_name>/romfs/...` and/or `<mod_name>/exefs/...`
- Modpack: `[PACK]_Name/romfs/...` and/or `[PACK]_Name/exefs/...`

## Quick Start

1. Copy the latest release to the root of your SD card.
2. Create `switch/Simple_Mod_alchemist/mods/<title_id>/` for each game you want to manage.
3. Put each mod in its own folder inside that game folder.
4. Put `romfs` and/or `exefs` inside each mod folder as needed.
5. Prefix a folder with `[PACK]_` if you want it treated as a modpack.
6. Launch the app, select the game, then use the Mods and Modpacks tabs.
7. Use **Disable all mods** before deleting or replacing files by hand.

## Backups

If you want a backup, copy these folders somewhere safe:

- `switch/Simple_Mod_alchemist/mods/`
- `/atmosphere/contents/`

When restoring, delete the old folders first. Do not merge them.

## Mod Management

- To delete a mod, disable it first, then remove its folder from `switch/Simple_Mod_alchemist/mods/<title_id>/`.
- To inspect a mod, look inside its `romfs` and/or `exefs` folders.
- If a mod only partially applies, another file is already occupying the same path in `/atmosphere/contents/<title_id>/`.

## Build From Source

## Prerequisites (using Bash)

- DevKitPro: https://github.com/devkitPro/pacman/releases

```bash
sudo installer -pkg /path/to/devkitpro-pacman-installer.pkg -target /
```

- Define environment (add the following lines to your bashrc):

```bash
function setup_devkitpro()
{
    echo "Seting up DevKitPro..." >&2
    export DEVKITPRO=/opt/devkitpro
    export DEVKITA64=${DEVKITPRO}/devkitA64
    export DEVKITARM=${DEVKITPRO}/devkitARM
    export DEVKITPPC=${DEVKITPRO}/devkitPPC
    export PORTLIBS_PREFIX=${DEVKITPRO}/portlibs/switch

    export PATH=${DEVKITPRO}/tools/bin:$PATH
    export PATH=${DEVKITA64}/bin/:$PATH

    source $DEVKITPRO/switchvars.sh
    return;
}
export -f setup_devkitpro
```

- Source your bashrc and execute `setup_devkitpro`

- Install packages

```bash
sudo dkp-pacman -Sy \
  switch-bulletphysics switch-bzip2 switch-curl\
  switch-examples switch-ffmpeg switch-flac switch-freetype\
  switch-giflib switch-glad switch-glfw switch-glm\
  switch-jansson switch-libass switch-libconfig\
  switch-libdrm_nouveau switch-libexpat switch-libfribidi\
  switch-libgd switch-libjpeg-turbo switch-libjson-c\
  switch-liblzma switch-liblzo2 switch-libmad switch-libmikmod\
  switch-libmodplug switch-libogg switch-libopus\
  switch-libpcre2 switch-libpng switch-libsamplerate\
  switch-libsodium switch-libtheora switch-libtimidity\
  switch-libvorbis switch-libvorbisidec switch-libvpx\
  switch-libwebp switch-libxml2 switch-mbedtls switch-mesa\
  switch-miniupnpc switch-mpg123 switch-ode switch-oniguruma\
  switch-opusfile switch-pkg-config switch-sdl2 switch-sdl2_gfx\
  switch-sdl2_image switch-sdl2_mixer switch-sdl2_net\
  switch-sdl2_ttf switch-smpeg2 switch-zlib switch-zziplib\
  devkitA64 devkitpro-keyring general-tools pkg-config\
  libnx libfilesystem switch-tools devkitpro-pkgbuild-helpers\
  -r /System/Volumes/Data
sudo dkp-pacman -Suy -r /System/Volumes/Data
```

### Compile

```bash
git clone https://github.com/gtiersma/SimpleModAlchemist.git
cd SimpleModAlchemist
cmake -B build_switch -DPLATFORM_SWITCH=ON
make -C build_switch nx-modloader-gx.nro -j$(nproc)
```

## Credits

Ggtiersma & nadrino
nadrino: original SimpleModManager project and the UI work
Ggtiersma: idea for moving files using State Alchemist. (still used for small mods)


## Legal Disclaimers

This software is built with the sole intention of running unofficially on the Nintendo Switch console. The Nintendo Switch is a product consisting of both hardware and software developed by Nintendo Co. Ltd. This software has not been licensed by Nintendo in any form. Nintendo is not affiliated with the creation of this software in any form.

The Nintendo Switch logo is a trademark of Nintendo Co. Ltd.

This software is purely a non-profit endeavor. Any usages of copyrighted material comprised within this software have not been used in a manner to gain compensation in any manner.

All other portions of this software are licensed under the GPL 3.0 standard, giving the general public permission to use, modify, or distribute this software in accordance with the terms and conditions that can be viewed here.

The author assumes no responsibility for any problems that may occur from modified derivative works from this software.

This software is not intended to be used in any manner that involves or encourages digital piracy. The author assumes no responsibility for any crimes users may perform through the use of this software.

The author provides no guarantee that the software will operate as intended or if it will operate at all. By using this software in any form or manner, you agree that the author will not be held responsible for any damage that may occur to the device(s) it may be used with, whether the damage be physical or digital.


This software is provided without warranty. Use it at your own risk.
