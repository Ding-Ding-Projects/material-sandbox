# Roadmap

## GitHub Pages Material 3 workspace

- Keep the browser-local documentation workspace aligned with every shipped
  feature article and its route registry.
- Keep the Page-level contracts current for subpath-safe article loading,
  bounded preference import/export, focused-mode filtering, local schedules,
  keyboard tab semantics, and non-blocking notifications.
- Treat a successful Pages deployment as evidence for the static site only. It
  is not evidence for the Qt desktop binary, service, driver, installer, or
  privileged credential routes.

## Explicit static-site boundary

GitHub Pages cannot safely enumerate installed fonts, access the operating
system credential vault, run a shared unlock service, host local Git history,
or make trusted Home Assistant/API requests. Those capabilities remain owned
by the native application and their dedicated feature documentation. The Pages
workspace must state that boundary instead of simulating those integrations.

## Next maintenance gates

1. Update the route registry and local renderer when an article is added.
2. Run all Pages contracts before each deployment.
3. Verify the deployed project-subpath site, including an in-site article route
   and its keyboard-accessible tab/navigation surface.
4. Run the non-cancelling `Windows Release` workflow from a clean full-history
   `main` revision and verify its one monotonic non-draft release, exact target
   commit, downloadable unsigned installer, SHA-256, authoritative timing, line
   table, attribution arithmetic, and public dim-sum catalog link.
5. Keep native-build and publication evidence pending until that hosted run is
   terminal; local counter and workflow-shape checks do not replace it.
