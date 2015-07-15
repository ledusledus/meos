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

#include "resource.h"

#include <commctrl.h>
#include <commdlg.h>
#include <cassert>

#include "oEvent.h"
#include "xmlparser.h"
#include "gdioutput.h"
#include "csvparser.h"
#include "SportIdent.h"
#include "gdifonts.h"
#include "IOF30Interface.h"
#include "meosexception.h"
#include "MeOSFeatures.h"

#include "TabCourse.h"
#include "TabCompetition.h"
#include "meos_util.h"
#include "pdfwriter.h"

TabCourse::TabCourse(oEvent *poe):TabBase(poe)
{
  courseId = 0;
  addedCourse = false;
}

TabCourse::~TabCourse(void)
{
}

void LoadCoursePage(gdioutput &gdi);
void LoadClassPage(gdioutput &gdi);

void TabCourse::selectCourse(gdioutput &gdi, pCourse pc)
{
  if (gdi.hasField("Rogaining")) {
    gdi.setText("TimeLimit", "");
    gdi.disableInput("TimeLimit");
    gdi.setText("PointLimit", "");
    gdi.disableInput("PointLimit");
    gdi.setText("PointReduction", "");
    gdi.disableInput("PointReduction");
    gdi.check("ReductionPerMinute", false);
    gdi.disableInput("ReductionPerMinute");
    gdi.selectItemByData("Rogaining", 0);
  }

  if (pc) {
    pc->synchronize();

    gdi.setText("Controls", pc->getControlsUI());
    gdi.setText("Name", pc->getName());

    gdi.setTextZeroBlank("Length", pc->getLength());
    gdi.setTextZeroBlank("NumberMaps", pc->getNumberMaps());

    gdi.check("FirstAsStart", pc->useFirstAsStart());
    gdi.check("LastAsFinish", pc->useLastAsFinish());

    if (gdi.hasField("Rogaining")) {
      int rt = pc->getMaximumRogainingTime();
      int rp = pc->getMinimumRogainingPoints();

      if ( rt > 0 ) {
        gdi.selectItemByData("Rogaining", 1);
        gdi.enableInput("TimeLimit");
        gdi.setText("TimeLimit", formatTimeHMS(rt));
        gdi.enableInput("PointReduction");
        gdi.setText("PointReduction", itos(pc->getRogainingPointsPerMinute()));
        gdi.enableInput("ReductionPerMinute");
        gdi.check("ReductionPerMinute", pc->getDCI().getInt("RReductionMethod") != 0);
      }
      else if (rp > 0) {
        gdi.selectItemByData("Rogaining", 2);
        gdi.enableInput("PointLimit");
        gdi.setText("PointLimit", itos(rp));
      }
    }

    courseId = pc->getId();
    gdi.enableInput("Remove");
    gdi.enableInput("Save");

    gdi.selectItemByData("Courses", pc->getId());
    gdi.setText("CourseProblem", lang.tl(pc->getCourseProblems()), true);
    vector<pClass> cls;
    vector<pCourse> crs;
    oe->getClasses(cls);
    string usedInClasses;
    for (size_t k = 0; k < cls.size(); k++) {
      int nleg = max<int>(cls[k]->getNumStages(), 1);
      vector<int> usage;
      for (int j = 0; j < nleg; j++) {
        cls[k]->getCourses(j, crs);
        for (size_t i = 0; i < crs.size(); i++) {
          if (crs[i] == pc) {
            usage.push_back(j+1);
            break;
          }
        }
      }
      string add;
      if (usage.size() == 1) {
        add = cls[k]->getName();
      }
      else if (usage.size() > 1) {
        add = cls[k]->getName();
      }

      if (!add.empty()) {
        if (!usedInClasses.empty())
          usedInClasses += ", ";
        usedInClasses += add;
      }
    }
    gdi.setText("CourseUse", usedInClasses, true);

    gdi.enableEditControls(true);

    fillCourseControls(gdi, *oe, pc->getControlsUI());
    int cc = pc->getCommonControl();
    gdi.check("WithLoops", cc != 0);
    gdi.setInputStatus("CommonControl", cc != 0);
    if (cc) {
      gdi.selectItemByData("CommonControl", cc);
    }
  }
  else {
    gdi.setText("Name", "");
    gdi.setText("Controls", "");
    gdi.setText("Length", "");
    gdi.setText("NumberMaps", "");
    gdi.check("FirstAsStart", false);
    gdi.check("LastAsFinish", false);
    courseId = 0;
    gdi.disableInput("Remove");
    gdi.disableInput("Save");
    gdi.selectItemByData("Courses", -1);
    gdi.setText("CourseProblem", "", true);
    gdi.setText("CourseUse", "", true);
    gdi.check("WithLoops", false);
    gdi.clearList("CommonControl");
    gdi.setInputStatus("CommonControl", false);

    gdi.enableEditControls(false);
  }
}

