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

class TabCourse :
  public TabBase
{
  int courseId;
  void save(gdioutput &gdi);
  int courseCB(gdioutput &gdi, int type, void *data);
  bool addedCourse;

  string time_limit;
  string point_limit;
  string point_reduction;

  void fillCourseControls(gdioutput &gdi, oEvent &oe, const string &ctrl);

public:
  void selectCourse(gdioutput &gdi, pCourse pc);

  bool loadPage(gdioutput &gdi);
  TabCourse(oEvent *oe);
  ~TabCourse(void);

  static void runCourseImport(gdioutput& gdi, const string &filename,
                              oEvent *oe, bool addClasses);

  static void setupCourseImport(gdioutput& gdi, GUICALLBACK cb);

  friend int CourseCB(gdioutput *gdi, int type, void *data);
};
