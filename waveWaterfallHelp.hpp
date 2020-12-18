
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



QString prefsText = 
  waveWaterfall::tr ("<img source=\":/icons/prefs.png\"> Click this button to change program preferences.  This includes "
                     "position format and the colors.");
QString modeText = 
  waveWaterfall::tr ("<img source=\":/icons/mode_line.png\"> <img source=\":/icons/mode_dot.png\"> Click this button to toggle between line and dot drawing "
                     "modes for the wave display.  When selected the waves are drawn as lines, when unselected the waves are drawn "
                     "as unconnected dots.");

QString quitText = 
  waveWaterfall::tr ("<img source=\":/icons/quit.png\"> Click this button to <b><em>exit</em></b> from the program.  "
                     "You can also use the <b>Quit</b> entry in the <b>File</b> menu.");
QString mapText = 
  waveWaterfall::tr ("This is the waveWaterfall program, a companion to the pfmEdit program for viewing Charts "
                     "LIDAR HOF waveforms as a waterfall display.  Help is available on all fields in waveWaterfall "
                     "using the What's This pointer.");

QString bGrpText = 
  waveWaterfall::tr ("Select the format in which you want all geographic positions to be displayed.");

QString closePrefsText = 
  waveWaterfall::tr ("Click this button to close the preferences dialog.");

QString restoreDefaultsText = 
  waveWaterfall::tr ("Click this button to restore colors, size, and position format to the default settings.");

QString lineLabelText = 
  waveWaterfall::tr ("This is the name of the input line from which this waveform was taken.");

QString distLabelText = 
  waveWaterfall::tr ("This is the distance between the selected (highlighted) line and the neighboring line.");

QString dateLabelText = 
  waveWaterfall::tr ("This is the date and time of the waveform.  The fields are, in order, year, month, day, "
                     "(julian day), hour, minutes, and second.");