int CourseCB(gdioutput *gdi, int type, void *data)
{
  TabCourse &tc = dynamic_cast<TabCourse &>(*gdi->getTabs().get(TCourseTab));

  return tc.courseCB(*gdi, type, data);
}

void TabCourse::save(gdioutput &gdi)
{
  DWORD cid = courseId;

  pCourse pc;
  string name=gdi.getText("Name");

  if (name.empty()) {
    gdi.alert("Banan måste ha ett namn.");
    return;
  }

  bool create=false;
  if (cid>0)
    pc=oe->getCourse(cid);
  else {
    pc=oe->addCourse(name);
    create=true;
  }

  bool firstAsStart = gdi.isChecked("FirstAsStart");
  bool lastAsFinish = gdi.isChecked("LastAsFinish");
  bool oldFirstAsStart = pc->useFirstAsStart();
  if (!oldFirstAsStart && firstAsStart) {
    vector<pRunner> cr;
    oe->getRunners(0, pc->getId(), cr, false);
    bool hasRes = false;
    for (size_t k = 0; k < cr.size(); k++) {
      if (cr[k]->getCard() != 0) {
        hasRes = true;
        break;
      }
    }
    if (hasRes) {
      firstAsStart = gdi.ask("ask:firstasstart");
    }
  }


  pc->setName(name);
  pc->importControls(gdi.getText("Controls"));
  pc->setLength(gdi.getTextNo("Length"));
  pc->setNumberMaps(gdi.getTextNo("NumberMaps"));
  pc->firstAsStart(firstAsStart);
  pc->lastAsFinish(lastAsFinish);

  if (gdi.isChecked("WithLoops")) {
    int cc = gdi.getTextNo("CommonControl");
    if (cc == 0)
      throw meosException("Ange en varvningskontroll för banan");
    pc->setCommonControl(cc);
  }
  else
    pc->setCommonControl(0);

  if (gdi.hasField("Rogaining")) {
    string t;
    pc->setMaximumRogainingTime(convertAbsoluteTimeMS(gdi.getText("TimeLimit")));
    pc->setMinimumRogainingPoints(atoi(gdi.getText("PointLimit").c_str()));
    int pr = atoi(gdi.getText("PointReduction").c_str());
    pc->setRogainingPointsPerMinute(pr);
    if (pr > 0) {
      int rmethod = gdi.isChecked("ReductionPerMinute") ? 1 : 0;
      pc->getDI().setInt("RReductionMethod", rmethod);
    }
  }

  pc->synchronize();//Update SQL

  oe->fillCourses(gdi, "Courses");
  oe->reEvaluateCourse(pc->getId(), true);

  if (gdi.getData("FromClassPage", cid)) {
    assert(false);
  }
  else if (addedCourse || create)
    selectCourse(gdi, 0);
  else
    selectCourse(gdi, pc);

}

