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

#include "oFreeImport.h"

class TabCompetition :
  public TabBase
{
  string eventorBase;
  void textSizeControl(gdioutput &gdi) const;

  bool showConnectionPage;
  bool importFile(HWND hWnd, gdioutput &gdi);
  bool exportFileAs(HWND hWnd, gdioutput &gdi);

  bool save(gdioutput &gdi, bool write = true);

  void loadRunnerDB(gdioutput &gdi, int tableToShow, bool updateTableOnly);

  // Events from Eventor
  vector<CompetitionInfo> events;

  oFreeImport fi;
  string entryText;
  vector<oEntryBlock> entries;
  void loadConnectionPage(gdioutput &gdi);

  string defaultServer;
  string defaultName;
  string defaultPwd;
  string defaultPort;

  void copyrightLine(gdioutput &gdi) const;
  void loadAboutPage(gdioutput &gdi) const;

  int organizorId;

  int lastChangeClassType;

  struct {
    string name;
    string careOf;
    string street;
    string city;
    string zipCode;
    string account;
    string email;
  } eventor;

  int getOrganizer(bool updateEvent);
  void getAPIKey(vector< pair<string, string> > &key) const;
  void getEventorCompetitions(gdioutput &gdi,
                              const string &fromDate,
                              vector<CompetitionInfo> &events) const;

  void getEventorCmpData(gdioutput &gdi, int id,
                         const string &eventFile,
                         const string &clubFile,
                         const string &classFile,
                         const string &entryFile,
                         const string &dbFile) const;

  void loadMultiEvent(gdioutput &gdi);
  void saveMultiEvent(gdioutput &gdi);

  string eventorOrigin; // The command used when checking eventor
  bool checkEventor(gdioutput &gdi, ButtonInfo &bi);

  bool useEventor() const;
  bool useEventorUTC() const;

  void openCompetition(gdioutput &gdi, int id);
  void selectTransferClasses(gdioutput &gdi, bool expand);

  // Welcome page for new users
  void welcomeToMeOS(gdioutput &gdi);

  // Class id for last selected class for entry
  int lastSelectedClass;

  set<int> allTransfer;

  void displayRunners(gdioutput &gdi, const vector<pRunner> &changedClass) const;

  void meosFeatures(gdioutput &gdi, bool newGuide);

  void newCompetitionGuide(gdioutput &gdi, int step);

  void entryForm(gdioutput &gdi, bool isGuide);
  void saveEntries(gdioutput &gdi, bool updateClass, bool isGuide);

  void selectStartlistOptions(gdioutput &gdi);
  void selectExportSplitOptions(gdioutput &gdi);

  void entryChoice(gdioutput &gdi);
  void createCompetition(gdioutput &gdi);
public:
  void saveMeosFeatures(gdioutput &gdi, bool write);
  void updateFeatureStatus(gdioutput &gdi);

  void setEventorServer(const string &server);
  void setEventorUTC(bool useUTC);

  int competitionCB(gdioutput &gdi, int type, void *data);
  int restoreCB(gdioutput &gdi, int type, void *data);
  int newGuideCB(gdioutput &gdi, int type, void *data);

  bool loadPage(gdioutput &gdi);
  TabCompetition(oEvent *oe);
  ~TabCompetition(void);
};
