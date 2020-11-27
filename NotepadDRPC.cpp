#include <iostream>
#include <discord_rpc.h>
#include <cstring>
#include <Windows.h>
#include <thread>
#include <chrono>
#include <WtsApi32.h>
#include <tchar.h>
#include <cstdio>
#include <ctime>
#include <string>
#include "HAPIH.h"

#pragma comment(lib, "wtsapi32.lib")

int startTime = -1;
bool shouldChange = 1;
std::string previousFileName = "";

void updatePresence(HWND hWnd, DWORD winId, std::wstring title) {
	if (_wcsicmp(title.c_str(), L"Notepad") == 0)return;
	DiscordRichPresence discordPresence;
	memset(&discordPresence, 0, sizeof(discordPresence));
	title = title.substr(0, title.find(L" - Notepad"));
	if (title[0] == '*')title = title.substr(1);
	std::string s_title(title.begin(), title.end());
	std::string ending;
	HackIH MyObj;
	MyObj.bind("notepad.exe");
	if (s_title.find(".") == std::string::npos) {
		ending = "Editing file. Line: " + std::to_string(MyObj.Read<int>({ MyObj.GetModuleAddress("notepad.exe"), 0x2597C })) + ".";
	}
	else {
		ending = "Editing " + s_title.substr(s_title.find(".") + 1) + " file. Line: " + std::to_string(MyObj.Read<int>({ MyObj.GetModuleAddress("notepad.exe"), 0x2597C })) + ".";
	}
	s_title = "Working on " + s_title;
	discordPresence.details = (s_title).c_str();
	discordPresence.state = ending.c_str();
	discordPresence.largeImageKey = "logo_notepad";
	discordPresence.largeImageText = "Notepad";
	discordPresence.startTimestamp = startTime;
	Discord_UpdatePresence(&discordPresence);
}

BOOL __stdcall enumWindowsProc(HWND hWnd, LPARAM lParam){
	DWORD winId;
	GetWindowThreadProcessId(hWnd, &winId);
	if (winId == (DWORD)lParam) {
		std::wstring title(GetWindowTextLengthW(hWnd) + 1, L'\0');
		GetWindowTextW(hWnd, &title[0], title.size());
		if (title.find((L"Notepad")) != std::wstring::npos) {
			title = title.substr(0, title.find(L" - Notepad"));
			if (title[0] == '*')title = title.substr(1);
			std::string s_title(title.begin(), title.end());
			if (previousFileName.size() == 0)previousFileName = s_title;
			if (s_title != previousFileName)shouldChange = 1;
			if (shouldChange) {
				startTime = time(0);
				shouldChange = 0;
				previousFileName = s_title;
			}
			updatePresence(hWnd, winId, title);
			return TRUE;
		}
	}
	return TRUE;
}

void update() {
	WTS_PROCESS_INFO* pWPIs = NULL;
	DWORD dwProcCount = 0;
	if (WTSEnumerateProcesses(WTS_CURRENT_SERVER_HANDLE, NULL, 1, &pWPIs, &dwProcCount))
	{
		for (DWORD i = 0; i < dwProcCount; i++)
		{
			if (_tcsicmp(pWPIs[i].pProcessName, TEXT("notepad.exe")) == 0) {
				EnumWindows(enumWindowsProc, pWPIs[i].ProcessId);
				return;
			}
		}
	}
	if (pWPIs)
	{
		WTSFreeMemory(pWPIs);
		pWPIs = NULL;
	}
	shouldChange = 1;
	Discord_ClearPresence();
}

int main() {
	DiscordEventHandlers handlers;
	memset(&handlers, 0, sizeof(handlers));
	Discord_Initialize("781538875591491586", &handlers, 1, NULL);
	std::cout << "Running. Press ESC to close. Sometimes you'll need to hold it but still...\n";
	std::cout << "Alt+F4 should also work.\n";
	while (true) {
		if (GetAsyncKeyState(VK_ESCAPE) & 0x1) {
			break;
		}
		update();
		std::this_thread::sleep_for(std::chrono::seconds(1));
	}
	Discord_Shutdown();
	return 0;
}