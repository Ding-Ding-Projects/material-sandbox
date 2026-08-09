# Windows build entrypoints

The repository exposes `build.bat` for the supported Windows desktop build and
`build-installer.bat` for the staged unsigned installer. Both accept `/s`,
`--silent`, or `SILENT=1`; silent mode never opens a prompt and exits non-zero
when the pinned Qt/MSVC toolchain or a build step is unavailable.

`build.bat` calls the supported `SandboxiePlus/qmake_plus.cmd` path and verifies
that `SandMan.exe` exists for the selected architecture. `build-installer.bat`
stages the x64 output through `Installer/copy_build.cmd`, removes the legacy
Inno Setup `SignTool` directive from a temporary script, builds an explicitly
unsigned installer, and prints its SHA-256 digest.

The scripts do not collect credentials, install signing material, publish, or
create releases. If the pinned Qt kit or Visual Studio environment is absent,
the scripts report the exact path and stop. This checkout currently has no local
Qt qmake kit, so hosted Windows CI remains the build proof.

## Suggested articles

- [Material Design 3](../material-design.md)
- [Native CI evidence](native-ci-evidence.md)
- [Contributor capability](contributor-build-audit.md)
