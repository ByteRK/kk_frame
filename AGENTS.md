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

- [ ] Fix fixed-length packet framing in `BtnAck` and `McuAck`: one input chunk can contain multiple frames, but the current parser consumes the whole chunk, validates only the first frame, then clears the buffer and loses trailing frames.
- [ ] Make `TuyaAck` reject and recover from impossible declared lengths. A valid header with a length greater than `BUF_LEN - MIN_LEN` currently fills the receive buffer and permanently wedges the decoder because future `add()` calls return zero.
- [ ] Fix `Transport::initEventDispatcher()` failure detection. `Looper::addFd()` returns `-1` on failure, while the current code checks only for zero and can report an unusable dispatcher as ready.
- [ ] Make `TcpClient::stop()` able to interrupt DNS lookup, connection establishment, and reconnect delay. The socket is not published until blocking `connect()` succeeds, so `stop()` can block indefinitely in `join()`.
- [ ] Define safe TCP socket ownership during send/close. Both TCP implementations copy an fd under lock and send after releasing the lock, allowing close and fd reuse to race with transmission.
- [ ] Serialize TCP writes or provide an explicit single-writer contract. Concurrent partial writes can interleave protocol frames, and blocking sockets currently allow `send()` to stall the caller without a timeout.
- [ ] Give `PacketChannel<TcpServer>` independent decoder state per client and preserve the client id through packet dispatch. The current shared decoder can combine bytes from different clients and cannot route business replies to their source.
- [ ] Widen and bounds-check `IAsk::setData()` length/offset parameters. The current `uint8_t` length silently truncates Tuya payload copies larger than 255 bytes even though callers and buffer allocation use `uint16_t` lengths.

### Medium Priority

- [ ] Correct the Tuya real-frame-length expression in `TuyaAck::add()` so header length and `MIN_LEN` are added with explicit parentheses; the current precedence fails when the low byte addition carries.
- [ ] Preserve lifecycle callbacks during explicit TCP shutdown. `stop()` posts disconnect events and immediately calls `shutdownEventDispatcher()`, which clears those events before normal Looper delivery.
- [ ] Bound or backpressure `Transport`'s event queue so a stalled Looper or sustained receive load cannot grow memory without limit.
- [ ] Replace or guard the TCP server's `select()`/`FD_SET` usage. An accepted fd at or above `FD_SETSIZE` causes out-of-bounds writes.
- [ ] Handle UART `POLLERR`, `POLLHUP`, and `POLLNVAL`; the current polling path only checks `POLLIN` and can leave a failed or removed device reported as connected.
- [ ] Validate packet-buffer allocation lengths before narrowing them into signed `short` fields, check allocation failure, and prevent negative or wrapped lengths from reaching `memset`, `memcmp`, or transport sends.
- [ ] Decide and document callback reentrancy rules. Stopping or destroying a transport from a callback can leave already-swapped local events dispatching after shutdown.
- [ ] Make handler dispatch robust if a callback adds or removes handlers; mutating the current handler vector during range iteration can invalidate iterators.

### Integration Checks Before Re-enabling UART Managers

- [ ] Resolve the `/dev/ttyS2` conflict: `BtnMgr` and `TuyaMgr` currently open and consume the same serial device independently.
- [ ] Propagate `PacketChannel::init()` failures from `ConnMgr`, `BtnMgr`, and `TuyaMgr` instead of scheduling inactive channels and returning success.
- [ ] Make manager initialization idempotent and clean up owned packet buffers, registered handlers, channels, and application event handlers consistently.
- [ ] Keep in mind that all three UART manager initializers are currently commented out in `main.cc`; packet/UART fixes need targeted tests because the default startup path does not exercise them.
