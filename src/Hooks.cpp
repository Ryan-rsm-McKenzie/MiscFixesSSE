#include "Hooks.h"

#include "skse64_common/BranchTrampoline.h"  // g_branchTrampoline
#include "skse64_common/Relocation.h"  // RelocPtr
#include "skse64_common/SafeWrite.h"  // SafeWrite64

#include <typeinfo>  // typeid

#include "RE/Actor.h"  // Actor
#include "RE/TESObjectBOOK.h"  // TESObjectBOOK


namespace Hooks
{
	class ActorEx : public RE::Actor
	{
	public:
		bool Hook_IsRunning()
		{
			return this ? IsRunning() : false;
		}


		static void InstallHooks()
		{
			// 48 89 5C 24 08 57 48 83  EC 20 33 FF 49 8B D9 49  89 39 48 85 C9 ?? ?? 80  79 1A 3E 48 0F 44 F9 48  8B CF ?? ?? ?? ?? ?? 84
			constexpr uintptr_t GameFunc_Native_IsRunning = 0x002DB800;	// 1_5_62

			RelocAddr<uintptr_t> call_IsRunning(GameFunc_Native_IsRunning + 0x22);
			g_branchTrampoline.Write5Call(call_IsRunning.GetUIntPtr(), GetFnAddr(&Hook_IsRunning));
			_DMESSAGE("[DEBUG] Installed hooks for (%s)", typeid(ActorEx).name());
		}
	};


	class TESObjectBookEx : public RE::TESObjectBOOK
	{
	public:
		typedef void _LoadBuffer_t(RE::TESObjectBOOK* a_this, RE::BGSLoadFormBuffer* a_buf);
		static _LoadBuffer_t* orig_LoadBuffer;


		void Hook_LoadBuffer(RE::BGSLoadFormBuffer* a_buf)
		{
			using Flag = Data::Flag;

			constexpr UInt32 INVALID = 0xFFFFFFFF;

			orig_LoadBuffer(this, a_buf);

			if (data.teaches.skill == INVALID) {
				if (TeachesSkill()) {
					data.flags = Flag(data.flags & ~Flag::kFlag_Skill);
				}
				if (TeachesSpell()) {
					data.flags = Flag(data.flags & ~Flag::kFlag_Spell);
				}
			}
		}


		static void InstallHooks()
		{
			constexpr uintptr_t TES_OBJECT_BOOK_VTBL = 0x01573318;	// 1_5_62

			RelocPtr<_LoadBuffer_t*> vtbl_LoadBuffer(TES_OBJECT_BOOK_VTBL + (0x0F * 0x8));
			orig_LoadBuffer = *vtbl_LoadBuffer;
			SafeWrite64(vtbl_LoadBuffer.GetUIntPtr(), GetFnAddr(&Hook_LoadBuffer));
			_DMESSAGE("[DEBUG] Installed hooks for (%s)", typeid(TESObjectBookEx).name());
		}
	};


	TESObjectBookEx::_LoadBuffer_t* TESObjectBookEx::orig_LoadBuffer;


	void PatchUseAfterFree()
	{
		// 48 8B C4 48 89 50 10 55  53 56 57 41 54 41 55 41  56 41 57 48 8D A8 38 FB
		constexpr uintptr_t BadUseFuncBase = 0x0133C590;	// 1_5_62

		RelocAddr<uintptr_t> BadUse(BadUseFuncBase + 0x1AFD);
		UInt8 patch[] = { 0x4D, 0x8B, 0xCF, 0x90, 0x90, 0x90, 0x90 };
		// mov r9, 15
		// nop
		// nop
		// nop
		// nop
		for (UInt32 i = 0; i < sizeof(patch); ++i) {
			SafeWrite8(BadUse.GetUIntPtr() + i, patch[i]);
		}
		_DMESSAGE("[DEBUG] Installed patch for use after free");
	}


	void InstallHooks()
	{
		ActorEx::InstallHooks();
		TESObjectBookEx::InstallHooks();
		PatchUseAfterFree();
	}
}