int TabCourse::courseCB(gdioutput &gdi, int type, void *data)
{
  if (type==GUI_BUTTON) {
    ButtonInfo bi=*(ButtonInfo *)data;

    if (bi.id=="Save") {
      save(gdi);
    }
    else if (bi.id=="BrowseCourse") {
      vector< pair<string, string> > ext;
      ext.push_back(make_pair("Alla banfiler", "*.xml;*.csv;*.txt"));
      ext.push_back(make_pair("Banor, OCAD semikolonseparerat", "*.csv;*.txt"));
      ext.push_back(make_pair("Banor, IOF (xml)", "*.xml"));

      string file=gdi.browseForOpen(ext, "csv");

      if (file.length()>0)
        gdi.setText("FileName", file);
    }
    else if (bi.id=="Print") {
      gdi.print(oe);
    }
    else if (bi.id=="PDF") {
      vector< pair<string, string> > ext;
      ext.push_back(make_pair("Portable Document Format (PDF)", "*.pdf"));

      int index;
      string file=gdi.browseForSave(ext, "pdf", index);

      if (!file.empty()) {
        pdfwriter pdf;
        pdf.generatePDF(gdi, gdi.toWide(file), "Report", "MeOS", gdi.getTL());
        gdi.openDoc(file.c_str());
      }
    }
    else if (bi.id == "WithLoops") {
      bool w = gdi.isChecked(bi.id);
      gdi.setInputStatus("CommonControl", w);
      if (w && gdi.getTextNo("CommonControl") == 0)
        gdi.selectFirstItem("CommonControl");
    }
    else if (bi.id == "ExportCourses") {
      int FilterIndex=0;
      vector< pair<string, string> > ext;
      ext.push_back(make_pair("IOF CourseData, version 3.0 (xml)", "*.xml"));
      string save = gdi.browseForSave(ext, "xml", FilterIndex);
      if (save.length()>0) {
        IOF30Interface iof30(oe);
        xmlparser xml(gdi.getEncoding() == ANSI ? 0 : &gdi);
        xml.openOutput(save.c_str(), false);
        iof30.writeCourses(xml);
        xml.closeOut();
      }
    }
    else if (bi.id=="ImportCourses") {
      setupCourseImport(gdi, CourseCB);
    }
    else if (bi.id=="DoImportCourse") {
      string filename = gdi.getText("FileName");
      if (filename.empty())
        return 0;
      gdi.disableInput("DoImportCourse");
      gdi.disableInput("Cancel");
      gdi.disableInput("BrowseCourse");
      gdi.disableInput("AddClasses");

      try {
        TabCourse::runCourseImport(gdi, filename, oe, gdi.isChecked("AddClasses"));
      }
      catch (std::exception &) {
        gdi.enableInput("DoImportCourse");
        gdi.enableInput("Cancel");
        gdi.enableInput("BrowseCourse");
        gdi.enableInput("AddClasses");
        throw;
      }
      gdi.addButton("Cancel", "OK", CourseCB);
      gdi.dropLine();
      gdi.refresh();
    }
    else if (bi.id=="Add") {
      if (courseId>0) {
        string ctrl = gdi.getText("Controls");
        string name = gdi.getText("Name");
        pCourse pc = oe->getCourse(courseId);
        if (pc && !name.empty() && !ctrl.empty() &&  pc->getControlsUI() != ctrl) {
          if (name == pc->getName()) {
            // Make name unique if same name
            int len = name.length();
            if (len > 2 && (isdigit(name[len-1]) || isdigit(name[len-2]))) {
              ++name[len-1]; // course 1 ->  course 2, course 1a -> course 1b
            }
            else
              name += " 2";
          }
          if (gdi.ask("Vill du lägga till banan 'X' (Y)?#" + name + "#" + ctrl)) {
            pc = oe->addCourse(name);
            courseId = pc->getId();
            gdi.setText("Name", name);
            save(gdi);
            return true;
          }
        }
        save(gdi);
      }
      pCourse pc = oe->addCourse(oe->getAutoCourseName());
      pc->synchronize();
      oe->fillCourses(gdi, "Courses");
      selectCourse(gdi, pc);
      gdi.setInputFocus("Name", true);
      addedCourse = true;
    }
    else if (bi.id=="Remove"){
      DWORD cid = courseId;
      if (cid==0){
        gdi.alert("Ingen bana vald.");
        return 0;
      }

      if (oe->isCourseUsed(cid))
        gdi.alert("Banan används och kan inte tas bort.");
      else
        oe->removeCourse(cid);

      oe->fillCourses(gdi, "Courses");

      selectCourse(gdi, 0);
    }
    else if (bi.id=="Cancel"){
      LoadPage("Banor");
    }
  }
  else if (type==GUI_LISTBOX){
    ListBoxInfo bi=*(ListBoxInfo *)data;

    if (bi.id=="Courses") {
      if (gdi.isInputChanged(""))
        save(gdi);

      pCourse pc=oe->getCourse(bi.data);
      selectCourse(gdi, pc);
      addedCourse = false;
    }
    else if (bi.id=="Rogaining") {
      string t;
      t = gdi.getText("TimeLimit");
      if (!t.empty() && t != "-")
        time_limit = t;
      t = gdi.getText("PointLimit");
      if (!t.empty())
        point_limit = t;
      t = gdi.getText("PointReduction");
      if (!t.empty())
        point_reduction = t;

      string tl, pl, pr;
      if (bi.data == 1) {
        tl = time_limit;
        pr = point_reduction;
      }
      else if (bi.data == 2) {
        pl = point_limit;
      }

      gdi.setInputStatus("TimeLimit", !tl.empty());
      gdi.setText("TimeLimit", tl);

      gdi.setInputStatus("PointLimit", !pl.empty());
      gdi.setText("PointLimit", pl);

      gdi.setInputStatus("PointReduction", !pr.empty());
      gdi.setInputStatus("ReductionPerMinute", !pr.empty());
      gdi.setText("PointReduction", pr);

    }
  }
  else if (type == GUI_INPUT) {
    InputInfo ii=*(InputInfo *)data;
    if (ii.id == "Controls") {
      int current = gdi.getTextNo("CommonControl");
      fillCourseControls(gdi, *oe, ii.text);
      if (gdi.isChecked("WithLoops") && current != 0)
        gdi.selectItemByData("CommonControl", current);
    }
  }
  else if (type==GUI_CLEAR) {
    if (courseId>0)
      save(gdi);

    return true;
  }
  return 0;
}

