/************************************************************************
    MeOS - Orienteering Software
    Copyright (C) 2009-2015 Melin Software HB

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

    Melin Software HB - software@melin.nu - www.melin.nu
    Stigbergsvägen 7, SE-75242 UPPSALA, Sweden

************************************************************************/

#include "stdafx.h"
#include "TabBase.h"

#include "oEvent.h"

#include "TabRunner.h"
#include "TabTeam.h"
#include "TabList.h"
#include "TabSpeaker.h"
#include "TabClass.h"
#include "TabCourse.h"
#include "TabControl.h"
#include "TabClub.h"
#include "TabSI.h"
#include "TabCompetition.h"
#include "TabAuto.h"

extern oEvent *gEvent;

FixedTabs::FixedTabs() {
  runnerTab = 0;
  teamTab = 0;
  classTab = 0;
  courseTab = 0;
  controlTab = 0;
  siTab = 0;
  listTab = 0;
  cmpTab = 0;
  speakerTab = 0;
  clubTab = 0;
  autoTab = 0;
 }

FixedTabs::~FixedTabs() {
  delete runnerTab;
  runnerTab = 0;

  delete teamTab;
  teamTab = 0;

  delete classTab;
  classTab = 0;

  delete courseTab;
  courseTab = 0;

  delete controlTab;
  controlTab = 0;

  delete siTab;
  siTab = 0;

  delete listTab;
  listTab = 0;

  delete cmpTab;
  cmpTab = 0;

  delete speakerTab;
  speakerTab = 0;

  delete clubTab;
  clubTab = 0;

  delete autoTab;
  autoTab = 0;
}

TabBase *FixedTabs::get(const TabType tab) {
  switch(tab) {
    case TCmpTab:
      if (!cmpTab)
        cmpTab = new TabCompetition(gEvent);
      return cmpTab;
    break;
    case TRunnerTab:
      if (!runnerTab)
        runnerTab = new TabRunner(gEvent);
      return runnerTab;
    break;
    case TTeamTab:
      if (!teamTab)
        teamTab = new TabTeam(gEvent);
      return teamTab;
    break;

    case TListTab:
      if (!listTab)
        listTab = new TabList(gEvent);
      return listTab;
    break;

    case TClassTab:
      if (!classTab)
        classTab = new TabClass(gEvent);
      return classTab;
    break;

    case TCourseTab:
      if (!courseTab)
        courseTab = new TabCourse(gEvent);
      return courseTab;
    break;

    case TControlTab:
      if (!controlTab)
        controlTab = new TabControl(gEvent);
      return controlTab;
    break;

    case TClubTab:
      if (!clubTab)
        clubTab = new TabClub(gEvent);
      return clubTab;
    break;

    case TSpeakerTab:
      if (!speakerTab)
        speakerTab = new TabSpeaker(gEvent);
      return speakerTab;
    break;

    case TSITab:
      if (!siTab)
        siTab = new TabSI(gEvent);
      return siTab;
    break;

    case TAutoTab:
      if (!autoTab)
        autoTab = new TabAuto(gEvent);
      return autoTab;
    break;

    default:
      throw new std::exception("Bad tab type");
  }

  return 0;
}


TabObject::TabObject(TabBase *t)
{
  tab = t;
  tab->tabId = id;
}

TabObject::~TabObject()
{
  //delete tab;
}


bool TabObject::leavePage(gdioutput &gdi)
{
  if (tab)
    return tab->leavePage(gdi);
  else return false;  
}
bool TabObject::loadPage(gdioutput &gdi)
{
  if (tab)
    return tab->loadPage(gdi);
  else return false;
}