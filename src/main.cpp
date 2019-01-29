#include "skse64/PluginAPI.h"  // PluginHandle, SKSEMessagingInterface, SKSEInterface, PluginInfo
#include "skse64_common/BranchTrampoline.h"  // g_branchTrampoline
#include "skse64_common/skse_version.h"  // RUNTIME_VERSION

#include <clocale>  // setlocale

#include <ShlObj.h>  // CSIDL_MYDOCUMENTS

#include "Hooks.h"  // InstallHooks()
#include "version.h"  // MISCFIXESSSE_VERSION_VERSTRING


static PluginHandle	g_pluginHandle = kPluginHandle_Invalid;
static SKSEMessagingInterface* g_messaging = 0;


void MessageHandler(SKSEMessagingInterface::Message* a_msg)
{
	switch (a_msg->type) {
	case SKSEMessagingInterface::kMessage_PostPostLoad:
		break;
	case SKSEMessagingInterface::kMessage_InputLoaded:
		break;
	}
}


extern "C" {
	bool SKSEPlugin_Query(const SKSEInterface* a_skse, PluginInfo* a_info)
	{
		std::setlocale(LC_ALL, "");

		gLog.OpenRelative(CSIDL_MYDOCUMENTS, "\\My Games\\Skyrim Special Edition\\SKSE\\MiscFixesSSE.log");
		gLog.SetPrintLevel(IDebugLog::kLevel_DebugMessage);
		gLog.SetLogLevel(IDebugLog::kLevel_DebugMessage);

		_MESSAGE("MiscFixesSSE v%s", MISCFIXESSSE_VERSION_VERSTRING);

		a_info->infoVersion = PluginInfo::kInfoVersion;
		a_info->name = "MiscFixesSSE";
		a_info->version = 1;

		g_pluginHandle = a_skse->GetPluginHandle();

		if (a_skse->isEditor) {
			_FATALERROR("[FATAL ERROR] Loaded in editor, marking as incompatible!\n");
			return false;
		}

		if (a_skse->runtimeVersion != RUNTIME_VERSION_1_5_62) {
			_FATALERROR("[FATAL ERROR] Unsupported runtime version %08X!\n", a_skse->runtimeVersion);
			return false;
		}

		if (g_branchTrampoline.Create(1024 * 8)) {
			_MESSAGE("[MESSAGE] Branch trampoline creation successful");
		} else {
			_FATALERROR("[FATAL ERROR] Branch trampoline creation failed!\n");
			return false;
		}

		if (g_localTrampoline.Create(1024 * 8)) {
			_MESSAGE("[MESSAGE] Local trampoline creation successful");
		} else {
			_FATALERROR("[FATAL ERROR] Local trampoline creation failed!\n");
			return false;
		}

		return true;
	}


	bool SKSEPlugin_Load(const SKSEInterface* a_skse)
	{
		_MESSAGE("[MESSAGE] MiscFixesSSE loaded");

		g_messaging = (SKSEMessagingInterface*)a_skse->QueryInterface(kInterface_Messaging);
		if (g_messaging->RegisterListener(g_pluginHandle, "SKSE", MessageHandler)) {
			_MESSAGE("[MESSAGE] Messaging interface registration successful");
		} else {
			_FATALERROR("[FATAL ERROR] Messaging interface registration failed!\n");
			return false;
		}

		Hooks::InstallHooks();
		_MESSAGE("[MESSAGE] Installed hooks");

		return true;
	}
};
