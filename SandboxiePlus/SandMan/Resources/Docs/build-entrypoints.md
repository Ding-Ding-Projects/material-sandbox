# Windows build entrypoints

Use `build.bat /s` for a silent Windows build through `SandboxiePlus/qmake_plus.cmd`.
Use `build-installer.bat /s` to stage the x64 output and build an explicitly
unsigned installer from the canonical Inno Setup script. The wrapper verifies
that the result is `NotSigned`; the obsolete installer entry point fails closed.
Missing pinned Qt or MSVC dependencies are reported as a concrete stop; no
credentials or signing material are requested.

The unsigned-packaging validator rejects signing directives, executable routes,
signing inputs, and any driver configuration whose WDK signing mode is not
explicitly `Off` before they can re-enter the supported packaging path.

See the repository article for the exact paths and verification boundary.
