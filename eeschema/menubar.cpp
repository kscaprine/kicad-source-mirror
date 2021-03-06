/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2018 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2009 Wayne Stambaugh <stambaughw@gmail.com>
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


#include <kiface_i.h>
#include <menus_helpers.h>
#include <pgm_base.h>
#include <tool/conditional_menu.h>
#include <tool/tool_manager.h>
#include <tools/ee_selection_tool.h>
#include <tools/ee_actions.h>

#include "eeschema_id.h"
#include "general.h"
#include "help_common_strings.h"
#include "ee_hotkeys.h"
#include "sch_edit_frame.h"

class CONDITIONAL_MENU;

// helper functions that build specific submenus:

// Build the place submenu
static void preparePlaceMenu( CONDITIONAL_MENU* aParentMenu, EE_SELECTION_TOOL* selTool );

// Build the files menu. Because some commands are available only if
// Eeschemat is run outside a project (run alone), aIsOutsideProject is false
// when Eeschema is run from Kicad manager, and true is run as stand alone app.
static void prepareFilesMenu( wxMenu* aParentMenu, bool aIsOutsideProject );

// Build the inspect menu
static void prepareInspectMenu( wxMenu* aParentMenu );

// Build the tools menu
static void prepareToolsMenu( wxMenu* aParentMenu );

// Build the help menu
static void prepareHelpMenu( wxMenu* aParentMenu );

// Build the edit menu
static void prepareEditMenu( wxMenu* aParentMenu );

// Build the view menu
static void prepareViewMenu( CONDITIONAL_MENU* aParentMenu, EE_SELECTION_TOOL* selTool );

// Build the preferences menu
static void preparePreferencesMenu( SCH_EDIT_FRAME* aFrame, wxMenu* aParentMenu );


void SCH_EDIT_FRAME::ReCreateMenuBar()
{
    EE_SELECTION_TOOL* selTool = m_toolManager->GetTool<EE_SELECTION_TOOL>();
    // wxWidgets handles the Mac Application menu behind the scenes, but that means
    // we always have to start from scratch with a new wxMenuBar.
    wxMenuBar* oldMenuBar = GetMenuBar();
    wxMenuBar* menuBar = new wxMenuBar();
    wxString   text;

    // Recreate all menus:

    // Menu File:
    wxMenu* fileMenu = new wxMenu;
    prepareFilesMenu( fileMenu, Kiface().IsSingle() );

    // Menu Edit:
    wxMenu* editMenu = new wxMenu;
    prepareEditMenu( editMenu );

    // Menu View:
    CONDITIONAL_MENU* viewMenu = new CONDITIONAL_MENU( false, selTool );
    prepareViewMenu( viewMenu, selTool );

    // Menu place:
    CONDITIONAL_MENU* placeMenu = new CONDITIONAL_MENU( false, selTool );
    preparePlaceMenu( placeMenu, selTool );

    // Menu Inspect:
    wxMenu* inspectMenu = new wxMenu;
    prepareInspectMenu( inspectMenu );

    // Menu Tools:
    wxMenu* toolsMenu = new wxMenu;
    prepareToolsMenu( toolsMenu );

    // Menu Preferences:
    wxMenu* preferencesMenu = new wxMenu;
    preparePreferencesMenu( this, preferencesMenu );

    // Help Menu:
    wxMenu* helpMenu = new wxMenu;
    prepareHelpMenu( helpMenu );

    // Create the menubar and append all submenus
    menuBar->Append( fileMenu, _( "&File" ) );
    menuBar->Append( editMenu, _( "&Edit" ) );
    menuBar->Append( viewMenu, _( "&View" ) );
    menuBar->Append( placeMenu, _( "&Place" ) );
    menuBar->Append( inspectMenu, _( "&Inspect" ) );
    menuBar->Append( toolsMenu, _( "&Tools" ) );
    menuBar->Append( preferencesMenu, _( "P&references" ) );
    menuBar->Append( helpMenu, _( "&Help" ) );

    SetMenuBar( menuBar );
    delete oldMenuBar;
}


