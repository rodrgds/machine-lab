{ pkgs, ... }:

{
  packages = [
    pkgs.clang
    pkgs.cli11
    pkgs.cmake
    pkgs.ninja
    pkgs.pkg-config
    pkgs.sdl3
    pkgs.sdl3-image
    pkgs.sdl3-ttf
    pkgs.sdl3-mixer
    pkgs.ffmpeg
    pkgs.glib
    pkgs.harfbuzz
    pkgs.freetype
    pkgs.dejavu_fonts
  ];

  env.LCOM_FONT = "${pkgs.dejavu_fonts}/share/fonts/truetype/DejaVuSansMono.ttf";

  scripts.lcom-configure.exec = ''
    cmake -S . -B build -G Ninja -DLCOM_WITH_SDL=ON -DCMAKE_BUILD_TYPE=Debug
  '';

  scripts.lcom-build.exec = ''
    cmake -S . -B build -G Ninja -DLCOM_WITH_SDL=ON -DCMAKE_BUILD_TYPE=Debug
    cmake --build build
  '';

  scripts.lcom-test.exec = ''
    cmake -S . -B build -G Ninja -DLCOM_WITH_SDL=ON -DCMAKE_BUILD_TYPE=Debug
    cmake --build build
    ctest --test-dir build --output-on-failure
  '';

  scripts.lcom-run-sdl.exec = ''
    cmake -S . -B build -G Ninja -DLCOM_WITH_SDL=ON -DCMAKE_BUILD_TYPE=Debug
    cmake --build build
    build/lcom run --display sdl -- build/examples/sdl_demo
  '';

  scripts.lcom-replay-flappy-video.exec = ''
    cmake -S . -B build -G Ninja -DLCOM_WITH_SDL=ON -DCMAKE_BUILD_TYPE=Debug
    cmake --build build
    mkdir -p build/replays
    build/lcom replay scripts/flappy_mouse_demo.lcomscript --headless --video build/replays/flappy.mp4 -- build/examples/flappy_bird
  '';

  enterShell = ''
    echo "lcom-ng dev shell"
    echo "  lcom-build     configure and build with SDL"
    echo "  lcom-test      build and run tests"
    echo "  lcom-run-sdl   run the interactive SDL demo"
    echo "  lcom-replay-flappy-video render a deterministic Flappy replay MP4"
  '';
}
