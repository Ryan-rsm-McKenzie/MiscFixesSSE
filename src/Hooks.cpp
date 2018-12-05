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
			RelocAddr<uintptr_t> call_IsRunning(0x002DB800 + 0x22);
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
			constexpr uintptr_t TES_OBJECT_BOOK_VTBL = 0x01573318;
			RelocPtr<_LoadBuffer_t*> vtbl_LoadBuffer(TES_OBJECT_BOOK_VTBL + (0x0F * 0x8));
			orig_LoadBuffer = *vtbl_LoadBuffer;
			SafeWrite64(vtbl_LoadBuffer.GetUIntPtr(), GetFnAddr(&Hook_LoadBuffer));
			_DMESSAGE("[DEBUG] Installed hooks for (%s)", typeid(TESObjectBookEx).name());
		}
	};


	TESObjectBookEx::_LoadBuffer_t* TESObjectBookEx::orig_LoadBuffer;


	void PatchUseAfterFree()
	{
		RelocAddr<uintptr_t> BadUse(0x133C590 + 0x1AFD);
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
