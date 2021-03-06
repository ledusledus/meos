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
    Stigbergsv�gen 7, SE-75242 UPPSALA, Sweden

************************************************************************/

#include "stdafx.h"

#include <algorithm>
#include <cassert>

#include "iof30interface.h"
#include "oEvent.h"
#include "gdioutput.h"
#include "gdifonts.h"
#include "xmlparser.h"
#include "RunnerDB.h"
#include "meos_util.h"
#include "meosException.h"
#include "localizer.h"

string &getFirst(string &inout, int maxNames);
string getMeosCompectVersion();

void IOF30Interface::readCourseData(gdioutput &gdi, const xmlobject &xo, bool updateClass,
                                    int &courseCount, int &failed) {
  string ver;
  xo.getObjectString("iofVersion", ver);
  if (!ver.empty() && ver > "3.0")
    gdi.addString("", 0, "Varning, ok�nd XML-version X#" + ver);
  courseCount = 0;
  failed = 0;
  xmlList xl;
  xo.getObjects("RaceCourseData", xl);
  xmlList::const_iterator it;
  xmlobject xRaceCourses;
  if (xl.size() == 1) {
    xRaceCourses = xl[0];
  }
  else {
    int nr = getStageNumber();
    int ix = -1;
    for (size_t k = 0; k < xl.size(); k++) {
      if (xl[k].getObjectInt("raceNumber") == nr) {
        ix = k;
        break;
      }
    }
    if (ix == -1)
      throw meosException("Filen inneh�ller flera upps�ttningar banor, men ingen har samma etappnummer som denna etapp (X).#" + itos(nr));
    else
      xRaceCourses = xl[ix];
  }

  xmlList xControls, xCourse, x;
  xRaceCourses.getObjects("Control", xControls);
  xRaceCourses.getObjects("Course", xCourse);

  for (size_t k = 0; k < xControls.size(); k++) {
    readControl(xControls[k]);
  }

  map<string, pCourse> courses;
  map<string, vector<pCourse> > coursesFamilies;

  for (size_t k = 0; k < xCourse.size(); k++) {
    pCourse pc = readCourse(xCourse[k]);
    if (pc) {
      courseCount++;
      if (courses.count(pc->getName()))
        gdi.addString("", 0, "Varning: Banan 'X' f�rekommer flera g�nger#" + pc->getName());

      courses[pc->getName()] = pc;

      string family;
      xCourse[k].getObjectString("CourseFamily", family);

      if (!family.empty()) {
        coursesFamilies[family].push_back(pc);
      }
    }
    else
      failed++;
  }

  if (!updateClass)
    return;


  xmlList xClassAssignment, xTeamAssignment, xPersonAssignment;
  xRaceCourses.getObjects("ClassCourseAssignment", xClassAssignment);
  if (xClassAssignment.size() > 0)
    classCourseAssignment(gdi, xClassAssignment, courses, coursesFamilies);

  xRaceCourses.getObjects("PersonCourseAssignment", xPersonAssignment);
  if (xPersonAssignment.size() > 0)
    personCourseAssignment(gdi, xPersonAssignment, courses);

  xRaceCourses.getObjects("TeamCourseAssignment", xTeamAssignment);
  if (xTeamAssignment.size() > 0)
    teamCourseAssignment(gdi, xTeamAssignment, courses);

  xmlList xAssignment;
  xRaceCourses.getObjects("CourseAssignment", xAssignment);
  if (xAssignment.size() > 0) {
    classAssignmentObsolete(gdi, xAssignment, courses, coursesFamilies);
  }

}

void IOF30Interface::classCourseAssignment(gdioutput &gdi, xmlList &xAssignment,
                                           const map<string, pCourse> &courses,
                                           const map<string, vector<pCourse> > &coursesFamilies) {

  map< pair<int, int>, vector<string> > classIdLegToCourse;

  for (size_t k = 0; k < xAssignment.size(); k++) {
    xmlobject &xClsAssignment = xAssignment[k];
    map<int, vector<int> > cls2Stages;

    xmlList xClsId;
    xClsAssignment.getObjects("ClassId", xClsId);
    for (size_t j = 0; j <xClsId.size(); j++) {
      int id = xClsId[j].getInt();
      if (oe.getClass(id) == 0) {
        gdi.addString("", 0, "Klass saknad").setColor(colorRed);
      }
      else
        cls2Stages.insert(make_pair(id, vector<int>()));
    }

    if (cls2Stages.empty()) {
      string cname;
      xClsAssignment.getObjectString("ClassName", cname);
      if (cname.length() > 0) {
        pClass pc = oe.getClassCreate(0, cname);
        if (pc)
          cls2Stages.insert(make_pair(pc->getId(), vector<int>()) );
      }
    }

    if (cls2Stages.empty()) {
      gdi.addString("", 0, "Klass saknad").setColor(colorRed);
      continue;
    }

    // Allowed on leg
    xmlList xLeg;
    xClsAssignment.getObjects("AllowedOnLeg", xLeg);

    for (map<int, vector<int> >::iterator it = cls2Stages.begin(); it != cls2Stages.end(); ++it) {
      pClass defClass = oe.getClass(it->first);
      vector<int> &legs = it->second;

      // Convert from leg/legorder to real leg number
      for (size_t j = 0; j <xLeg.size(); j++) {
        int leg = xLeg[j].getInt()-1;
        if (defClass && defClass->getNumStages() > 0) {
          for (unsigned i = 0; i < defClass->getNumStages(); i++) {
            int realLeg, legIx;
            defClass->splitLegNumberParallel(i, realLeg, legIx);
            if (realLeg == leg)
              legs.push_back(i);
          }
        }
        else
          legs.push_back(leg);
      }
      if (legs.empty())
        legs.push_back(-1); // All legs
    }
    // Extract courses / families
    xmlList xCourse;
    xClsAssignment.getObjects("CourseName", xCourse);

    xmlList xFamily;
    string t, t1, t2;
    xClsAssignment.getObjects("CourseFamily", xFamily);

    for (map<int, vector<int> >::iterator it = cls2Stages.begin(); it != cls2Stages.end(); ++it) {
      const vector<int> &legs = it->second;
      for (size_t m = 0; m < legs.size(); m++) {
        int leg = legs[m];
        for (size_t j = 0; j < xFamily.size(); j++) {
          for (size_t i = 0; i < xCourse.size(); i++) {
            string crs = constructCourseName(xFamily[j].getObjectString(0, t1),
                                             xCourse[i].getObjectString(0, t2));
            classIdLegToCourse[make_pair(it->first, leg)].push_back(crs);
          }
        }
        if (xFamily.empty()) {
          for (size_t i = 0; i < xCourse.size(); i++) {
            string crs = constructCourseName("", xCourse[i].getObjectString(0, t));
            classIdLegToCourse[make_pair(it->first, leg)].push_back(crs);
          }
        }
        if (xCourse.empty()) {
          for (size_t j = 0; j < xFamily.size(); j++) {
            map<string, vector<pCourse> >::const_iterator res  =
                         coursesFamilies.find(xFamily[j].getObjectString(0, t));


            if (res != coursesFamilies.end()) {
              const vector<pCourse> &family = res->second;
              for (size_t i = 0; i < family.size(); i++) {
                classIdLegToCourse[make_pair(it->first, leg)].push_back(family[i]->getName());
              }
            }
          }
        }
      }
    }
  }

  map< pair<int, int>, vector<string> >::iterator it;
  for (it = classIdLegToCourse.begin(); it != classIdLegToCourse.end(); ++it) {
    pClass pc = oe.getClass(it->first.first);
    if (pc) {
      pc->setCourse(0);
      for (size_t k = 0; k < pc->getNumStages(); k++)
        pc->clearStageCourses(k);
    }
  }
  for (it = classIdLegToCourse.begin(); it != classIdLegToCourse.end(); ++it) {
    pClass pc = oe.getClass(it->first.first);
    unsigned leg = it->first.second;
    const vector<string> &crs = it->second;
    vector<pCourse> pCrs;
    for (size_t k = 0; k < crs.size(); k++) {
      map<string, pCourse>::const_iterator res = courses.find(crs[k]);
      pCourse c = res != courses.end() ? res->second : 0;
      if (c == 0)
        gdi.addString("", 0, "Varning: Banan 'X' finns inte#" + crs[k]).setColor(colorRed);
      pCrs.push_back(c);
    }
    if (pCrs.empty())
      continue;

    if (leg == -1) {
      if (pCrs.size() > 1) {
        if (!pc->hasMultiCourse()) {
          pc->setNumStages(1);
        }
      }

      if (pc->hasMultiCourse()) {
        for (size_t k = 0; k < pc->getNumStages(); k++) {
          for (size_t j = 0; j < pCrs.size(); j++)
            pc->addStageCourse(k, pCrs[j]);
        }
      }
      else
        pc->setCourse(pCrs[0]);
    }
    else if (leg == 0 && pCrs.size() == 1) {
      if (pc->hasMultiCourse())
        pc->addStageCourse(0, pCrs[0]);
      else
        pc->setCourse(pCrs[0]);
    }
    else {
      if (leg >= pc->getNumStages())
        pc->setNumStages(leg+1);

      for (size_t j = 0; j < pCrs.size(); j++)
        pc->addStageCourse(leg, pCrs[j]);
    }
  }
}

void IOF30Interface::personCourseAssignment(gdioutput &gdi, xmlList &xAssignment,
                                            const map<string, pCourse> &courses) {
  vector<pRunner> allR;
  oe.getRunners(0, 0, allR, false);
  map<string, pRunner> bib2Runner;
  multimap<string, pRunner> name2Runner;
  for (size_t k = 0; k < allR.size(); k++) {
    string bib = allR[k]->getBib();
    if (!bib.empty())
      bib2Runner[bib] = allR[k];

    name2Runner.insert(make_pair(allR[k]->getName(), allR[k]));
  }

  for (size_t k = 0; k < xAssignment.size(); k++) {
    xmlobject &xPAssignment = xAssignment[k];
    pRunner r = 0;

    string runnerText;
    string bib;
    xPAssignment.getObjectString("BibNumber", bib);

    if (!bib.empty()) {
      runnerText = bib;
      r = bib2Runner[bib];
    }

    if (r == 0) {
      int id = xPAssignment.getObjectInt("EntryId"); // This assumes entryId = personId, which may or may not be the case.
      if (id != 0) {
        runnerText = "Id = "+itos(id);
        r = oe.getRunner(id, 0);
      }
    }

    if (r == 0) {
      string person;
      xPAssignment.getObjectString("PersonName", person);
      if (!person.empty()) {
        runnerText = person;
        string cls;
        xPAssignment.getObjectString("ClassName", cls);
        multimap<string, pRunner>::const_iterator res = name2Runner.find(person);
        while (res != name2Runner.end() && person == res->first) {
          if (cls.empty() || res->second->getClass() == cls) {
            r = res->second;
            break;
          }
          ++res;
        }
      }
    }

    if (r == 0) {
      gdi.addString("", 0, "Varning: Deltagaren 'X' finns inte.#" + runnerText).setColor(colorRed);
      continue;
    }

    pCourse c = findCourse(gdi, courses, xPAssignment);
    if (c == 0)
      continue;

    r->setCourseId(c->getId());
  }
}

pCourse IOF30Interface::findCourse(gdioutput &gdi,
                                   const map<string, pCourse> &courses,
                                   xmlobject &xPAssignment) {
  string course;
  xPAssignment.getObjectString("CourseName", course);
  string family;
  xPAssignment.getObjectString("CourseFamily", family);
  string fullCrs = constructCourseName(family, course);

  map<string, pCourse>::const_iterator res = courses.find(fullCrs);
  pCourse c = res != courses.end() ? res->second : 0;
  if (c == 0) {
    gdi.addString("", 0, "Varning: Banan 'X' finns inte.#" + fullCrs).setColor(colorRed);
  }
  return c;
}

