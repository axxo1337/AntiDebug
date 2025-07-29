# AntiDebug

## What is this?

A simple terminal interface tool to test Windows x86_64 anti-debugging techniques. Like many of our public projects this was made because we needed it to better explain
some of [our courses](https://haxo.games/courses).

## Build

All this requires is Visual Studio 2022 (minimum) and the "Desktop Development with C++" package installed. The only dependencies this project uses are the Windows API 
and [FTXUI](https://github.com/ArthurSonzogni/FTXUI). FTXUI is directly included within the `dependencies/` folder. I'm aware this is bad practice, but I want to keep
the build environment as simplistic and similar to what is used throughout the course as possible for beginners.

## Usage

Simply run the executable and attach your debugger to the process. The console is conveniently titled "AntiDebug" on launch for you to easily find it. You can toggle
detections by navigating to them using your up and down arrow keys and pressing `ENTER` or `SPACE`. You can terminate the program with `Q`. Dections checks run continuously
when enabled so just experiment and see what detects you, good luck!

![Screenshot of AntiDebug running.](repo/preview.png)

### Adding detections

If you'd like to add detections you first need to create the callback for said detection. To do so head into `antidebug.h`, make its definition with the following format:
`void callbackMyDetectionName(AntiDebugOption& option);`, then head into `antidebug.cpp` and create its declaration like so:
```c++
void AntiDebug::callbackMyDetectionName(AntiDebugOption& option)
{
	// Example detection
	option.detected = IsDebuggerPresent();
}
```
Setting `option.detected` to true or false will instantly update the UI once the detection is fully integrated. You don't need to handle anything else than setting this
value as you wish here. Disable logic and UI events are already handled in the `main` function.
<br />
Now to integrate the detection, locate `options` inside of the `antidebug.h` and add a new member in its initializer list like so:
```c++
AntiDebug::AntiDebugOptions anti_debug_options
{
	ADD_ANTI_DEBUG_OPTION("IsDebuggerPresent", true, callbackIsDebuggerPresent),
	ADD_ANTI_DEBUG_OPTION("MyDetectionName", true, callbackMyDetectionName), // <- Example integration
};
```
The first argument is the title to be displayed in the UI, the second sets if the detection should be enabled or not by default and the third is your callback. You can now 
also specify a fourth argument for the delay in milliseconds to wait after executing each callback in case your callback is somewhat resource-intensive.

## Documentation

In this section I will do my best to document the different anti-debugging techniques used, and maybe add extra details like how they can be avoided etc. For any further
information please join [our Discord server](https://discord.gg/f6AbaCATMg).

### `IsDebuggerPresent`

`IsDebuggerPresent` is a Windows API function that simply accesses the `BeingDebugged` boolean from the thread environment block (TEB) of the current process. This value
is true when the process is being debugged. This can be bypassed by simply setting `BeingDebugged` to 0. Patching `IsDebuggerPresent` may seem idiotic since anyone with a
minimum of common sense would not invoke it directly, and yet most programs out there do.

### `BeingDebugged`

This technique directly reads the `BeingDebugged` member of the process environment block (PEB) for the current process. If true, it is being debugged. Can be bypassed by
simply setting the value to 0.

### `CheckRemoteDebuggerPresent`

`CheckRemoteDebuggerPresent` is a Windows API function which which calls `NtQueryInformationProcess` for the `ProcessDebugPort` value under the hood. If this value is 0
no debugger is attached, otherwise it sets the pointer to the boolean it was passed to true. This can be bypassed by hooking `NtQueryInformationProcess` or the function
itself. `ProcessDebugPort` being in the `EPROCESS` structure it is immune to usermode shenanigans.

### `NtQueryInformationProcess`

`NtQueryInformationProcess` can be used to get `ProcessDebugPort` which can indicate if a debugger is present. It can also query a few values in an undocumented way such
as the heap flags (`ProcessDebugFlags`) and the debug object handle (`ProcessDebugHandle`).

### `FindWindowByTitle`

Searches for debugging tools by enumerating windows and checking their titles. It searches for substrings in window titles such as "Cheat Engine" or "Process Hacker". 
The substring matching provides flexibility to detect multiple versions of tools even when version numbers are appended to titles. This can be bypassed by hiding debugger 
windows, changing window titles and hooking `FindWindowExA` functions.

### `FindWindowByClass`

Searches for debugging tools by enumerating windows and checking their classes. It uses `FindWindowA` from Windows API to detect specific window classes like "OLLYDBG" and "WinDbgFrameClass", 
This can be bypassed by hiding debugger windows, changing window classes and hooking `FindWindowA`/`FindWindowExA` functions.

### `GetThreadContext`

`GetThreadContext` is a Windows API function that retrieves the current thread context, including hardware debug registers (Dr0, Dr1, Dr2, Dr3). When hardware breakpoints are set by debuggers, the corresponding debug register contains the breakpoint address. This technique checks if any of these registers contain non-zero values, indicating active hardware breakpoints. Hardware breakpoints are preferred by advanced debuggers since they don't modify process memory. This can be bypassed by clearing debug registers before detection, hooking `GetThreadContext` to return clean registers.

### `NtQuerySystemInformation`

`NtQuerySystemInformation` can be used to get `DebuggerInformation`. It's an undocumented structured made up of three booleans:
```c++
typedef struct _SYSTEM_KERNEL_DEBUGGER_INFORMATION_EX 
{
	BOOLEAN DebuggerAllowed;
	BOOLEAN DebuggerEnabled;
	BOOLEAN DebuggerPresent;
} SYSTEM_KERNEL_DEBUGGER_INFORMATION_EX, * PSYSTEM_KERNEL_DEBUGGER_INFORMATION_EX;
```
`DebuggerAllowed` is true when a kernel debugger is allowed on the system. This will usually become true when the system gets a debugger attached to it. `KernelDebuggerEnabled` will
be true if the kernel was initialized with debugging enabled. `DebuggerPresent` is true when there currently is a debugger attached to the system.

### `CloseHandle`

When closing an invalid handle an `EXCEPTION_INVALID_HANDLE (0xC0000008)` will be raised if a debugger is attached. This can then be cought by an `__except` block once the debugger passes
control back to the procediss. If no debugger is present no exception is thrown. This detection is disabled by default because it causes an exception.

### `DbgPrint`

This method will raise the exception `DBG_PRINTEXCEPTION_C (0x40010006)`. If it is handled by the program itself then no debugger is attached. Otherwise one obviously is. It is usually is raised
by functions like `DbgPrint`.

### `NtGlobalFlag`

`NtGlobalFlag` is a member of the PEB which, when a debugger is attached, has three bit flags set to 1 notably: `0x10: EAP_ENABLE_TAIL_CHECK`, `0x20: HEAP_ENABLE_FREE_CHECK` and `0x40: HEAP_VALIDATE_PARAMETERS`. 
Bypassing is as simple setting it to any value that does not match any of those flags (0 will work great). Also it seems these flags are only set when a program is launched with a debugger. When it is attached 
after startup they aren't.

### `EnumDeviceDrivers`

`EnumDeviceDrivers` just enumerates the different loaded device drivers. Debuggers can be detected by looking for signed debugger-related kernel drivers such as `dbk64.sys` used for DBVM.

### `CyclesPassed`

`CyclesPassed` uses MSVC intrinsics to check if the CPU cycles delta between function's prologue and epilogue was abnormally huge. If such a thing happend more than or equal 3 times then it assumes that there was a breakpoint set inside it.

## Contributing

This project is free and open source and will remain so forever. You are welcome to contribute. Simply make a pull request for whatever it is you would like to add, but
there are a few things you need to keep in mind:
1. C++17 only for now.
2. snake_case for variable names, PascalCase for namespaces and structures and camelCase for methods and function names (there might be more specefics so please just
base your style on the already existing code).
3. Make an issue first, and specify your intent to address said issue.
4. When making a branch give it a meaningful name like `feature/name_of_feature` for example.

## Special thanks

Thank you to all who contributed to the project already and thank you to [vergiliusproject.com](https://www.vergiliusproject.com/) for documenting undocumented windows structures
and [anti-debug.checkpoint.com](https://anti-debug.checkpoint.com/) for documenting many anti-debugging tricks.

## License

This project is licensed under the MIT License with some specifications - see the [LICENSE](LICENSE) file for details.
