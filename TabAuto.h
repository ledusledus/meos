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
#include "gdioutput.h"
#include "Printer.h"
#include <string>
#include "oListInfo.h"

using namespace std;

class TabAuto;
class gdioutput;
class oEvent;

enum AutoSyncType {SyncNone, SyncTimer, SyncDataUp};

enum Machines {
  mPunchMachine,
  mPrintResultsMachine,
  mSplitsMachine,
  mPrewarningMachine,
  mOnlineResults,
  mOnlineInput,
};

class AutoMachine
{
protected:
  bool editMode;

  void settingsTitle(gdioutput &gdi, char *title);
  enum IntervalType {IntervalNone, IntervalMinute, IntervalSecond};
  void startCancelInterval(gdioutput &gdi, char *startCommand, bool created, IntervalType type, const string &interval);

public:
  static AutoMachine* construct(Machines);
  void setEditMode(bool em) {editMode = em;}
  string name;
  DWORD interval; //Interval seconds
  DWORD timeout; //Timeout (TickCount)
  bool synchronize;
  bool synchronizePunches;
  virtual void settings(gdioutput &gdi, oEvent &oe, bool created) = 0;
  virtual void save(oEvent &oe, gdioutput &gdi) {}
  virtual void process(gdioutput &gdi, oEvent *oe, AutoSyncType ast) = 0;
  virtual bool isEditMode() const {return editMode;}
  virtual void status(gdioutput &gdi) = 0;
  virtual bool stop() {return true;}
  virtual AutoMachine *clone() const = 0;
  AutoMachine(const string &s) : name(s), interval(0), timeout(0),
            synchronize(false), synchronizePunches(false), editMode(false) {}
  virtual ~AutoMachine() = 0 {}
};

class PrintResultMachine :
  public AutoMachine
{
protected:
  string exportFile;
  string exportScript;
  bool doExport;
  bool doPrint;
  bool structuredExport;
  PrinterObject po;
  set<int> classesToPrint;
  bool pageBreak;
  bool showInterResult;
  bool splitAnalysis;
  bool notShown;
  oListInfo listInfo;
  bool readOnly;
  int htmlRefresh;
  bool lock; // true while printing
public:
  PrintResultMachine *clone() const {
    PrintResultMachine *prm = new PrintResultMachine(*this);
    prm->lock = false;
    return prm;
  }
  void status(gdioutput &gdi);
  void process(gdioutput &gdi, oEvent *oe, AutoSyncType ast);
  void settings(gdioutput &gdi, oEvent &oe, bool created);

  PrintResultMachine(int v):AutoMachine("Resultatutskrift") {
    interval=v;
    pageBreak = true;
    showInterResult = true;
    notShown = true;
    splitAnalysis = true;
    lock = false;
    readOnly = false;
    doExport = false;
    doPrint = true;
    structuredExport = true;
    htmlRefresh = v;
  }
  PrintResultMachine(int v, const oListInfo &li):AutoMachine("Utskrift / export"), listInfo(li) {
    interval=v;
    pageBreak = true;
    showInterResult = true;
    notShown = true;
    splitAnalysis = true;
    lock = false;
    readOnly = true;
    doExport = false;
    doPrint = true;
    structuredExport = false;
    htmlRefresh = v;
  }
  friend class TabAuto;
};


class PrewarningMachine :
  public AutoMachine
{
protected:
  string waveFolder;
  set<int> controls;
  set<int> controlsSI;
public:
  void settings(gdioutput &gdi, oEvent &oe, bool created);
  PrewarningMachine *clone() const {return new PrewarningMachine(*this);}
  void status(gdioutput &gdi);
  void process(gdioutput &gdi, oEvent *oe, AutoSyncType ast);
  PrewarningMachine():AutoMachine("Förvarningsröst") {}
  friend class TabAuto;
};

class MySQLReconnect :
  public AutoMachine
{
protected:
  string error;
  string timeError;
  string timeReconnect;
  HANDLE hThread;
public:
  void settings(gdioutput &gdi, oEvent &oe, bool created);
  MySQLReconnect *clone() const {return new MySQLReconnect(*this);}
  void status(gdioutput &gdi);
  void process(gdioutput &gdi, oEvent *oe, AutoSyncType ast);
  bool stop();
  MySQLReconnect(const string &error);
  virtual ~MySQLReconnect();
  friend class TabAuto;
};

bool isThreadReconnecting();

class PunchMachine :
  public AutoMachine
{
protected:
  int radio;
public:
  PunchMachine *clone() const {return new PunchMachine(*this);}
  void settings(gdioutput &gdi, oEvent &oe, bool created);
  void status(gdioutput &gdi);
  void process(gdioutput &gdi, oEvent *oe, AutoSyncType ast);
  PunchMachine():AutoMachine("Stämplingsautomat"), radio(0) {}
  friend class TabAuto;
};

class SplitsMachine :
  public AutoMachine
{
protected:
  string file;
  set<int> classes;
  int leg;
public:
  SplitsMachine *clone() const {return new SplitsMachine(*this);}
  void settings(gdioutput &gdi, oEvent &oe, bool created);
  void status(gdioutput &gdi);
  void process(gdioutput &gdi, oEvent *oe, AutoSyncType ast);
  SplitsMachine() : AutoMachine("Sträcktider/WinSplits"), leg(-1) {}
  friend class TabAuto;
};



class TabAuto :
  public TabBase
{
private:
  //DWORD printResultIntervalSec;
  //DWORD printResultTimeOut;
  bool editMode;

  bool synchronize;
  bool synchronizePunches;
  void updateSyncInfo();

  list<AutoMachine *> machines;
  void setTimer(AutoMachine *am);

  void timerCallback(gdioutput &gdi);
  void syncCallback(gdioutput &gdi);

  void settings(gdioutput &gdi, AutoMachine *sm, Machines type);

public:

  AutoMachine *getMachine(const string &name);
  bool stopMachine(AutoMachine *am);
  void killMachines();
  void addMachine(const AutoMachine &am) {
    machines.push_back(am.clone());
    setTimer(machines.back());
  }

  int processButton(gdioutput &gdi, const ButtonInfo &bu);
  int processListBox(gdioutput &gdi, const ListBoxInfo &bu);

  bool loadPage(gdioutput &gdi);
  TabAuto(oEvent *poe);
  ~TabAuto(void);

  friend class AutoTask;
  friend void tabForceSync(gdioutput &gdi, pEvent oe);
};

void tabAutoKillMachines();
void tabAutoRegister(TabAuto *ta);
void tabAutoAddMachinge(const AutoMachine &am);
