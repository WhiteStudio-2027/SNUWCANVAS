# Future account and server monitoring work (design notes only)

No account, login, registration or monitoring implementation is included in this UI change.

## Placement

- Account entry belongs in the project sidebar footer, separate from generation options.
- A future monitoring page can join the existing workspace section navigation.
- Keep image viewing and generation usable without an unrelated top or bottom app bar.

## Decisions for a later implementation

- Authentication server ownership, registration policy and account recovery flow.
- Account sessions and existing server API keys must be distinct credentials.
- Per-user project and job ownership must be checked server-side.
- Decide what telemetry the server actually exposes: queue state, worker status,
  GPU utilization/VRAM, connectivity and recent failures.
- Monitor asynchronously, with bounded polling and a disconnected state; never block Tk.
- Plan migration and sign-out semantics before changing existing project JSON or credentials.

These notes do not change endpoints, credential storage, account data or network behavior.
