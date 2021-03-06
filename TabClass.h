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
#include "oEventDraw.h"

class TabClass :
  public TabBase
{
  struct PursuitSettings {
    bool use;
    int firstTime;
    int maxTime;

    PursuitSettings(oClass &c) {
      firstTime = 3600;
      use = c.interpretClassType() != ctOpen;
      maxTime = 3600;
    }
  };

  map<int, PursuitSettings> pSettings;

  enum DrawMethod {
    NOMethod = -1,

    DMRandom = 1,
    DMSOFT = 2,
    DMClumped = 3,
    DMSimultaneous = 4,

    DMPursuit = 11,
    DMReversePursuit = 12,
    DMSeeded = 13
  };

  bool EditChanged;
  int ClassId;
  int currentStage;
  string storedNStage;
  string storedStart;
  oEvent::PredefinedTypes storedPredefined;
  bool showForkingGuide;

  bool checkClassSelected(const gdioutput &gdi) const;
  void save(gdioutput &gdi, bool skipReload);
  void legSetup(gdioutput &gdi);
  vector<ClassInfo> cInfo;

  map<int, ClassInfo> cInfoCache;

  DrawInfo drawInfo;
  void setMultiDayClass(gdioutput &gdi, bool hasMulti, TabClass::DrawMethod defaultMethod);
  void drawDialog(gdioutput &gdi, TabClass::DrawMethod method, const oClass &cls);

  void pursuitDialog(gdioutput &gdi);

  bool hasWarnedDirect;
  bool tableMode;
  DrawMethod lastDrawMethod;
  // Generate a table with class settings
  void showClassSettings(gdioutput &gdi);

  void visualizeField(gdioutput &gdi);

  // Read input from the table with class settings
  void readClassSettings(gdioutput &gdi);

  // Prepare for drawing by declaring starts and blocks
  void prepareForDrawing(gdioutput &gdi);

  void showClassSelection(gdioutput &gdi, int &bx, int &by) const;

  // Set simultaneous start in a class
  void simultaneous(int classId, string time);

  void updateFairForking(gdioutput &gdi, pClass pc) const;
  void selectCourses(gdioutput &gdi, int legNo);
  bool showMulti(bool singleOnly) const;

  void defineForking(gdioutput &gdi, bool clearSettings);
  vector< vector<int> > forkingSetup;
  static const char *getCourseLabel(bool pool);
public:
  void clear();

  void closeWindow(gdioutput &gdi);

  void multiCourse(gdioutput &gdi, int nLeg);
  bool loadPage(gdioutput &gdi);
  void selectClass(gdioutput &gdi, int cid);

  int classCB(gdioutput &gdi, int type, void *data);
  int multiCB(gdioutput &gdi, int type, void *data);

  TabClass(oEvent *oe);
  ~TabClass(void);
};
