#
# This program source code file is part of KiCad, a free EDA CAD application.
#
# Copyright (C) 2017 CERN
# @author Alejandro García Montoro <alejandro.garciamontoro@gmail.com>
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, you may find one here:
# http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
# or you may search the http://www.gnu.org website for the version 2 license,
# or you may write to the Free Software Foundation, Inc.,
# 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA


include_directories( BEFORE ${INC_BEFORE} )
include_directories( AFTER ${INC_AFTER} )


add_executable( qa_eeschema
    # A single top to load the pcnew kiface
    # ../../common/single_top.cpp

    # stuff from common due to...units?
    ../../common/base_units.cpp
    ../../common/eda_text.cpp

    # stuff from common which is needed...why?
    ../../common/colors.cpp
    ../../common/observable.cpp

    eeschema_test_utils.cpp

    # The main test entry points
    test_module.cpp

    test_eagle_plugin.cpp
)

target_link_libraries( qa_eeschema
    eeschema_kiface
    common
    gal
    qa_utils
    unit_test_utils
    ${GDI_PLUS_LIBRARIES}
    ${Boost_LIBRARIES}
)

# Eeschema tests, so pretend to be eeschema (for units, etc)
target_compile_definitions( qa_eeschema
    PUBLIC EESCHEMA
)

# Pass in the default data location
set_source_files_properties( eeschema_test_utils.cpp PROPERTIES
    COMPILE_DEFINITIONS "QA_EESCHEMA_DATA_LOCATION=(\"${CMAKE_CURRENT_SOURCE_DIR}/data\")"
)

kicad_add_boost_test( qa_eeschema eeschema )