void prepareViewMenu( CONDITIONAL_MENU* aParentMenu, EE_SELECTION_TOOL* selTool )
{
    SCH_EDIT_FRAME* frame = static_cast<SCH_EDIT_FRAME*>( selTool->GetManager()->GetEditFrame() );

    auto belowRootSheetCondition = [] ( const SELECTION& aSel ) {
        return g_CurrentSheet->Last() != g_RootSheet;
    };

    auto gridShownCondition = [ frame ] ( const SELECTION& aSel ) {
        return frame->IsGridVisible();
    };

    auto imperialUnitsCondition = [ frame ] ( const SELECTION& aSel ) {
        return frame->GetUserUnits() == INCHES;
    };

    auto metricUnitsCondition = [ frame ] ( const SELECTION& aSel ) {
        return frame->GetUserUnits() == MILLIMETRES;
    };

    auto fullCrosshairCondition = [ frame ] ( const SELECTION& aSel ) {
        return frame->GetGalDisplayOptions().m_fullscreenCursor;
    };

    auto hiddenPinsCondition = [ frame ] ( const SELECTION& aSel ) {
        return frame->GetShowAllPins();
    };

    aParentMenu->AddItem( EE_ACTIONS::showLibraryBrowser, EE_CONDITIONS::ShowAlways );
    aParentMenu->AddItem( EE_ACTIONS::navigateHierarchy,  EE_CONDITIONS::ShowAlways );
    aParentMenu->AddItem( EE_ACTIONS::leaveSheet,         belowRootSheetCondition );

    aParentMenu->AddSeparator();
    aParentMenu->AddItem( ACTIONS::zoomInCenter,    EE_CONDITIONS::ShowAlways );
    aParentMenu->AddItem( ACTIONS::zoomOutCenter,   EE_CONDITIONS::ShowAlways );
    aParentMenu->AddItem( ACTIONS::zoomFitScreen,   EE_CONDITIONS::ShowAlways );
    aParentMenu->AddItem( ACTIONS::zoomTool,        EE_CONDITIONS::ShowAlways );
    aParentMenu->AddItem( ACTIONS::zoomRedraw,      EE_CONDITIONS::ShowAlways );

    aParentMenu->AddSeparator();
    aParentMenu->AddCheckItem( ACTIONS::toggleGrid, gridShownCondition );
    aParentMenu->AddItem( ACTIONS::gridProperties,  EE_CONDITIONS::ShowAlways );

    // Units submenu
    CONDITIONAL_MENU* unitsSubMenu = new CONDITIONAL_MENU( false, selTool );
    unitsSubMenu->SetTitle( _( "&Units" ) );
    unitsSubMenu->AddCheckItem( ACTIONS::imperialUnits,      imperialUnitsCondition );
    unitsSubMenu->AddCheckItem( ACTIONS::metricUnits,        metricUnitsCondition );
    aParentMenu->AddMenu( unitsSubMenu );

    aParentMenu->AddCheckItem( ACTIONS::toggleCursorStyle,   fullCrosshairCondition );

    aParentMenu->AddSeparator();
    aParentMenu->AddCheckItem( EE_ACTIONS::toggleHiddenPins, hiddenPinsCondition );

#ifdef __APPLE__
    aParentMenu->AppendSeparator();
#endif
}


