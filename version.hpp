
/*********************************************************************************************

    This is public domain software that was developed by or for the U.S. Naval Oceanographic
    Office and/or the U.S. Army Corps of Engineers.

    This is a work of the U.S. Government. In accordance with 17 USC 105, copyright protection
    is not available for any work of the U.S. Government.

    Neither the United States Government, nor any employees of the United States Government,
    nor the author, makes any warranty, express or implied, without even the implied warranty
    of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE, or assumes any liability or
    responsibility for the accuracy, completeness, or usefulness of any information,
    apparatus, product, or process disclosed, or represents that its use would not infringe
    privately-owned rights. Reference herein to any specific commercial products, process,
    or service by trade name, trademark, manufacturer, or otherwise, does not necessarily
    constitute or imply its endorsement, recommendation, or favoring by the United States
    Government. The views and opinions of authors expressed herein do not necessarily state
    or reflect those of the United States Government, and shall not be used for advertising
    or product endorsement purposes.
*********************************************************************************************/


/****************************************  IMPORTANT NOTE  **********************************

    Comments in this file that start with / * ! or / / ! are being used by Doxygen to
    document the software.  Dashes in these comment blocks are used to create bullet lists.
    The lack of blank lines after a block of dash preceeded comments means that the next
    block of dash preceeded comments is a new, indented bullet list.  I've tried to keep the
    Doxygen formatting to a minimum but there are some other items (like <br> and <pre>)
    that need to be left alone.  If you see a comment that starts with / * ! or / / ! and
    there is something that looks a bit weird it is probably due to some arcane Doxygen
    syntax.  Be very careful modifying blocks of Doxygen comments.

*****************************************  IMPORTANT NOTE  **********************************/




#ifndef VERSION

#define     VERSION     "PFM Software - waveWaterfall V1.47 - 10/23/17"

#endif

/*

    Version 1.0
    Jan C. Depner
    11/23/07

    First working version.


    Version 1.01
    Jan C. Depner
    01/04/08

    Now uses the parent process ID of the bin viewer plus _pfm or _abe for the shared memory ID's.  This removes the single instance
    per user restriction from ABE.


    Version 1.02
    Jan C. Depner
    04/01/08

    Added acknowledgments to the Help pulldown menu.


    Version 1.03
    Jan C. Depner
    04/07/08

    Replaced single .h and .hpp files from utility library with include of nvutility.h and nvutility.hpp


    Version 1.04
    Jan C. Depner
    07/15/08

    Removed pfmShare shared memory usage and replaced with abeShare.


    Version 1.05
    Jan C. Depner
    11/03/08

    Converted to use new mwShare.multi... type records in the ABE_SHARE structure.


    Version 1.06
    Jan C. Depner
    11/16/08

    Lock trackCursor during snapshot of waveforms.


    Version 1.07
    Jan C. Depner
    11/28/08

    Added kill_switch option.


    Version 1.20
    Jan C. Depner
    03/13/09

    Added ability to handle WLF data.  Friday the 13th - Oh No!


    Version 1.21
    Jan C. Depner
    03/25/09

    Waveforms from the same line as the first are colored the same as the first line.


    Version 1.22
    Jan C. Depner
    04/08/09

    Changes to deal with "flush" argument on all nvmapp.cpp (nvutility library) drawing functions.


    Version 1.23
    Jan C. Depner
    04/13/09

    Use NINT instead of typecasting to NV_INT32 when saving Qt window state.  Integer truncation was inconsistent on Windows.


    Version 1.24
    Jan C. Depner
    04/23/09

    Changed the acknowledgments help to include Qt and a couple of others.


    Version 1.25
    Jan C. Depner
    04/27/09

    Replaced QColorDialog::getRgba with QColorDialog::getColor.


    Version 1.26
    Jan C. Depner
    06/15/09

    Added support for PFM_CHARTS_HOF_DATA.


    Version 1.27
    Jan C. Depner
    08/06/09

    Added waveform selected point mark for WLF data.


    Version 1.28
    Jan C. Depner
    09/10/09

    Made window icon dependent on data type (APD or PMT).


    Version 1.29
    Jan C. Depner
    09/11/09

    Fixed getColor calls so that cancel works.  Never forget!


    Version 1.30
    Jan C. Depner
    09/16/09

    Set killed flag in abe_share when program exits other than from kill switch from parent.


    Version 1.31
    Jan C. Depner
    11/17/09

    Color coded the status bar backgrounds to match the waveforms.


    Version 1.32
    Jan C. Depner
    08/30/10

    Fixed APD and PMT location settings problem.


    Version 1.33
    Jan C. Depner
    01/06/11

    Correct problem with the way I was instantiating the main widget.  This caused an intermittent error on Windows7/XP.


    Version 1.34
    Jan C. Depner
    11/30/11

    Converted .xpm icons to .png icons.


    Version 1.35
    Jan C. Depner
    07/17/12

    Added support for preliminary CZMIL data formats.  Changed APD and PMT tags to shallow and deep respectively.


    Version 1.36
    Jan C. Depner
    09/11/12

    - Check for change of subrecord with CZMIL data since each subrecord has a unique waveform.
    - Never forget!


    Version 1.37
    Jan C. Depner
    10/23/12

    - Now uses interest_point in CZMIL CPF record to place X on waveform.
    - Only 8 shopping days left to retirement!


    Version 1.38
    Jan C. Depner (PFM Software)
    12/09/13

    Switched to using .ini file in $HOME (Linux) or $USERPROFILE (Windows) in the ABE.config directory.  Now
    the applications qsettings will not end up in unknown places like ~/.config/navo.navy.mil/blah_blah_blah on
    Linux or, in the registry (shudder) on Windows.


    Version 1.39
    Jan C. Depner (PFM Software)
    03/17/14

    Removed WLF support.  Top o' the mornin' to ye!


    Version 1.40
    Jan C. Depner (PFM Software)
    03/19/14

    - Straightened up the Open Source acknowledgments.


    Version 1.41
    Jan C. Depner (PFM Software)
    05/27/14

    - Added the new LGPL licensed GSF library to the acknowledgments.


    Version 1.42
    Jan C. Depner (PFM Software)
    07/01/14

    - Replaced all of the old, borrowed icons with new, public domain icons.  Mostly from the Tango set
      but a few from flavour-extended and 32pxmania.


    Version 1.43
    Jan C. Depner (PFM Software)
    07/23/14

    - Switched from using the old NV_INT64 and NV_U_INT32 type definitions to the C99 standard stdint.h and
      inttypes.h sized data types (e.g. int64_t and uint32_t).


    Version 1.44
    Jan C. Depner (PFM Software)
    02/16/15

    - To give better feedback to shelling programs in the case of errors I've added the program name to all
      output to stderr.


    Version 1.45
    Jan C. Depner (PFM Software)
    08/27/16

    - Now uses the same font as all other ABE GUI apps.  Font can only be changed in pfmView Preferences.


    Version 1.46
    Jan C. Depner (PFM Software)
    05/11/17

    - Fixed bug in Windows caused by not reading the HOF header prior to reading the record.


    Version 1.47
    Jan C. Depner (PFM Software)
    10/20/17

    - A bunch of changes to support doing translations in the future.  There is a generic
      waveWaterfall_xx.ts file that can be run through Qt's "linguist" to translate to another language.

</pre>*/
