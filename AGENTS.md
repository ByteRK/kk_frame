# Project Notes for Codex

- Compile validation for this project should be done by running `./fastCheck.sh` from the project root.
- Treat `./fastCheck.sh` success as the primary build verification signal. A successful run ends with `[100%] Built target kk_frame`.
- The script may build outside the app workspace under `/home/ricken/cdroid/outX64-Debug`, so running it can require filesystem escalation in sandboxed sessions.
- Not every change requires running `./fastCheck.sh`. Run it for C/C++ source, headers, CMake/build configuration, resources that affect packaging/runtime behavior, or any change that could affect compiled output. It is acceptable to skip it for script-only, documentation-only, `.gitignore`, or agent-memory updates that do not touch application logic; mention the skip explicitly in the final response.
- Commit messages should follow the existing history style: start with an emoji that describes the change type, then an English summary in title-style wording. Keep simple maintenance changes concise, but use a more descriptive phrase when the behavior or scope needs it. Common prefixes include `✨` for additions/updates/support/adjustments, `🐛` for fixes, `✏️` for small edits, `🗑️` for removals, `👓` for tooling/scripts, and `⚒️` for follow-up work. Examples from history: `✨ Update Some Utils`, `✨ The registration tool that adjusts the page and pop`, `🐛 Clean up wild pointers`, `🗑️ Turn off unnecessary click sound effects.`, `👓 Add Python Script :  Audio Gain Tool`.

## Temporary Task Memory: Communication-Layer Risk Backlog

- Remove this section when the user confirms that the communication-layer risk backlog is complete.
- Treat unchecked items as known risks to address incrementally. After fixing an item, verify the relevant behavior and mark it complete instead of deleting the remaining context.

### High Priority

- [x] Make `PacketChannel::onRecv()` continue from the decoder's reported consumption so one transport read can deliver multiple packets. Protocol parsers remain responsible for reporting their true per-packet consumption; `BtnAck` and `McuAck` apply their fixed-length correction locally, similar to `TuyaAck`. Verified with joined/split packet tests and `./fastCheck.sh`.
- [x] Application-layer responsibility: validate and recover from impossible packet lengths in concrete protocol parsers such as `TuyaAck`. Do not add protocol-specific length policy to the communication layer.
- [x] Fix `Transport::initEventDispatcher()` failure detection. Negative `Looper::addFd()` results now trigger eventfd cleanup and initialization failure. Verified with `./fastCheck.sh`.
- [x] Make `TcpClient::stop()` able to interrupt DNS lookup, connection establishment, and reconnect delay. DNS waits are isolated from client lifetime, connect uses nonblocking polling with stop checks, and reconnect delay is condition-variable driven. Verified with `./fastCheck.sh`.
- [x] Define safe TCP socket ownership during send/close. Both TCP implementations now hold a send lock from fd lookup through the complete write, and every close path takes the same lock before invalidating or closing published sockets. Verified with `./fastCheck.sh`.
- [x] Serialize TCP writes and bound their duration. Complete writes are serialized per transport, use nonblocking sends with an absolute configurable timeout (`sendTimeoutMs`, default 1000 ms), and return the partial byte count on timeout or failure. Verified with `./fastCheck.sh`.
- [x] Guard `PacketChannel<TcpServer>` against cross-client decoder contamination and preserve the client id through packet dispatch. The default single-id mode rejects unexpected ids with `LOGE`; defining `PACKET_CHANNEL_ENABLE_MULTI_ID=1` creates and releases independent decoders per client. `IHandler` provides a backward-compatible source-aware callback overload. Verified with `./fastCheck.sh`.
- [x] Widen and bounds-check `IAsk::setData()` length/offset parameters. Offsets and lengths now use `size_t`, bulk input is const, and invalid or out-of-range writes are rejected with `LOGE` diagnostics. Verified with `./fastCheck.sh`.

### Medium Priority

- [ ] Correct the Tuya real-frame-length expression in `TuyaAck::add()` so header length and `MIN_LEN` are added with explicit parentheses; the current precedence fails when the low byte addition carries.
- [ ] Preserve lifecycle callbacks during explicit TCP shutdown. `stop()` posts disconnect events and immediately calls `shutdownEventDispatcher()`, which clears those events before normal Looper delivery.
- [x] Bound and backpressure `Transport`'s event queue. The queue defaults to 256 events, producers wait up to 100 ms for space, and sustained overload is dropped with a cumulative counter and rate-limited diagnostics. Verified with `./fastCheck.sh`.
- [ ] Replace or guard the TCP server's `select()`/`FD_SET` usage. An accepted fd at or above `FD_SETSIZE` causes out-of-bounds writes.
- [ ] Handle UART `POLLERR`, `POLLHUP`, and `POLLNVAL`; the current polling path only checks `POLLIN` and can leave a failed or removed device reported as connected.
- [ ] Validate packet-buffer allocation lengths before narrowing them into signed `short` fields, check allocation failure, and prevent negative or wrapped lengths from reaching `memset`, `memcmp`, or transport sends.
- [ ] Decide and document callback reentrancy rules. Stopping or destroying a transport from a callback can leave already-swapped local events dispatching after shutdown.
- [ ] Make handler dispatch robust if a callback adds or removes handlers; mutating the current handler vector during range iteration can invalidate iterators.

### Integration Checks Before Re-enabling UART Managers

- [x] Prevent duplicate UART device initialization. `UartClient` now tracks claimed device paths in a process-wide mutex-protected set and calls `FailFast` when a second instance initializes the same device; failed initialization and `stop()` release the claim. Verified with `./fastCheck.sh`.
- [x] Propagate `PacketChannel::init()` failures from `ConnMgr`, `BtnMgr`, and `TuyaMgr`. Failed temporary channels are destroyed and no packet handler or App event is registered. Verified with `./fastCheck.sh`.
- [x] Make protocol manager initialization idempotent and cleanup symmetric. Successful initialization registers each handler/event once; destruction unregisters them and releases the channel before its packet buffer. Verified with `./fastCheck.sh`.
- [ ] Keep in mind that all three UART manager initializers are currently commented out in `main.cc`; packet/UART fixes need targeted tests because the default startup path does not exercise them.
