set windows-shell := ["powershell.exe", "-NoLogo", "-Command"]

config := "Release"
core_dir := "build/core"
app_dir := "build/app"

# Windows: set QT_PREFIX to your Qt kit, e.g.
# C:/Qt/6.7.3/msvc2019_64 or C:/Qt/6.8.0/mingw_64. On Nix/Linux leave it empty.
qt_prefix := env("QT_PREFIX", "")
prefix_arg := if qt_prefix == "" { "" } else { "-DCMAKE_PREFIX_PATH=" + qt_prefix }

default:
    @just --list

# Build core library + tests (no Qt needed)
core:
    cmake -S . -B {{ core_dir }} -DQR_BUILD_APP=OFF -DCMAKE_BUILD_TYPE={{ config }}
    cmake --build {{ core_dir }} --config {{ config }}

# Run all tests
test: core
    ctest --test-dir {{ core_dir }} -C {{ config }} --output-on-failure

# Build the full Qt app
app:
    cmake -S . -B {{ app_dir }} -DQR_BUILD_APP=ON -DQR_BUILD_TESTS=OFF -DCMAKE_BUILD_TYPE={{ config }} {{ prefix_arg }}
    cmake --build {{ app_dir }} --config {{ config }}

[unix]
run: app
    ./{{ app_dir }}/qrapp

[windows]
run: app
    if (Test-Path '.\{{ app_dir }}\{{ config }}\qrapp.exe') { .\{{ app_dir }}\{{ config }}\qrapp.exe } else { .\{{ app_dir }}\qrapp.exe }

# Everything CI does; run before pushing
check: test app

clean:
    cmake -E remove_directory build