void IOF30Interface::teamCourseAssignment(gdioutput &gdi, xmlList &xAssignment,
                                            const map<string, pCourse> &courses) {
  vector<pTeam> allT;
  oe.getTeams(0, allT, false);

  map<string, pTeam> bib2Team;
  map<pair<string, string>, pTeam> nameClass2Team;
  for (size_t k = 0; k < allT.size(); k++) {
    string bib = allT[k]->getBib();
    if (!bib.empty())
      bib2Team[bib] = allT[k];

    nameClass2Team[make_pair(allT[k]->getName(), allT[k]->getClass())] = allT[k];
  }

  for (size_t k = 0; k < xAssignment.size(); k++) {
    xmlobject &xTAssignment = xAssignment[k];
    pTeam t = 0;
    string teamText;
    string bib;
    xTAssignment.getObjectString("BibNumber", bib);

    if (!bib.empty()) {
      teamText = bib;
      t = bib2Team[bib];
    }

    if (t == 0) {
      string team;
      xTAssignment.getObjectString("TeamName", team);
      if (!team.empty()) {
        string cls;
        xTAssignment.getObjectString("ClassName", cls);
        t = nameClass2Team[make_pair(team, cls)];
        teamText = team + " / " + cls;
      }
    }

    if (t == 0) {
      gdi.addString("", 0, "Varning: Laget 'X' finns inte.#" + teamText).setColor(colorRed);
      continue;
    }

    xmlList teamMemberAssignment;
    xTAssignment.getObjects("TeamMemberCourseAssignment", teamMemberAssignment);
    assignTeamCourse(gdi, *t, teamMemberAssignment, courses);
  }
}

void IOF30Interface::assignTeamCourse(gdioutput &gdi, oTeam &team, xmlList &xAssignment,
                                      const map<string, pCourse> &courses) {

  if (!team.getClassRef())
    return;
  for (size_t k = 0; k <xAssignment.size(); k++) {

    // Extract courses / families
    pCourse c = findCourse(gdi, courses, xAssignment[k]);
    if (c == 0)
      continue;

    xmlobject xLeg = xAssignment[k].getObject("Leg");
    if (xLeg) {
      int leg = xLeg.getInt() - 1;
      int legorder = 0;
      xmlobject xLegOrder = xAssignment[k].getObject("LegOrder");
      if (xLegOrder)
        legorder = xLegOrder.getInt() - 1;

      int legId = team.getClassRef()->getLegNumberLinear(leg, legorder);
      if (legId>=0) {
        pRunner r = team.getRunner(legId);
        if (r == 0) {
          r = oe.addRunner(lang.tl("N.N."), team.getClubId(), team.getClassId(), 0, 0, false);
          team.setRunner(legId, r, false);
          r = team.getRunner(legId);
        }
        if (r) {
          r->setCourseId(c->getId());
        }
      }
      else
        gdi.addString("", 0, "Bantilldelning f�r 'X' h�nvisar till en str�cka som inte finns#" + team.getClass()).setColor(colorRed);
    }
    else {
      string name;
      xAssignment[k].getObjectString("TeamMemberName", name);
      if (!name.empty()) {
        for (int j = 0; j < team.getNumRunners(); j++) {
          pRunner r = team.getRunner(j);
          if (r && r->getName() == name) {
            r->setCourseId(c->getId());
            break;
          }
        }
      }
    }
  }
}


void IOF30Interface::classAssignmentObsolete(gdioutput &gdi, xmlList &xAssignment,
                                             const map<string, pCourse> &courses,
                                             const map<string, vector<pCourse> > &coursesFamilies) {
  map<int, vector<pCourse> > class2Courses;
  map<int, set<string> > class2Families;

  multimap<string, pRunner> bib2Runners;
  typedef multimap<string, pRunner>::iterator bibIterT;
  bool b2RInit = false;

  map<pair<string, string>, pTeam> clsName2Team;
  typedef map<pair<string, string>, pTeam>::iterator teamIterT;
  bool c2TeamInit = false;

  for (size_t k = 0; k < xAssignment.size(); k++) {
    string name = constructCourseName(xAssignment[k]);
    string family;
    xAssignment[k].getObjectString("CourseFamily", family);

    if ( courses.find(name) == courses.end() )
      gdi.addString("", 0, "Varning: Banan 'X' finns inte#" + name);
    else {
      pCourse pc = courses.find(name)->second;
      xmlList xCls, xPrs;
      xAssignment[k].getObjects("Class", xCls);
      xAssignment[k].getObjects("Person", xPrs);

      for (size_t j = 0; j < xCls.size(); j++) {
        string cName;
        xCls[j].getObjectString("Name", cName);
        int id = xCls[j].getObjectInt("Id");
        pClass cls = oe.getClassCreate(id, cName);
        if (cls) {
          class2Courses[cls->getId()].push_back(pc);

          if (!family.empty()) {
            class2Families[cls->getId()].insert(family);
          }
        }
      }

      for (size_t j = 0; j < xPrs.size(); j++) {
        string bib;
        int leg = xPrs[j].getObjectInt("Leg");
        int legOrder = xPrs[j].getObjectInt("LegOrder");

        xPrs[j].getObjectString("BibNumber", bib);
        if (!bib.empty()) {
          if (!b2RInit) {
            // Setup bib2runner map
            vector<pRunner> r;
            oe.getRunners(0, 0, r);
            for (size_t i = 0; i < r.size(); i++) {
              string b = r[i]->getBib();
              if (!b.empty())
                bib2Runners.insert(make_pair(b, r[i]));
            }
            b2RInit = true;
          }

          pair<bibIterT, bibIterT> range = bib2Runners.equal_range(bib);
          for (bibIterT it = range.first; it != range.second; ++it) {
            int ln = it->second->getLegNumber();
            int rLegNumber = 0, rLegOrder = 0;
            if (it->second->getClassRef())
              it->second->getClassRef()->splitLegNumberParallel(ln, rLegNumber, rLegOrder);
            bool match = true;
            if (leg != 0 && leg != rLegNumber+1)
              match = false;
            if (legOrder != 0 && legOrder != rLegOrder+1)
              match = false;

            if (match) {
              it->second->setCourseId(pc->getId());
              it->second->synchronize();
            }
          }
          continue;
        }

        string className, teamName;
        xPrs[j].getObjectString("ClassName", className);
        xPrs[j].getObjectString("TeamName", teamName);

        if (!teamName.empty()) {
          if (!c2TeamInit) {
            vector<pTeam> t;
            oe.getTeams(0, t);
            for (size_t i = 0; i < t.size(); i++)
              clsName2Team[make_pair(t[i]->getClass(), t[i]->getName())] = t[i];
            c2TeamInit = true;
          }

          teamIterT res = clsName2Team.find(make_pair(className, teamName));

          if (res != clsName2Team.end()) {
            pClass cls = res->second->getClassRef();
            if (cls) {
              int ln = cls->getLegNumberLinear(leg, legOrder);
              pRunner r = res->second->getRunner(ln);
              if (r) {
                r->setCourseId(pc->getId());
                r->synchronize();
              }
            }
          }
          continue;
        }

        // Note: entryId is assumed to be equal to personId,
        // which is the only we have. This might not be true.
        int entryId = xPrs[j].getObjectInt("EntryId");
        pRunner r = oe.getRunner(entryId, 0);
        if (r) {
          r->setCourseId(pc->getId());
          r->synchronize();
        }
      }
    }
  }

  if (!class2Families.empty()) {
    vector<pClass> c;
    oe.getClasses(c);
    for (size_t k = 0; k < c.size(); k++) {
      bool assigned = false;

      if (class2Families.count(c[k]->getId())) {
        const set<string> &families = class2Families[c[k]->getId()];

        if (families.size() == 1) {
          int nl = c[k]->getNumStages();
          const vector<pCourse> &crsFam = coursesFamilies.find(*families.begin())->second;
          if (nl == 0) {
            if (crsFam.size() == 1)
              c[k]->setCourse(crsFam[0]);
            else {
              c[k]->setNumStages(1);
              c[k]->clearStageCourses(0);
              for (size_t j = 0; j < crsFam.size(); j++)
                c[k]->addStageCourse(0, crsFam[j]->getId());
            }
          }
          else {
            int nFam = crsFam.size();
            for (int i = 0; i < nl; i++) {
              c[k]->clearStageCourses(i);
              for (int j = 0; j < nFam; j++)
                c[k]->addStageCourse(i, crsFam[(j + i)%nFam]->getId());
            }
          }
          assigned = true;
        }
        else if (families.size() > 1) {
          int nl = c[k]->getNumStages();
          if (nl == 0) {
            c[k]->setNumStages(families.size());
            nl = families.size();
          }

          set<string>::const_iterator fit = families.begin();
          for (int i = 0; i < nl; i++, ++fit) {
            if (fit == families.end())
              fit = families.begin();
            c[k]->clearStageCourses(i);
            const vector<pCourse> &crsFam = coursesFamilies.find(*fit)->second;
            int nFam = crsFam.size();
            for (int j = 0; j < nFam; j++)
              c[k]->addStageCourse(i, crsFam[j]->getId());
          }

          assigned = true;
        }
      }

      if (!assigned && class2Courses.count(c[k]->getId())) {
        const vector<pCourse> &crs = class2Courses[c[k]->getId()];
        int nl = c[k]->getNumStages();

        if (crs.size() == 1 && nl == 0) {
          c[k]->setCourse(crs[0]);
        }
        else if (crs.size() > 1) {
          int nCrs = crs.size();
          for (int i = 0; i < nl; i++) {
            c[k]->clearStageCourses(i);
            for (int j = 0; j < nCrs; j++)
              c[k]->addStageCourse(i, crs[(j + i)%nCrs]->getId());
          }
        }
      }
      c[k]->synchronize();
    }
  }
}

void IOF30Interface::readCompetitorList(gdioutput &gdi, const xmlobject &xo, int &personCount) {
  if (!xo)
    return;

  string ver;
  xo.getObjectString("iofVersion", ver);
  if (!ver.empty() && ver > "3.0")
    gdi.addString("", 0, "Varning, ok�nd XML-version X#" + ver);

  xmlList xl;
  xo.getObjects(xl);

  xmlList::const_iterator it;

  for (it=xl.begin(); it != xl.end(); ++it) {
    if (it->is("Competitor")) {
      if (readXMLCompetitorDB(*it))
        personCount++;
    }
  }
}

void IOF30Interface::readClubList(gdioutput &gdi, const xmlobject &xo, int &clubCount) {
  if (!xo)
    return;

  string ver;
  xo.getObjectString("iofVersion", ver);
  if (!ver.empty() && ver > "3.0")
    gdi.addString("", 0, "Varning, ok�nd XML-version X#" + ver);

  xmlList xl;
  xo.getObjects(xl);

  xmlList::const_iterator it;
  for (it=xl.begin(); it != xl.end(); ++it) {
    if (it->is("Organisation")) {
      if (readOrganization(gdi, *it, true))
        clubCount++;
    }
  }
}


void IOF30Interface::readEntryList(gdioutput &gdi, xmlobject &xo, int &entRead, int &entFail) {
  string ver;
  xo.getObjectString("iofVersion", ver);
  if (!ver.empty() && ver > "3.0")
    gdi.addString("", 0, "Varning, ok�nd XML-version X#" + ver);

  xmlobject xEvent = xo.getObject("Event");
  map<int, vector<LegInfo> > teamClassConfig;

  if (xEvent) {
    readEvent(gdi, xEvent, teamClassConfig);
  }

  xmlList pEntries;
  xo.getObjects("PersonEntry", pEntries);

  for (size_t k = 0; k < pEntries.size(); k++) {
    if (readPersonEntry(gdi, pEntries[k], 0, teamClassConfig))
      entRead++;
    else
      entFail++;
  }

  xo.getObjects("TeamEntry", pEntries);
  for (size_t k = 0; k < pEntries.size(); k++) {
    setupClassConfig(0, pEntries[k], teamClassConfig);
  }

  setupRelayClasses(teamClassConfig);

  for (size_t k = 0; k < pEntries.size(); k++) {
    if (readTeamEntry(gdi, pEntries[k], teamClassConfig))
      entRead++;
    else
      entFail++;
  }
}


