#include "Hooks.h"

#include "Utility.h"

namespace Hooks
{
    void Install() noexcept
    {
        stl::write_vfunc<RE::Character, RemoveItem>();
        logger::info("Installed Character::RemoveItem hook");

        stl::write_vfunc<RE::Character, Load3D>();
        logger::info("Installed Character::Load3D hook");
    }

    RE::ObjectRefHandle* RemoveItem::Thunk(RE::Character* a_this, RE::ObjectRefHandle* a_hidden_return_argument, RE::TESBoundObject* a_item, std::int32_t a_count,
                                           RE::ITEM_REMOVE_REASON a_reason, RE::ExtraDataList* a_extraList, RE::TESObjectREFR* a_moveToRef, const RE::NiPoint3* a_dropLoc,
                                           const RE::NiPoint3* a_rotate) noexcept
    {
        if (!a_this) {
            return func(a_this, a_hidden_return_argument, a_item, a_count, a_reason, a_extraList, a_moveToRef, a_dropLoc, a_rotate);
        }

        const auto worn{ a_this->GetWornArmor(body_slot, true) };

        if (!worn || !a_moveToRef || !a_item) {
            return func(a_this, a_hidden_return_argument, a_item, a_count, a_reason, a_extraList, a_moveToRef, a_dropLoc, a_rotate);
        }

        if (!a_this->GetActorBase()) {
            return func(a_this, a_hidden_return_argument, a_item, a_count, a_reason, a_extraList, a_moveToRef, a_dropLoc, a_rotate);
        }

        if (!a_moveToRef->IsPlayerRef() || a_this->IsPlayerTeammate() || a_this->IsChild() || !a_this->HasKeywordString("ActorTypeNPC"sv)) {
            return func(a_this, a_hidden_return_argument, a_item, a_count, a_reason, a_extraList, a_moveToRef, a_dropLoc, a_rotate);
        }

        if (const auto race{ a_this->GetRace() }) {
            if (!strcmp(race->GetFormEditorID(), "ManakinRace")) {
                return func(a_this, a_hidden_return_argument, a_item, a_count, a_reason, a_extraList, a_moveToRef, a_dropLoc, a_rotate);
            }
        }

        if (const auto armo{ a_item->As<RE::TESObjectARMO>() }) {
            if (const auto armo_form_id{ armo->GetFormID() }; armo->HasPartOf(body_slot) && armo_form_id <=> worn->GetFormID() == 0) {
                if (Utility::underwear_formids.contains(armo_form_id)) {
                    return func(a_this, a_hidden_return_argument, a_item, a_count, a_reason, a_extraList, a_moveToRef, a_dropLoc, a_rotate);
                }

                const auto actor_name{ a_this->GetName() };
                const auto actor_form_id{ a_this->GetFormID() };
                logger::debug("{} (0x{:x}): Slot 32 item {} (0x{:x}) being taken from inventory", actor_name, actor_form_id, armo->GetName(), armo_form_id);
                const auto inv{ a_this->GetInventory(RE::TESObjectREFR::DEFAULT_INVENTORY_FILTER, true) };
                const auto manager{ RE::ActorEquipManager::GetSingleton() };
                for (const auto& obj : inv | std::views::keys) {
                    if (!Utility::underwear_formids.contains(obj->GetFormID())) {
                        continue;
                    }

                    manager->EquipObject(a_this, obj, nullptr, 1, nullptr, true, false, false, false);
                    logger::debug("{} (0x{:x}): Equipped existing {} (0x{:x}) found in inventory", actor_name, actor_form_id, obj->GetName(), obj->GetFormID());
                    return func(a_this, a_hidden_return_argument, a_item, a_count, a_reason, a_extraList, a_moveToRef, a_dropLoc, a_rotate);
                }
                std::random_device            rd;
                std::mt19937                  rng(rd());
                const auto                    size{ static_cast<int>(Utility::underwear.size() - 1) };
                std::uniform_int_distribution dist(0, size);

                const auto undie{ Utility::underwear[dist(rng)] };
                a_this->AddObjectToContainer(undie, nullptr, 1, nullptr);
                manager->EquipObject(a_this, undie, nullptr, 1, nullptr, true, false, false, false);
                logger::debug("Equipped {} (0x{:x}) to {} (0x{:x})", undie->GetName(), undie->GetFormID(), actor_name, actor_form_id);
            }
        }

        return func(a_this, a_hidden_return_argument, a_item, a_count, a_reason, a_extraList, a_moveToRef, a_dropLoc, a_rotate);
    }

