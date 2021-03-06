/**
 * @file dialog_gendrill.cpp
 */

/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 1992-2019 Jean_Pierre Charras <jp.charras at wanadoo.fr>
 * Copyright (C) 1992-2019 KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#include <fctsys.h>
#include <kiface_i.h>
#include <confirm.h>
#include <pcbnew.h>
#include <pcb_edit_frame.h>
#include <pcbplot.h>
#include <gendrill_Excellon_writer.h>
#include <gendrill_gerber_writer.h>
#include <bitmaps.h>

#include <class_board.h>
#include <class_track.h>
#include <class_module.h>

#include <dialog_gendrill.h>
#include <wildcards_and_files_ext.h>
#include <reporter.h>

#include <wx/stdpaths.h>


// Keywords for read and write parameters in project config
#define ZerosFormatKey          wxT( "DrillZerosFormat" )
#define MirrorKey               wxT( "DrillMirrorYOpt" )
#define MinimalHeaderKey        wxT( "DrillMinHeader" )
#define MergePTHNPTHKey         wxT( "DrillMergePTHNPTH" )
#define UnitDrillInchKey        wxT( "DrillUnit" )
#define DrillMapFileTypeKey     wxT( "DrillMapFileType" )
#define DrillFileFormatKey      wxT( "DrillFileType" )
#define OvalHolesRouteModeKey   wxT( "OvalHolesRouteMode" )

// list of allowed precision for EXCELLON files, for integer format:
// Due to difference between inches and mm,
// there are 2 precision values, one for inches and one for metric
// Note: for decimla format, the precision is not used
static DRILL_PRECISION precisionListForInches( 2, 4 );
static DRILL_PRECISION precisionListForMetric( 3, 3 );


/* This function displays the dialog frame for drill tools
 */
void PCB_EDIT_FRAME::InstallDrillFrame( wxCommandEvent& event )
{
    DIALOG_GENDRILL dlg( this, this );

    dlg.ShowModal();
}


DIALOG_GENDRILL::DIALOG_GENDRILL(  PCB_EDIT_FRAME* aPcbEditFrame, wxWindow* aParent  ) :
    DIALOG_GENDRILL_BASE( aParent )
{
    m_pcbEditFrame = aPcbEditFrame;
    m_board  = m_pcbEditFrame->GetBoard();
    m_config = Kiface().KifaceSettings();
    m_plotOpts = m_pcbEditFrame->GetPlotSettings();

    // We use a sdbSizer to get platform-dependent ordering of the action buttons, but
    // that requires us to correct the button labels here.
    m_sdbSizerOK->SetLabel( _( "Generate Drill File" ) );
    m_sdbSizerApply->SetLabel( _( "Generate Map File" ) );
    m_sdbSizerCancel->SetLabel( _( "Close" ) );
    m_buttonsSizer->Layout();

    m_sdbSizerOK->SetDefault();
    SetReturnCode( 1 );
    initDialog();
    GetSizer()->SetSizeHints( this );
}


// Static members of DIALOG_GENDRILL
int DIALOG_GENDRILL::m_UnitDrillIsInch  = true;     // Only for Excellon format
int DIALOG_GENDRILL::m_ZerosFormat      = EXCELLON_WRITER::DECIMAL_FORMAT;
bool DIALOG_GENDRILL::m_MinimalHeader   = false;    // Only for Excellon format
bool DIALOG_GENDRILL::m_Mirror = false;             // Only for Excellon format
bool DIALOG_GENDRILL::m_Merge_PTH_NPTH  = false;    // Only for Excellon format
int DIALOG_GENDRILL::m_mapFileType      = 1;
int DIALOG_GENDRILL::m_drillFileType    = 0;
bool DIALOG_GENDRILL::m_UseRouteModeForOvalHoles = true;    // Use G00 route mode to "drill" oval holes

DIALOG_GENDRILL::~DIALOG_GENDRILL()
{
}


void DIALOG_GENDRILL::initDialog()
{
    m_config->Read( ZerosFormatKey, &m_ZerosFormat );
    m_config->Read( MirrorKey, &m_Mirror );
    m_config->Read( MergePTHNPTHKey, &m_Merge_PTH_NPTH );
    m_config->Read( MinimalHeaderKey, &m_MinimalHeader );
    m_config->Read( UnitDrillInchKey, &m_UnitDrillIsInch );
    m_drillOriginIsAuxAxis = m_plotOpts.GetUseAuxOrigin();
    m_config->Read( DrillMapFileTypeKey, &m_mapFileType );
    m_config->Read( DrillFileFormatKey, &m_drillFileType );
    m_config->Read( OvalHolesRouteModeKey, &m_UseRouteModeForOvalHoles );

    InitDisplayParams();
}