void IOF30Interface::readStartList(gdioutput &gdi, xmlobject &xo, int &entRead, int &entFail) {
  string ver;
  xo.getObjectString("iofVersion", ver);
  if (!ver.empty() && ver > "3.0")
    gdi.addString("", 0, "Varning, ok�nd XML-version X#" + ver);

  map<int, vector<LegInfo> > teamClassConfig;

  xmlobject xEvent = xo.getObject("Event");
  if (xEvent) {
    readEvent(gdi, xEvent, teamClassConfig);
  }

  xmlList cStarts;
  xo.getObjects("ClassStart", cStarts);

  struct RaceInfo {
    int courseId;
    int length;
    int climb;
    string startName;
  };

  for (size_t k = 0; k < cStarts.size(); k++) {
    xmlobject &xClassStart = cStarts[k];

    pClass pc = readClass(xClassStart.getObject("Class"),
                          teamClassConfig);
    int classId = pc ? pc->getId() : 0;


    map<int, RaceInfo> raceToInfo;

    xmlList courses;
    xClassStart.getObjects("Course", courses);
    for (size_t k = 0; k < courses.size(); k++) {
      int raceNo = courses[k].getObjectInt("raceNumber");
      if (raceNo > 0)
        raceNo--;
      RaceInfo &raceInfo = raceToInfo[raceNo];

      raceInfo.courseId = courses[k].getObjectInt("Id");
      raceInfo.length = courses[k].getObjectInt("Length");
      raceInfo.climb = courses[k].getObjectInt("Climb");
    }

    xmlList startNames;
    xClassStart.getObjects("StartName", startNames);
    for (size_t k = 0; k < startNames.size(); k++) {
      int raceNo = startNames[k].getObjectInt("raceNumber");
      if (raceNo > 0)
        raceNo--;
      RaceInfo &raceInfo = raceToInfo[raceNo];
      startNames[k].getObjectString(0, raceInfo.startName);
      pc->setStart(raceInfo.startName);
    }

    if (raceToInfo.size() == 1) {
      RaceInfo &raceInfo = raceToInfo.begin()->second;
      if (raceInfo.courseId > 0) {
        if (pc->getCourse() == 0) {
          pCourse crs = oe.addCourse(pc->getName(), raceInfo.length, raceInfo.courseId);
          crs->setStart(raceInfo.startName, false);
          crs->getDI().setInt("Climb", raceInfo.climb);
          pc->setCourse(crs);
          crs->synchronize();
        }
      }
    }
    else if (raceToInfo.size() > 1) {
    }

    xmlList xPStarts;
    xClassStart.getObjects("PersonStart", xPStarts);

    for (size_t k = 0; k < xPStarts.size(); k++) {
      if (readPersonStart(gdi, pc, xPStarts[k], 0, teamClassConfig))
        entRead++;
      else
        entFail++;
    }

    xmlList tEntries;
    xClassStart.getObjects("TeamStart", tEntries);
    for (size_t k = 0; k < tEntries.size(); k++) {
      setupClassConfig(classId, tEntries[k], teamClassConfig);
    }

    //setupRelayClasses(teamClassConfig);
    if (pc && teamClassConfig.count(pc->getId()) && !teamClassConfig[pc->getId()].empty()) {
      setupRelayClass(pc, teamClassConfig[pc->getId()]);
    }

    for (size_t k = 0; k < tEntries.size(); k++) {
      if (readTeamStart(gdi, pc, tEntries[k], teamClassConfig))
        entRead++;
      else
        entFail++;
    }

    pc->synchronize();
  }
}

void IOF30Interface::readClassList(gdioutput &gdi, xmlobject &xo, int &entRead, int &entFail) {
  string ver;
  xo.getObjectString("iofVersion", ver);
  if (!ver.empty() && ver > "3.0")
    gdi.addString("", 0, "Varning, ok�nd XML-version X#" + ver);

  map<int, vector<LegInfo> > teamClassConfig;

  xmlobject xEvent = xo.getObject("Event");
  if (xEvent) {
    readEvent(gdi, xEvent, teamClassConfig);
  }

  xmlList cClass;
  xo.getObjects("Class", cClass);


  for (size_t k = 0; k < cClass.size(); k++) {
    xmlobject &xClass = cClass[k];

    pClass pc = readClass(xClass, teamClassConfig);

    if (pc)
      entRead++;
    else
      entFail++;

    if (pc && teamClassConfig.count(pc->getId()) && !teamClassConfig[pc->getId()].empty()) {
      setupRelayClass(pc, teamClassConfig[pc->getId()]);
    }

    pc->synchronize();
  }
}

void IOF30Interface::readEventList(gdioutput &gdi, xmlobject &xo) {
  if (!xo)
    return;

  string ver;
  xo.getObjectString("iofVersion", ver);
  if (!ver.empty() && ver > "3.0")
    gdi.addString("", 0, "Varning, ok�nd XML-version X#" + ver);

  xmlList xl;
  xo.getObjects(xl);

  xmlList::const_iterator it;
  map<int, vector<LegInfo> > teamClassConfig;
  for (it=xl.begin(); it != xl.end(); ++it) {
    if (it->is("Event")) {
      readEvent(gdi, *it, teamClassConfig);
      return;
    }
  }
}

void IOF30Interface::readEvent(gdioutput &gdi, const xmlobject &xo,
                               map<int, vector<LegInfo> > &teamClassConfig) {

  string name;
  xo.getObjectString("Name", name);
  oe.setName(name);

  int id = xo.getObjectInt("Id");
  if (id>0)
    oe.setExtIdentifier(id);

  xmlobject date = xo.getObject("StartTime");

  if (date) {
    string dateStr;
    date.getObjectString("Date", dateStr);
    oe.setDate(dateStr);
    string timeStr;
    date.getObjectString("Time", timeStr);
    if (!timeStr.empty()) {
      int t = convertAbsoluteTimeISO(timeStr);
      if (t >= 0 && oe.getNumRunners() == 0) {
        int zt = t - 3600;
        if (zt < 0)
          zt += 3600*24;
        oe.setZeroTime(formatTimeHMS(zt));
      }
    }
    //oe.setZeroTime(...);
  }

  xmlobject xOrg = xo.getObject("Organiser");
  oDataInterface DI = oe.getDI();

  if (xOrg) {
    string name;
    xOrg.getObjectString("Name", name);
    if (name.length() > 0)
      DI.setString("Organizer", name);

    xmlobject address = xOrg.getObject("Address");

    string tmp;

    if (address) {
      DI.setString("CareOf", address.getObjectString("CareOf", tmp));
      DI.setString("Street", address.getObjectString("Street", tmp));
      string city, zip, state;
      address.getObjectString("City", city);
      address.getObjectString("ZipCode", zip);
      address.getObjectString("State", state);
      if (state.empty())
        DI.setString("Address", zip + " " + city);
      else
        DI.setString("Address", state + ", " + zip + " " + city);
    }

    xmlList xContact;
    xOrg.getObjects("Contact", xContact);

    string phone;
    for (size_t k = 0; k < xContact.size(); k++) {
      string type;
      xContact[k].getObjectString("type", type);
      string c;
      xContact[k].getObjectString(0, c);

      if (type == "PhoneNumber" || "MobilePhoneNumber")
        phone += phone.empty() ? c : ", " + c;
      else if (type == "EmailAddress")
        DI.setString("EMail", c);
      else if (type == "WebAddress")
        DI.setString("Homepage", c);
    }
    if (!phone.empty())
      DI.setString("Phone", phone);
  }

  string account;
  xo.getObjectString("Account", account);
  if (!account.empty())
    DI.setString("Account", account);

  xmlList xClass;
  xo.getObjects("Class", xClass);
  for (size_t k = 0; k < xClass.size(); k++)
    readClass(xClass[k], teamClassConfig);

  if (!feeStatistics.empty()) {
    set<int> fees;
    set<int> factors;
    for (size_t i = 0; i < feeStatistics.size(); i++) {
      int fee = int(100 * feeStatistics[i].fee);
      int factor = int(100 * feeStatistics[i].lateFactor) - 100;
      fees.insert(fee);
      if (factor > 0)
        factors.insert(factor);
    }
    int n = 0, y = 0, e = 0;

    if (fees.size() >= 3) {
      y = *fees.begin();
      fees.erase(fees.begin());
      n = *fees.begin();
      fees.erase(fees.begin());
      e = *fees.rbegin();
    }
    else if (fees.size() == 2) {
      y = *fees.begin();
      fees.erase(fees.begin());
      e = n = *fees.begin();
    }
    else if (fees.size() == 1) {
      e = n = y = *fees.begin();
    }

    if (n > 0) {
      DI.setInt("EliteFee", oe.interpretCurrency(double(e) * 0.01, ""));
      DI.setInt("EntryFee", oe.interpretCurrency(double(n) * 0.01, ""));
      DI.setInt("YouthFee", oe.interpretCurrency(double(y) * 0.01, ""));
    }

    if (!factors.empty()) {
      char lf[16];
      sprintf_s(lf, "%d %%", *factors.rbegin());
      DI.setString("LateEntryFactor", lf);
    }
  }
  oe.synchronize();
}

void IOF30Interface::setupClassConfig(int classId, const xmlobject &xTeam, map<int, vector<LegInfo> > &teamClassConfig) {

  // Get class
  xmlobject xClass = xTeam.getObject("Class");
  if (xClass) {
    pClass pc = readClass(xClass, teamClassConfig);
    classId = pc->getId();
  }
  vector<LegInfo> &teamClass = teamClassConfig[classId];

  // Get team entriess
  xmlList xEntries;
  xTeam.getObjects("TeamEntryPerson", xEntries);
  for (size_t k = 0; k < xEntries.size(); k++) {
    int leg = xEntries[k].getObjectInt("Leg");
    int legorder = xEntries[k].getObjectInt("LegOrder");
    leg = max(0, leg - 1);
    legorder = max(1, legorder);
    if (int(teamClass.size()) <= leg)
      teamClass.resize(leg + 1);
    teamClass[leg].setMaxRunners(legorder);
  }

  // Get team starts
  xmlList xMemberStarts;
  xTeam.getObjects("TeamMemberStart", xMemberStarts);
  for (size_t k = 0; k < xMemberStarts.size(); k++) {
    xmlList xStarts;
    xMemberStarts[k].getObjects("Start", xStarts);
    for (size_t j = 0; j < xStarts.size(); j++) {
      int leg = xStarts[j].getObjectInt("Leg");
      int legorder = xStarts[j].getObjectInt("LegOrder");
      leg = max(0, leg - 1);
      legorder = max(1, legorder);
      if (int(teamClass.size()) <= leg)
        teamClass.resize(leg + 1);
      teamClass[leg].setMaxRunners(legorder);
    }
  }
}

pTeam IOF30Interface::readTeamEntry(gdioutput &gdi, xmlobject &xTeam,
                                    const map<int, vector<LegInfo> > &teamClassConfig) {

  pTeam t = getCreateTeam(gdi, xTeam);

  if (!t)
    return 0;

  // Class
  map<int, vector<LegInfo> > localTeamClassConfig;
  pClass pc = readClass(xTeam.getObject("Class"), localTeamClassConfig);

  if (pc)
    t->setClassId(pc->getId());

  string bib;
  xTeam.getObjectString("BibNumber", bib);
  t->setBib(bib, true, false);

  oDataInterface di = t->getDI();
  string entryTime;
  xTeam.getObjectString("EntryTime", entryTime);
  di.setDate("EntryDate", entryTime);

  double fee = 0, paid = 0, taxable = 0, percentage = 0;
  string currency;
  xmlList xAssigned;
  xTeam.getObjects("AssignedFee", xAssigned);
  for (size_t j = 0; j < xAssigned.size(); j++) {
    getAssignedFee(xAssigned[j], fee, paid, taxable, percentage, currency);
  }
  fee += fee * percentage; // OLA / Eventor stupidity

  di.setInt("Fee", oe.interpretCurrency(fee, currency));
  di.setInt("Paid", oe.interpretCurrency(paid, currency));
  di.setInt("Taxable", oe.interpretCurrency(fee, currency));

  xmlList xEntries;
  xTeam.getObjects("TeamEntryPerson", xEntries);

  for (size_t k = 0; k<xEntries.size(); k++) {
    readPersonEntry(gdi, xEntries[k], t, teamClassConfig);
  }

  t->synchronize();
  return t;
}

