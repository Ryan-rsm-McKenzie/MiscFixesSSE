#include "Events.h"

#include "RE/BSTEvent.h"  // EventResult, BSTEventSource
#include "RE/TESEquipEvent.h"  // TESEquipEvent


TESEquipEventHandler::TESEquipEventHandler()
{}


TESEquipEventHandler::~TESEquipEventHandler()
{}


RE::EventResult TESEquipEventHandler::ReceiveEvent(RE::TESEquipEvent* a_event, RE::BSTEventSource<RE::TESEquipEvent>* a_eventSource)
{
	return RE::EventResult::kContinue;
}


TESEquipEventHandler* TESEquipEventHandler::GetSingleton()
{
	if (!_singleton) {
		_singleton = new TESEquipEventHandler();
	}
	return _singleton;
}


void TESEquipEventHandler::Free()
{
	delete _singleton;
	_singleton = 0;
}


TESEquipEventHandler* TESEquipEventHandler::_singleton = 0;