    RE::NiAVObject* Load3D::Thunk(RE::Character* a_this, bool a_arg1)
    {
        if (!a_this) {
            return func(a_this, a_arg1);
        }

        if (!a_this->GetActorBase()) {
            return func(a_this, a_arg1);
        }

        if (!a_this->IsDead() || a_this->IsPlayerRef() || a_this->IsPlayerTeammate() || a_this->IsChild() || !a_this->HasKeywordString("ActorTypeNPC"sv)) {
            return func(a_this, a_arg1);
        }

        if (const auto race{ a_this->GetRace() }) {
            if (!strcmp(race->GetFormEditorID(), "ManakinRace")) {
                return func(a_this, a_arg1);
            }
        }

        const auto body_worn{ a_this->GetWornArmor(body_slot) };
        const auto head_worn{ a_this->GetWornArmor(head_slot) };
        const auto hands_worn{ a_this->GetWornArmor(hands_slot) };
        const auto feet_worn{ a_this->GetWornArmor(feet_slot) };

        if (body_worn) {
            return func(a_this, a_arg1);
        }

        const auto actor_name{ a_this->GetName() };
        const auto actor_form_id{ a_this->GetFormID() };
        const auto manager{ RE::ActorEquipManager::GetSingleton() };
        const auto inv{ a_this->GetInventory(RE::TESObjectREFR::DEFAULT_INVENTORY_FILTER, true) };
        bool       body_found{}, head_found{}, hands_found{}, feet_found{};
        for (const auto& item : inv | std::views::keys) {
            if (const auto armo{ item->As<RE::TESObjectARMO>() }) {
                const auto armo_name{ armo->GetName() };
                const auto armo_form_id{ armo->GetFormID() };
                if (armo->HasPartOf(hands_slot) && !hands_worn && !hands_found) {
                    hands_found = true;
                    manager->EquipObject(a_this, armo, nullptr, 1, nullptr, true, false, false, false);
                    logger::debug("{} (0x{:x}): Equipped hands slot item {} (0x{:x}) found in inventory on 3D load", actor_name, actor_form_id, armo_name, armo_form_id);
                }
                if (armo->HasPartOf(feet_slot) && !feet_worn && !feet_found) {
                    feet_found = true;
                    manager->EquipObject(a_this, armo, nullptr, 1, nullptr, true, false, false, false);
                    logger::debug("{} (0x{:x}): Equipped feet slot item {} (0x{:x}) found in inventory on 3D load", actor_name, actor_form_id, armo_name, armo_form_id);
                }
                if (armo->HasPartOf(head_slot) && !head_worn && !head_found) {
                    head_found = true;
                    manager->EquipObject(a_this, armo, nullptr, 1, nullptr, true, false, false, false);
                    logger::debug("{} (0x{:x}): Equipped head slot item {} (0x{:x}) found in inventory on 3D load", actor_name, actor_form_id, armo_name, armo_form_id);
                }
                if (armo->HasPartOf(body_slot) && !body_found) {
                    body_found = true;
                    manager->EquipObject(a_this, armo, nullptr, 1, nullptr, true, false, false, false);
                    logger::debug("{} (0x{:x}): Equipped body slot item {} (0x{:x}) found in inventory on 3D load", actor_name, actor_form_id, armo_name, armo_form_id);
                }
            }
        }

        return func(a_this, a_arg1);
    }
} // namespace Hooks