pTeam IOF30Interface::readTeamStart(gdioutput &gdi, pClass pc, xmlobject &xTeam,
                                    const map<int, vector<LegInfo> > &teamClassConfig) {
  pTeam t = getCreateTeam(gdi, xTeam);

  if (!t)
    return 0;

  // Class
  if (pc)
    t->setClassId(pc->getId());

  string bib;
  xTeam.getObjectString("BibNumber", bib);
  t->setBib(bib, atoi(bib.c_str()) > 0, false);

  xmlList xEntries;
  xTeam.getObjects("TeamMemberStart", xEntries);

  for (size_t k = 0; k<xEntries.size(); k++) {
    readPersonStart(gdi, pc, xEntries[k], t, teamClassConfig);
  }

  t->synchronize();
  return t;
}

pTeam IOF30Interface::getCreateTeam(gdioutput &gdi, const xmlobject &xTeam) {
  string name;
  xTeam.getObjectString("Name", name);

  if (name.empty())
    return 0;

  int id = xTeam.getObjectInt("Id");
  pTeam t = 0;

  if (id)
    t = oe.getTeam(id);
  else
    t = oe.getTeamByName(name);

  if (!t) {
    if (id > 0) {
      oTeam tr(&oe, id);
      t = oe.addTeam(tr, true);
    }
    else {
      oTeam tr(&oe);
      t = oe.addTeam(tr, true);
    }
  }

  if (!t)
    return 0;

  t->setName(name);

  // Club
  pClub c = 0;
  xmlList xOrgs;
  xTeam.getObjects("Organisation", xOrgs);
  if (xOrgs.empty())
    xTeam.getObjects("Organization", xOrgs);

  for (size_t k = 0; k < xOrgs.size(); k++) {
    if (c == 0)
      c = readOrganization(gdi, xOrgs[k], false);
    else
      readOrganization(gdi, xOrgs[k], false);// Just include in competition
  }

  if (c)
    t->setClubId(c->getId());

  return t;
}

int IOF30Interface::getIndexFromLegPos(int leg, int legorder, const vector<LegInfo> &setup) {
  int ix = 0;
  for (int k = 0; k < leg - 1; k++)
    ix += k < int(setup.size()) ? max(setup[k].maxRunners, 1) : 1;
  if (legorder > 0)
    ix += legorder - 1;
  return ix;
}

pRunner IOF30Interface::readPersonEntry(gdioutput &gdi, xmlobject &xo, pTeam team,
                                        const map<int, vector<LegInfo> > &teamClassConfig) {
  xmlobject xPers = xo.getObject("Person");
  // Card
  const int cardNo = xo.getObjectInt("ControlCard");

  pRunner r = 0;

  if (xPers)
    r = readPerson(gdi, xPers);

  if (cardNo > 0 && r == 0 && team) {
    // We got no person, but a card number. Add the runner anonymously.
    r = oe.addRunner("N.N.", team->getClubId(), team->getClassId(), cardNo, 0, false);
    r->synchronize();
  }

  if (r == 0)
    return 0;

  // Club
  pClub c = readOrganization(gdi, xo.getObject("Organisation"), false);
  if (!c)
    c = readOrganization(gdi, xo.getObject("Organization"), false);

  if (c)
    r->setClubId(c->getId());

  // Class
  map<int, vector<LegInfo> > localTeamClassConfig;
  pClass pc = readClass(xo.getObject("Class"), localTeamClassConfig);

  if (pc)
    r->setClassId(pc->getId());

  if (team) {
    int leg = xo.getObjectInt("Leg");
    int legorder = xo.getObjectInt("LegOrder");
    int legindex = max(0, leg - 1);
    map<int, vector<LegInfo> >::const_iterator res = teamClassConfig.find(team->getClassId());
    if (res != teamClassConfig.end()) {
      legindex = getIndexFromLegPos(leg, legorder, res->second);
    }
    team->setRunner(legindex, r, false);
    if (r->getClubId() == 0)
      r->setClubId(team->getClubId());
  }

  // Card
  if (cardNo > 0)
    r->setCardNo(cardNo, false);

  oDataInterface di = r->getDI();

  string entryTime;
  xo.getObjectString("EntryTime", entryTime);
  di.setDate("EntryDate", entryTime);

  double fee = 0, paid = 0, taxable = 0, percentage = 0;
  string currency;
  xmlList xAssigned;
  xo.getObjects("AssignedFee", xAssigned);
  for (size_t j = 0; j < xAssigned.size(); j++) {
    getAssignedFee(xAssigned[j], fee, paid, taxable, percentage, currency);
  }
  fee += fee * percentage; // OLA / Eventor stupidity

  di.setInt("Fee", oe.interpretCurrency(fee, currency));
  di.setInt("Paid", oe.interpretCurrency(paid, currency));
  di.setInt("Taxable", oe.interpretCurrency(fee, currency));

  r->synchronize();
  return r;
}

pRunner IOF30Interface::readPersonStart(gdioutput &gdi, pClass pc, xmlobject &xo, pTeam team,
                                        const map<int, vector<LegInfo> > &teamClassConfig) {
  xmlobject xPers = xo.getObject("Person");
  pRunner r = 0;
  if (xPers)
    r = readPerson(gdi, xPers);
  if (r == 0)
    return 0;

  // Club
  pClub c = readOrganization(gdi, xo.getObject("Organisation"), false);
  if (!c)
    c = readOrganization(gdi, xo.getObject("Organization"), false);

  if (c)
    r->setClubId(c->getId());

  xmlList starts;
  xo.getObjects("Start", starts);

  for (size_t k = 0; k < starts.size(); k++) {
    int race = starts[k].getObjectInt("raceNumber");
    pRunner rRace = r;
    if (race > 1 && r->getNumMulti() > 0) {
      pRunner rr = r->getMultiRunner(race - 1);
      if (rr)
        rRace = rr;
    }
    if (rRace) {
      // Card
      int cardNo = starts[k].getObjectInt("ControlCard");
      if (cardNo > 0)
        rRace->setCardNo(cardNo, false);

      xmlobject startTime = starts[k].getObject("StartTime");

      if (team) {
        int leg = starts[k].getObjectInt("Leg");
        int legorder = starts[k].getObjectInt("LegOrder");
        int legindex = max(0, leg - 1);
        map<int, vector<LegInfo> >::const_iterator res = teamClassConfig.find(team->getClassId());
        if (res != teamClassConfig.end()) {
          legindex = getIndexFromLegPos(leg, legorder, res->second);
        }
        team->setRunner(legindex, rRace, false);
        if (rRace->getClubId() == 0)
          rRace->setClubId(team->getClubId());

        if (startTime && pc) {
          pc->setStartType(legindex, STDrawn);

        }
      }

      string bib;
      starts[k].getObjectString("BibNumber", bib);
      rRace->getDI().setString("Bib", bib);

      rRace->setStartTime(parseISO8601Time(startTime), true, false);
    }
  }

  if (pc)
    r->setClassId(pc->getId());

  r->synchronize();
  return r;
}



pRunner IOF30Interface::readPerson(gdioutput &gdi, const xmlobject &person) {

  xmlobject pname = person.getObject("Name");

  int pid = person.getObjectInt("Id");

  pRunner r = 0;

  if (pid)
    r = oe.getRunner(pid, 0);

  if (!r) {
    if ( pid > 0) {
      oRunner or(&oe, pid);
      r = oe.addRunner(or, true);
    }
    else {
      oRunner or(&oe);
      r = oe.addRunner(or, true);
    }
  }

  string given, family;
  if (pname)
    r->setName(getFirst(pname.getObjectString("Given", given), 2)+" "+pname.getObjectString("Family", family));
  else
    r->setName("N.N.");

  r->setExtIdentifier(pid);

  oDataInterface DI=r->getDI();
  string tmp;

  r->setSex(interpretSex(person.getObjectString("sex", tmp)));
  person.getObjectString("BirthDate", tmp);
  if (tmp.length()>=4) {
    tmp = tmp.substr(0, 4);
    r->setBirthYear(atoi(tmp.c_str()));
  }

  getNationality(person.getObject("Nationality"), DI);

  return r;
}

pClub IOF30Interface::readOrganization(gdioutput &gdi, const xmlobject &xclub, bool saveToDB) {
  if (!xclub)
    return 0;
  int clubId = xclub.getObjectInt("Id");
  string name, shortName;
  xclub.getObjectString("Name", name);
  xclub.getObjectString("ShortName", shortName);

  if (shortName.length() > 4 && shortName.length() < name.length())
    swap(name, shortName);

  if (name.length()==0 || !IsCharAlphaNumeric(name[0]))
    return 0;

  pClub pc=0;

  if ( !saveToDB ) {
    if (clubId)
      pc = oe.getClubCreate(clubId, name);

    if (!pc) return false;
  }
  else {
    pc = new oClub(&oe, clubId);
    //pc->setID->Id = clubId;
  }

  pc->setName(name);

  pc->setExtIdentifier(clubId);

  oDataInterface DI=pc->getDI();

  string tmp;

  int district = xclub.getObjectInt("ParentOrganisationId");
  if (district > 0)
    DI.setInt("District", district);

  xmlobject address = xclub.getObject("Address");

  if (shortName.length() <= 4)
    DI.setString("ShortName", shortName);

  string str;

  if (address) {
    DI.setString("CareOf", address.getObjectString("CareOf", tmp));
    DI.setString("Street", address.getObjectString("Street", tmp));
    DI.setString("City", address.getObjectString("City", tmp));
    DI.setString("ZIP", address.getObjectString("ZipCode", tmp));
    DI.setString("State", address.getObjectString("State", tmp));
    getNationality(address.getObject("Country"), DI);
  }

  xmlList xContact;
  xclub.getObjects("Contact", xContact);

  string phone;
  for (size_t k = 0; k < xContact.size(); k++) {
    string type;
    xContact[k].getObjectString("type", type);
    string c;
    xContact[k].getObjectString(0, c);

    if (type == "PhoneNumber" || type == "MobilePhoneNumber")
      phone += phone.empty() ? c : ", " + c;
    else if (type == "EmailAddress")
      DI.setString("EMail", c);
  }
  DI.setString("Phone", phone);

  getNationality(xclub.getObject("Country"), DI);

  xclub.getObjectString("type", str);
  if (!str.empty())
    DI.setString("Type", str);

  if (saveToDB) {
    oe.getRunnerDatabase().importClub(*pc, false);
    delete pc;
  }
  else {
    pc->synchronize();
  }

  return pc;
}

void IOF30Interface::getNationality(const xmlobject &xCountry, oDataInterface &di) {
  if (xCountry) {
    string code, country;

    xCountry.getObjectString("code", code);
    xCountry.getObjectString(0, country);

    if (!code.empty())
        di.setString("Nationality", code);

    if (!country.empty())
        di.setString("Country", country);
  }
}

void IOF30Interface::getAmount(const xmlobject &xAmount, double &amount, string &currency) {
  amount = 0; // Do no clear currency. It is filled in where found (and assumed to be constant)
  if (xAmount) {
    string tmp;
    xAmount.getObjectString(0, tmp);
    amount = atof(tmp.c_str());
    xAmount.getObjectString("currency", currency);
  }
}

void IOF30Interface::getFeeAmounts(const xmlobject &xFee, double &fee, double &taxable, double &percentage, string &currency) {
  xmlobject xAmount = xFee.getObject("Amount");
  xmlobject xPercentage = xFee.getObject("Percentage"); // Eventor / OLA stupidity
  if (xPercentage) {
    string tmp;
    xPercentage.getObjectString(0, tmp);
    percentage = atof(tmp.c_str()) * 0.01;
  }
  else
    getAmount(xAmount, fee, currency);
  getAmount(xFee.getObject("TaxableAmount"), taxable, currency);
}

void IOF30Interface::getAssignedFee(const xmlobject &xFee, double &fee, double &paid, double &taxable, double &percentage, string &currency) {
  currency.clear();
  if (xFee) {
    getFeeAmounts(xFee.getObject("Fee"), fee, taxable, percentage, currency);
    getAmount(xFee.getObject("PaidAmount"), paid, currency);
  }
}

void IOF30Interface::getFee(const xmlobject &xFee, FeeInfo &fee) {
  getFeeAmounts(xFee, fee.fee, fee.taxable, fee.percentage, fee.currency);

  xFee.getObjectString("ValidFromTime", fee.fromTime);
  xFee.getObjectString("ValidToTime", fee.toTime);

  xFee.getObjectString("FromDateOfBirth", fee.fromBirthDate);
  xFee.getObjectString("ToDateOfBirth", fee.toBirthDate);
}

