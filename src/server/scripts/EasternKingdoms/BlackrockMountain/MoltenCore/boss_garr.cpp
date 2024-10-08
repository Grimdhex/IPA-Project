/*
 * This file is part of the TrinityCore Project. See AUTHORS file for Copyright information
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "ScriptMgr.h"
#include "molten_core.h"
#include "ObjectMgr.h"
#include "ScriptedCreature.h"

enum Spells
{
    // Garr
    SPELL_ANTIMAGIC_PULSE       = 19492,
    SPELL_MAGMA_SHACKLES        = 19496,
    SPELL_FRENZY                = 19516,

    // Adds
    SPELL_ERUPTION              = 19497,
    SPELL_IMMOLATE              = 15732,
    SPELL_SEPARATION_ANXIETY    = 23492
};

enum Events
{
    EVENT_ANTIMAGIC_PULSE    = 1,
    EVENT_MAGMA_SHACKLES     = 2,
    EVENT_FRENZY             = 3
};

struct boss_garr : public BossAI
{
    boss_garr(Creature* creature) : BossAI(creature, BOSS_GARR) { }

    void JustEngagedWith(Unit* victim) override
    {
        BossAI::JustEngagedWith(victim);

        events.ScheduleEvent(EVENT_ANTIMAGIC_PULSE, 15s);
        events.ScheduleEvent(EVENT_MAGMA_SHACKLES, 10s);
        events.ScheduleEvent(EVENT_FRENZY, 10min);
    }

    void ExecuteEvent(uint32 eventId) override
    {
        switch (eventId)
        {
            case EVENT_ANTIMAGIC_PULSE:
                DoCastSelf(SPELL_ANTIMAGIC_PULSE);
                events.Repeat(20s);
                break;
            case EVENT_MAGMA_SHACKLES:
                DoCastSelf(SPELL_MAGMA_SHACKLES);
                events.Repeat(15s);
                break;
            case EVENT_FRENZY:
                DoCastSelf(SPELL_FRENZY);
                break;
            default:
                break;
        }
    }
};

struct npc_firesworn : public ScriptedAI
{
    npc_firesworn(Creature* creature) : ScriptedAI(creature) { }

    void ScheduleTasks()
    {
        // Timers for this are probably wrong
        _scheduler.Schedule(4s, [this](TaskContext context)
        {
            if (Unit* target = SelectTarget(SelectTargetMethod::Random, 0))
                DoCast(target, SPELL_IMMOLATE);

            context.Repeat(5s, 10s);
        });

        // Separation Anxiety - Periodically check if Garr is nearby
        // ...and enrage if he is not.
        _scheduler.Schedule(3s, [this](TaskContext context)
        {
            if (!me->FindNearestCreature(NPC_GARR, 20.0f))
                DoCastSelf(SPELL_SEPARATION_ANXIETY);
            else if (me->HasAura(SPELL_SEPARATION_ANXIETY))
                me->RemoveAurasDueToSpell(SPELL_SEPARATION_ANXIETY);

            context.Repeat();
        });
    }

    void Reset() override
    {
        _scheduler.CancelAll();
    }

    void JustEngagedWith(Unit* /*who*/) override
    {
        ScheduleTasks();
    }

    void DamageTaken(Unit* /*attacker*/, uint32& damage, DamageEffectType /*damageType*/, SpellInfo const* /*spellInfo = nullptr*/) override
    {
        if (me->HealthBelowPctDamaged(10, damage))
        {
            DoCastVictim(SPELL_ERUPTION);
            me->DespawnOrUnsummon();
        }
    }

    void UpdateAI(uint32 diff) override
    {
        if (!UpdateVictim())
            return;

        _scheduler.Update(diff, [this]
        {
            DoMeleeAttackIfReady();
        });
    }

private:
    TaskScheduler _scheduler;
};

void AddSC_boss_garr()
{
    RegisterMoltenCoreCreatureAI(boss_garr);
    RegisterMoltenCoreCreatureAI(npc_firesworn);
}