void preparePlaceMenu( CONDITIONAL_MENU* aParentMenu, EE_SELECTION_TOOL* selTool )
{
    aParentMenu->AddItem( EE_ACTIONS::placeSymbol,            EE_CONDITIONS::ShowAlways );
    aParentMenu->AddItem( EE_ACTIONS::placePower,             EE_CONDITIONS::ShowAlways );
    aParentMenu->AddItem( EE_ACTIONS::drawWire,               EE_CONDITIONS::ShowAlways );
    aParentMenu->AddItem( EE_ACTIONS::drawBus,                EE_CONDITIONS::ShowAlways );
    aParentMenu->AddItem( EE_ACTIONS::placeBusWireEntry,      EE_CONDITIONS::ShowAlways );
    aParentMenu->AddItem( EE_ACTIONS::placeBusBusEntry,       EE_CONDITIONS::ShowAlways );
    aParentMenu->AddItem( EE_ACTIONS::placeNoConnect,         EE_CONDITIONS::ShowAlways );
    aParentMenu->AddItem( EE_ACTIONS::placeJunction,          EE_CONDITIONS::ShowAlways );
    aParentMenu->AddItem( EE_ACTIONS::placeLabel,             EE_CONDITIONS::ShowAlways );
    aParentMenu->AddItem( EE_ACTIONS::placeGlobalLabel,       EE_CONDITIONS::ShowAlways );

    aParentMenu->AddSeparator();
    aParentMenu->AddItem( EE_ACTIONS::placeHierarchicalLabel, EE_CONDITIONS::ShowAlways );
    aParentMenu->AddItem( EE_ACTIONS::drawSheet,              EE_CONDITIONS::ShowAlways );
    aParentMenu->AddItem( EE_ACTIONS::importSheetPin,         EE_CONDITIONS::ShowAlways );
    aParentMenu->AddItem( EE_ACTIONS::placeSheetPin,          EE_CONDITIONS::ShowAlways );

    aParentMenu->AddSeparator();
    aParentMenu->AddItem( EE_ACTIONS::drawLines,              EE_CONDITIONS::ShowAlways );
    aParentMenu->AddItem( EE_ACTIONS::placeSchematicText,     EE_CONDITIONS::ShowAlways );
    aParentMenu->AddItem( EE_ACTIONS::placeImage,             EE_CONDITIONS::ShowAlways );
}


void prepareFilesMenu( wxMenu* aParentMenu, bool aIsOutsideProject )
{
    wxString text;

    // @todo: static probably not OK in multiple open projects.
    // Open Recent submenu
    static wxMenu* openRecentMenu;

    // Add this menu to list menu managed by m_fileHistory
    // (the file history will be updated when adding/removing files in history
    if( openRecentMenu )
        Kiface().GetFileHistory().RemoveMenu( openRecentMenu );

    openRecentMenu = new wxMenu();

    Kiface().GetFileHistory().UseMenu( openRecentMenu );
    Kiface().GetFileHistory().AddFilesToMenu( openRecentMenu );

    if( aIsOutsideProject )   // not when under a project mgr
    {
        text = AddHotkeyName( _( "&New..." ), g_Schematic_Hotkeys_Descr, HK_NEW );
        AddMenuItem( aParentMenu, ID_NEW_PROJECT, text,
                     _( "Start new schematic root sheet" ),
                     KiBitmap( new_document_xpm ) );

        text = AddHotkeyName( _( "&Open..." ), g_Schematic_Hotkeys_Descr, HK_OPEN );
        AddMenuItem( aParentMenu, ID_LOAD_PROJECT, text,
                     _( "Open existing schematic" ),
                     KiBitmap( open_document_xpm ) );

        AddMenuItem( aParentMenu, openRecentMenu, -1, _( "Open &Recent" ),
                     _( "Open recently opened schematic" ),
                     KiBitmap( recent_xpm ) );

        aParentMenu->AppendSeparator();
    }

    text = AddHotkeyName( _( "&Save" ), g_Schematic_Hotkeys_Descr, HK_SAVE );
    AddMenuItem( aParentMenu, ID_SAVE_PROJECT, text,
                 _( "Save changes" ),
                 KiBitmap( save_xpm ) );

    AddMenuItem( aParentMenu, ID_UPDATE_ONE_SHEET, _( "Save &Current Sheet" ),
                 _( "Save only the current sheet" ),
                 KiBitmap( save_xpm ) );

    text = AddHotkeyName( _( "Save C&urrent Sheet As..." ), g_Schematic_Hotkeys_Descr, HK_SAVEAS );
    AddMenuItem( aParentMenu, ID_SAVE_ONE_SHEET_UNDER_NEW_NAME, text,
                 _( "Save a copy of the current sheet" ),
                 KiBitmap( save_as_xpm ) );

    aParentMenu->AppendSeparator();

    AddMenuItem( aParentMenu, ID_APPEND_PROJECT, _( "App&end Schematic Sheet Content..." ),
                 _( "Append schematic sheet content from another project to the current sheet" ),
                 KiBitmap( add_document_xpm ) );

    AddMenuItem( aParentMenu, ID_IMPORT_NON_KICAD_SCH, _( "&Import Non KiCad Schematic..." ),
                 _( "Replace current schematic sheet with one imported from another application" ),
                 KiBitmap( import_document_xpm ) );   // TODO needs a different icon

    aParentMenu->AppendSeparator();

    // Import submenu
    wxMenu* submenuImport = new wxMenu();

    AddMenuItem( submenuImport, ID_BACKANNO_ITEMS, _( "&Footprint Association File..." ),
                 HELP_IMPORT_FOOTPRINTS,
                 KiBitmap( import_footprint_names_xpm ) );

    AddMenuItem( aParentMenu, submenuImport, ID_GEN_IMPORT_FILE, _( "&Import" ),
                 _( "Import files" ),
                 KiBitmap( import_xpm ) );


    // Export submenu
    wxMenu* submenuExport = new wxMenu();

    AddMenuItem( submenuExport, ID_GEN_COPY_SHEET_TO_CLIPBOARD, _( "Drawing to C&lipboard" ),
                 _( "Export drawings to clipboard" ),
                 KiBitmap( copy_xpm ) );

    AddMenuItem( submenuExport, ID_GET_NETLIST, _( "&Netlist..." ),
                 _( "Export netlist file" ),
                 KiBitmap( netlist_xpm ) );

    AddMenuItem( aParentMenu, submenuExport, ID_GEN_EXPORT_FILE, _( "E&xport" ),
                 _( "Export files" ),
                 KiBitmap( export_xpm ) );

    aParentMenu->AppendSeparator();

    // Edit page layout:
    AddMenuItem( aParentMenu, ID_SHEET_SET, _( "Page S&ettings..." ),
                 _( "Settings for sheet size and frame references" ),
                 KiBitmap( sheetset_xpm ) );

    text = AddHotkeyName( _( "&Print..." ), g_Schematic_Hotkeys_Descr, HK_PRINT );
    AddMenuItem( aParentMenu, wxID_PRINT, text,
                 _( "Print schematic sheet" ),
                 KiBitmap( print_button_xpm ) );

    AddMenuItem( aParentMenu, ID_GEN_PLOT_SCHEMATIC, _( "P&lot..." ),
                 _( "Plot schematic sheet in PostScript, PDF, SVG, DXF or HPGL format" ),
                 KiBitmap( plot_xpm ) );

    aParentMenu->AppendSeparator();

    // Quit
    AddMenuItem( aParentMenu, wxID_EXIT, _( "&Exit" ),
                 _( "Close Eeschema" ),
                 KiBitmap( exit_xpm ) );
}


