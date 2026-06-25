# Project Notes for Codex

- Compile validation for this project should be done by running `./fastCheck.sh` from the project root.
- Treat `./fastCheck.sh` success as the primary build verification signal. A successful run ends with `[100%] Built target kk_frame`.
- The script may build outside the app workspace under `/home/ricken/cdroid/outX64-Debug`, so running it can require filesystem escalation in sandboxed sessions.
- Not every change requires running `./fastCheck.sh`. Run it for C/C++ source, headers, CMake/build configuration, resources that affect packaging/runtime behavior, or any change that could affect compiled output. It is acceptable to skip it for script-only, documentation-only, `.gitignore`, or agent-memory updates that do not touch application logic; mention the skip explicitly in the final response.
- Commit messages should follow the existing history style: start with an emoji that describes the change type, then an English summary in title-style wording. Keep simple maintenance changes concise, but use a more descriptive phrase when the behavior or scope needs it. Common prefixes include `✨` for additions/updates/support/adjustments, `🐛` for fixes, `✏️` for small edits, `🗑️` for removals, `👓` for tooling/scripts, and `⚒️` for follow-up work. Examples from history: `✨ Update Some Utils`, `✨ The registration tool that adjusts the page and pop`, `🐛 Clean up wild pointers`, `🗑️ Turn off unnecessary click sound effects.`, `👓 Add Python Script :  Audio Gain Tool`.

## Temporary Task Memory: Communication Layer Clean Rebuild

- Remove this section when the user says the communication-layer task is complete.
- Current branch purpose: cleanly rebuild `src/comm` from scratch after committing the deletion of the old implementation.
- Do not preserve compatibility with the old `src/comm` implementation. Avoid reusing old `SocketClient`, `Client`, `socket_server`, `uart2socket`, or `uart.h` wrapper patterns.
- `UartClient` must initialize and operate UART internally with direct platform calls such as `open`, `tcsetattr`, `poll/select`, `read`, and `write`; it must not block or special-case `PRODUCT_X64`, because X64 can use serial devices too.
- Do not carry old debug-TCP compatibility fields such as `mDebugIp` or debug-port arguments into the new UART API.
- The communication layer should provide clean, extensible transport/channel abstractions for UART, TCP client, and TCP server. It should only handle connection/lifecycle, byte I/O, and thread/event delivery.
- Business protocol concerns, including heartbeat, handshake, packet semantics, checksums, and application state machines, belong outside `src/comm` unless explicitly requested otherwise.
- Upper protocol managers should be able to switch communication channels through a small type/configuration change rather than rewriting business logic.