bool TabCourse::loadPage(gdioutput &gdi)
{
  oe->checkDB();
  gdi.selectTab(tabId);

  DWORD ClassID=0, RunnerID=0;

  time_limit = "01:00:00";
  point_limit = "10";
  point_reduction = "1";

  gdi.getData("ClassID", ClassID);
  gdi.getData("RunnerID", RunnerID);

  gdi.clearPage(false);

  gdi.setData("ClassID", ClassID);
  gdi.setData("RunnerID", RunnerID);
  gdi.addString("", boldLarge, "Banor");
  gdi.pushY();

  gdi.fillDown();
  gdi.addListBox("Courses", 200, 360, CourseCB, "Banor (antal kontroller)").isEdit(false).ignore(true);
  gdi.setTabStops("Courses", 160);

  oe->fillCourses(gdi, "Courses");

  gdi.dropLine(1);
  gdi.fillRight();
  gdi.addButton("ImportCourses", "Importera från fil...", CourseCB);
  gdi.addButton("ExportCourses", "Exportera...", CourseCB);
  gdi.newColumn();
  gdi.fillDown();

  gdi.popY();

  gdi.addString("", 10, "help:25041");

  gdi.dropLine(0.5);
  gdi.pushX();
  gdi.fillRight();
  gdi.addInput("Name", "", 16, 0, "Namn:");
  gdi.addInput("Length", "", 8, 0, "Längd (m):");
  gdi.fillDown();
  gdi.addInput("NumberMaps", "", 6, 0, "Antal kartor:");
  gdi.popX();

  gdi.addInput("Controls", "", 54, CourseCB, "Kontroller:");
  gdi.dropLine(0.5);
  gdi.addString("", 10, "help:12662");
  gdi.dropLine(1.5);

  gdi.fillRight();
  gdi.addCheckbox("FirstAsStart", "Använd första kontrollen som start");
  gdi.fillDown();
  gdi.addCheckbox("LastAsFinish", "Använd sista kontrollen som mål");
  gdi.popX();

  gdi.fillRight();
  gdi.addCheckbox("WithLoops", "Bana med slingor", CourseCB);
  gdi.addString("", 0, "Varvningskontroll:");
  gdi.fillDown();
  gdi.dropLine(-0.2);
  gdi.addSelection("CommonControl", 50, 200, 0, "", "En bana med slingor tillåter deltagaren att ta slingorna i valfri ordning");

  gdi.dropLine(1);
  gdi.popX();

  if (oe->getMeOSFeatures().hasFeature(MeOSFeatures::Rogaining)) {
    RECT rc;
    rc.top = gdi.getCY() -5;
    rc.left = gdi.getCX();
    gdi.setCX(gdi.getCX()+gdi.scaleLength(10));

    gdi.addString("", 1, "Rogaining");
    gdi.dropLine(0.5);
    gdi.fillRight();
    gdi.addSelection("Rogaining", 120, 80, CourseCB);
    gdi.addItem("Rogaining", lang.tl("Ingen rogaining"), 0);
    gdi.addItem("Rogaining", lang.tl("Tidsgräns"), 1);
    gdi.addItem("Rogaining", lang.tl("Poänggräns"), 2);

    gdi.setCX(gdi.getCX()+gdi.scaleLength(20));
    gdi.dropLine(-0.8);
    int cx = gdi.getCX();
    gdi.addInput("PointLimit", "", 8, 0, "Poänggräns:").isEdit(false);
    gdi.addInput("TimeLimit", "", 8, 0, "Tidsgräns:").isEdit(false);
    gdi.addInput("PointReduction", "", 8, 0, "Poängavdrag (per minut):").isEdit(false);
    gdi.dropLine(3.5);
    rc.right = gdi.getCX() + 5;
    gdi.setCX(cx);
    gdi.fillDown();
    gdi.addCheckbox("ReductionPerMinute", "Poängavdrag per påbörjad minut");

    rc.bottom = gdi.getCY() + 5;
    gdi.addRectangle(rc, colorLightBlue, true);
  }

  gdi.popX();
  gdi.fillDown();
  gdi.dropLine(2);
  gdi.addString("CourseUse", 0, "").setColor(colorDarkBlue);
  gdi.dropLine();
  gdi.addString("CourseProblem", 1, "").setColor(colorRed);
  gdi.dropLine(2);
  gdi.fillRight();
  gdi.addButton("Save", "Spara", CourseCB, "help:save").setDefault();
  gdi.addButton("Remove", "Radera", CourseCB);
  gdi.addButton("Add", "Ny bana", CourseCB);
  gdi.disableInput("Remove");
  gdi.disableInput("Save");

  selectCourse(gdi, oe->getCourse(courseId));
  gdi.setOnClearCb(CourseCB);

  gdi.refresh();

  return true;
}