void IOF30Interface::writeAmount(xmlparser &xml, const char *tag, int amount) const {
  if (amount > 0) {
    string code = oe.getDCI().getString("CurrencyCode");
    if (code.empty())
      xml.write(tag, oe.formatCurrency(amount, false));
    else
      xml.write(tag, "currency", code, oe.formatCurrency(amount, false));
  }
}

void IOF30Interface::writeAssignedFee(xmlparser &xml, const oDataConstInterface &dci) const {
  int fee = dci.getInt("Fee");
  int taxable = dci.getInt("Taxable");
  int paid = dci.getInt("Paid");

  if (fee == 0 && taxable == 0 && paid == 0)
    return;

  xml.startTag("AssignedFee");

  xml.startTag("Fee");
  xml.write("Name", "Entry fee");
  writeAmount(xml, "Amount", fee);
  writeAmount(xml, "TaxableAmount", taxable);
  xml.endTag();

  writeAmount(xml, "PaidAmount", paid);

  xml.endTag();
}

void IOF30Interface::writeRentalCardService(xmlparser &xml, int cardFee) const {
  xml.startTag("ServiceRequest"); {

    xml.startTag("Service"); {
      xml.write("Name", "Card Rental");
    }
    xml.endTag();

    xml.write("RequestedQuantity", "1");

    xml.startTag("AssignedFee"); {
      xml.startTag("Fee"); {
        xml.write("Name", "Card Rental Fee");
        writeAmount(xml, "Amount", cardFee);
      }
      xml.endTag();
    }
    xml.endTag();
  }
  xml.endTag();
}

void IOF30Interface::getAgeLevels(const vector<FeeInfo> &fees, const vector<int> &ix,
                                  int &normalIx, int &redIx, string &youthLimit, string &seniorLimit) {
  assert(!ix.empty());
  if (ix.size() == 1) {
    normalIx = ix[0];
    redIx = ix[0];
    return;
  }
  else {
    normalIx = redIx = ix[0];
    for (size_t k = 0; k < ix.size(); k++) {
      if (fees[ix[k]] < fees[redIx])
        redIx = ix[k];
      if (fees[normalIx] < fees[ix[k]])
        normalIx = ix[k];

      const string &to = fees[ix[k]].toBirthDate;
      const string &from = fees[ix[k]].fromBirthDate;

      if (!from.empty() && (youthLimit.empty() || youthLimit > from))
        youthLimit = from;

      if (!to.empty() && (seniorLimit.empty() || seniorLimit > to))
        seniorLimit = to;
    }
  }
}

int getAgeFromDate(const string &date) {
  int y = getThisYear();
  SYSTEMTIME st;
  convertDateYMS(date, st);
  return y - st.wYear;
}

void IOF30Interface::FeeInfo::add(IOF30Interface::FeeInfo &fi) {
  fee += fi.fee;
  fee += fee*percentage;

  taxable += fi.taxable;

  if (fi.toTime.empty() || (fi.toTime > fromTime && !fromTime.empty())) {
    fi.toTime = fromTime;
    if (!fi.toTime.empty()) {
      SYSTEMTIME st;
      convertDateYMS(fi.toTime, st);
      __int64 sec = SystemTimeToInt64Second(st);
      sec -= 3600;
      fi.toTime = convertSystemDate(Int64SecondToSystemTime(sec));
    }
  }
  //if (fi.fromTime.empty() || (fi.fromTime < toTime && !toTime.empty()))
  //  fi.fromTime = toTime;
}

pClass IOF30Interface::readClass(const xmlobject &xclass,
                                 map<int, vector<LegInfo> > &teamClassConfig) {
  if (!xclass)
    return 0;
  int classId = xclass.getObjectInt("Id");
  string name, shortName, longName;
  xclass.getObjectString("Name", name);
  xclass.getObjectString("ShortName", shortName);

  if (!shortName.empty()) {
    longName = name;
    name = shortName;
  }

  pClass pc = 0;

  if (classId) {
    pc = oe.getClass(classId);

    if (!pc) {
      oClass c(&oe, classId);
      pc = oe.addClass(c);
    }
  }
  else
    pc = oe.addClass(name);

  oDataInterface DI = pc->getDI();

  if (!longName.empty()) {
    pc->setName(name);
    DI.setString("LongName", longName);
  }
  else {
    if (pc->getName() != name && DI.getString("LongName") != name)
      pc->setName(name);
  }
  xmlList legs;
  xclass.getObjects("Leg", legs);
  if (!legs.empty()) {
    vector<LegInfo> &legInfo = teamClassConfig[pc->getId()];
    if (legInfo.size() < legs.size())
      legInfo.resize(legs.size());

    for (size_t k = 0; k < legs.size(); k++) {
      legInfo[k].setMaxRunners(legs[k].getObjectInt("maxNumberOfCompetitors"));
      legInfo[k].setMinRunners(legs[k].getObjectInt("minNumberOfCompetitors"));
    }
  }

  string tmp;
  // Status
  xclass.getObjectString("Status", tmp);

  if (tmp == "Invalidated")
    DI.setString("Status", "I"); // No refund
  else if (tmp == "InvalidatedNoFee")
    DI.setString("Status", "IR"); // Refund

  // No timing
  xclass.getObjectString("resultListMode", tmp);
  if (tmp == "UnorderedNoTimes")
    pc->setNoTiming(true);

  int minAge = xclass.getObjectInt("minAge");
  if (minAge > 0)
    DI.setInt("LowAge", minAge);

  int highAge = xclass.getObjectInt("maxAge");
  if (highAge > 0)
    DI.setInt("HighAge", highAge);

  xclass.getObjectString("sex", tmp);
  if (!tmp.empty())
    DI.setString("Sex", tmp);

  xmlobject type = xclass.getObject("ClassType");
  if (type) {
    DI.setString("ClassType", type.getObjectString("Id", tmp));
  }

  // XXX we only care about the existance of one race class
  xmlobject raceClass = xclass.getObject("RaceClass");

  if (raceClass) {
    xmlList xFees;
    raceClass.getObjects("Fee", xFees);
    if (xFees.size() > 0) {
      vector<FeeInfo> fees(xFees.size());
      int feeIx = 0;
      int feeLateIx = 0;
      int feeRedIx = 0;
      int feeRedLateIx = 0;

      map<string, vector<int> > feePeriods;
      for (size_t k = 0; k < xFees.size(); k++) {
        getFee(xFees[k], fees[k]);
      }

      for (size_t k = 0; k < fees.size(); k++) {
        for (size_t j = k+1; j < fees.size(); j++) {
          if (fees[k].includes(fees[j]))
            fees[j].add(fees[k]);
          if (fees[j].includes(fees[k]))
            fees[k].add(fees[j]);
        }
        feePeriods[fees[k].getDateKey()].push_back(k);
      }

      string youthLimit;
      string seniorLimit;

      vector<int> &earlyEntry = feePeriods.begin()->second;
      getAgeLevels(fees, earlyEntry, feeIx, feeRedIx, youthLimit, seniorLimit);
      const string &lastODate = fees[earlyEntry[0]].toTime;
      if (!lastODate.empty()) {
        oe.getDI().setDate("OrdinaryEntry", lastODate);
      }
      vector<int> &lateEntry = feePeriods.rbegin()->second;
      getAgeLevels(fees, lateEntry, feeLateIx, feeRedLateIx, youthLimit, seniorLimit);

      if (!youthLimit.empty())
        oe.getDI().setInt("YouthAge", getAgeFromDate(youthLimit));

      if (!seniorLimit.empty())
        oe.getDI().setInt("SeniorAge", getAgeFromDate(seniorLimit));

      DI.setInt("ClassFee", oe.interpretCurrency(fees[feeIx].fee, fees[feeIx].currency));
      DI.setInt("HighClassFee", oe.interpretCurrency(fees[feeLateIx].fee, fees[feeLateIx].currency));

      DI.setInt("ClassFeeRed", oe.interpretCurrency(fees[feeRedIx].fee, fees[feeRedIx].currency));
      DI.setInt("HighClassFeeRed", oe.interpretCurrency(fees[feeRedLateIx].fee, fees[feeRedLateIx].currency));

      FeeStatistics feeStat;
      feeStat.fee = fees[feeIx].fee;
      if (feeStat.fee > 0) {
        feeStat.lateFactor = fees[feeLateIx].fee / feeStat.fee;
        feeStatistics.push_back(feeStat);
      }
    }
  }
  pc->synchronize();

  return pc;
}

void IOF30Interface::setupRelayClasses(const map<int, vector<LegInfo> > &teamClassConfig) {
  for (map<int, vector<LegInfo> >::const_iterator it = teamClassConfig.begin();
       it != teamClassConfig.end(); ++it) {
    int classId = it->first;
    const vector<LegInfo> &legs = it->second;
    if (legs.empty())
      continue;
    if (classId > 0) {
      pClass pc = oe.getClass(classId);
      if (!pc) {
        pc = oe.getClassCreate(classId, "tmp" + itos(classId));
      }
      setupRelayClass(pc, legs);
    }
  }
}

void IOF30Interface::setupRelayClass(pClass pc, const vector<LegInfo> &legs) {
  if (pc) {
    int nStage = 0;
    for (size_t k = 0; k < legs.size(); k++) {
      nStage += legs[k].maxRunners;
    }
    pc->setNumStages(nStage);
    pc->setStartType(0, STTime);
    pc->setStartData(0, oe.getAbsTime(3600));

    int ix = 0;
    for (size_t k = 0; k < legs.size(); k++) {
      for (int j = 0; j < legs[k].maxRunners; j++) {
        if (j>0) {
          if (j < legs[k].minRunners)
            pc->setLegType(ix, LTParallel);
          else
            pc->setLegType(ix, LTExtra);

          pc->setStartType(ix, STChange);
        }
        else if (k>0) {
          pc->setLegType(ix, LTNormal);
          pc->setStartType(ix, STChange);
        }
        ix++;
      }
    }
  }
}

string IOF30Interface::getCurrentTime() const {
  // Don't call this method at midnight!
  return getLocalDate() + "T" + getLocalTimeOnly();
}

int IOF30Interface::parseISO8601Time(const xmlobject &xo) {
  if (!xo)
    return 0;
  const char *t = xo.get();
  int tIx = -1;
  int zIx = -1;
  for (int k = 0; t[k] != 0; k++) {
    if (t[k] == 'T' || t[k] == 't') {
      if (tIx == -1)
        tIx = k;
      else ;
        // Bad format
    }
    else if (t[k] == '+' || t[k] == '-' || t[k] == 'Z') {
      if (zIx == -1 && tIx != -1)
        zIx = k;
      else ;
        // Bad format
    }
  }
  string date = t;
  string time = tIx >= 0 ? date.substr(tIx+1) : date;
  string zone = (tIx >= 0 && zIx > 0) ? time.substr(zIx - tIx - 1) : "";

  if (tIx > 0) {
    date = date.substr(0, tIx);

    if (zIx > 0)
      time = time.substr(0, zIx - tIx - 1);
  }

  return oe.getRelativeTime(date, time, zone);
}

void IOF30Interface::getProps(vector<string> &props) const {
  props.push_back("xmlns");
  props.push_back("http://www.orienteering.org/datastandard/3.0");

  props.push_back("xmlns:xsi");
  props.push_back("http://www.w3.org/2001/XMLSchema-instance");

  props.push_back("iofVersion");
  props.push_back("3.0");

  props.push_back("createTime");
  props.push_back(getCurrentTime());

  props.push_back("creator");
  props.push_back("MeOS " + getMeosCompectVersion());
}

