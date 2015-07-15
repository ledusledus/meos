#pragma once
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
#include "tabbase.h"

struct TeamLineup;

class TabTeam :
  public TabBase
{
private:
  bool save(gdioutput &gdi, bool dontReloadTeams);

  string lastSearchExpr;
  stdext::hash_set<int> lastFilter;
  DWORD timeToFill;
  int inputId;
  int searchCB(gdioutput &gdi, int type, void *data);

  int teamId;
  int classId;
  void selectTeam(gdioutput &gdi, pTeam t);
  void updateTeamStatus(gdioutput &gdi, pTeam t);
  void loadTeamMembers(gdioutput &gdi, int ClassId,
                       int ClubId, pTeam t);

  int shownRunners;
  int shownDistinctRunners;
  const string &getSearchString() const;

  void fillTeamList(gdioutput &gdi);
  void addToolbar(gdioutput &gdi) const;

  int currentMode;

  void showTeamImport(gdioutput &gdi);
  void doTeamImport(gdioutput &gdi);
  void saveTeamImport(gdioutput &gdi, bool useExisting);

  pRunner findRunner(const string &name, int cardNo) const;
  vector<TeamLineup> teamLineup;

public:

  int teamCB(gdioutput &gdi, int type, void *data);

  bool loadPage(gdioutput &gdi, int id);
  bool loadPage(gdioutput &gdi);
  TabTeam(oEvent *oe);
  ~TabTeam(void);
  friend int teamSearchCB(gdioutput *gdi, int type, void *data);

};
