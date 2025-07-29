#pragma once

#include <vector>
#include <mutex>
#include <string_view>
#include <cstdint>

#include <ftxui/component/component.hpp>
#define NOMINMAX
#include <windows.h>
#include <winternl.h>
#include <psapi.h>

namespace AntiDebug
{
	//
	// [SECTION] Types
	//

	struct AntiDebugOption
	{
		using AntiDebugCallback = std::function<void(AntiDebugOption&)>;

		std::string_view name;
		bool enabled;
		bool detected;
		bool was_enabled;
		std::chrono::milliseconds delay;
		std::chrono::time_point<std::chrono::steady_clock> time_start{ std::chrono::steady_clock::now() };
		std::chrono::milliseconds elapsed_time;
		AntiDebugCallback callback;

		AntiDebugOption(std::string_view n, bool e, AntiDebugCallback c, std::chrono::milliseconds delay = std::chrono::milliseconds(0))
			: name(n), enabled(e), callback(c), detected(false), was_enabled(e), delay(delay), elapsed_time(0)
		{}
	};

	using AntiDebugOptions = std::vector<AntiDebugOption>;

	//
	// [SECTION] Variables
	//

	inline std::mutex options_mutex;

	//
	// [SECTION] Functions
	//

	AntiDebugOptions& getOptions();

	void callbackIsDebuggerPresent(AntiDebugOption& option);
	void callbackBeingDebugged(AntiDebugOption& option);
	void callbackNtGlobalFlag(AntiDebugOption& option);
	void callbackCheckRemoteDebuggerPresent(AntiDebugOption& option);
	void callbackNtQueryInformationProcessProcessDebugPort(AntiDebugOption& option);
	void callbackNtQueryInformationProcessProcessDebugFlags(AntiDebugOption& option);
	void callbackNtQueryInformationProcessProcessDebugHandle(AntiDebugOption& option);
	void callbackFindWindowByTitle(AntiDebugOption& option);
	void callbackFindWindowByClass(AntiDebugOption& option);
	void callbackGetThreadContext(AntiDebugOption& option);
	void callbackNtQuerySystemInformation_DebuggerInformation(AntiDebugOption& option);
	void callbackCloseHandle(AntiDebugOption& option);
	void callbackDbgPrint(AntiDebugOption& option);
	void callbackEnumDeviceDrivers(AntiDebugOption& option);
	void callbackCyclesPassed(AntiDebugOption& option);
	void callbackIsWindowsFunctionBreakpointed(AntiDebugOption& option);
}
