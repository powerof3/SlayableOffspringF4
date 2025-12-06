#pragma once

class Settings
{
public:
	[[nodiscard]] static Settings* GetSingleton()
	{
		static Settings singleton;
		return std::addressof(singleton);
	}

	void Load()
	{
		constexpr auto path = L"Data/F4SE/Plugins/po3_SlayableOffspring.ini";

		CSimpleIniA ini;
		ini.SetUnicode();

		ini.LoadFile(path);

		ini::get_value(ini, aggression, "AI Settings", "Aggression", ";Set base aggression of all child NPCs.\n;0 - Unaggressive, 1 - Aggressive, 2 - Very Aggressive, 3 - Frenzied.");
		ini::get_value(ini, confidence, "AI Settings", "Confidence", ";Set base confidence of all child NPCs.\n;0 - Cowardly, 1 - Cautious, 2 - Average, 3 - Brave, 4 - Foolhardy.");
		ini::get_value(ini, assistance, "AI Settings", "Assistance", ";Set base assistance of all child NPCs.\n;0 - Helps Nobody, 1 - Helps Allies, 2 - Helps Friends and Allies.");

		(void)ini.SaveFile(path);
	}

	void PatchAVs() const
	{
		auto avList = RE::ActorValue::GetSingleton();

		if (const auto dataHandler = RE::TESDataHandler::GetSingleton(); dataHandler) {
			for (const auto& npc : dataHandler->GetFormArray<RE::TESNPC>()) {
				if (npc && npc->formRace && (npc->formRace->data.flags & 4) != 0) {
					logger::info("Patching NPC: {} (FormID: {:08X})", RE::TESFullName::GetFullName(*npc), npc->formID);
					if (npc->GetActorValue(*avList->aggression) < aggression) {
						npc->SetActorValue(*avList->aggression, aggression);
					}
					if (npc->GetActorValue(*avList->confidence) < confidence) {
						npc->SetActorValue(*avList->confidence, confidence);
					}
					if (npc->GetActorValue(*avList->assistance) < assistance) {
						npc->SetActorValue(*avList->assistance, assistance);
					}
					logger::info("\tAggression: {} -> {}", npc->GetActorValue(*avList->aggression), aggression);
					logger::info("\tConfidence: {} -> {}", npc->GetActorValue(*avList->confidence), confidence);
					logger::info("\tAssistance: {} -> {}", npc->GetActorValue(*avList->assistance), assistance);
					npc->spectatorOverRidePackList = nullptr;
					npc->enterCombatOverRidePackList = nullptr;
				}
			}
		}
	}

	float aggression{ 1.0f };
	float confidence{ 2.0f };
	float assistance{ 1.0f };
};
