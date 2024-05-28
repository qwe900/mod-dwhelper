#include "ScriptMgr.h"
#include "InstanceScript.h"
#include "Player.h"
#include "Creature.h"
#include "ScriptedGossip.h"
#include "ScriptedCreature.h"

enum DreamwalkerHealer
{
    DATA_VALITHRIA_DREAMWALKER = 10,
    NPC_VALITHRIA_DREAMWALKER = 36789,
    NPC_PRIEST = 123456,
    NPC_PALADIN = 123457,
    NPC_SHAMAN = 123458
};

enum Actions
{
    ACTION_ENTER_COMBAT = 1,
    MISSED_PORTALS = 2,
    ACTION_DEATH = 3,
};

enum Spells
{
    SPELL_BEACON_OF_LIGHT = 53563,
    SPELL_HOLY_LIGHT = 48782,
    SPELL_FLASH_OF_LIGHT = 48785,
    SPELL_HOLY_SHOCK = 48824,
    SPELL_SACRED_SHIELD = 53601,
    SPELL_CLEANSE = 4987,
    SPELL_LAY_ON_HANDS = 48788,
    SPELL_DIVINE_FAVOR = 20216,
    SPELL_DIVINE_ILLUMINATION = 31842,
    SPELL_HAND_OF_SACRIFICE = 6940,
    AURA_DREAMSTATE = 70766,
};

class npc_dreamwalker_helper : public CreatureScript
{
public:
    npc_dreamwalker_helper() : CreatureScript("npc_dreamwalker_helper") { }

    bool OnGossipHello(Player* player, Creature* creature) override
    {
        if (InstanceScript* instance = creature->GetInstanceScript())
        {
            if (instance->GetBossState(DATA_VALITHRIA_DREAMWALKER) != DONE)
            {
                AddGossipItemFor(player, GOSSIP_ICON_INTERACT_1, "|cff00ff00|TInterface\\icons\\spell_holy_holybolt:30|t|r Ich wünsche mir Hilfe von Anni", GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 1);
                AddGossipItemFor(player, GOSSIP_ICON_INTERACT_1, "|cff00ff00|TInterface\\icons\\spell_nature_magicimmunity:30|t|r Ich wünsche mir Hilfe von Corilla", GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 2);
                SendGossipMenuFor(player, 50001, creature->GetGUID());
                return true;
            }
            else
            {
                creature->Whisper("Valithria Dreamwalker ist bereits gerettet", LANG_UNIVERSAL, player);
                return true;
            }
        }
        SendGossipMenuFor(player, 50001, creature->GetGUID());
        return true;
    }

    bool OnGossipSelect(Player* player, Creature* creature, uint32 /*sender*/, uint32 action) override
    {
        int helperEntry = 0;
        ClearGossipMenuFor(player);

        switch (action)
        {
        case GOSSIP_ACTION_INFO_DEF + 1:
            helperEntry = 650001;
            break;
        case GOSSIP_ACTION_INFO_DEF + 2: // second healer is not implemented yet
            helperEntry = 650001;
            break;
        default:
            return false;
        }

        if (!player->FindNearestCreature(650001, 500.0f) && !player->FindNearestCreature(650002, 500.0f))
        {
            Position pos;
            creature->GetNearPoint(creature, pos.m_positionX, pos.m_positionY, pos.m_positionZ, 0, 5.0f, 0);
            if (Creature* helper = player->SummonCreature(helperEntry, pos, TEMPSUMMON_TIMED_OR_DEAD_DESPAWN, 1000 * 60 * 60))
            {
                helper->GetMotionMaster()->MoveFollow(player, PET_FOLLOW_DIST, PET_FOLLOW_ANGLE);
            }

            CloseGossipMenuFor(player);
            return true;
        }
        else
        {
            CloseGossipMenuFor(player);
            creature->Whisper("Du hast bereits einen Helfer beschworen", LANG_UNIVERSAL, player);
            return false;
        }
    }
};

class npc_DW_Helper : public CreatureScript
{
public:
    npc_DW_Helper() : CreatureScript("npc_DW_Helper_Paladin") {}