void TabCourse::runCourseImport(gdioutput& gdi, const string &filename,
                                oEvent *oe, bool addClasses) {
  csvparser csv;
  if (csv.iscsv(filename.c_str())) {
    gdi.fillRight();
    gdi.pushX();
    gdi.addString("", 0, "Importerar OCAD csv-fil...");
    gdi.refreshFast();

    if (csv.ImportOCAD_CSV(*oe, filename.c_str(), addClasses)) {
      gdi.addString("", 1, "Klart.").setColor(colorGreen);
    }
    else gdi.addString("", 0, "Operationen misslyckades.").setColor(colorRed);
    gdi.popX();
    gdi.dropLine(2.5);
    gdi.fillDown();
  }
  else {
    oe->importXML_EntryData(gdi, filename.c_str(), addClasses);
  }
  if (addClasses) {
    // There is specific course-class matching inside the import of each format,
    // that uses additional information. Here we try to match based on a generic approach.
    vector<pClass> cls;
    vector<pCourse> crs;
    oe->getClasses(cls);
    oe->getCourses(crs);

    map<string, pCourse> name2Course;
    map<int, vector<pClass> > course2Class;
    for (size_t k = 0; k < crs.size(); k++)
      name2Course[crs[k]->getName()] = crs[k];
    bool hasMissing = false;
    for (size_t k = 0; k < cls.size(); k++) {
      vector<pCourse> usedCrs;
      cls[k]->getCourses(-1, usedCrs);

      if (usedCrs.empty()) {
        map<string, pCourse>::iterator res = name2Course.find(cls[k]->getName());
        if (res != name2Course.end()) {
          usedCrs.push_back(res->second);
          if (cls[k]->getNumStages()==0) {
            cls[k]->setCourse(res->second);
          }
          else {
            for (size_t i = 0; i<cls[k]->getNumStages(); i++)
              cls[k]->addStageCourse(i, res->second->getId());
          }
        }
        else {
          hasMissing = true;
        }
      }

      for (size_t j = 0; j < usedCrs.size(); j++) {
        course2Class[usedCrs[j]->getId()].push_back(cls[k]);
      }
    }

    for (size_t k = 0; k < crs.size(); k++) {
      pClass bestClass;
      if (hasMissing && (bestClass = oe->getBestClassMatch(crs[k]->getName())) != 0) {
        vector<pCourse> usedCrs;
        bestClass->getCourses(-1, usedCrs);
        if (usedCrs.empty()) {
          course2Class[crs[k]->getId()].push_back(bestClass);
          if (bestClass->getNumStages()==0) {
            bestClass->setCourse(crs[k]);
          }
          else {
            for (size_t i = 0; i<bestClass->getNumStages(); i++)
              bestClass->addStageCourse(i, crs[k]->getId());
          }
        }
      }
    }

    gdi.addString("", 1, "Klasser");
    int yp = gdi.getCY();
    int xp = gdi.getCX();
    int w = gdi.scaleLength(200);
    for (size_t k = 0; k < cls.size(); k++) {
      vector<pCourse> usedCrs;
      cls[k]->getCourses(-1, usedCrs);
      string c;
      for (size_t j = 0; j < usedCrs.size(); j++) {
        if (j>0)
          c += ", ";
        c += usedCrs[j]->getName();
      }
      TextInfo &ci = gdi.addStringUT(yp, xp, 0, cls[k]->getName(), w);
      if (c.empty()) {
        c = MakeDash("-");
        ci.setColor(colorRed);
      }
      gdi.addStringUT(yp, xp + w, 0, c);
      yp += gdi.getLineHeight();
    }

    gdi.dropLine();
    gdi.addString("", 1, "Banor");
    yp = gdi.getCY();
    for (size_t k = 0; k < crs.size(); k++) {
      string c;
      vector<pClass> usedCls = course2Class[crs[k]->getId()];
      for (size_t j = 0; j < usedCls.size(); j++) {
        if (j>0)
          c += ", ";
        c += usedCls[j]->getName();
      }
      TextInfo &ci = gdi.addStringUT(yp, xp, 0, crs[k]->getName(), w);
      if (c.empty()) {
        c = MakeDash("-");
        ci.setColor(colorRed);
      }
      gdi.addStringUT(yp, xp + w, 0, c);
      yp += gdi.getLineHeight();
    }
    gdi.dropLine();
  }

  gdi.addButton(gdi.getWidth()+20, 45,  gdi.scaleLength(120),
                "Print", "Skriv ut...", CourseCB,
                "Skriv ut listan.", true, false);
  gdi.addButton(gdi.getWidth()+20, 75,  gdi.scaleLength(120),
                "PDF", "PDF...", CourseCB,
                "Spara som PDF.", true, false);

  gdi.setWindowTitle(oe->getTitleName());
  oe->updateTabs();
  gdi.refresh();
}

