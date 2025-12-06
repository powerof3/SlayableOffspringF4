#include "Hooks.h"
#include "Settings.h"

namespace Hooks
{
	namespace NotAChild
	{
		void Install()
		{
			//FF 90 F0 03 00 00	- call qword ptr [rax+3F0h]

			std::array targets{
#if FALLOUT_AE
				std::make_pair(2204414, 0xE7),   // Script::DamageActorValueFunction
				std::make_pair(2204420, 0xC9),   // Script::KillActor
				std::make_pair(2224729, 0x36),   // VatsMenu::IsValidTarget
				std::make_pair(2229596, 0x15),   // Actor::HasBlood
				std::make_pair(2229598, 0x1B),   // Actor::GetBloodSprayModel
				std::make_pair(2229623, 0x32),   // Actor::CanKillMe
				std::make_pair(2229965, 0x176),  // Actor::GetShouldAttack
				std::make_pair(2230058, 0x12B),  // Actor::DismemberLimbs
				std::make_pair(2230117, 0x126),  // Actor::CreateBlood
				std::make_pair(2230339, 0x166),  // Actor::KillImpl
				std::make_pair(2231181, 0x2BB),  // Actor::CombatHit
				std::make_pair(2236899, 0x13E),  // MissileProjectile::ProcessStick
				std::make_pair(2237076, 0x1F4),  // Projectile::CheckExplosionProximity
				std::make_pair(2240105, 0xEA),   // CombatState::CheckShouldFlee
				std::make_pair(2240416, 0x82),   // CombatTargetSelector::IsValidTarget
				std::make_pair(2253402, 0x92),   // GameScript::`anonymous namespace'::mem_ObjectReference_DamageValue
#else
				std::make_pair(1548658, 0xE7),   // Script::DamageActorValueFunction
				std::make_pair(1421490, 0xC7),   // Script::KillActor
				std::make_pair(391948, 0x3C),    // VatsMenu::IsValidTarget
				std::make_pair(318503, 0x15),    // Actor::HasBlood
				std::make_pair(1009286, 0x1B),   // Actor::GetBloodSprayModel
				std::make_pair(101434, 0x32),    // Actor::CanKillMe
				std::make_pair(35540, 0x180),    // Actor::GetShouldAttack
				std::make_pair(348276, 0x140),   // Actor::DismemberLimbs
				std::make_pair(1007554, 0x9D),   // Actor::CreateBlood
				std::make_pair(319011, 0x174),   // Actor::KillImpl
				std::make_pair(401356, 0x27C),   // Actor::CombatHit
				std::make_pair(1389073, 0x14D),  // MissileProjectile::ProcessStick
				std::make_pair(1529534, 0x16D),  // Projectile::CheckExplosionProximity
				std::make_pair(13341, 0x87),     // CombatState::CheckShouldFlee
				std::make_pair(491731, 0x82),    // CombatTargetSelector::IsValidTarget
				std::make_pair(3612960, 0x8C),   // GameScript::`anonymous namespace'::mem_ObjectReference_DamageValue
#endif
			};

			struct Patch : Xbyak::CodeGenerator
			{
				Patch()
				{
					call(ptr[rax + 0x4E0]);  // Actor::ShouldSaveAnimationOnUnloading { return false; }
				}
			};

			Patch patch;
			patch.ready();

			for (const auto& [id, offset] : targets) {
				REL::Relocation<std::uintptr_t> target{ REL::ID(id) };
				REL::safe_write(target.address() + offset, std::span{ patch.getCode(), patch.getSize() });
			}
		}
	}

	namespace MakeVunerable
	{
		struct IsInvunerable
		{
			static bool thunk(RE::MagicTarget* a_this)
			{
				auto result = func(a_this);
				if (result && a_this) {
					if (const auto actor = stl::adjust_pointer<RE::Actor>(a_this, -0x110); actor && actor->IsChild()) {
						result = false;
					}
				}
				return result;
			}
			static inline REL::Relocation<decltype(thunk)> func;
			static inline constexpr std::size_t vtbl_idx = 0x04;
		};

		void Install()
		{
			stl::write_vfunc<RE::Actor, 8, IsInvunerable>();
			logger::info("Installed invunerability patch"sv);
		}
	}

	namespace DisableFlee
	{
		struct CheckShouldFlee
		{
			static bool thunk(void*)
			{
				return false;
			}
			static inline REL::Relocation<decltype(thunk)> func;
		};

		void Install()
		{
			stl::hook_function_prologue<CheckShouldFlee, 6>(REL::ID(2240105).address());
			logger::info("Installed CheckShouldFlee patch"sv);
		}
	}

	namespace NoAliasPackages
	{
		struct FindEnterCombatOverrideInteruptPackage
		{
			static RE::TESPackage* thunk(RE::ExtraAliasInstanceArray* a_this, RE::Actor* a_actor)
			{
				const auto package = func(a_this, a_actor);
				return a_actor && a_actor->IsChild() ? nullptr : package;
			}
			static inline REL::Relocation<decltype(thunk)> func;
		};

		struct FindSpectatorOverrideInteruptPackage
		{
			static RE::TESPackage* thunk(RE::ExtraAliasInstanceArray* a_this, RE::Actor* a_actor)
			{
				const auto package = func(a_this, a_actor);
				return a_actor && a_actor->IsChild() ? nullptr : package;
			}
			static inline REL::Relocation<decltype(thunk)> func;
		};

		void Install()
		{
			REL::Relocation<std::uintptr_t> combat_override_interupt{
#if FALLOUT_AE
				REL::ID(2207391),
#else
				REL::ID(613063),
#endif
				0x27
			};
			stl::write_thunk_call<FindEnterCombatOverrideInteruptPackage>(combat_override_interupt.address());

			REL::Relocation<std::uintptr_t> spectator_override_interupt{
#if FALLOUT_AE
				REL::ID(2207388),
#else
				REL::ID(626905),
#endif
				0x27
			};
			stl::write_thunk_call<FindSpectatorOverrideInteruptPackage>(spectator_override_interupt.address());
		}
	}

	void InstallOnPostLoad()
	{
		Settings::GetSingleton()->Load();

		NotAChild::Install();
		MakeVunerable::Install();
		NoAliasPackages::Install();

		//DisableFlee::Install();
	}
}