    struct npc_DW_HelperAI : public ScriptedAI
    {
        npc_DW_HelperAI(Creature* creature) : ScriptedAI(creature), _instance(creature->GetInstanceScript())
        {
            me->SetStat(STAT_INTELLECT, 20000);
        }

        void Reset() override
        {
            _events.Reset();
            _events.ScheduleEvent(EVENT_HEALING, 1000); // Schedule healing event
        }

        std::vector<std::string> randomTexts = {
            "Hodor! Hold The Door!",
            "I am going to stack up, please Protect Dreamwalker",
            "PAW PAW PAW PAW PATROL!",
            "Life is like a box of chocolates. You never know what you're gonna get.",
            "May the Force be with you."
        };

        bool inVoid = false;
        bool _portalfound = false;
        bool OnMovement = false;
        bool meIsInCombat = false;
        Position StandardPosition = Position(4233.2495, 2484.4174, 364.872192, M_PI);

        EventMap _events;
        InstanceScript* _instance;

        enum Events
        {
            EVENT_PORTAL = 1,
            EVENT_SEARCH_NEXT_TARGET = 2,
            EVENT_CLICK_PORTAL = 3,
            EVENT_HEALING = 4
        };

        enum Points
        {
            POINT_PORTAL = 1,
            POINT_TARGET = 2
        };

        enum Creatures
        {
            VoidSpawnedBeforePortal = 38186,
            PortalToUse = 37945,
            CloudToCollectNonHeroic = 37985,
            CloudToCollectHeroic = 38421
        };

        Unit* SelectLowestFriendlyHpTarget(float range)
        {
            Unit* lowestHpUnit = nullptr;
            float lowestHpPct = 101.0f;

            for (auto const& ref : me->GetMap()->GetPlayers())
            {
                Player* player = ref.GetSource();

                if (!player || player->isDead() || !me->IsWithinDistInMap(player, range) || !me->IsFriendlyTo(player))
                    continue;

                float hpPct = player->GetHealthPct();
                if (hpPct < lowestHpPct)
                {
                    lowestHpPct = hpPct;
                    lowestHpUnit = player;
                }
            }

            if (me->GetHealthPct() < lowestHpPct)
            {
                lowestHpPct = me->GetHealthPct();
                lowestHpUnit = me;
            }

            if (lowestHpUnit == nullptr || lowestHpUnit->GetHealthPct() > 90)
            {
                Creature* dreamwalker = me->FindNearestCreature(NPC_VALITHRIA_DREAMWALKER, 150.0f);

                if (dreamwalker && dreamwalker->GetHealthPct() < 100.0f)
                {
                    lowestHpUnit = dreamwalker->ToUnit();
                }
            }

            if (lowestHpUnit == nullptr)
            {
                lowestHpUnit = me;
            }

            return lowestHpUnit;
        }

        void JustEngagedWith(Unit* target) override
        {
            me->setActive(true);
            me->SetInCombatWithZone();
            meIsInCombat = true;
            _instance->SendEncounterUnit(ENCOUNTER_FRAME_ENGAGE, me);
        }

        void GoToStandardPosition()
        {
            me->GetMotionMaster()->MovePoint(POINT_TARGET, StandardPosition);
        }

        void MovementInform(uint32 type, uint32 id) override
        {
            if (type != POINT_MOTION_TYPE)
                return;

            if (id == POINT_TARGET)
            {
                // Schedule the event with a 500ms delay if needed
            }

            if (type == IDLE_MOTION_TYPE)
            {
                OnMovement = false;
            }
        }

        void CastEnhancedHealSpell(Unit* target, uint32 spellId)
        {
            if (!target)
                return;

            me->CastSpell(target, spellId, false);
        }

