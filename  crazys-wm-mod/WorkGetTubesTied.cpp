/*
* Copyright 2009, 2010, The Pink Petal Development Team.
* The Pink Petal Devloment Team are defined as the game's coders
* who meet on http://pinkpetal.org     // old site: http://pinkpetal .co.cc
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#pragma region //	Includes and Externs			//
#include "cJobManager.h"
#include "cBrothel.h"
#include "cClinic.h"
#include "cCustomers.h"
#include "cRng.h"
#include "cInventory.h"
#include "sConfig.h"
#include "cRival.h"
#include <sstream>
#include "CLog.h"
#include "cTrainable.h"
#include "cTariff.h"
#include "cGold.h"
#include "cGangs.h"
#include "cMessageBox.h"
#include "libintl.h"

extern cRng g_Dice;
extern CLog g_LogFile;
extern cCustomers g_Customers;
extern cInventory g_InvManager;
extern cBrothelManager g_Brothels;
extern cClinicManager g_Clinic;
extern cGangManager g_Gangs;
extern cMessageQue g_MessageQue;
extern cGold g_Gold;

#pragma endregion

// `J` Job Clinic - Surgery
bool cJobManager::WorkGetTubesTied(sGirl* girl, sBrothel* brothel, bool Day0Night1, string& summary)
{
#pragma region //	Job setup				//
	stringstream ss; string girlName = girl->m_Realname; ss << girlName;
	// if she was not in surgery last turn, reset working days to 0 before proceding
	if (girl->m_YesterDayJob != JOB_TUBESTIED) { girl->m_WorkingDay = girl->m_PrevWorkingDay = 0; }
	girl->m_DayJob = girl->m_NightJob = JOB_TUBESTIED;	// it is a full time job

	if (girl->is_pregnant() || girl->has_trait( "Sterile"))
	{
		if (girl->has_trait( "Sterile"))	ss << " is already Sterile so she was sent to the waiting room.";
		else if (girl->is_pregnant())		ss << " is pregant.\nShe must either have her baby or get an abortion before She can get her Tubes Tied.";
		if (Day0Night1 == SHIFT_DAY)	girl->m_Events.AddMessage(ss.str(), IMGTYPE_PROFILE, EVENT_WARNING);
		girl->m_PrevDayJob = girl->m_PrevNightJob = girl->m_YesterDayJob = girl->m_YesterNightJob = girl->m_DayJob = girl->m_NightJob = JOB_CLINICREST;
		girl->m_WorkingDay = girl->m_PrevWorkingDay = 0;
		return false;	// not refusing
	}
	bool hasDoctor = g_Clinic.GetNumGirlsOnJob(0, JOB_DOCTOR, Day0Night1) > 0;
	int numnurse = g_Clinic.GetNumGirlsOnJob(0, JOB_NURSE, Day0Night1);
	if (!hasDoctor)
	{
		ss << " does nothing. You don't have any Doctors working. (require 1) ";
		girl->m_Events.AddMessage(ss.str(), IMGTYPE_PROFILE, EVENT_WARNING);
		return false;	// not refusing
	}
	ss << " is in the Clinic to get her tubes tied.\n\n";

	int msgtype = Day0Night1;
	g_Girls.UnequipCombat(girl);	// not for patient

#pragma endregion
#pragma region //	Count the Days				//

	if (Day0Night1 == SHIFT_DAY)	// the Doctor works on her durring the day
	{
		girl->m_WorkingDay++;
	}
	else	// and if there are nurses on duty, they take care of her at night
	{
		if (g_Clinic.GetNumGirlsOnJob(0, JOB_NURSE, 1) > 0)
		{
			girl->m_WorkingDay++;
			g_Girls.UpdateStat(girl, STAT_HEALTH, 10);
			g_Girls.UpdateStat(girl, STAT_HAPPINESS, 10);
			g_Girls.UpdateStat(girl, STAT_MANA, 10);
		}
	}

#pragma endregion
#pragma region //	In Progress				//

	if (girl->m_WorkingDay < 5 || Day0Night1 == SHIFT_DAY)
	{
		int wdays = (5 - girl->m_WorkingDay);
		if (g_Clinic.GetNumGirlsOnJob(0, JOB_NURSE, 1) > 0)
		{
			if (wdays >= 3)		{ wdays = 3; }
			else if (wdays > 1)	{ wdays = 2; }
			else				{ wdays = 1; }
		}
		ss << "The operation is in progress (" << wdays << " day remaining).\n";
		if (g_Clinic.GetNumGirlsOnJob(0, JOB_NURSE, 1) > 1)		{ ss << "The Nurses are taking care of her at night."; }
		else if (g_Clinic.GetNumGirlsOnJob(0, JOB_NURSE, 1) > 0){ ss << "The Nurse is taking care of her at night."; }
		else							{ ss << "Having a Nurse on duty will speed up her recovery."; }
	}

#pragma endregion
#pragma region //	Surgery Finished			//

	else
	{
		ss << "The surgery is a success.\n";
		msgtype = EVENT_GOODNEWS;
		girl->m_WorkingDay = girl->m_PrevWorkingDay = 0;

		if (numnurse > 2)
		{
			ss << "The Nurses kept her healthy and happy during her recovery.\n";
			g_Girls.UpdateStat(girl, STAT_HEALTH, g_Dice.bell(0, 20));
			g_Girls.UpdateStat(girl, STAT_HAPPINESS, g_Dice.bell(0, 10));
			g_Girls.UpdateStat(girl, STAT_SPIRIT, g_Dice.bell(0, 10));
			g_Girls.UpdateStat(girl, STAT_MANA, g_Dice.bell(0, 20));
			g_Girls.UpdateStat(girl, STAT_BEAUTY, g_Dice.bell(0, 2));
			g_Girls.UpdateStat(girl, STAT_CHARISMA, g_Dice.bell(0, 2));
		}
		else if (numnurse > 0)
		{
			ss << "The Nurse" << (numnurse > 1 ? "s" : "") << " helped her during her recovery.\n";
			g_Girls.UpdateStat(girl, STAT_HEALTH, g_Dice.bell(0, 10));
			g_Girls.UpdateStat(girl, STAT_HAPPINESS, g_Dice.bell(0, 5));
			g_Girls.UpdateStat(girl, STAT_SPIRIT, g_Dice.bell(0, 5));
			g_Girls.UpdateStat(girl, STAT_MANA, g_Dice.bell(0, 10));
			g_Girls.UpdateStat(girl, STAT_BEAUTY, g_Dice % 2);
			g_Girls.UpdateStat(girl, STAT_CHARISMA, g_Dice % 2);
		}
		else
		{
			ss << "She is sad and has lost some health during the operation.\n";
			g_Girls.UpdateStat(girl, STAT_HEALTH, g_Dice.bell(-20, 2));
			g_Girls.UpdateStat(girl, STAT_HAPPINESS, g_Dice.bell(-10, 1));
			g_Girls.UpdateStat(girl, STAT_SPIRIT, g_Dice.bell(-5, 1));
			g_Girls.UpdateStat(girl, STAT_MANA, g_Dice.bell(-20, 3));
			g_Girls.UpdateStat(girl, STAT_BEAUTY, g_Dice.bell(-1, 1));
			g_Girls.UpdateStat(girl, STAT_CHARISMA, g_Dice.bell(-1, 1));
		}

		girl->m_PrevDayJob = girl->m_PrevNightJob = girl->m_YesterDayJob = girl->m_YesterNightJob = girl->m_DayJob = girl->m_NightJob = JOB_CLINICREST;
		ss << g_Girls.AdjustTraitGroupFertility(girl, -10, false);
	}

#pragma endregion
#pragma region	//	Finish the shift			//

	girl->m_Events.AddMessage(ss.str(), IMGTYPE_PROFILE, msgtype);

	// Improve girl
	int libido = 1;
	if (girl->has_trait( "Lesbian"))		libido += numnurse;
	if (girl->has_trait( "Masochist"))	libido += 1;
	if (girl->has_trait( "Nymphomaniac"))	libido += 2;
	g_Girls.UpdateStatTemp(girl, STAT_LIBIDO, libido);
	if (g_Dice % 10 == 0)
		g_Girls.UpdateSkill(girl, SKILL_MEDICINE, 1);	// `J` she watched what the doctors and nurses were doing

#pragma endregion
	return false;
}

double cJobManager::JP_GetTubesTied(sGirl* girl, bool estimate)
{
	if (girl->has_trait( "Sterile"))		return -1000;	// X - not needed
	if (girl->is_pregnant())					return 0;		// E - needs abortion or birth first
	if (girl->has_trait( "Broodmother"))	return 200;		// A
	if (girl->has_trait( "Fertile"))		return 100;		// B
	return 150;													// C
}