void prepareEditMenu( wxMenu* aParentMenu )
{
    wxString text;

    // Undo
    text = AddHotkeyName( _( "&Undo" ), g_Schematic_Hotkeys_Descr, HK_UNDO );

    AddMenuItem( aParentMenu, wxID_UNDO, text, HELP_UNDO, KiBitmap( undo_xpm ) );

    // Redo
    text = AddHotkeyName( _( "&Redo" ), g_Schematic_Hotkeys_Descr, HK_REDO );

    AddMenuItem( aParentMenu, wxID_REDO, text, HELP_REDO, KiBitmap( redo_xpm ) );

    aParentMenu->AppendSeparator();

    text = AddHotkeyName( _( "&Cut" ), g_Schematic_Hotkeys_Descr, HK_EDIT_CUT );
    AddMenuItem( aParentMenu, wxID_CUT, text,
                 _( "Cuts the selected item(s) to the Clipboard" ),
                 KiBitmap( cut_xpm ) );

    text = AddHotkeyName( _( "&Copy" ), g_Schematic_Hotkeys_Descr, HK_EDIT_COPY );
    AddMenuItem( aParentMenu, wxID_COPY, text,
                 _( "Copies the selected item(s) to the Clipboard" ),
                 KiBitmap( copy_xpm ) );

    text = AddHotkeyName( _( "&Paste" ), g_Schematic_Hotkeys_Descr, HK_EDIT_PASTE );
    AddMenuItem( aParentMenu, wxID_PASTE, text,
                 _( "Pastes item(s) from the Clipboard" ),
                 KiBitmap( paste_xpm ) );

    // Delete
    aParentMenu->AppendSeparator();
    AddMenuItem( aParentMenu, ID_MENU_DELETE_ITEM_BUTT,
                 _( "&Delete" ), HELP_DELETE_ITEMS,
                 KiBitmap( delete_xpm ) );

    // Find
    aParentMenu->AppendSeparator();
    text = AddHotkeyName( _( "&Find..." ), g_Schematic_Hotkeys_Descr, HK_FIND_ITEM );
    AddMenuItem( aParentMenu, ID_FIND_ITEMS, text, HELP_FIND, KiBitmap( find_xpm ) );

    // Find/Replace
    text = AddHotkeyName( _( "Find and Re&place..." ), g_Schematic_Hotkeys_Descr,
                          HK_FIND_REPLACE );
    AddMenuItem( aParentMenu, wxID_REPLACE, text, HELP_REPLACE,
                 KiBitmap( find_replace_xpm ) );

    aParentMenu->AppendSeparator();

    // Update field values
    AddMenuItem( aParentMenu, ID_UPDATE_FIELDS,
                 _( "Update Fields from Library..." ),
                 _( "Sets symbol fields to original library values" ),
                 KiBitmap( update_fields_xpm ) );
}