void DIALOG_GENDRILL::InitDisplayParams()
{
    m_browseButton->SetBitmap( KiBitmap( folder_xpm ) );

    m_rbExcellon->SetValue( m_drillFileType == 0 );
    m_rbGerberX2->SetValue( m_drillFileType == 1 );
    m_Choice_Unit->SetSelection( m_UnitDrillIsInch ? 1 : 0 );
    m_Choice_Zeros_Format->SetSelection( m_ZerosFormat );
    UpdatePrecisionOptions();
    m_Check_Minimal->SetValue( m_MinimalHeader );

    if( m_drillOriginIsAuxAxis )
        m_Choice_Drill_Offset->SetSelection( 1 );

    m_Check_Mirror->SetValue( m_Mirror );
    m_Check_Merge_PTH_NPTH->SetValue( m_Merge_PTH_NPTH );
    m_Choice_Drill_Map->SetSelection( m_mapFileType );
    m_radioBoxOvalHoleMode->SetSelection( m_UseRouteModeForOvalHoles ? 0 : 1 );

    m_platedPadsHoleCount    = 0;
    m_notplatedPadsHoleCount = 0;
    m_throughViasCount = 0;
    m_microViasCount   = 0;
    m_blindOrBuriedViasCount = 0;

    for( MODULE* module = m_board->m_Modules;  module;  module = module->Next() )
    {
        for( D_PAD* pad = module->PadsList(); pad != NULL; pad = pad->Next() )
        {
            if( pad->GetDrillShape() == PAD_DRILL_SHAPE_CIRCLE )
            {
                if( pad->GetDrillSize().x != 0 )
                {
                    if( pad->GetAttribute() == PAD_ATTRIB_HOLE_NOT_PLATED )
                        m_notplatedPadsHoleCount++;
                    else
                        m_platedPadsHoleCount++;
                }
            }
            else
            {
                if( pad->GetDrillSize().x != 0 && pad->GetDrillSize().y != 0 )
                {
                    if( pad->GetAttribute() == PAD_ATTRIB_HOLE_NOT_PLATED )
                        m_notplatedPadsHoleCount++;
                    else
                        m_platedPadsHoleCount++;
                }
            }
        }
    }

    for( TRACK* track = m_board->m_Track; track != NULL; track = track->Next() )
    {
        const VIA *via = dynamic_cast<const VIA*>( track );
        if( via )
        {
            switch( via->GetViaType() )
            {
            case VIA_THROUGH:
                m_throughViasCount++;
                break;

            case VIA_MICROVIA:
                m_microViasCount++;
                break;

            case VIA_BLIND_BURIED:
                m_blindOrBuriedViasCount++;
                break;

            default:
                break;
            }
        }
    }

    // Display hole counts:
    m_PlatedPadsCountInfoMsg->   SetLabel( wxString() << m_platedPadsHoleCount );
    m_NotPlatedPadsCountInfoMsg->SetLabel( wxString() << m_notplatedPadsHoleCount );
    m_ThroughViasInfoMsg->       SetLabel( wxString() << m_throughViasCount );
    m_MicroViasInfoMsg->         SetLabel( wxString() << m_microViasCount );
    m_BuriedViasInfoMsg->        SetLabel( wxString() << m_blindOrBuriedViasCount );

    // Output directory
    m_outputDirectoryName->SetValue( m_plotOpts.GetOutputDirectory() );

    wxCommandEvent dummy;
    onFileFormatSelection( dummy );
}


void DIALOG_GENDRILL::onFileFormatSelection( wxCommandEvent& event )
{
    bool enbl_Excellon = m_rbExcellon->GetValue();

    m_drillFileType = enbl_Excellon ? 0 : 1;

    m_Choice_Unit->Enable( enbl_Excellon );
	m_Choice_Zeros_Format->Enable( enbl_Excellon );
    m_Check_Mirror->Enable( enbl_Excellon );
    m_Check_Minimal->Enable( enbl_Excellon );
    m_Check_Merge_PTH_NPTH->Enable( enbl_Excellon );
    m_radioBoxOvalHoleMode->Enable( enbl_Excellon );

    if( enbl_Excellon )
        UpdatePrecisionOptions();
    else
    {
        m_staticTextPrecision->Enable( true );
        m_staticTextPrecision->SetLabel( m_plotOpts.GetGerberPrecision() == 6 ? "4.6" : "4.5" );
    }
}


