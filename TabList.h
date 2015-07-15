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
#include "oListInfo.h"

class ListEditor;
class MethodEditor;

class TabList :
  public TabBase
{
protected:
  EStdListType currentListType;
  oListInfo currentList;
  string SelectedList;
  string lastInputNumber;
  int lastLimitPer;
  bool lastInterResult;
  bool lastSplitState;
  bool lastLargeSize;

  int infoCX;
  int infoCY;

  static void createListButtons(gdioutput &gdi);

  void generateList(gdioutput &gdi);
  void selectGeneralList(gdioutput &gdi, EStdListType type);

  int offsetY;
  int offsetX;
  set<int> lastClassSelection;

  bool hideButtons;
  bool ownWindow;
  ListEditor *listEditor;
  MethodEditor *methodEditor;

  bool noReEvaluate;
  void enableFromTo(gdioutput &gdi, bool from, bool to);

  int baseButtons(gdioutput &gdi, int extraButtons);
public:
  bool loadPage(gdioutput &gdi);
  bool loadPage(gdioutput &gdi, const string &command);

  int listCB(gdioutput &gdi, int type, void *data);
  void loadGeneralList(gdioutput &gdi);
  void rebuildList(gdioutput &gdi);
  void settingsResultList(gdioutput &gdi);

  static void splitPrintSettings(oEvent &oe, gdioutput &gdi, TabType returnMode);
  static void customTextLines(oEvent &oe, const char *dataField, gdioutput &gdi);
  static void saveExtraLines(oEvent &oe, const char *dataField, gdioutput &gdi);

  ListEditor *getListeditor() const {return listEditor;}


  TabList(oEvent *oe);
  ~TabList(void);
  friend int ListsEventCB(gdioutput *gdi, int type, void *data);
};