void TabCourse::setupCourseImport(gdioutput& gdi, GUICALLBACK cb) {

  gdi.clearPage(true);
  gdi.addString("", 2, "Importera banor/klasser");
  gdi.addString("", 0, "help:importcourse");
  gdi.dropLine();

  gdi.fillRight();
  gdi.pushX();
  gdi.addInput("FileName", "", 48, 0, "Filnamn:");
  gdi.dropLine();
  gdi.fillDown();
  gdi.addButton("BrowseCourse", "Bläddra...", CourseCB);

  gdi.dropLine(0.5);
  gdi.popX();

  gdi.fillDown();
  gdi.addCheckbox("AddClasses", "Lägg till klasser", 0, true);

  gdi.dropLine();
  gdi.fillRight();
  gdi.addButton("DoImportCourse", "Importera", cb).setDefault();
  gdi.fillDown();
  gdi.addButton("Cancel", "Avbryt", cb).setCancel();
  gdi.setInputFocus("FileName");
  gdi.popX();
}

void TabCourse::fillCourseControls(gdioutput &gdi, oEvent &oe, const string &ctrl) {
  vector<int> nr;
  oCourse::splitControls(ctrl, nr);

  vector< pair<string, size_t> > item;
  map<int, int> used;
  for (size_t k = 0; k < nr.size(); k++) {
    pControl pc = oe.getControl(nr[k], false);
    if (pc) {
      if (pc->getStatus() == oControl::StatusOK)
        ++used[pc->getFirstNumber()];
    }
    else
      ++used[nr[k]];
  }

  set<int> added;
  for (int i = 10; i > 0; i--) {
    for (map<int, int>::iterator it = used.begin(); it != used.end(); ++it) {
      if (it->second >= i && !added.count(it->first)) {
        added.insert(it->first);
        item.push_back(make_pair(itos(it->first), it->first));
      }
    }
  }

  gdi.clearList("CommonControl");
  gdi.addItem("CommonControl", item);
}