void IOF30Interface::writeResultList(xmlparser &xml, const set<int> &classes,
                                     int leg,  bool useUTC_, bool teamsAsIndividual_, bool unrollLoops_) {
  useGMT = useUTC_;
  teamsAsIndividual = teamsAsIndividual_;
  unrollLoops = unrollLoops_;
  vector<string> props;
  getProps(props);

  props.push_back("status");
  props.push_back("Complete");

  xml.startTag("ResultList", props);

  writeEvent(xml);

  vector<pClass> c;
  oe.getClasses(c);

  for (size_t k = 0; k < c.size(); k++) {
//    bool indRel = c[k]->getClassType() == oClassIndividRelay;

    if (classes.empty() || classes.count(c[k]->getId())) {
 /*     oe.getRunners(c[k]->getId(), r, false);
      vector<pRunner> rToUse;
      rToUse.reserve(r.size());

      for (size_t j = 0; j < r.size(); j++) {
        if (leg == -1 || leg == r[j]->getLegNumber()) {
          if (leg == -1 && indRel && r[j]->getLegNumber() != 0)
            continue; // Skip all but leg 0 for individual relay

          if (leg == -1 && !indRel && r[j]->getTeam())
            continue; // For teams, skip presonal results, unless individual relay

          if (r[j]->getStatus() == StatusUnknown)
            continue;

          rToUse.push_back(r[j]);
        }
      }

      vector<pTeam> tToUse;

      if (leg == -1) {
        oe.getTeams(c[k]->getId(), t, false);
        tToUse.reserve(t.size());

        for (size_t j = 0; j < t.size(); j++) {
          for (int n = 0; n < t[j]->getNumRunners(); n++) {
            pRunner tr = t[j]->getRunner(n);
            if (tr && tr->getStatus() != StatusUnknown) {
              tToUse.push_back(t[j]);
              break;
            }
          }
        }

      }
   */
      vector<pRunner> rToUse;
      vector<pTeam> tToUse;
      getRunnersToUse(c[k], rToUse, tToUse, leg, false);

      if (!rToUse.empty() || !tToUse.empty()) {
        writeClassResult(xml, *c[k], rToUse, tToUse);
      }
    }
  }


  xml.endTag();
}

void IOF30Interface::writeClassResult(xmlparser &xml,
                                      const oClass &c,
                                      const vector<pRunner> &r,
                                      const vector<pTeam> &t) {
  pCourse stdCourse = haveSameCourse(r);

  xml.startTag("ClassResult");
  writeClass(xml, c);
  if (stdCourse)
    writeCourse(xml, *stdCourse);

  bool hasInputTime = false;
  for (size_t k = 0; !hasInputTime && k < r.size(); k++) {
    if (r[k]->hasInputData())
      hasInputTime = true;
  }

  for (size_t k = 0; !hasInputTime && k < t.size(); k++) {
    if (t[k]->hasInputData())
      hasInputTime = true;
  }

  for (size_t k = 0; k < r.size(); k++) {
    writePersonResult(xml, *r[k], stdCourse == 0, false, hasInputTime);
  }

  for (size_t k = 0; k < t.size(); k++) {
    writeTeamResult(xml, *t[k], hasInputTime);
  }

  xml.endTag();
}

pCourse IOF30Interface::haveSameCourse(const vector<pRunner> &r) const {
  bool sameCourse = true;
  pCourse stdCourse = r.size() > 0 ? r[0]->getCourse(false) : 0;
  for (size_t k = 1; sameCourse && k < r.size(); k++) {
    int nr = r[k]->getNumMulti();
    for (int j = 0; j <= nr; j++) {
      pRunner tr = r[k]->getMultiRunner(j);
      if (tr && stdCourse != tr->getCourse(true)) {
        sameCourse = false;
        return 0;
      }
    }
  }
  return stdCourse;
}

void IOF30Interface::writeClass(xmlparser &xml, const oClass &c) {
  xml.startTag("Class");
  xml.write("Id", c.getId());
  xml.write("Name", c.getName());

  oClass::ClassStatus stat = c.getClassStatus();
  if (stat == oClass::Invalid)
    xml.write("Status", "Invalidated");
  else if (stat == oClass::InvalidRefund)
    xml.write("Status", "InvalidatedNoFee");


  xml.endTag();
}

void IOF30Interface::writeCourse(xmlparser &xml, const oCourse &c) {
  xml.startTag("Course");
  writeCourseInfo(xml, c);
  xml.endTag();
}

void IOF30Interface::writeCourseInfo(xmlparser &xml, const oCourse &c) {
  xml.write("Id", c.getId());
  xml.write("Name", c.getName());
  int len = c.getLength();
  if (len > 0)
    xml.write("Length", len);
  int climb = c.getDCI().getInt("Climb");
  if (climb > 0)
    xml.write("Climb", climb);
}


string formatStatus(RunnerStatus st) {
  switch (st) {
    case StatusOK:
      return "OK";
    case StatusDNS:
      return "DidNotStart";
    case StatusMP:
      return "MissingPunch";
    case StatusDNF:
      return "DidNotFinish";
    case StatusDQ:
      return "Disqualified";
    case StatusMAX:
      return "OverTime";
    case StatusNotCompetiting:
      return "NotCompeting";
    default:
      return "Inactive";
  }
}

void IOF30Interface::writePersonResult(xmlparser &xml, const oRunner &r,
                                       bool includeCourse, bool teamMember, bool hasInputTime) {
  if (!teamMember)
    xml.startTag("PersonResult");
  else
    xml.startTag("TeamMemberResult");

  writePerson(xml, r);
  const pClub pc = r.getClubRef();

  if (pc && !r.isVacant())
    writeClub(xml, *pc, false);

  if (teamMember) {
    oRunner const *resultHolder = &r;
    cTeam t = r.getTeam();
    pClass cls = r.getClassRef();

    if (t && cls) {
      int leg = r.getLegNumber();
      const int legOrg = leg;
      while (cls->getLegType(leg) == LTIgnore && leg > 0) {
        leg--;
      }
      if (leg < legOrg && t->getRunner(leg))
        resultHolder = t->getRunner(leg);
    }

    writeResult(xml, r, *resultHolder, includeCourse, r.getNumMulti() > 0 || r.getRaceNo() > 0, teamMember, hasInputTime);
  }
  else {
    if (r.getNumMulti() > 0) {
      for (int k = 0; k <= r.getNumMulti(); k++) {
        const pRunner tr = r.getMultiRunner(k);
        if (tr)
          writeResult(xml, *tr, *tr, includeCourse, true, teamMember, hasInputTime);
      }
    }
    else
      writeResult(xml, r, r, includeCourse, false, teamMember, hasInputTime);
  }


  xml.endTag();
}

void IOF30Interface::writeResult(xmlparser &xml, const oRunner &rPerson, const oRunner &r,
                                 bool includeCourse, bool includeRaceNumber,
                                 bool teamMember, bool hasInputTime) {

  vector<SplitData> dummy;
  if (!includeRaceNumber && getStageNumber() == 0)
    xml.startTag("Result");
  else {
    int rn = getStageNumber();
    if (rn == 0)
      rn = 1;
    rn += rPerson.getRaceNo();
    xml.startTag("Result", "raceNumber", itos(rn));
  }

  if (teamMember)
    writeLegOrder(xml, rPerson);

  string bib = rPerson.getBib();
  if (!bib.empty())
    xml.write("BibNumber", bib);

  if (r.getStartTime() > 0)
    xml.write("StartTime", oe.getAbsDateTimeISO(r.getStartTime(), true, useGMT));

  if (r.getFinishTime() > 0)
    xml.write("FinishTime", oe.getAbsDateTimeISO(r.getFinishTimeAdjusted(), true, useGMT));

  if (r.getRunningTime() > 0)
    xml.write("Time", r.getRunningTime());

  int after = r.getTimeAfter();
  if (after >= 0) {
    if (teamMember) {
      xml.write("TimeBehind", "type", "Leg", itos(after));

      after = r.getTimeAfterCourse();
      if (after >= 0)
        xml.write("TimeBehind", "type", "Course", itos(after));

    }
    else
      xml.write("TimeBehind", after);
  }

  if (r.getClassRef()) {

    if (r.statusOK() && r.getClassRef()->getNoTiming() == false) {
      if (!teamMember && r.getPlace() > 0) {
        xml.write("Position", r.getPlace());
      }
      else if (teamMember) {
        int pos = r.getTeam()->getLegPlace(r.getLegNumber(), false);
        if (pos > 0)
          xml.write("Position", "type", "Leg", itos(pos));

        pos = r.getCoursePlace();
        if (pos > 0)
          xml.write("Position", "type", "Course", itos(pos));
      }
    }

    xml.write("Status", formatStatus(r.getStatus()));

    if ( (r.getTeam() && r.getClassRef()->getClassType() != oClassPatrol && !teamsAsIndividual) || hasInputTime) {
      xml.startTag("OverallResult");
      int rt = r.getTotalRunningTime();
      if (rt > 0)
        xml.write("Time", rt);

      bool hasTiming = r.getClassRef()->getNoTiming() == false;
      RunnerStatus stat = r.getTotalStatus();

      int tleg = r.getLegNumber() >= 0 ? r.getLegNumber() : 0;

      if (stat == StatusOK && hasTiming) {
        int after = r.getTotalRunningTime() - r.getClassRef()->getTotalLegLeaderTime(tleg, true);
        if (after >= 0)
          xml.write("TimeBehind", after);
      }

      if (stat == StatusOK && hasTiming)
        xml.write("Position", r.getTotalPlace());

      xml.write("Status", formatStatus(stat));

      xml.endTag();
    }

    pCourse crs = r.getCourse(!unrollLoops);
    if (crs) {
      if (includeCourse)
        writeCourse(xml, *crs);

      const vector<SplitData> &sp = r.getSplitTimes(unrollLoops);
      if (r.getStatus()>0 && r.getStatus() != StatusDNS && r.getStatus() != StatusNotCompetiting) {
        int nc = crs->getNumControls();
        bool hasRogaining = crs->hasRogaining();
        int firstControl = crs->useFirstAsStart() ? 1 : 0;
        if (crs->useLastAsFinish()) {
          nc--;
        }
        set< pair<unsigned, int> > rogaining;
        for (int k = firstControl; k<nc; k++) {
          if (size_t(k) >= sp.size())
            break;
          if (crs->getControl(k)->isRogaining(hasRogaining)) {
            if (sp[k].hasTime()) {
              int time = sp[k].time - r.getStartTime();
              int control = crs->getControl(k)->getFirstNumber();
              rogaining.insert(make_pair(time, control));
            }
            else if (!sp[k].isMissing()) {
              int control = crs->getControl(k)->getFirstNumber();
              rogaining.insert(make_pair(-1, control));
            }
            continue;
          }

          if (sp[k].isMissing())
            xml.startTag("SplitTime", "status", "Missing");
          else
            xml.startTag("SplitTime");
          xml.write("ControlCode", crs->getControl(k)->getFirstNumber());
          if (sp[k].hasTime())
            xml.write("Time", sp[k].time - r.getStartTime());
          xml.endTag();
        }

        for (set< pair<unsigned, int> >::iterator it = rogaining.begin(); it != rogaining.end(); ++it) {
          xml.startTag("SplitTime", "status", "Additional");
          xml.write("ControlCode", it->second);
          if (it->first != -1)
            xml.write("Time", it->first);
          xml.endTag();
        }
      }
    }
  }

  if (rPerson.getCardNo() > 0)
    xml.write("ControlCard", rPerson.getCardNo());

  writeAssignedFee(xml, rPerson.getDCI());

  int cardFee = rPerson.getDCI().getInt("CardFee");
  if (cardFee > 0)
    writeRentalCardService(xml, cardFee);

  xml.endTag();
}

void IOF30Interface::writeTeamResult(xmlparser &xml, const oTeam &t, bool hasInputTime) {
  xml.startTag("TeamResult");

  xml.write("EntryId", t.getId());
  xml.write("Name", t.getName());

  if (t.getClubRef())
    writeClub(xml, *t.getClubRef(), false);

  string bib = t.getBib();
  if (!bib.empty())
    xml.write("BibNumber", bib);

  for (int k = 0; k < t.getNumRunners(); k++) {
    if (t.getRunner(k))
      writePersonResult(xml, *t.getRunner(k), true, true, hasInputTime);
  }

  writeAssignedFee(xml, t.getDCI());
  xml.endTag();
}

int IOF30Interface::getStageNumber() {
  if (cachedStageNumber >= 0)
    return cachedStageNumber;
  int sn = oe.getDCI().getInt("EventNumber");
  if (sn != 0) {
    if (sn < 0)
      sn = 0;
    cachedStageNumber = sn;
    return sn;
  }
  bool pre = oe.hasPrevStage();
  bool post = oe.hasNextStage();

  if (!pre && !post) {
    cachedStageNumber = 0;
  }
  else if (!pre && post) {
    cachedStageNumber = 1;
  }
  else {
    cachedStageNumber = 1;
    // Guess from stage name
    string name = oe.getName();
    if (!name.empty()) {
      char d = name[name.length() -1];
      if (d>='1' && d<='9')
        cachedStageNumber = d - '0';
    }
  }

  return cachedStageNumber;
}