        void UpdateAI(uint32 diff) override
        {
            if (me->HasUnitState(UNIT_STATE_MOVING))
                return;

            _events.Update(diff);
            me->SetPower(POWER_MANA, 50000);

            while (uint32 eventId = _events.ExecuteEvent())
            {
                switch (eventId)
                {
                case EVENT_CLICK_PORTAL:
                    if (Creature* portal = me->FindNearestCreature(PortalToUse, 100.0f))
                    {
                        portal->HandleSpellClick(me);
                        inVoid = true;
                        _portalfound = false;
                    }
                    break;
                case EVENT_SEARCH_NEXT_TARGET:
                    if (me->HasAura(AURA_DREAMSTATE))
                    {
                        me->SetDisableGravity(true);
                        Unit* target = me->FindNearestCreature(CloudToCollectNonHeroic, 100.0f);
                        if (!target)
                            target = me->FindNearestCreature(CloudToCollectHeroic, 100.0f);

                        if (target && me->GetDistance(target) > 2.0f)
                        {
                            me->SetSpeed(MOVE_RUN, me->GetCreatureTemplate()->speed_run * 1.5);
                            me->GetMotionMaster()->MovePoint(POINT_TARGET, target->GetPositionX(), target->GetPositionY(), target->GetPositionZ());
                        }
                    }
                    break;
                case EVENT_HEALING:
                    if (me->HasUnitState(UNIT_STATE_CASTING))
                    {
                        _events.ScheduleEvent(EVENT_HEALING, 1s);
                        break;
                    }

                    if (Creature* dreamwalker = me->FindNearestCreature(NPC_VALITHRIA_DREAMWALKER, 150.0f))
                    {
                        if (!dreamwalker->HasAura(SPELL_BEACON_OF_LIGHT))
                        {
                            me->CastSpell(dreamwalker, SPELL_BEACON_OF_LIGHT, false);
                        }
                        if (!dreamwalker->HasAura(SPELL_SACRED_SHIELD))
                        {
                            me->CastSpell(dreamwalker, SPELL_SACRED_SHIELD, false);
                        }
                    }

                    Unit* target = SelectLowestFriendlyHpTarget(40.0f);

                    if (!me->HasSpellCooldown(SPELL_HOLY_SHOCK))
                    {
                        me->Yell("Holy Shock", LANG_UNIVERSAL);
                        CastEnhancedHealSpell(target, SPELL_HOLY_SHOCK);
                        me->AddSpellCooldown(SPELL_HOLY_SHOCK, 0, 8000);
                    }

                    if (!OnMovement)
                    {
                        if (!me->HasSpellCooldown(SPELL_FLASH_OF_LIGHT))
                        {
                            me->Yell("Flash of Light", LANG_UNIVERSAL);
                            CastEnhancedHealSpell(target, SPELL_FLASH_OF_LIGHT);
                            me->AddSpellCooldown(SPELL_FLASH_OF_LIGHT, 0, 7000);
                        }

                        CastEnhancedHealSpell(target, SPELL_HOLY_LIGHT);
                    }

                    _events.ScheduleEvent(EVENT_HEALING, 1s);
                    break;
                default:
                    break;
                }
            }

            if (Creature* voidZone = me->FindNearestCreature(VoidSpawnedBeforePortal, 100.0f))
            {
                if (me->GetDistance(voidZone) < 2.0f)
                {
                    OnMovement = false;
                }
                if (!inVoid && !_portalfound)
                {
                    _portalfound = true;
                    std::string randomYell = randomTexts[urand(0, randomTexts.size() - 1)];
                    me->Yell(randomYell.c_str(), LANG_UNIVERSAL);

                    Position newPosition = voidZone->GetPosition();
                    newPosition.m_positionX += cos(voidZone->GetOrientation()) * 2;
                    newPosition.m_positionY += sin(voidZone->GetOrientation()) * 2;
                    OnMovement = true;
                    me->GetMotionMaster()->MovePoint(POINT_PORTAL, newPosition);

                    _events.ScheduleEvent(EVENT_CLICK_PORTAL, 16s);
                }
            }

            if (me->HasAura(AURA_DREAMSTATE))
            {
                _events.ScheduleEvent(EVENT_SEARCH_NEXT_TARGET, 2500ms);
            }
            else
            {
                inVoid = false;
            }

            if (!me->HasAura(AURA_DREAMSTATE) && me->GetMotionMaster()->GetCurrentMovementGeneratorType() != FOLLOW_MOTION_TYPE && !_portalfound)
            {
                GoToStandardPosition();
                me->SetReactState(REACT_DEFENSIVE);
            }
        }
    };

    CreatureAI* GetAI(Creature* creature) const override
    {
        return new npc_DW_HelperAI(creature);
    }
};

void AdddwhelperScripts()
{
    new npc_dreamwalker_helper();
    new npc_DW_Helper();
}
