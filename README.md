# Garage Door Controller

The state machine of a garage door opener in C, tested in a [CppModel](https://www.cppmodel.com) simulation with a scripted sequence of test steps, verification rules and requirement IDs.

This is the code of the tutorial [Garage Door Controller](https://www.cppmodel.com/docs/GarageDoorTutorial). The tutorial walks through it step by step, shows the results and explains the fix.

## The controller

A button, a photo-eye and two limit switches in, a motor out. Modes: `CLOSED`, `OPENING`, `OPEN`, `CLOSING`, `STOPPED` and `REVERSING`.

- A press of the wall button opens a closed door and closes an open one. A press while the door moves stops it, and the next press sends it back the way it came.
- A limit switch at each end stops the motor.
- If the photo-eye beam is interrupted while the door closes, the door goes back up. An open door doesn't close while the beam is blocked.
- The motor rests 0.5 s before it changes direction.
- A travel that takes longer than 15 s stops the motor.

## Requirements and verification

| ID | Requirement | Verification rule |
| -- | ----------- | ----------------- |
| GD-1 | The door never runs down while the photo-eye is blocked. | `never closes on an obstruction` (Always) |
| GD-2 | An obstruction while closing sends the door back up within 1 s. | `reopens within 1 s` (Response) |
| GD-3 | The motor rests at least 0.5 s before it changes direction. | `mode sequence` (Mode: allowed transitions, minimum dwell) |
| GD-4 | No travel lasts longer than 15 s. | `mode sequence` (Mode: maximum dwell) |

The test is a sequence of 28 steps over 50 s, with a reusable input profile for a button press: open the door; close it and block the photo-eye halfway; press while the photo-eye is blocked; stop the door halfway and send it back; close it. Check steps confirm that the door is open, stopped or closed where it should be.

## This version fails

This is the starting point of the tutorial, and the simulation **fails** on purpose. Every check step passes, but a rule doesn't:

```
CppModel.Verify.never closes on an obstruction: PASS held at 5027 checked step(s)
CppModel.Verify.reopens within 1 s: PASS 1 trigger(s) answered, slowest in 0 s (deadline 1 s)
CppModel.Verify.mode sequence: FAIL illegal transition 3 → 1 at t=16.01 s
CppModel.Verify: 2 of 3 rules passed
CppModel.Requirements: GD-1, GD-2, GD-3, GD-4
```

The program exits with code 8, the number of the first failing step: step 8 blocks the photo-eye. The door does go back up, but the motor goes from full down to full up in one cycle, against GD-3. Can you find the cause in `garageDoor.c` before reading the [tutorial](https://www.cppmodel.com/docs/GarageDoorTutorial#the-fix)?

## Files

| File | What it is |
| ---- | ---------- |
| `garageDoor.h`, `garageDoor.c` | The controller under test: plain C, no dependency on CppModel |
| `SimulateGarageDoor.c` | The simulation: door model, test steps, verification rules and requirement IDs |
| `CMakeLists.txt` | Builds the simulation and registers it as a CTest test |

## Build and run

You need a C compiler, CMake 3.21 or later, and the CppModel SDK 0.18.0 or later.

1. Log in to [CppModel Workspace](https://workspace.cppmodel.com) (a free account works), open the `Download` menu and download the SDK for your platform.
2. Extract it into a `dependencies` folder here. On Linux and macOS:

   ```bash
   mkdir -p dependencies
   tar -xzf CppModel-x.x.x-Linux-x86_64.tar.gz -C dependencies --strip-components=1
   ```

   On Windows, extract the `.zip` and move the contents of its top-level folder into `dependencies`. See [Installation](https://www.cppmodel.com/docs/Installation).
3. Build and run:

   ```bash
   cmake -S . -B build
   cmake --build build
   ./build/SimulateGarageDoor
   ```

On the first run, the program prints a login link; open it to connect to your workspace. When the run finishes, it prints each rule's outcome, the requirements it verified and a link to the results. The exit code is 0 only when every step and rule passed, so `ctest --test-dir build` runs it as a test.

Each run also writes the requirements it verified, and whether it passed, to `cppmodel-requirements.json`, which a CI job can collect into a coverage report (see [Requirements](https://www.cppmodel.com/docs/Verification#requirements)).

In CI, set `CPPMODEL_USERNAME` and `CPPMODEL_PASSWORD` instead of the browser login (see [Command Line and Environment](https://www.cppmodel.com/docs/CommandLineTool)).

## Parameters

| Parameter | Default |
| --------- | ------- |
| `ReversePause [ms]` | 500 |
| `MaxRunTime [ms]` | 15000 |

They can be changed in the workspace, or from a file with `--inputs`, without rebuilding.

## License

[MIT](LICENSE)
