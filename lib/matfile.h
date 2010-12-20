/************************************************************************
 * MechSys - Open Library for Mechanical Systems                        *
 * Copyright (C) 2005 Dorival M. Pedroso, Raúl D. D. Farfan             *
 *                                                                      *
 * This program is free software: you can redistribute it and/or modify *
 * it under the terms of the GNU General Public License as published by *
 * the Free Software Foundation, either version 3 of the License, or    *
 * any later version.                                                   *
 *                                                                      *
 * This program is distributed in the hope that it will be useful,      *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of       *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the         *
 * GNU General Public License for more details.                         *
 *                                                                      *
 * You should have received a copy of the GNU General Public License    *
 * along with this program. If not, see <http://www.gnu.org/licenses/>  *
 ************************************************************************/

#ifndef MECHSYS_MATFILE_H
#define MECHSYS_MATFILE_H

// Std Lib
#include <iostream>
#include <sstream>
#include <fstream>

// wxWidgets
#ifdef USE_WXWIDGETS
  #include <mechsys/gui/wxdict.h>
  #include <mechsys/gui/common.h>
#endif

// MechSys
#include <mechsys/util/maps.h>
#include <mechsys/util/fatal.h>
#include <mechsys/util/util.h>
#include <mechsys/fem/fem.h>

using std::cout;
using std::endl;

#ifdef USE_WXWIDGETS
class MatFile : public wxWindow
#else
class MatFile
#endif
{
public:
    // Constructor
#ifdef USE_WXWIDGETS
     MatFile (wxFrame * Parent);
    ~MatFile () { Aui.UnInit(); }
#else
    MatFile () {}
#endif

    // Methods
    void Read (char const * FileName);
    void Save (char const * FileName);

    // Data
    Dict ID2Prms; ///< maps ID to parameters
    Dict ID2Inis; ///< maps ID to initial values

#ifdef USE_WXWIDGETS
    // typedefs
    typedef std::map<String,GUI::WxDict*> MdlName2Dict_t;

    // Methods
    void Sync () { TransferDataFromWindow(); } ///< Synchronise (validate/transfer) data in controls

    // Data
    wxAuiManager     Aui;      ///< Aui manager
    wxString         LstDir;   ///< Last accessed directory
    wxTextCtrl     * TxtFName; ///< Control with input file filename
    String           FName;    ///< Material file (.mat) filename
    MdlName2Dict_t   Dicts;    ///< model name => dictionaries with parameters

    // Events
    void OnLoad (wxCommandEvent & Event);
    void OnSave (wxCommandEvent & Event);
    DECLARE_EVENT_TABLE();
#endif
};


/////////////////////////////////////////////////////////////////////////////////////////// Implementation /////


