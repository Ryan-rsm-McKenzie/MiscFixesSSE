#pragma once

#include "RE/BSTEvent.h"  // BSTEventSink, EventResult, BSTEventSource
#include "RE/TESEquipEvent.h"  // TESEquipEvent


class TESEquipEventHandler : public RE::BSTEventSink<RE::TESEquipEvent>
{
public:
	TESEquipEventHandler();
	virtual ~TESEquipEventHandler();

	virtual	RE::EventResult ReceiveEvent(RE::TESEquipEvent* a_event, RE::BSTEventSource<RE::TESEquipEvent>* a_eventSource) override;

	static TESEquipEventHandler* GetSingleton();
	static void Free();

private:
	static TESEquipEventHandler*	_singleton;
};
