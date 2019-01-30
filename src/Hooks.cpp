#include "Hooks.h"

#undef min
#undef max

#include "skse64_common/BranchTrampoline.h"  // g_branchTrampoline
#include "skse64_common/Relocation.h"  // RelocPtr
#include "skse64_common/SafeWrite.h"  // SafeWrite64
#include "xbyak/xbyak.h"  // CodeGenerator

#include <wchar.h>  // wcsrtombs_s
#include <climits>  // numeric_limits
#include <string>  // string, wstring
#include <fstream>  // ofstream
#include <typeinfo>  // typeid

#include <stringapiset.h>  // WideCharToMultiByte

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

			orig_LoadBuffer(this, a_buf);

			if (data.teaches.skill == RE::ActorValue::kNone) {
				if (TeachesSkill()) {
					data.flags &= ~Flag::kTeachesSkill;
				}
				if (TeachesSpell()) {
					data.flags &= ~Flag::kTeachesSpell;
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

		struct Patch : Xbyak::CodeGenerator
		{
			Patch(void* buf) : Xbyak::CodeGenerator(1024, buf)
			{
				mov(r9, r15);
				nop();
				nop();
				nop();
				nop();
			}
		};

		void* patchBuf = g_localTrampoline.StartAlloc();
		Patch patch(patchBuf);
		g_localTrampoline.EndAlloc(patch.getCurr());

		ASSERT(patch.getSize() == 7);

		for (UInt32 i = 0; i < patch.getSize(); ++i) {
			SafeWrite8(BadUse.GetUIntPtr() + i, *(patch.getCode() + i));
		}

		_DMESSAGE("[DEBUG] Installed patch for use after free");
	}


	struct UnkData
	{
		void*		unk00;	// 00
		void*		unk08;	// 08
		void*		unk10;	// 10
		std::size_t	size;	// 18
		char		buf[1];	// 20 - buf[size]?
	};


	errno_t Hook_wcsrtombs_s(std::size_t* a_retval, char* a_dst, rsize_t a_dstsz, const wchar_t** a_src, rsize_t a_len, std::mbstate_t* a_ps)
	{
		int numChars = WideCharToMultiByte(CP_UTF8, 0, *a_src, a_len, NULL, 0, NULL, NULL);

		std::string str;
		char* dst = 0;
		rsize_t dstsz = 0;
		if (a_dst) {
			dst = a_dst;
			dstsz = a_dstsz;
		} else {
			str.resize(numChars);
			dst = str.data();
			dstsz = str.max_size();
		}

		bool err;
		if (a_src && numChars != 0 && numChars <= dstsz) {
			err = WideCharToMultiByte(CP_UTF8, 0, *a_src, a_len, dst, numChars, NULL, NULL) ? false : true;
		} else {
			err = true;
		}

		if (err) {
			if (a_retval) {
				*a_retval = static_cast<std::size_t>(-1);
			}
			if (a_dst && a_dstsz != 0 && a_dstsz <= std::numeric_limits<rsize_t>::max()) {
				a_dst[0] = '\0';
			}
			return GetLastError();
		}

		if (a_retval) {
			*a_retval = static_cast<std::size_t>(numChars);
		}
		return 0;
	}


	void InstallBNetCrashFix()
	{
		constexpr std::uintptr_t TARGET_FUNC = 0x011551A0;	// 1_5_62
		RelocAddr<std::uintptr_t> targetCall(TARGET_FUNC + 0x2A);
		g_branchTrampoline.Write6Call(targetCall.GetUIntPtr(), GetFnAddr(&Hook_wcsrtombs_s));
	}


	void InstallEquipEventSpamFix()
	{
		constexpr std::uintptr_t TARGET_FUNC = 0x006325B0;
		constexpr std::uintptr_t BRANCH_OFF = 0x17A;
		constexpr std::uintptr_t SEND_EVENT_BEGIN = 0x18A;
		constexpr std::uintptr_t SEND_EVENT_END = 0x236;
		constexpr std::size_t EQUIPPED_SHOUT = offsetof(RE::Actor, equippedShout);
		constexpr UInt32 BRANCH_SIZE = 5;
		constexpr UInt32 CODE_CAVE_SIZE = 16;
		constexpr UInt32 DIFF = CODE_CAVE_SIZE - BRANCH_SIZE;
		constexpr UInt8 NOP = 0x90;

		RelocAddr<std::uintptr_t> funcBase(TARGET_FUNC);

		struct Patch : Xbyak::CodeGenerator
		{
			Patch(void* a_buf, UInt64 a_funcBase) : Xbyak::CodeGenerator(1024, a_buf)
			{
				Xbyak::Label exitLbl;
				Xbyak::Label exitIP;
				Xbyak::Label sendEvent;

				// r14 = Actor*
				// rdi = TESShout*

				cmp(ptr[r14 + EQUIPPED_SHOUT], rdi);	// if (actor->equippedShout != shout)
				je(exitLbl);
				mov(ptr[r14 + EQUIPPED_SHOUT], rdi);	// actor->equippedShout = shout;
				test(rdi, rdi);							// if (shout)
				jz(exitLbl);
				jmp(ptr[rip + sendEvent]);


				L(exitLbl);
				jmp(ptr[rip + exitIP]);

				L(exitIP);
				dq(a_funcBase + SEND_EVENT_END);

				L(sendEvent);
				dq(a_funcBase + SEND_EVENT_BEGIN);
			}
		};

		void* patchBuf = g_localTrampoline.StartAlloc();
		Patch patch(patchBuf, funcBase.GetUIntPtr());
		g_localTrampoline.EndAlloc(patch.getCurr());

		g_branchTrampoline.Write5Branch(funcBase.GetUIntPtr() + BRANCH_OFF, reinterpret_cast<std::uintptr_t>(patch.getCode()));

		for (UInt32 i = 0; i < DIFF; ++i) {
			SafeWrite8(funcBase.GetUIntPtr() + BRANCH_OFF + BRANCH_SIZE + i, NOP);
		}

		_DMESSAGE("[DEBUG] Installed patch for equip event spam (size == %zu)", patch.getSize());
	}


	void InstallHooks()
	{
		ActorEx::InstallHooks();
		TESObjectBookEx::InstallHooks();
		PatchUseAfterFree();
		InstallBNetCrashFix();

		InstallEquipEventSpamFix();
	}
}
