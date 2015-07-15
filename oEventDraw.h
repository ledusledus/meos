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

/** Struct with info to draw a class */
struct ClassInfo {
  int classId;
  pClass pc;

  int firstStart;
  int interval;

  int unique;
  int courseId;
  int nRunners;
  int nRunnersGroup; // Number of runners in group
  int nRunnersCourse; // Number of runners on this course

  bool nVacantSpecified;
  int nVacant;

  bool nExtraSpecified;
  int nExtra;

  int sortFactor;

  // Algorithm status. Extra time needed to start this class.
  int overShoot;

  bool hasFixedTime;

  ClassInfo() {
    memset(this, 0, sizeof(ClassInfo));
    nVacant = -1;
  }

  ClassInfo(pClass pClass) {
    memset(this, 0, sizeof(ClassInfo));
    pc = pClass;
    classId = pc->getId();
    nVacant = -1;
  }

  // Selection of sorting method
  static int sSortOrder;
  bool operator<(ClassInfo &ci);
};


/**Structure for optimizing start order */
struct DrawInfo {
  DrawInfo();
  double vacancyFactor;
  double extraFactor;
  int minVacancy;
  int maxVacancy;
  int baseInterval;
  int minClassInterval;
  int maxClassInterval;
  int nFields;
  int firstStart;
  int maxCommonControl;
  bool allowNeighbourSameCourse;
  // Statistics output from optimize start order
  int numDistinctInit;
  int numRunnerSameInitMax;
  int numRunnerSameCourseMax;
  int minimalStartDepth;

  map<int, ClassInfo> classes;
  string startName;
};