void IOF30Interface::writeEvent(xmlparser &xml) {
  xml.startTag("Event");
  xml.write64("Id", oe.getExtIdentifier());
  xml.write("Name", oe.getName());
  xml.startTag("StartTime");
  xml.write("Date", oe.getDate());
  xml.write("Time", oe.getAbsDateTimeISO(0, false, useGMT));
  xml.endTag();

  if (getStageNumber()) {
    xml.startTag("Race");
    xml.write("RaceNumber", getStageNumber());

    xml.write("Name", oe.getName());

    xml.startTag("StartTime");
    xml.write("Date", oe.getDate());
    xml.write("Time", oe.getAbsDateTimeISO(0, false, useGMT));
    xml.endTag();

    xml.endTag();
  }

  xml.endTag();
}

void IOF30Interface::writePerson(xmlparser &xml, const oRunner &r) {
  xml.startTag("Person");

  __int64 id = r.getExtIdentifier();
  if (id > 0)
    xml.write64("Id", id);

  xml.startTag("Name");
  xml.write("Family", r.getFamilyName());
  xml.write("Given", r.getGivenName());
  xml.endTag();

  xml.endTag();
}

void IOF30Interface::writeClub(xmlparser &xml, const oClub &c, bool writeExtended) const {
  if (c.isVacant() || c.getName().empty())
    return;

  if (writeExtended) {
    const string &type = c.getDCI().getString("Type");
    if (type.empty())
      xml.startTag("Organisation");
    else
      xml.startTag("Organisation", "type", type);
  }
  else {
    xml.startTag("Organisation");
  }
  __int64 id = c.getExtIdentifier();
  if (id > 0)
    xml.write64("Id", id);

  xml.write("Name", c.getName());

  string ctry = c.getDCI().getString("Country");
  string nat = c.getDCI().getString("Nationality");

  if (!ctry.empty() || !nat.empty()) {
    if (ctry.empty()) {
      if (nat == "SWE")
        ctry = "Sweden";
      else
        ctry = nat;
    }
    xml.write("Country", "code", nat, ctry);
  }

  if (writeExtended) {
    oDataConstInterface di = c.getDCI();
    const string &street = di.getString("Street");
    const string &co = di.getString("CareOf");
    const string &city = di.getString("City");
    const string &state = di.getString("State");
    const string &zip = di.getString("ZIP");
    const string &email = di.getString("EMail");
    const string &phone = di.getString("Phone");

    if (!street.empty()) {
      xml.startTag("Address");
      xml.write("Street", street);
      if (!co.empty())
        xml.write("CareOf", co);
      if (!zip.empty())
        xml.write("ZipCode", zip);
      if (!city.empty())
        xml.write("City", city);
      if (!state.empty())
        xml.write("State", state);
      if (!ctry.empty())
        xml.write("Country", ctry);
      xml.endTag();
    }

    if (!email.empty())
      xml.write("Contact", "type", "EmailAddress", email);

    if (!phone.empty())
      xml.write("Contact", "type", "PhoneNumber", phone);

    int dist = di.getInt("District");
    if (dist > 0)
      xml.write("ParentOrganisationId", dist);

    /*
    oClubData->addVariableInt("District", oDataContainer::oIS32, "Organisation");

  oClubData->addVariableString("ShortName", 8, "Kortnamn");
  oClubData->addVariableString("CareOf", 31, "c/o");
  oClubData->addVariableString("Street", 41, "Gata");
  oClubData->addVariableString("City", 23, "Stad");
  oClubData->addVariableString("State", 23, "Region");
  oClubData->addVariableString("ZIP", 11, "Postkod");
  oClubData->addVariableString("EMail", 64, "E-post");
  oClubData->addVariableString("Phone", 32, "Telefon");
  oClubData->addVariableString("Nationality", 3, "Nationalitet");
  oClubData->addVariableString("Country", 23, "Land");
  oClubData->addVariableString("Type", 20, "Typ");

    */
  }

  xml.endTag();
}

void IOF30Interface::writeStartList(xmlparser &xml, const set<int> &classes, bool useUTC_, bool teamsAsIndividual_) {
  useGMT = useUTC_;
  teamsAsIndividual = teamsAsIndividual_;
  vector<string> props;
  getProps(props);

  xml.startTag("StartList", props);

  writeEvent(xml);

  vector<pClass> c;
  oe.getClasses(c);

  for (size_t k = 0; k < c.size(); k++) {

    if (classes.empty() || classes.count(c[k]->getId())) {
      vector<pRunner> rToUse;
      vector<pTeam> tToUse;
      getRunnersToUse(c[k], rToUse, tToUse, -1, true);
      if (!rToUse.empty() || !tToUse.empty()) {
        writeClassStartList(xml, *c[k], rToUse, tToUse);
      }
    }
  }
  xml.endTag();
}

void IOF30Interface::getRunnersToUse(const pClass cls, vector<pRunner> &rToUse,
                                     vector<pTeam> &tToUse, int leg, bool includeUnknown) const {

  rToUse.clear();
  tToUse.clear();
  vector<pRunner> r;
  vector<pTeam> t;

  int classId = cls->getId();
  bool indRel = cls->getClassType() == oClassIndividRelay;

  oe.getRunners(classId, 0, r, false);
  rToUse.reserve(r.size());

  for (size_t j = 0; j < r.size(); j++) {
    if (leg == -1 || leg == r[j]->getLegNumber()) {

      if (!teamsAsIndividual) {
        if (leg == -1 && indRel && r[j]->getLegNumber() != 0)
          continue; // Skip all but leg 0 for individual relay

        if (leg == -1 && !indRel && r[j]->getTeam())
          continue; // For teams, skip presonal results, unless individual relay

        if (!includeUnknown && r[j]->getStatus() == StatusUnknown)
          continue;
      }
      rToUse.push_back(r[j]);
    }
  }

  if (leg == -1 && !teamsAsIndividual) {
    oe.getTeams(classId, t, false);
    tToUse.reserve(t.size());

    for (size_t j = 0; j < t.size(); j++) {
      if (includeUnknown)
         tToUse.push_back(t[j]);
      else {
        for (int n = 0; n < t[j]->getNumRunners(); n++) {
          pRunner tr = t[j]->getRunner(n);
          if (tr && tr->getStatus() != StatusUnknown) {
            tToUse.push_back(t[j]);
            break;
          }
        }
      }
    }
  }
}

void IOF30Interface::writeClassStartList(xmlparser &xml, const oClass &c,
                                         const vector<pRunner> &r,
                                         const vector<pTeam> &t) {

  pCourse stdCourse = haveSameCourse(r);

  xml.startTag("ClassStart");
  writeClass(xml, c);
  if (stdCourse)
    writeCourse(xml, *stdCourse);

  string start = c.getStart();
  if (!start.empty())
    xml.write("StartName", start);

  for (size_t k = 0; k < r.size(); k++) {
    writePersonStart(xml, *r[k], stdCourse == 0, false);
  }

  for (size_t k = 0; k < t.size(); k++) {
    writeTeamStart(xml, *t[k]);
  }
  xml.endTag();
}

void IOF30Interface::writePersonStart(xmlparser &xml, const oRunner &r, bool includeCourse, bool teamMember) {
  if (!teamMember)
    xml.startTag("PersonStart");
  else
    xml.startTag("TeamMemberStart");

  writePerson(xml, r);
  const pClub pc = r.getClubRef();

  if (pc && !r.isVacant())
    writeClub(xml, *pc, false);

  if (teamMember) {
    writeStart(xml, r, includeCourse, r.getNumMulti() > 0 || r.getRaceNo() > 0, teamMember);
  }
  else {
    if (r.getNumMulti() > 0) {
      for (int k = 0; k <= r.getNumMulti(); k++) {
        const pRunner tr = r.getMultiRunner(k);
        if (tr)
          writeStart(xml, *tr, includeCourse, true, teamMember);
      }
    }
    else
      writeStart(xml, r, includeCourse, false, teamMember);
  }

  xml.endTag();
}

void IOF30Interface::writeTeamStart(xmlparser &xml, const oTeam &t) {
  xml.startTag("TeamStart");

  xml.write("EntryId", t.getId());
  xml.write("Name", t.getName());

  if (t.getClubRef())
    writeClub(xml, *t.getClubRef(), false);

  string bib = t.getBib();
  if (!bib.empty())
    xml.write("BibNumber", bib);

  for (int k = 0; k < t.getNumRunners(); k++) {
    if (t.getRunner(k))
      writePersonStart(xml, *t.getRunner(k), true, true);
  }

  writeAssignedFee(xml, t.getDCI());
  xml.endTag();
}

void IOF30Interface::writeStart(xmlparser &xml, const oRunner &r,
                                bool includeCourse, bool includeRaceNumber,
                                bool teamMember) {
  if (!includeRaceNumber && getStageNumber() == 0)
    xml.startTag("Start");
  else {
    int rn = getStageNumber();
    if (rn == 0)
      rn = 1;
    rn += r.getRaceNo();
    xml.startTag("Start", "raceNumber", itos(rn));
  }
  if (teamMember)
    writeLegOrder(xml, r);

  string bib = r.getBib();
  if (!bib.empty())
    xml.write("BibNumber", bib);

  if (r.getStartTime() > 0)
    xml.write("StartTime", oe.getAbsDateTimeISO(r.getStartTime(), true, useGMT));

  pCourse crs = r.getCourse(true);
  if (crs && includeCourse)
    writeCourse(xml, *crs);

  if (r.getCardNo() > 0)
    xml.write("ControlCard", r.getCardNo());

  writeAssignedFee(xml, r.getDCI());

  int cardFee = r.getDCI().getInt("CardFee");

  if (cardFee > 0)
    writeRentalCardService(xml, cardFee);

  xml.endTag();
}

void IOF30Interface::writeLegOrder(xmlparser &xml, const oRunner &r) const {
  // Team member race result
  int legNumber, legOrder;
  const pClass pc = r.getClassRef();
  if (pc) {
    bool par = pc->splitLegNumberParallel(r.getLegNumber(), legNumber, legOrder);
    xml.write("Leg", legNumber + 1);
    if (par)
      xml.write("LegOrder", legOrder + 1);
  }
}

bool IOF30Interface::readXMLCompetitorDB(const xmlobject &xCompetitor) {

  if (!xCompetitor) return false;

  xmlobject person = xCompetitor.getObject("Person");

  if (!person) return false;

  int pid = person.getObjectInt("Id");

  xmlobject pname = person.getObject("Name");
  if (!pname) return false;

  int cardno=0;
  string tmp;

  xmlList cards;
  xCompetitor.getObjects("ControlCard", cards);

  for (size_t k= 0; k<cards.size(); k++) {
    xmlobject &card = cards[k];
    if (card) {
      xmlattrib pSystem = card.getAttrib("punchingSystem");
      if (!pSystem || _stricmp(pSystem.get(), "SI") == 0) {
        cardno = card.getObjectInt(0);
        break;
      }
    }
  }

  string given;
  pname.getObjectString("Given", given);
  getFirst(given, 2);
  string family;
  pname.getObjectString("Family", family);

  if (given.empty() || family.empty())
    return false;

  string name(given+" "+family);

  char sex[2];
  person.getObjectString("sex", sex, 2);

  int birth = person.getObjectInt("BirthDate");

  xmlobject nat=person.getObject("Nationality");

  char national[4]={0,0,0,0};
  if (nat) {
    nat.getObjectString("code", national, 4);
  }

  int clubId = 0;
  xmlobject xClub = xCompetitor.getObject("Organisation");
  if (xClub) {
    clubId = xClub.getObjectInt("Id");
  }

  RunnerDB &runnerDB = oe.getRunnerDatabase();

  RunnerDBEntry *rde = runnerDB.getRunnerById(pid);

  if (!rde) {
    rde = runnerDB.getRunnerByCard(cardno);

    if (rde && rde->getExtId()!=0)
      rde = 0; //Other runner, same card

    if (!rde)
      rde = runnerDB.addRunner(name.c_str(), pid, clubId, cardno);
  }

  if (rde) {
    rde->setExtId(pid);
    rde->setName(name.c_str());
    rde->clubNo = clubId;
    rde->birthYear = extendYear(birth);
    rde->sex = sex[0];
    memcpy(rde->national, national, 3);
  }
  return true;
}