void prepareInspectMenu( wxMenu* aParentMenu )
{
    AddMenuItem( aParentMenu, ID_GET_ERC,
                 _( "Electrical Rules &Checker" ),
                 _( "Perform electrical rules check" ),
                 KiBitmap( erc_xpm ) );
}


void prepareToolsMenu( wxMenu* aParentMenu )
{
    wxString text;

    text = AddHotkeyName( _( "Update PCB from Schematic..." ), g_Schematic_Hotkeys_Descr,
                          HK_UPDATE_PCB_FROM_SCH );

    AddMenuItem( aParentMenu,
                 ID_UPDATE_PCB_FROM_SCH,
                 text, _( "Update PCB design with current schematic." ),
                 KiBitmap( update_pcb_from_sch_xpm ) );

    // Run Pcbnew
    AddMenuItem( aParentMenu, ID_RUN_PCB, _( "&Open PCB Editor" ),
                 _( "Run Pcbnew" ),
                 KiBitmap( pcbnew_xpm ) );

    aParentMenu->AppendSeparator();

    AddMenuItem( aParentMenu, ID_RUN_LIBRARY, _( "Symbol Library &Editor" ),
                 HELP_RUN_LIB_EDITOR,
                 KiBitmap( libedit_xpm ) );

    AddMenuItem( aParentMenu, ID_RESCUE_CACHED, _( "&Rescue Symbols..." ),
                 _( "Find old symbols in project and rename/rescue them" ),
                 KiBitmap( rescue_xpm ) );

    AddMenuItem( aParentMenu, ID_REMAP_SYMBOLS, _( "Remap S&ymbols..." ),
                 _( "Remap legacy library symbols to symbol library table" ),
                 KiBitmap( rescue_xpm ) );

    aParentMenu->AppendSeparator();

    AddMenuItem( aParentMenu, ID_OPEN_CMP_TABLE, _( "Edit Symbol Field&s..." ),
                 KiBitmap( spreadsheet_xpm ) );

    AddMenuItem( aParentMenu, ID_EDIT_COMPONENTS_TO_SYMBOLS_LIB_ID,
                 _( "Edit Symbol &Library References..." ),
                 _( "Edit links between schematic symbols and library symbols" ),
                 KiBitmap( edit_cmp_symb_links_xpm ) );

    aParentMenu->AppendSeparator();

    AddMenuItem( aParentMenu, ID_GET_ANNOTATE, _( "&Annotate Schematic..." ),
                 HELP_ANNOTATE,
                 KiBitmap( annotate_xpm ) );

    AddMenuItem( aParentMenu, ID_BUS_MANAGER, _( "Bus &Definitions..." ),
                 HELP_BUS_MANAGER,
                 KiBitmap( bus_definition_tool_xpm ) );

    aParentMenu->AppendSeparator();

    // Run CvPcb
    AddMenuItem( aParentMenu, ID_RUN_CVPCB, _( "A&ssign Footprints..." ),
                 _( "Assign PCB footprints to schematic symbols" ),
                 KiBitmap( cvpcb_xpm ) );

    AddMenuItem( aParentMenu, ID_GET_TOOLS, _( "Generate Bill of &Materials..." ),
                 HELP_GENERATE_BOM,
                 KiBitmap( bom_xpm ) );

    aParentMenu->AppendSeparator();

#ifdef KICAD_SPICE
    // Simulator
    AddMenuItem( aParentMenu, ID_SIM_SHOW, _("Simula&tor"),
                 _( "Simulate circuit" ),
                 KiBitmap( simulator_xpm ) );
#endif /* KICAD_SPICE */

}


