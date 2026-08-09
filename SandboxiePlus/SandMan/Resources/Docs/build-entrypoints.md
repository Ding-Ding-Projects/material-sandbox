# Windows build entrypoints

Use `build.bat /s` for a silent Windows build through `SandboxiePlus/qmake_plus.cmd`.
Use `build-installer.bat /s` to stage the x64 output and build an explicitly
unsigned installer without invoking the legacy signing hook. Missing pinned Qt
or MSVC dependencies are reported as a concrete stop; no credentials or signing
material are requested.

See the repository article for the exact paths and verification boundary.