inline void MatFile::Read (char const * FileName)
{
    // open material file
    std::fstream mat_file(FileName, std::ios::in);
    if (!mat_file.is_open()) throw new Fatal("MatFile::Read: Could not open file <%s>",FileName);

    // parse
    int  idxprm       = 0;
    int  idxini       = 0;
    int  nprms        = 0;
    int  ninis        = 0;
    int  line_num     = 1;
    bool reading_name = false;
    bool reading_prms = false;
    bool reading_inis = false;
    int  ID           = 0;
    String model_name;
    Array<String> const * prm_names = NULL;
    Array<String> const * ivs_names = NULL;
    ID2Prms.clear ();
    ID2Inis.clear ();
    while (!mat_file.eof())
    {
        String line,key,equal,strval;
        std::getline (mat_file,line);
        std::istringstream iss(line);
        if (iss >> key >> equal >> strval)
        {
            if (key[0]=='#') { line_num++; continue; }
            else if (key=="ID")
            {
                ID = atoi(strval.CStr());
                if (ID<0) throw new Fatal("MatFile::Read: Error in <%s> file at line # %d: IDs must be greater than zero. %d is invalid",FileName,line_num,ID);
                reading_name = true;
                reading_prms = false;
                reading_inis = false;
            }
            else if (reading_name)
            {
                if (key=="name")
                {
                    // iterators to prm and ivs names
                    Str2ArrayStr_t::const_iterator ita = MODEL_PRM_NAMES.find(strval);
                    Str2ArrayStr_t::const_iterator itb = MODEL_IVS_NAMES.find(strval);

                    // check
                    if (ID2Prms.HasKey(ID))              throw new Fatal("MatFile::Read: Error in <%s> file at line # %d: IDs must be unique. %d is repeated",FileName,line_num,ID);
                    if (MODEL.find(strval)==MODEL.end()) throw new Fatal("MatFile::Read: Error in <%s> file at line # %d: Model 'name' = %s is not available in MODEL",FileName,line_num,strval.CStr());
                    if (ita==MODEL_PRM_NAMES.end())      throw new Fatal("MatFile::Read: Error in <%s> file at line # %d: Model 'name' = %s is not available in MODEL_PRM_NAMES",FileName,line_num,strval.CStr());
                    if (itb==MODEL_IVS_NAMES.end())      throw new Fatal("MatFile::Read: Error in <%s> file at line # %d: Model 'name' = %s is not available in MODEL_IVS_NAMES",FileName,line_num,strval.CStr());

                    // set
                    ID2Prms.Set (ID, "name", MODEL(strval));
                    model_name   = strval;
                    reading_name = false;
                    reading_prms = true;
                    reading_inis = false;
                    nprms        = 0;
                    idxprm       = -1;
                    prm_names    = &ita->second;
                    ivs_names    = &itb->second;
                }
                else throw new Fatal("MatFile::Read: Error in <%s> file at line # %d: 'name' must follow 'tags'. '%s' is invalid or in the wrong place",FileName,line_num,key.CStr());
            }
            else if (reading_prms)
            {
                if (nprms==0)
                {
                    if (key=="nprms") nprms = atoi(strval.CStr());
                    else throw new Fatal("MatFile::Read: Error in <%s> file at line # %d: 'nprms' must follow 'name'",FileName,line_num);
                }
                else if (key=="ninis")
                {
                    if (idxprm==nprms-1)
                    {
                        ninis        = atoi(strval.CStr());
                        idxini       = -1;
                        reading_name = false;
                        reading_prms = false;
                        if (ninis>0) reading_inis = true;
                        else
                        {
                            ID2Inis.Set (ID, SDPair());
                            reading_inis = false;
                        }
                    }
                    else throw new Fatal("MatFile::Read: Error in <%s> file at line # %d: 'ninis' must appear after all parameters are read. nprms=%d and %d were read so far",FileName,line_num,nprms,idxprm+1);
                }
                else if (idxprm<nprms)
                {
                    if (prm_names->Has(key))
                    {
                        ID2Prms.Set (ID, key.CStr(), atof(strval.CStr()));
                        idxprm++;
                    }
                    else throw new Fatal("MatFile::Read: Error in <%s> file at line # %d: parameter named '%s' is not available for model '%s'",FileName,line_num,key.CStr(),model_name.CStr());
                }
                else throw new Fatal("MatFile::Read: Error in <%s> file at line # %d: there are more parameters than what specified by nprms=%d. The reading of parameters finishes when 'ninis' is found. '%s' is invalid or in the wrong place (idxprm=%d)",FileName,line_num,nprms,key.CStr(),idxprm);
            }
            else if (reading_inis)
            {
                if (key=="tags" || key=="name" || key=="nprms" || key=="ninis") throw new Fatal("MatFile: Error in <%s> file at line # %d: There are not enough initial values corresponding to ninis=%d. '%s' is the wrong place",FileName,line_num,ninis,key.CStr());
                else if (idxini==ninis-1)
                {
                    reading_name = false;
                    reading_prms = false;
                    reading_inis = false;
                }
                else if (idxini<ninis)
                {
                    if (ivs_names->Has(key))
                    {
                        ID2Inis.Set (ID, key.CStr(), atof(strval.CStr()));
                        idxini++;
                    }
                    else throw new Fatal("MatFile::Read: Error in <%s> file at line # %d: initial value '%s' is not available for model '%s'",FileName,line_num,key.CStr(),model_name.CStr());
                }
                else throw new Fatal("MatFile::Read: Error in <%s> file at line # %d: there are more initial values than what specified by ninis=%d. The reading of initial values finishes when %d values are read. '%s' is invalid or in the wrong place (idxini)",FileName,line_num,ninis,ninis,key.CStr(),idxini);
            }
            else throw new Fatal("MatFile::Read: Problem with <%s> file at line # %d. '%s' is invalid or in the wrong place",FileName,line_num,key.CStr());
        }
        line_num++;
    }
}

inline void MatFile::Save (char const * FileName)
{
    std::ostringstream oss;
    oss << "############ Materials ##############\n\n";
    String buf;
    for (size_t i=0; i<ID2Prms.Keys.Size(); ++i)
    {
        int            id   = ID2Prms.Keys[i];
        SDPair const & prms = ID2Prms(id);
        SDPair const & inis = ID2Inis(id);
        String name;
        MODEL.Val2Key (prms("name"), name);
        buf.Printf("%-8s = %d\n", "ID",    id);                  oss<<buf;
        buf.Printf("%-8s = %s\n", "name",  name.CStr());         oss<<buf;
        buf.Printf("%-8s = %d\n", "nprms", prms.Keys.Size()-1);  oss<<buf;
        for (size_t j=0; j<prms.Keys.Size(); ++j)
        {
            if (prms.Keys[j]!="name")
            {
                buf.Printf("%-8s = %g\n", prms.Keys[j].CStr(), prms(prms.Keys[j]));
                oss << buf;
            }
        }
        buf.Printf("%-8s = %d\n", "ninis", inis.Keys.Size());
        oss << buf;
        for (size_t j=0; j<inis.Keys.Size(); ++j)
        {
            buf.Printf("%-8s = %g\n", inis.Keys[j].CStr(), inis(inis.Keys[j]));
            oss << buf;
        }
        oss << std::endl;
    }
    std::fstream of(FileName, std::ios::out);
    of << oss.str();
    of.close();
}