void DIALOG_GENDRILL::UpdateConfig()
{
    UpdateDrillParams();

    m_config->Write( ZerosFormatKey, m_ZerosFormat );
    m_config->Write( MirrorKey, m_Mirror );
    m_config->Write( MergePTHNPTHKey, m_Merge_PTH_NPTH );
    m_config->Write( MinimalHeaderKey, m_MinimalHeader );
    m_config->Write( UnitDrillInchKey, m_UnitDrillIsInch );
    m_config->Write( DrillMapFileTypeKey, m_mapFileType );
    m_config->Write( DrillFileFormatKey, m_drillFileType );
    m_config->Write( OvalHolesRouteModeKey, m_UseRouteModeForOvalHoles );
}


void DIALOG_GENDRILL::OnSelDrillUnitsSelected( wxCommandEvent& event )
{
    UpdatePrecisionOptions();
}


void DIALOG_GENDRILL::OnGenMapFile( wxCommandEvent& event )
{
    GenDrillAndMapFiles( false, true);
}


void DIALOG_GENDRILL::OnGenDrillFile( wxCommandEvent& event )
{
    GenDrillAndMapFiles(true, false);
}


void DIALOG_GENDRILL::OnSelZerosFmtSelected( wxCommandEvent& event )
{
    UpdatePrecisionOptions();
}


void DIALOG_GENDRILL::UpdatePrecisionOptions()
{
    if( m_Choice_Unit->GetSelection()== 1 )     // Units = inches
        m_staticTextPrecision->SetLabel( precisionListForInches.GetPrecisionString() );
    else
    {
        // metric options
        m_staticTextPrecision->SetLabel( precisionListForMetric.GetPrecisionString() );
    }

    if( m_Choice_Zeros_Format->GetSelection() == EXCELLON_WRITER::DECIMAL_FORMAT )
        m_staticTextPrecision->Enable( false );
    else
        m_staticTextPrecision->Enable( true );
}


void DIALOG_GENDRILL::OnOutputDirectoryBrowseClicked( wxCommandEvent& event )
{
    // Build the absolute path of current output plot directory
    // to preselect it when opening the dialog.
    wxFileName  fn( m_outputDirectoryName->GetValue() );
    wxString    path = Prj().AbsolutePath( m_outputDirectoryName->GetValue() );

    wxDirDialog dirDialog( this, _( "Select Output Directory" ), path );

    if( dirDialog.ShowModal() == wxID_CANCEL )
        return;

    wxFileName      dirName = wxFileName::DirName( dirDialog.GetPath() );

    fn = Prj().AbsolutePath( m_board->GetFileName() );
    wxString defaultPath = fn.GetPathWithSep();
    wxString msg;
    msg.Printf( _( "Do you want to use a path relative to\n\"%s\"" ), GetChars( defaultPath ) );

    wxMessageDialog dialog( this, msg, _( "Plot Output Directory" ),
                            wxYES_NO | wxICON_QUESTION | wxYES_DEFAULT );

    if( dialog.ShowModal() == wxID_YES )
    {
        if( !dirName.MakeRelativeTo( defaultPath ) )
            wxMessageBox( _( "Cannot make path relative (target volume different from file volume)!" ),
                          _( "Plot Output Directory" ), wxOK | wxICON_ERROR );
    }

    m_outputDirectoryName->SetValue( dirName.GetFullPath() );
}


void DIALOG_GENDRILL::UpdateDrillParams()
{
    wxString msg;

    // Set output directory and replace backslashes with forward ones
    wxString dirStr;
    dirStr = m_outputDirectoryName->GetValue();
    dirStr.Replace( wxT( "\\" ), wxT( "/" ) );
    m_plotOpts.SetOutputDirectory( dirStr );
    m_drillOriginIsAuxAxis = m_Choice_Drill_Offset->GetSelection();
    m_plotOpts.SetUseAuxOrigin( m_drillOriginIsAuxAxis );

    m_mapFileType = m_Choice_Drill_Map->GetSelection();

    m_UnitDrillIsInch = (m_Choice_Unit->GetSelection() == 0) ? false : true;
    m_MinimalHeader   = m_Check_Minimal->IsChecked();
    m_Mirror = m_Check_Mirror->IsChecked();
    m_Merge_PTH_NPTH = m_Check_Merge_PTH_NPTH->IsChecked();
    m_ZerosFormat = m_Choice_Zeros_Format->GetSelection();
    m_UseRouteModeForOvalHoles = m_radioBoxOvalHoleMode->GetSelection() == 0;

    if( m_Choice_Drill_Offset->GetSelection() == 0 )
        m_FileDrillOffset = wxPoint( 0, 0 );
    else
        m_FileDrillOffset = m_pcbEditFrame->GetAuxOrigin();

    if( m_UnitDrillIsInch )
        m_Precision = precisionListForInches;
    else
        m_Precision = precisionListForMetric;

    m_board->SetPlotOptions( m_plotOpts );
}