void IOF30Interface::writeXMLCompetitorDB(xmlparser &xml, const RunnerDBEntry &rde) const {
  string s = rde.getSex();

  xml.startTag("Competitor");

  if (s.empty())
    xml.startTag("Person");
  else
    xml.startTag("Person", "sex", s);

  long long pid = rde.getExtId();
  if (pid > 0)
    xml.write64("Id", pid);

  xml.startTag("Name");
  xml.write("Given", rde.getGivenName());
  xml.write("Family", rde.getFamilyName());
  xml.endTag();

  if (rde.getBirthYear() > 1900)
    xml.write("BirthDate", itos(rde.getBirthYear()) + "-01-01");

  string nat = rde.getNationality();
  if (!nat.empty()) {
    xml.write("Nationality", "code", nat.c_str());
  }

  xml.endTag(); // Person

  if (rde.cardNo > 0) {
    xml.write("ControlCard", "punchingSystem", "SI", itos(rde.cardNo));
  }


  if (rde.clubNo > 0) {
    xml.startTag("Organisation");
    xml.write("Id", rde.clubNo);
    xml.endTag();
  }

  xml.endTag(); // Competitor
}

int getStartIndex(int sn);
int getFinishIndex(int sn);
string getStartName(const string &start);

int IOF30Interface::getStartIndex(const string &startId) {
  int num = getNumberSuffix(startId);
  if (num == 0 && startId.length()>0)
    num = int(startId[startId.length()-1])-'0';
  return ::getStartIndex(num);
}

bool IOF30Interface::readControl(const xmlobject &xControl) {
  if (!xControl)
    return false;

  string idStr;
  xControl.getObjectString("Id", idStr);

  if (idStr.empty())
    return false;

  int type = -1;
  int code = 0;
  if (idStr[0] == 'S')
    type = 1;
  else if (idStr[0] == 'F')
    type = 2;
  else {
    type = 0;
    code = atoi(idStr.c_str());
    if (code <= 0)
      return false;
  }

  xmlobject pos = xControl.getObject("MapPosition");

  int xp = 0, yp = 0;
  if (pos) {
    string x,y;
    pos.getObjectString("x", x);
    pos.getObjectString("y", y);
    xp = int(10.0 * atof(x.c_str()));
    yp = int(10.0 * atof(y.c_str()));
  }

  int longitude = 0, latitude = 0;

  xmlobject geopos = xControl.getObject("Position");

  if (geopos) {
    string lat,lng;
    geopos.getObjectString("lat", lat);
    geopos.getObjectString("lng", lng);
    latitude = int(1e6 * atof(lat.c_str()));
    longitude = int(1e6 * atof(lng.c_str()));
  }
  pControl pc = 0;

  if (type == 0) {
    pc = oe.getControl(code, true);
  }
  else if (type == 1) {
    string start = getStartName(trim(idStr));
    pc = oe.getControl(getStartIndex(idStr), true);
    pc->setNumbers("");
    pc->setName(start);
    pc->setStatus(oControl::StatusStart);
  }
  else if (type == 2) {
    string finish = trim(idStr);
    int num = getNumberSuffix(finish);
    if (num == 0 && finish.length()>0)
      num = int(finish[finish.length()-1])-'0';
    if (num > 0 && num<10)
      finish = lang.tl("M�l ") + itos(num);
    else
      finish = lang.tl("M�l");
    pc = oe.getControl(getFinishIndex(num), true);
    pc->setNumbers("");
    pc->setName(finish);
    pc->setStatus(oControl::StatusFinish);
  }

  if (pc) {
    pc->getDI().setInt("xpos", xp);
    pc->getDI().setInt("ypos", yp);
    pc->getDI().setInt("longcrd", longitude);
    pc->getDI().setInt("latcrd", latitude);
    pc->synchronize();
  }
  return true;
}

void IOF30Interface::readCourseGroups(xmlobject xClassCourse, vector< vector<pCourse> > &crs) {
  xmlList groups;
  xClassCourse.getObjects("CourseGroup", groups);

  for (size_t k = 0; k < groups.size(); k++) {
    xmlList courses;
    groups[k].getObjects("Course", courses);
    crs.push_back(vector<pCourse>());
    for (size_t j = 0; j < courses.size(); j++) {
      pCourse pc = readCourse(courses[j]);
      if (pc)
        crs.back().push_back(pc);
    }
  }
}

string IOF30Interface::constructCourseName(const string &family, const string &name) {
  if (family.empty())
    return trim(name);
  else
    return trim(family) + ":" + trim(name);
}

string IOF30Interface::constructCourseName(const xmlobject &xcrs) {
  string name, family;
  xcrs.getObjectString("Name", name);
  if (name.empty())
    // CourseAssignment case
    xcrs.getObjectString("CourseName", name);

  xcrs.getObjectString("CourseFamily", family);

  return constructCourseName(family, name);
}

pCourse IOF30Interface::readCourse(const xmlobject &xcrs) {
  if (!xcrs)
    return 0;

  int cid = xcrs.getObjectInt("Id");

  string name = constructCourseName(xcrs);
  /*, family;
  xcrs.getObjectString("Name", name);
  xcrs.getObjectString("CourseFamily", family);

  if (family.empty())
    name = trim(name);
  else
    name = trim(family) + ":" + trim(name);
*/
  int len = xcrs.getObjectInt("Length");
  int climb = xcrs.getObjectInt("Climb");

  xmlList xControls;
  xcrs.getObjects("CourseControl", xControls);

  vector<pControl> ctrlCode;
  vector<int> legLen;
  string startName;
  bool hasRogaining = false;

  for (size_t k = 0; k < xControls.size(); k++) {
    string type;
    xControls[k].getObjectString("type", type);
    if (type == "Start") {
      string idStr;
      xControls[k].getObjectString("Control", idStr);
      pControl pStart = oe.getControl(getStartIndex(idStr), false);
      if (pStart)
        startName = pStart->getName();
    }
    else if (type == "Finish") {
      legLen.push_back(xControls[k].getObjectInt("LegLength"));
    }
    else {
      xmlList xPunchControls;
      xControls[k].getObjects("Control", xPunchControls);
      pControl pCtrl = 0;
      if (xPunchControls.size() == 1) {
        pCtrl = oe.getControl(xPunchControls[0].getInt(), true);
      }
      else if (xPunchControls.size()>1) {
        pCtrl = oe.addControl(1000*cid + xPunchControls[0].getInt(),xPunchControls[0].getInt(), "");
        if (pCtrl) {
          string cc;
          for (size_t j = 0; j < xPunchControls.size(); j++)
            cc += string(xPunchControls[j].get()) + " ";

          pCtrl->setNumbers(cc);
        }
      }

      if (pCtrl) {
        legLen.push_back(xControls[k].getObjectInt("LegLength"));
        ctrlCode.push_back(pCtrl);
        int score = xControls[k].getObjectInt("Score");
        if (score > 0) {
          pCtrl->getDI().setInt("Rogaining", score);
          pCtrl->setStatus(oControl::StatusRogaining);
          hasRogaining = true;
        }
      }
    }
  }

  pCourse pc = 0;
  if (cid > 0)
    pc = oe.getCourseCreate(cid);
  else {
    pc = oe.getCourse(name);
    if (pc == 0)
      pc = oe.addCourse(name);
  }

  if (pc) {
    pc->setName(name);
    pc->setLength(len);
    pc->importControls("");
    for (size_t i = 0; i<ctrlCode.size(); i++) {
      pc->addControl(ctrlCode[i]->getId());
    }
    if (pc->getNumControls() + 1 == legLen.size())
      pc->setLegLengths(legLen);
    pc->getDI().setInt("Climb", climb);

    pc->setStart(startName, true);
    if (hasRogaining) {
      int mt = oe.getMaximalTime();
      if (mt == 0)
        mt = 3600;
      pc->setMaximumRogainingTime(mt);
    }

    pc->synchronize();
  }
  return pc;
}


void IOF30Interface::bindClassCourse(oClass &pc, const vector< vector<pCourse> > &crs) {
  if (crs.empty())
    return;
  if (crs.size() == 1 && crs[0].size() == 0)
    pc.setCourse(crs[0][0]);
  else {
    unsigned ns = pc.getNumStages();
    ns = max(ns, crs.size());
    pc.setNumStages(ns);
    for (size_t k = 0; k < crs.size(); k++) {
      pc.clearStageCourses(k);
      for (size_t j = 0; j < crs[k].size(); j++) {
        pc.addStageCourse(k, crs[k][j]->getId());
      }
    }
  }
}


void IOF30Interface::writeCourses(xmlparser &xml) {
  vector<string> props;
  getProps(props);
  xml.startTag("CourseData", props);

  writeEvent(xml);

  vector<pControl> ctrl;
  vector<pCourse> crs;
  oe.getControls(ctrl, false);

  xml.startTag("RaceCourseData");
  map<int, string> ctrlId2ExportId;

  // Start
  xml.startTag("Control");
  xml.write("Id", "S");
  xml.endTag();
  set<string> ids;
  for (size_t k = 0; k < ctrl.size(); k++) {
    if (ctrl[k]->getStatus() != oControl::StatusFinish && ctrl[k]->getStatus() != oControl::StatusStart) {
      string id = writeControl(xml, *ctrl[k], ids);
      ctrlId2ExportId[ctrl[k]->getId()] = id;
    }
  }

  // Finish
  xml.startTag("Control");
  xml.write("Id", "F");
  xml.endTag();

  oe.getCourses(crs);
  for (size_t k = 0; k < crs.size(); k++) {
    writeFullCourse(xml, *crs[k], ctrlId2ExportId);
  }

  xml.endTag();

  xml.endTag();
}

string IOF30Interface::writeControl(xmlparser &xml, const oControl &c, set<string> &writtenId) {
  int id = c.getFirstNumber();
  string ids = itos(id);
  if (writtenId.count(ids) == 0) {
    xml.startTag("Control");
    xml.write("Id", ids);
/*
    <!-- the position of the control given in latitude and longitude -->

<!-- coordinates west of the Greenwich meridian and south of the equator are expressed by negative numbers -->
 <Position lng="17.687623" lat="59.760069"/>
<!-- the position of the control on the printed map, relative to the map's lower left corner -->
 <MapPosition y="58" x="187" unit="mm"/>
 */

    xml.endTag();
    writtenId.insert(ids);
  }

  return ids;

}

void IOF30Interface::writeFullCourse(xmlparser &xml, const oCourse &c,
                                       const map<int, string> &ctrlId2ExportId) {

  xml.startTag("Course");
  writeCourseInfo(xml, c);

  xml.startTag("CourseControl", "type", "Start");
  xml.write("Control", "S");
  xml.endTag();

  for (int i = 0; i < c.getNumControls(); i++) {
    int id = c.getControl(i)->getId();
    xml.startTag("CourseControl", "type", "Control");
    if (ctrlId2ExportId.count(id))
      xml.write("Control", ctrlId2ExportId.find(id)->second);
    else
      throw exception();
    xml.endTag();
  }

  xml.startTag("CourseControl", "type", "Finish");
  xml.write("Control", "F");
  xml.endTag();

  xml.endTag();
}

void IOF30Interface::writeRunnerDB(const RunnerDB &db, xmlparser &xml) const {
  vector<string> props;
  getProps(props);

  xml.startTag("CompetitorList", props);

  const vector<RunnerDBEntry> &rdb = db.getRunnerDB();
  for (size_t k = 0; k < rdb.size(); k++) {
    if (!rdb[k].isRemoved())
      writeXMLCompetitorDB(xml, rdb[k]);
  }

  xml.endTag();
}

void IOF30Interface::writeClubDB(const RunnerDB &db, xmlparser &xml) const {
  vector<string> props;
  getProps(props);

  xml.startTag("OrganisationList", props);

  const vector<oDBClubEntry> &cdb = db.getClubDB();
  for (size_t k = 0; k < cdb.size(); k++) {
    if (!cdb[k].isRemoved())
      writeClub(xml, cdb[k], true);
  }

  xml.endTag();
}