std::ostream & operator<< (std::ostream & os, MatFile const & MF)
{
    os << "ID2Prms =\n";
    os << MF.ID2Prms << std::endl;
    os << "ID2Inis =\n";
    os << MF.ID2Inis << std::endl;
    return os;
}

#ifdef USE_WXWIDGETS

enum
{
    ID_MATFILE_LOAD = wxID_HIGHEST+2000,
    ID_MATFILE_SAVE ,
};

BEGIN_EVENT_TABLE(MatFile, wxWindow)
    EVT_BUTTON (ID_MATFILE_LOAD, MatFile::OnLoad)
    EVT_BUTTON (ID_MATFILE_SAVE, MatFile::OnSave)
END_EVENT_TABLE()

inline MatFile::MatFile (wxFrame * Parent)
    : wxWindow (Parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxFULL_REPAINT_ON_RESIZE)
{
    // force validation of child controls
    SetExtraStyle (wxWS_EX_VALIDATE_RECURSIVELY);

    // tell wxAuiManager to manage this window
    Aui.SetManagedWindow (this);

    // control panel
    ADD_WXPANEL     (pnl, szt, szr, 1, 3);
    ADD_WXBUTTON    (pnl, szr, ID_MATFILE_LOAD, c0, "Load");
    ADD_WXBUTTON    (pnl, szr, ID_MATFILE_SAVE, c1, "Save");
    ADD_WXTEXTCTRL_ (pnl, szr, wxID_ANY, TxtFName, "", FName);
    TxtFName->SetMinSize (wxSize(200,20));

    // models
    for (ModelFactory_t::iterator it=ModelFactory.begin(); it!=ModelFactory.end(); ++it)
    {
        Dicts[it->first] = new GUI::WxDict(this);
        Dicts[it->first]->ShowSK = false;
        Dicts[it->first]->SameSK = true;
        Str2ArrayStr_t::const_iterator mprms = MODEL_PRM_NAMES.find(it->first);
        if (mprms!=MODEL_PRM_NAMES.end())
        {
            Dicts[it->first]->HideCol0 = true;
            Dicts[it->first]->Tab->SetZero (-1, mprms->second);
            Dicts[it->first]->ReBuild      (false);
        }
        else WxError("MatFile::MatFile: __internal_error__ Model named <%s> is not in map: MODEL_PRM_NAMES",it->first.CStr());
    }

    // notebook
    ADD_WXNOTEBOOK (this, nbk);
    for (MdlName2Dict_t::const_iterator it=Dicts.begin(); it!=Dicts.end(); ++it) nbk->AddPage (it->second, it->first, false);

    // commit all changes to wxAuiManager
    Aui.AddPane (pnl, wxAuiPaneInfo().Name("cpnl").Caption("cpnl").Top().MinSize(wxSize(100,40)).DestroyOnClose(false).CaptionVisible(false) .CloseButton(false));
    Aui.AddPane (nbk, wxAuiPaneInfo().Name("nbk0").Caption("nbk0").Centre().Position(0).DestroyOnClose(false).CaptionVisible(false).CloseButton(false));
    Aui.Update  ();
}

inline void MatFile::OnLoad (wxCommandEvent & Event)
{
    wxFileDialog fd(this, "Load material (.mat) file", LstDir, "", "*.mat");
    if (fd.ShowModal()==wxID_OK)
    {
        TxtFName->SetValue (fd.GetFilename());
        LstDir = fd.GetDirectory ();
        Read (fd.GetPath().ToStdString().c_str());
        for (size_t i=0; i<ID2Prms.Keys.Size(); ++i)
        {
            int            id   = ID2Prms.Keys[i];
            SDPair const & pair = ID2Prms(id);
            String model_name;
            MODEL.Val2Key (pair("name"), model_name);
            MdlName2Dict_t::iterator it = Dicts.find(model_name);
            if (it!=Dicts.end())
            {
                SDPair const & prms = (*it->second->Tab)(-1);
                for (size_t j=0; j<prms.Keys.Size(); ++j)
                {
                    double val = (pair.HasKey(prms.Keys[j]) ? pair(prms.Keys[j]) : 0.0);
                    it->second->Tab->Set (id, prms.Keys[j].CStr(), val);
                }
                it->second->ReBuild ();
            }
            else throw new Fatal("MatFile::OnLoad: __internal_error__ Model named <%s> (from MODEL) wasn't found in Dicts (from ModelFactory)",model_name.CStr());
        }
        TransferDataToWindow ();
    }
}

inline void MatFile::OnSave (wxCommandEvent & Event)
{
    Sync ();
    wxFileDialog fd(this, "Save material (.mat) file", LstDir, "", "*.mat", wxFD_SAVE|wxFD_OVERWRITE_PROMPT);
    if (fd.ShowModal()==wxID_OK) Save (fd.GetPath().ToStdString().c_str());
}

#endif

#endif // MECHSYS_MATFILE_H
