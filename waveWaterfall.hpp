
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



/*  waveWaterfall class definitions.  */

#ifndef WAVEWATERFALL_H
#define WAVEWATERFALL_H

#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <getopt.h>
#include <cmath>

#include "nvutility.h"
#include "nvutility.hpp"

#include "pfm.h"

#include "FileHydroOutput.h"
#include "FileWave.h"

#include "czmil.h"

#include "version.hpp"


#include <QtCore>
#include <QtGui>
#if QT_VERSION >= 0x050000
#include <QtWidgets>
#endif


//  Savitzky-Golay

#define  NMAX  2048    /*  Maximum number of input data ordinates  */
#define  NP      50    /*  Maximum number of filter coefficients  */



#define WAVE_X_SIZE       294
#define WAVE_Y_SIZE       769
#define SPOT_SIZE         2
#define WAVE_OFFSET       20
#define WAVE_ALPHA        64


typedef struct
{
  int32_t             min_x;
  int32_t             max_x;
  int32_t             min_y;
  int32_t             max_y;
  int32_t             range_x;
  int32_t             range_y;
  int32_t             length;
} BOUNDS;


typedef struct
{
  int32_t        length;
  int32_t        rec_type;
  uint8_t        lidar;
  int32_t        num_packets;      //  CZMIL only
  uint8_t        channel_ndx;      //  CZMIL only
  int32_t        cx;               //  CZMIL only
  int32_t        cy;               //  CZMIL only
  int16_t        data[1024];       //  overkill
} WAVE;


class waveWaterfall:public QMainWindow
{
  Q_OBJECT 


public:

  waveWaterfall (int32_t *argc = 0, char **argv = 0, QWidget *parent = 0);
  ~waveWaterfall ();


protected:

  int32_t         key, pfm_handle[MAX_ABE_PFMS], num_pfms;

  uint32_t        kill_switch;

  char            progname[256];

  QSharedMemory   *abeShare;

  ABE_SHARE       *abe_share, l_share;

  PFM_OPEN_ARGS   open_args[MAX_ABE_PFMS];

  BOUNDS          wave_bounds[MAX_STACK_POINTS];
  int32_t         wave_len[MAX_STACK_POINTS], width, height, apd_width, apd_height, apd_window_x, apd_window_y, pmt_width, pmt_height,
                  pmt_window_x, pmt_window_y, adjusted_width;

  char            path[MAX_STACK_POINTS][512], wave_path[MAX_STACK_POINTS][512], line_name[MAX_STACK_POINTS][128], filename[MAX_STACK_POINTS][512];
  int32_t         recnum[MAX_STACK_POINTS];
  HOF_HEADER_T    hof_header;
  HYDRO_OUTPUT_T  hof_record[MAX_STACK_POINTS];
  CZMIL_CPF_Data  cpf_record[MAX_STACK_POINTS];
  CZMIL_CWF_Data  cwf_record[MAX_STACK_POINTS];

  uint8_t         secondary[MAX_STACK_POINTS], wave_line_mode;

  int32_t         wave_type;
  WAVE_DATA_T     wave_data;
  WAVE            wave[MAX_STACK_POINTS];

  QMessageBox     *filError;

  QCheckBox       *sMessage;

  QStatusBar      *statusBar[MAX_STACK_POINTS];

  QLabel          *dateLabel[MAX_STACK_POINTS], *lineLabel[MAX_STACK_POINTS], *distLabel[MAX_STACK_POINTS];

  QColor          waveColor[MAX_STACK_POINTS], primaryColor, secondaryColor, backgroundColor, transWaveColor, transWave2Color, 
                  transPrimaryColor, transSecondaryColor;

  QPalette        bPrimaryPalette, bSecondaryPalette, bBackgroundPalette, dateLabelPalette[MAX_STACK_POINTS], lineLabelPalette[MAX_STACK_POINTS],
                  distLabelPalette[MAX_STACK_POINTS];

  QPushButton     *bWave2Color, *bPrimaryColor, *bSecondaryColor, *bBackgroundColor, *bRestoreDefaults; 

  uint8_t         force_redraw, lock_track;

  nvMap           *map;

  NVMAP_DEF       mapdef;

  QButtonGroup    *bGrp;
  QDialog         *prefsD;
  QToolButton     *bQuit, *bPrefs, *bMode;

  QString         pos_format, timestamp, record, first, date_time[MAX_STACK_POINTS];

  QFont           font;                       //  Font used for all ABE GUI applications


  //  Savitzky-Golay

  float           signal[NMAX + 1], ysave[NMAX + 1];
  float           coeff[NP + 1];
  int32_t         ndex[NP + 1], nleft, nright, moment, ndata;


  void envin ();
  void envout ();

  void leftMouse (double x, double y);
  void midMouse (double x, double y);
  void rightMouse (double x, double y);
  void scaleWave (int32_t x, int32_t y, int32_t *new_x, int32_t *new_y, NVMAP_DEF l_mapdef, int32_t wave_num);
  void drawX (int32_t x, int32_t y, int32_t size, int32_t width, QColor color);
  void setFields ();


protected slots:

  void slotMousePress (QMouseEvent *e, double x, double y);
  void slotMouseMove (QMouseEvent *e, double x, double y);
  void slotResize (QResizeEvent *e);
  void closeEvent (QCloseEvent *event);
  void slotPlotWaves (NVMAP_DEF l_mapdef);

  void trackCursor ();

  void slotHelp ();

  void slotQuit ();

  void slotMode (bool state);

  void slotPrefs ();
  void slotPosClicked (int id);
  void slotClosePrefs ();

  void slotPrimaryColor ();
  void slotSecondaryColor ();
  void slotBackgroundColor ();
  void slotRestoreDefaults ();

  void about ();
  void slotAcknowledgments ();
  void aboutQt ();


 private:
};

#endif