void prepareHelpMenu( wxMenu* aParentMenu )
{
    AddMenuItem( aParentMenu, wxID_HELP, _( "Eeschema &Manual" ),
                 _( "Open Eeschema Manual" ),
                 KiBitmap( online_help_xpm ) );

    AddMenuItem( aParentMenu, wxID_INDEX, _( "&Getting Started in KiCad" ),
                 _( "Open \"Getting Started in KiCad\" guide for beginners" ),
                 KiBitmap( help_xpm ) );

    wxString text = AddHotkeyName( _( "&List Hotkeys..." ), g_Eeschema_Hotkeys_Descr, HK_HELP );
    AddMenuItem( aParentMenu, ID_PREFERENCES_HOTKEY_SHOW_CURRENT_LIST, text,
                 _( "Displays current hotkeys table and corresponding commands" ),
                 KiBitmap( hotkeys_xpm ) );

    aParentMenu->AppendSeparator();
    AddMenuItem( aParentMenu, ID_HELP_GET_INVOLVED, _( "Get &Involved" ),
                 _( "Contribute to KiCad (opens a web browser)" ),
                 KiBitmap( info_xpm ) );

    aParentMenu->AppendSeparator();
    AddMenuItem( aParentMenu, wxID_ABOUT, _( "&About KiCad" ), KiBitmap( about_xpm ) );
}


static void preparePreferencesMenu( SCH_EDIT_FRAME* aFrame, wxMenu* aParentMenu )
{
    // Path configuration edit dialog.
    AddMenuItem( aParentMenu, ID_PREFERENCES_CONFIGURE_PATHS, _( "Configure Pa&ths..." ),
                 _( "Edit path configuration environment variables" ),
                 KiBitmap( path_xpm ) );

    // Library
    AddMenuItem( aParentMenu, ID_EDIT_SYM_LIB_TABLE, _( "Manage Symbol Libraries..." ),
                 _( "Edit the global and project symbol library lists" ),
                 KiBitmap( library_table_xpm ) );

    // Options (Preferences on WXMAC)
    wxString text = AddHotkeyName( _( "&Preferences..." ), g_Eeschema_Hotkeys_Descr, HK_PREFERENCES );
    AddMenuItem( aParentMenu, wxID_PREFERENCES, text,
                 _( "Show preferences for all open tools" ),
                 KiBitmap( preference_xpm ) );

    aParentMenu->AppendSeparator();

    // Language submenu
    Pgm().AddMenuLanguageList( aParentMenu );

    aParentMenu->AppendSeparator();

    text = AddHotkeyName( _( "Modern Toolset (&Accelerated)" ), g_Eeschema_Hotkeys_Descr,
                          HK_CANVAS_OPENGL );
    AddMenuItem( aParentMenu, ID_MENU_CANVAS_OPENGL, text,
                 _( "Use Modern Toolset with hardware-accelerated graphics (recommended)" ),
                 KiBitmap( tools_xpm ), wxITEM_RADIO );

    text = AddHotkeyName( _( "Modern Toolset (Fallba&ck)" ), g_Eeschema_Hotkeys_Descr,
                          HK_CANVAS_CAIRO );
    AddMenuItem( aParentMenu, ID_MENU_CANVAS_CAIRO, text,
                 _( "Use Modern Toolset with software graphics (fall-back)" ),
                 KiBitmap( tools_xpm ), wxITEM_RADIO );

    aParentMenu->AppendSeparator();

    // Import/export
    AddMenuItem( aParentMenu, ID_CONFIG_SAVE, _( "&Save Project File..." ),
                 _( "Save project preferences into a project file" ),
                 KiBitmap( save_setup_xpm ) );

    AddMenuItem( aParentMenu, ID_CONFIG_READ, _( "Load P&roject File..." ),
                 _( "Load project preferences from a project file" ),
                 KiBitmap( import_setup_xpm ) );
}