void DIALOG_GENDRILL::GenDrillAndMapFiles( bool aGenDrill, bool aGenMap )
{
    UpdateConfig();     // set params and Save drill options

    m_pcbEditFrame->ClearMsgPanel();
    WX_TEXT_CTRL_REPORTER reporter( m_messagesBox );

    const PlotFormat filefmt[6] =
    {   // Keep these format ids in the same order than m_Choice_Drill_Map choices
        PLOT_FORMAT_HPGL, PLOT_FORMAT_POST, PLOT_FORMAT_GERBER,
        PLOT_FORMAT_DXF, PLOT_FORMAT_SVG, PLOT_FORMAT_PDF
    };

    unsigned choice = (unsigned) m_Choice_Drill_Map->GetSelection();

    if( choice >= arrayDim( filefmt ) )
        choice = 1;

    // Create output directory if it does not exist (also transform it in
    // absolute form). Bail if it fails
    wxFileName  outputDir = wxFileName::DirName( m_plotOpts.GetOutputDirectory() );
    wxString    boardFilename = m_board->GetFileName();

    if( !EnsureFileDirectoryExists( &outputDir, boardFilename, &reporter ) )
    {
        wxString msg;
        msg.Printf( _( "Could not write drill and/or map files to folder \"%s\"." ),
                    GetChars( outputDir.GetPath() ) );
        DisplayError( this, msg );
        return;
    }

    if( m_drillFileType == 0 )
    {
        EXCELLON_WRITER excellonWriter( m_board );
        excellonWriter.SetFormat( !m_UnitDrillIsInch, (EXCELLON_WRITER::ZEROS_FMT) m_ZerosFormat,
                                  m_Precision.m_lhs, m_Precision.m_rhs );
        excellonWriter.SetOptions( m_Mirror, m_MinimalHeader, m_FileDrillOffset, m_Merge_PTH_NPTH );
        excellonWriter.SetRouteModeForOvalHoles( m_UseRouteModeForOvalHoles );
        excellonWriter.SetMapFileFormat( filefmt[choice] );

        excellonWriter.CreateDrillandMapFilesSet( outputDir.GetFullPath(),
                                                  aGenDrill, aGenMap, &reporter );
    }
    else
    {
        GERBER_WRITER gerberWriter( m_board );
        // Set gerber precision: only 5 or 6 digits for mantissa are allowed
        // (SetFormat() accept 5 or 6, and any other value set the precision to 5)
        // the integer part precision is always 4, and units always mm
        gerberWriter.SetFormat( m_plotOpts.GetGerberPrecision() );
        gerberWriter.SetOptions( m_FileDrillOffset );
        gerberWriter.SetMapFileFormat( filefmt[choice] );

        gerberWriter.CreateDrillandMapFilesSet( outputDir.GetFullPath(),
                                                aGenDrill, aGenMap, &reporter );
    }
}


void DIALOG_GENDRILL::OnGenReportFile( wxCommandEvent& event )
{
    UpdateConfig(); // set params and Save drill options

    wxFileName fn = m_board->GetFileName();

    fn.SetName( fn.GetName() + wxT( "-drl" ) );
    fn.SetExt( ReportFileExtension );

    wxString defaultPath = Prj().AbsolutePath( m_plotOpts.GetOutputDirectory() );

    if( defaultPath.IsEmpty() )
        defaultPath = wxStandardPaths::Get().GetDocumentsDir();

    wxFileDialog dlg( this, _( "Save Drill Report File" ), defaultPath, fn.GetFullName(),
                      ReportFileWildcard(), wxFD_SAVE );

    if( dlg.ShowModal() == wxID_CANCEL )
        return;

    bool success;

    // Info is slightly different between Excellon and Gerber
    // (file ext, Merge PTH/NPTH option)
    if( m_drillFileType == 0 )
    {
        EXCELLON_WRITER excellonWriter( m_board );
        excellonWriter.SetMergeOption( m_Merge_PTH_NPTH );
        success = excellonWriter.GenDrillReportFile( dlg.GetPath() );
    }
    else
    {
        GERBER_WRITER gerberWriter( m_board );
        success = gerberWriter.GenDrillReportFile( dlg.GetPath() );
    }

    wxString   msg;

    if( ! success )
    {
        msg.Printf(  _( "** Unable to create %s **\n" ), GetChars( dlg.GetPath() ) );
        m_messagesBox->AppendText( msg );
    }
    else
    {
        msg.Printf( _( "Report file %s created\n" ), GetChars( dlg.GetPath() ) );
        m_messagesBox->AppendText( msg );
    }
}
