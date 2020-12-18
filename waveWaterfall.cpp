
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


#include "waveWaterfall.hpp"
#include "waveWaterfallHelp.hpp"
#include "acknowledgments.hpp"


double settings_version = 2.01;


/***************************************   IMPORTANT NOTE  ********************************** 

    For historical reasons (SHOALS and CHARTS) shallow and deep channels are referred to as 
    APD (avalanche photo diode) and PMT (photo multiplier tube).  If someone wants to get
    energetic and change all of this, be my guest.   JCD 07/16/12

*****************************************  IMPORTANT NOTE  **********************************/


waveWaterfall::waveWaterfall (int32_t *argc, char **argv, QWidget *parent):
  QMainWindow (parent, 0)
{
  extern char     *optarg;


  strcpy (progname, argv[0]);
  filError = NULL;
  lock_track = NVFalse;


  QResource::registerResource ("/icons.rcc");


  //  Read the acknowledgments file for the Acknowledgments Help button.  This way I only have
  //  to change one file and copy it to the other programs icon folders to change the Acknowledgments
  //  Help text instead of changing it in every single program I use it in.

  QFile *aDataFile = new QFile (":/icons/ACKNOWLEDGMENTS");

  if (aDataFile->open (QIODevice::ReadOnly))
    {
      char string[256];

      while (aDataFile->readLine (string, sizeof (string)) > 0)
        {
	  acknowledgmentsText.append (string);
        }
      aDataFile->close ();
    }


  //  Have to set the focus policy or keypress events don't work properly at first in Focus Follows Mouse mode

  setFocusPolicy (Qt::WheelFocus);


  wave_type = PMT;
  kill_switch = ANCILLARY_FORCE_EXIT;


  int32_t option_index = 0;
  while (NVTrue) 
    {
      static struct option long_options[] = {{"shared_memory_key", required_argument, 0, 0},
                                             {"kill_switch", required_argument, 0, 0},
                                             {0, no_argument, 0, 0}};

      char c = (char) getopt_long (*argc, argv, "ap", long_options, &option_index);
      if (c == -1) break;

      switch (c) 
        {
        case 0:

          switch (option_index)
            {
            case 0:
              sscanf (optarg, "%d", &key);
              break;

            case 1:
              sscanf (optarg, "%d", &kill_switch);
              break;
            }
          break;

        case 'a':
          wave_type = APD;
          break;

        case 'p':
          wave_type = PMT;
          break;
        }
    }


  if (!key)
    {
      fprintf (stderr, "%s %s %s %d - shared_memory_key argument not present on command line.  Terminating!\n", progname, __FILE__, __FUNCTION__, __LINE__);
      exit (-1);
    }


  //  This is the "tools" toolbar.  We have to do this here so that we can restore the toolbar location(s).

  QToolBar *tools = addToolBar (tr ("Tools"));
  tools->setObjectName (tr ("waveWaterfall main toolbar"));


  QString cap;
  if (wave_type == APD)
    {
      //  Set the main icon

      setWindowIcon (QIcon (":/icons/wave_waterfall_shallow.png"));


      cap = tr ("waveWaterfall (Shallow) :  %1").arg (VERSION);
    }
  else
    {
      //  Set the main icon

      setWindowIcon (QIcon (":/icons/wave_waterfall_deep.png"));


      cap = tr ("waveWaterfall (Deep) :  %1").arg (VERSION);
    }

  setWindowTitle (cap);


  /******************************************* IMPORTANT NOTE ABOUT SHARED MEMORY **************************************** \

      This is a little note about the use of shared memory within the Area-Based Editor (ABE) programs.  If you read
      the Qt documentation (or anyone else's documentation) about the use of shared memory they will say "Dear [insert
      name of omnipotent being of your choice here], whatever you do, always lock shared memory when you use it!".
      The reason they say this is that access to shared memory is not atomic.  That is, reading shared memory and then
      writing to it is not a single operation.  An example of why this might be important - two programs are running,
      the first checks a value in shared memory, sees that it is a zero.  The second program checks the same location
      and sees that it is a zero.  These two programs have different actions they must perform depending on the value
      of that particular location in shared memory.  Now the first program writes a one to that location which was
      supposed to tell the second program to do something but the second program thinks it's a zero.  The second program
      doesn't do what it's supposed to do and it writes a two to that location.  The two will tell the first program 
      to do something.  Obviously this could be a problem.  In real life, this almost never occurs.  Also, if you write
      your program properly you can make sure this doesn't happen.  In ABE we almost never lock shared memory because
      something much worse than two programs getting out of sync can occur.  If we start a program and it locks shared
      memory and then dies, all the other programs will be locked up.  When you look through the ABE code you'll see
      that we very rarely lock shared memory, and then only for very short periods of time.  This is by design.

  \******************************************* IMPORTANT NOTE ABOUT SHARED MEMORY ****************************************/


  //  Get the shared memory area.  If it doesn't exist, quit.  It should have already been created 
  //  by pfmView or fileEdit3D.

  QString skey;
  skey.sprintf ("%d_abe", key);

  abeShare = new QSharedMemory (skey);

  if (!abeShare->attach (QSharedMemory::ReadWrite))
    {
      fprintf (stderr, "%s %s %s %d - abeShare - %s\n", progname, __FILE__, __FUNCTION__, __LINE__, strerror (errno));
      exit (-1);
    }

  abe_share = (ABE_SHARE *) abeShare->data ();

  num_pfms = abe_share->pfm_count;

  for (int32_t pfm = 0 ; pfm < num_pfms ; pfm++)
    {
      open_args[pfm] = abe_share->open_args[pfm];

      if ((pfm_handle[pfm] = open_existing_pfm_file (&open_args[pfm])) < 0)
	{
	  QMessageBox::warning (this, tr ("Open PFM Structure"),
				tr ("The file %1 is not a PFM handle or list file or there was an error reading the file.\nThe error message returned was:\n\n%2").arg
                                (QDir::toNativeSeparators (QString (abe_share->open_args[pfm].list_path))).arg (pfm_error_str (pfm_error)));
	}
    }


  ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

  //  Just playing with the Savitzky-Golay filter in the following block.  I'm going to leave it here in case anyone wants to
  //  play around with it.  The overhead is pretty much non-existent.

  for (int32_t i = 1 ; i <= NMAX ; i++) signal[i] = ysave[i] = 0.0;
  nleft = 4;
  nright = 4;
  moment = 2;


  ndex[1] = 0;
  int32_t j = 3;
  for (int32_t i = 2 ; i <= nleft + 1 ; i++)
    {
      ndex[i] = i - j;
      j += 2;
    }

  j = 2;
  for (int32_t i = nleft + 2 ; i <= nleft + nright + 1 ; i++)
    {
      ndex[i] = i - j;
      j += 2;
    }


  //  Calculate Savitzky-Golay filter coefficients.

  savgol (coeff, nleft + nright + 1, nleft, nright, 0, moment);

  ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


  //  Get the user's defaults

  envin ();


  // Set the application font

  QApplication::setFont (font);


  //  Set the window size and location from the defaults

  if (wave_type == APD)
    {
      this->resize (apd_width, apd_height);
      this->move (apd_window_x, apd_window_y);
      width = apd_width;
      height = apd_height;
    }
  else
    {
      this->resize (pmt_width, pmt_height);
      this->move (pmt_window_x, pmt_window_y);
      width = pmt_width;
      height = pmt_height;
    }


  adjusted_width = width - MAX_STACK_POINTS * WAVE_OFFSET;


  //  Set the map values from the defaults

  mapdef.projection = NO_PROJECTION;
  mapdef.draw_width = width;
  mapdef.draw_height = height;
  mapdef.overlap_percent = 5;
  mapdef.grid_inc_x = 0.0;
  mapdef.grid_inc_y = 0.0;

  mapdef.coasts = NVFalse;
  mapdef.landmask = NVFalse;

  mapdef.border = 0;
  mapdef.coast_color = Qt::white;
  mapdef.grid_color = Qt::white;
  mapdef.background_color = backgroundColor;


  mapdef.initial_bounds.min_x = 0;
  mapdef.initial_bounds.min_y = 0;
  mapdef.initial_bounds.max_x = 500;
  mapdef.initial_bounds.max_y = 2048;


  QFrame *frame = new QFrame (this, 0);

  setCentralWidget (frame);


  //  Make the map.

  map = new nvMap (this, &mapdef);
  map->setWhatsThis (mapText);


  //  Connect to the signals from the map class.

  connect (map, SIGNAL (mousePressSignal (QMouseEvent *, double, double)), this, 
           SLOT (slotMousePress (QMouseEvent *, double, double)));
  connect (map, SIGNAL (mouseMoveSignal (QMouseEvent *, double, double)), this, 
           SLOT (slotMouseMove (QMouseEvent *, double, double)));
  connect (map, SIGNAL (resizeSignal (QResizeEvent *)), this, SLOT (slotResize (QResizeEvent *)));
  connect (map, SIGNAL (postRedrawSignal (NVMAP_DEF)), this, SLOT (slotPlotWaves (NVMAP_DEF)));


  //  Layouts, what fun!

  QVBoxLayout *vBox = new QVBoxLayout (frame);


  vBox->addWidget (map);


  for (int32_t i = 0 ; i < MAX_STACK_POINTS ; i++)
    {
      statusBar[i] = new QStatusBar (frame);
      statusBar[i]->setSizeGripEnabled (false);
      statusBar[i]->show ();
      vBox->addWidget (statusBar[i]);


      dateLabel[i] = new QLabel ("0000-00-00 (000) 00:00:00.00", this);
      dateLabel[i]->setAlignment (Qt::AlignCenter);
      dateLabel[i]->setMinimumSize (dateLabel[i]->sizeHint ());
      dateLabel[i]->setWhatsThis (dateLabelText);
      dateLabel[i]->setToolTip (tr ("Date and time"));
      dateLabel[i]->setAutoFillBackground (true);
      dateLabelPalette[i] = dateLabel[i]->palette ();

      lineLabel[i] = new QLabel ("Line 000-0", this);
      lineLabel[i]->setAlignment (Qt::AlignCenter);
      lineLabel[i]->setMinimumSize (lineLabel[i]->sizeHint ());
      lineLabel[i]->setWhatsThis (lineLabelText);
      lineLabel[i]->setToolTip (tr ("Line number"));
      lineLabel[i]->setAutoFillBackground (true);
      lineLabelPalette[i] = lineLabel[i]->palette ();

      distLabel[i] = new QLabel ("000.00", this);
      distLabel[i]->setAlignment (Qt::AlignCenter);
      distLabel[i]->setMinimumSize (distLabel[i]->sizeHint ());
      distLabel[i]->setWhatsThis (distLabelText);
      distLabel[i]->setToolTip (tr ("Distance in meters from selected points"));
      distLabel[i]->setAutoFillBackground (true);
      distLabelPalette[i] = distLabel[i]->palette ();

      statusBar[i]->addWidget (dateLabel[i], 1); 
      statusBar[i]->addWidget (lineLabel[i]);
      statusBar[i]->addWidget (distLabel[i]);
   }


  //  Button, button, who's got the buttons?

  bQuit = new QToolButton (this);
  bQuit->setIcon (QIcon (":/icons/quit.png"));
  bQuit->setToolTip (tr ("Quit"));
  bQuit->setWhatsThis (quitText);
  connect (bQuit, SIGNAL (clicked ()), this, SLOT (slotQuit ()));
  tools->addWidget (bQuit);


  bMode = new QToolButton (this);
  if (wave_line_mode)
    {
      bMode->setIcon (QIcon (":/icons/mode_line.png"));
    }
  else
    {
      bMode->setIcon (QIcon (":/icons/mode_dot.png"));
    }
  bMode->setToolTip (tr ("Wave drawing mode toggle"));
  bMode->setWhatsThis (modeText);
  bMode->setCheckable (true);
  bMode->setChecked (wave_line_mode);
  connect (bMode, SIGNAL (toggled (bool)), this, SLOT (slotMode (bool)));
  tools->addWidget (bMode);


  bPrefs = new QToolButton (this);
  bPrefs->setIcon (QIcon (":/icons/prefs.png"));
  bPrefs->setToolTip (tr ("Change application preferences"));
  bPrefs->setWhatsThis (prefsText);
  connect (bPrefs, SIGNAL (clicked ()), this, SLOT (slotPrefs ()));
  tools->addWidget (bPrefs);


  tools->addSeparator ();
  tools->addSeparator ();


  QAction *bHelp = QWhatsThis::createAction (this);
  bHelp->setIcon (QIcon (":/icons/contextHelp.png"));
  tools->addAction (bHelp);



  //  Setup the file menu.

  QAction *fileQuitAction = new QAction (tr ("&Quit"), this);
  fileQuitAction->setShortcut (tr ("Ctrl+Q"));
  fileQuitAction->setStatusTip (tr ("Exit from application"));
  connect (fileQuitAction, SIGNAL (triggered ()), this, SLOT (slotQuit ()));


  QMenu *fileMenu = menuBar ()->addMenu (tr ("&File"));
  fileMenu->addAction (fileQuitAction);


  //  Setup the help menu.

  QAction *aboutAct = new QAction (tr ("&About"), this);
  aboutAct->setShortcut (tr ("Ctrl+A"));
  aboutAct->setStatusTip (tr ("Information about waveWaterfall"));
  connect (aboutAct, SIGNAL (triggered ()), this, SLOT (about ()));

  QAction *acknowledgments = new QAction (tr ("A&cknowledgments"), this);
  acknowledgments->setShortcut (tr ("Ctrl+c"));
  acknowledgments->setStatusTip (tr ("Information about supporting libraries"));
  connect (acknowledgments, SIGNAL (triggered ()), this, SLOT (slotAcknowledgments ()));

  QAction *aboutQtAct = new QAction (tr ("About Qt"), this);
  aboutQtAct->setStatusTip (tr ("Information about Qt"));
  connect (aboutQtAct, SIGNAL (triggered ()), this, SLOT (aboutQt ()));

  QMenu *helpMenu = menuBar ()->addMenu (tr ("&Help"));
  helpMenu->addAction (aboutAct);
  helpMenu->addSeparator ();
  helpMenu->addAction (acknowledgments);
  helpMenu->addAction (aboutQtAct);


  map->setCursor (Qt::ArrowCursor);

  map->enableSignals ();


  QTimer *track = new QTimer (this);
  connect (track, SIGNAL (timeout ()), this, SLOT (trackCursor ()));
  track->start (10);
}



waveWaterfall::~waveWaterfall ()
{
}



void 
waveWaterfall::closeEvent (QCloseEvent *event __attribute__ ((unused)))
{
  slotQuit ();
}



void 
waveWaterfall::slotResize (QResizeEvent *e __attribute__ ((unused)))
{
  force_redraw = NVTrue;
}



void
waveWaterfall::slotHelp ()
{
  QWhatsThis::enterWhatsThisMode ();
}



void 
waveWaterfall::leftMouse (double x __attribute__ ((unused)), double y __attribute__ ((unused)))
{
  //  Placeholder
}



void 
waveWaterfall::midMouse (double x __attribute__ ((unused)), double y __attribute__ ((unused)))
{
  //  Placeholder
}



void 
waveWaterfall::rightMouse (double x __attribute__ ((unused)), double y __attribute__ ((unused)))
{
  //  Placeholder
}



//  Signal from the map class.

void 
waveWaterfall::slotMousePress (QMouseEvent * e, double x, double y)
{
  if (e->button () == Qt::LeftButton) leftMouse (x, y);
  if (e->button () == Qt::MidButton) midMouse (x, y);
  if (e->button () == Qt::RightButton) rightMouse (x, y);
}



//  Signal from the map class.

void
waveWaterfall::slotMouseMove (QMouseEvent *e __attribute__ ((unused)), double x __attribute__ ((unused)), double y __attribute__ ((unused)))
{
  //  Placeholder
}



//  Timer - timeout signal.  Very much like an X workproc.

void
waveWaterfall::trackCursor ()
{
  int32_t                 year, day, mday, month, hour, minute, chan, ret;
  float                   second;
  FILE                    *hfp = NULL, *wfp = NULL;
  int32_t                 cpf_handle, cwf_handle, return_type = 0;
  WAVE_HEADER_T           wave_header;
  CZMIL_CPF_Header        cpf_header;
  CZMIL_CWF_Header        cwf_header;
  static uint32_t         prev_rec = UINT32_MAX;
  static int32_t          prev_sub = -1;


  //  We want to exit if we have locked the tracker to update our saved waveforms (in slotPlotWaves).

  if (lock_track) return;


  //  Since this is always a child process of something we want to exit if we see the CHILD_PROCESS_FORCE_EXIT key.
  //  We also want to exit on the ANCILLARY_FORCE_EXIT key (from pfmEdit) or if our own personal kill signal
  //  has been placed in abe_share->key.

  if (abe_share->key == CHILD_PROCESS_FORCE_EXIT || abe_share->key == ANCILLARY_FORCE_EXIT || abe_share->key == kill_switch) slotQuit ();


  //  Locking makes sure another process does not have memory locked.  It will block until it can lock it.
  //  At that point we copy the contents and then unlock it so other processes can continue.

  abeShare->lock ();


  //  Check for change of record and correct record type (or change of record or subrecord for CZMIL data).

  uint8_t hit = NVFalse;
  if ((prev_rec != abe_share->mwShare.multiRecord[0] && (abe_share->mwShare.multiType[0] == PFM_SHOALS_1K_DATA ||
                                                         abe_share->mwShare.multiType[0] == PFM_CHARTS_HOF_DATA)) ||
      (abe_share->mwShare.multiType[0] == PFM_CZMIL_DATA && (prev_rec != abe_share->mwShare.multiRecord[0] ||
                                                             prev_sub != abe_share->mwShare.multiSubrecord[0])))
    {
      l_share = *abe_share;
      prev_rec = l_share.mwShare.multiRecord[0];
      prev_sub = l_share.mwShare.multiSubrecord[0];
      hit = NVTrue;
      return_type = abe_share->mwShare.multiType[0];
    }


  //  We use the waveMonitor force redraw since this program is displaying waveforms too.

  if (abe_share->modcode == WAVEMONITOR_FORCE_REDRAW) force_redraw = NVTrue;


  //  If the key is set to PFM_LAYERS_CHANGED we need to close the old and open the new PFM files.

  if (abe_share->key == PFM_LAYERS_CHANGED)
    {
      for (int32_t pfm = 0 ; pfm < num_pfms ; pfm++) close_pfm_file (pfm_handle[pfm]);


      num_pfms = abe_share->pfm_count;

      for (int32_t pfm = 0 ; pfm < num_pfms ; pfm++)
	{
	  open_args[pfm] = abe_share->open_args[pfm];
	  
	  if ((pfm_handle[pfm] = open_existing_pfm_file (&open_args[pfm])) < 0)
	    {
	      QMessageBox::warning (this, tr ("Open PFM Structure"),
				    tr ("The file %1 is not a PFM handle or list file or there was an error reading the file.\nThe error message returned was:\n\n%2").arg
                                    (QDir::toNativeSeparators (QString (abe_share->open_args[pfm].list_path))).arg (pfm_error_str (pfm_error)));
	    }
	}

      force_redraw = NVTrue;
    }


  abeShare->unlock ();


  //  Check for HOF or CZMIL, no memory lock, record change, key press, force_redraw.

  if (hit || force_redraw)
    {
      int32_t low_czmil = 99999, high_czmil = -99999;

      force_redraw = NVFalse;


      int16_t type;


      for (int32_t i = 0 ; i < MAX_STACK_POINTS ; i++)
        {
          //  Open the HOF or CZMIL files and read the data.

          read_list_file (pfm_handle[l_share.mwShare.multiPfm[i]], l_share.mwShare.multiFile[i], path[i], &type);


          strcpy (line_name[i], read_line_file (pfm_handle[l_share.mwShare.multiPfm[i]], l_share.mwShare.multiLine[i]));


          wave[i].rec_type = type;

          switch (type)
            {
            case PFM_SHOALS_1K_DATA:
            case PFM_CHARTS_HOF_DATA:

              strcpy (wave_path[i], path[i]);
              sprintf (&wave_path[i][strlen (wave_path[i]) - 4], ".inh");


              hfp = open_hof_file (path[i]);

              if (hfp == NULL)
                {
                  if (filError) filError->close ();
                  filError = new QMessageBox (QMessageBox::Warning, tr ("waveWaterfall"), tr ("Error opening %1\n%2").arg
                                              (QDir::toNativeSeparators (QString (l_share.nearest_filename))).arg (strerror (errno)), QMessageBox::NoButton, this, 
                                              (Qt::WindowFlags) Qt::WA_DeleteOnClose);
                  filError->show ();
                  return;
                }


              wfp = open_wave_file (wave_path[i]);

              if (wfp == NULL)
                {
                  if (filError) filError->close ();
                  filError = new QMessageBox (QMessageBox::Warning, tr ("Open INH file"), tr ("Error opening %1\n%2").arg
                                              (QDir::toNativeSeparators (QString (wave_path[i]))).arg (strerror (errno)), QMessageBox::NoButton, this, 
                                              (Qt::WindowFlags) Qt::WA_DeleteOnClose);
                  filError->show ();
                  return;
                }


              if (hfp != NULL && wfp != NULL)
                {
                  wave_read_header (wfp, &wave_header);


                  //  Save for slotPlotWaves.

                  recnum[i] = l_share.mwShare.multiRecord[i];


		  hof_read_header (hfp, &hof_header);
                  hof_read_record (hfp, l_share.mwShare.multiRecord[i], &hof_record[i]);
                  wave_read_record (wfp, l_share.mwShare.multiRecord[i], &wave_data);

                  fclose (hfp);
                  fclose (wfp);
                }


              if (wave_type == APD)
                {
                  wave[i].length = wave_header.apd_size - 1;

                  for (int32_t j = 0 ; j < wave[i].length ; j++) wave[i].data[j] = wave_data.apd[j + 1];
                }
              else
                {
                  wave[i].length = wave_header.pmt_size - 1;

                  for (int32_t j = 0 ; j < wave[i].length ; j++) wave[i].data[j] = wave_data.pmt[j + 1];
                }


              if (hof_record[i].sec_depth == -998.0) 
                {
                  secondary[i] = NVFalse;
                }
              else
                {
                  secondary[i] = NVTrue;
                }


              charts_cvtime (hof_record[i].timestamp, &year, &day, &hour, &minute, &second);
              charts_jday2mday (year, day, &month, &mday);
              month++;


              date_time[i] = tr ("%1-%2-%3 (%4) %5:%6:%L7<br>").arg (year + 1900).arg (month, 2, 10, QChar ('0')).arg (mday, 2, 10, QChar ('0')).arg
                (day, 3, 10, QChar ('0')).arg (hour, 2, 10, QChar ('0')).arg (minute, 2, 10, QChar ('0')).arg (second, 5, 'f', 2, QChar ('0'));


              //  Set the min and max values (always use the PMT size so that APD and PMT will match).

              wave_bounds[i].min_y = 0;
              wave_bounds[i].max_y = 256;
              wave_bounds[i].min_x = 0;
              wave_bounds[i].max_x = wave_header.pmt_size;


              //  Add 5% to the X axis.

              wave_bounds[i].range_x = wave_bounds[i].max_x - wave_bounds[i].min_x;
              wave_bounds[i].min_x = wave_bounds[i].min_x - NINT (((float) wave_bounds[i].range_x * 0.05 + 1));
              wave_bounds[i].max_x = wave_bounds[i].max_x + NINT (((float) wave_bounds[i].range_x * 0.05 + 1));
              wave_bounds[i].range_x = wave_bounds[i].max_x - wave_bounds[i].min_x;
              wave_bounds[i].range_y = wave_bounds[i].max_y - wave_bounds[i].min_y;

              wave[i].lidar = NVTrue;
              break;

            case PFM_CZMIL_DATA:

              strcpy (wave_path[i], path[i]);
              sprintf (&wave_path[i][strlen (wave_path[i]) - 4], ".cwf");


              if ((cpf_handle = czmil_open_cpf_file (path[i], &cpf_header, CZMIL_READONLY)) < 0)
                {
                  if (filError) filError->close ();
                  filError = new QMessageBox (QMessageBox::Warning, tr ("waveWaterfall"), tr ("Error opening %1\n%2").arg
                                              (QDir::toNativeSeparators (QString (path[i]))).arg (czmil_strerror ()), QMessageBox::NoButton, this, 
                                              (Qt::WindowFlags) Qt::WA_DeleteOnClose);
                  filError->show ();
                  return;
                }


              if ((cwf_handle = czmil_open_cwf_file (wave_path[i], &cwf_header, CZMIL_READONLY)) < 0)
                {
                  if (filError) filError->close ();
                  filError = new QMessageBox (QMessageBox::Warning, tr ("Open INH file"), tr ("Error opening %1\n%2").arg
                                              (QDir::toNativeSeparators (QString (wave_path[i]))).arg (czmil_strerror ()), QMessageBox::NoButton, this, 
                                              (Qt::WindowFlags) Qt::WA_DeleteOnClose);
                  filError->show ();
                  return;
                }


              //  Save for slotPlotWaves.

              recnum[i] = l_share.mwShare.multiRecord[i];


              czmil_read_cpf_record (cpf_handle, l_share.mwShare.multiRecord[i], &cpf_record[i]);
              czmil_read_cwf_record (cwf_handle, l_share.mwShare.multiRecord[i], &cwf_record[i]);


              czmil_close_cpf_file (cpf_handle);
              czmil_close_cwf_file (cwf_handle);


              chan = l_share.mwShare.multiSubrecord[i] / 100;
              ret = l_share.mwShare.multiSubrecord[i] % 100;


              if (wave_type == APD)
                {
                  //  We don't want deep data in the shallow window so we'll show the shallow central when that happens.

                  if (chan == CZMIL_DEEP_CHANNEL) chan = CZMIL_SHALLOW_CHANNEL_1;
                }
              else
                {
                  chan = CZMIL_DEEP_CHANNEL;
                }

              low_czmil = qMin ((int32_t) cwf_record[i].channel_ndx[chan][0], low_czmil);
              high_czmil = qMax ((int32_t) cwf_record[i].channel_ndx[chan][cwf_record[i].number_of_packets[chan] - 1], high_czmil);

              //fprintf (stderr, "%s %s %d %d %d %d %d\n",NVFFL,chan,cwf_record.channel_ndx[chan][0], cwf_record.number_of_packets[chan],l_share.mwShare.multiRecord[i]);
              //for (int32_t m = 0 ; m < cwf_record.number_of_packets[chan] ; m++) fprintf (stderr, "%s %s %d %d %d\n",NVFFL,chan,cwf_record.channel_ndx[chan][m]);

              secondary[i] = NVFalse;


              czmil_cvtime (cpf_record[i].timestamp, &year, &day, &hour, &minute, &second);
              czmil_jday2mday (year, day, &month, &mday);
              month++;


              date_time[i] = tr ("%1-%2-%3 (%4) %5:%6:%L7<br>").arg (year + 1900).arg (month, 2, 10, QChar ('0')).arg (mday, 2, 10, QChar ('0')).arg
                (day, 3, 10, QChar ('0')).arg (hour, 2, 10, QChar ('0')).arg (minute, 2, 10, QChar ('0')).arg (second, 5, 'f', 2, QChar ('0'));

              wave[i].lidar = NVTrue;
              break;


            default:
              wave[i].lidar = NVFalse;
              break;
            }
        }


      if (type == PFM_CZMIL_DATA)
        {
          for (int32_t i = 0 ; i < MAX_STACK_POINTS ; i++)
            {
              chan = l_share.mwShare.multiSubrecord[i] / 100;
              ret = l_share.mwShare.multiSubrecord[i] % 100;

              if (wave_type == APD)
                {
                  //  We don't want deep data in the shallow window so we'll show the shallow central when that happens.

                  if (chan == CZMIL_DEEP_CHANNEL) chan = CZMIL_SHALLOW_CHANNEL_1;
                }
              else
                {
                  chan = CZMIL_DEEP_CHANNEL;
                }

              int32_t wave_offset = cwf_record[i].channel_ndx[chan][0] - low_czmil;
              wave[i].length = wave_offset * 64 + cwf_record[i].number_of_packets[chan] * 64;
              //fprintf (stderr, "%s %s %d %d %d %d %d %d\n",NVFFL,chan,cwf_record[i].channel_ndx[chan][0], cwf_record[i].number_of_packets[chan],wave_offset,wave[i].length);


              //for (int32_t m = 0 ; m < cwf_record[i].number_of_packets[chan] ; m++) fprintf (stderr, "%s %s %d %d %d\n",NVFFL,chan,cwf_record[i].channel_ndx[chan][m]);

              wave_offset *= 64;
              for (int32_t j = 0 ; j < wave[i].length ; j++)
                {
                  if (j < wave_offset)
                    {
                      wave[i].data[j] = -32767;
                    }
                  else
                    {
                      wave[i].data[j] = cwf_record[i].channel[chan][j - wave_offset];
                    }
                }


              wave[i].num_packets = cwf_record[i].number_of_packets[chan];
              wave[i].channel_ndx = cwf_record[i].channel_ndx[chan][ret];

              wave[i].cx = wave[i].cy = 0.0;

              if (cpf_record[i].channel[chan][ret].interest_point != 0.0)
                {
                  int32_t prev_point = (int32_t) cpf_record[i].channel[chan][ret].interest_point;
                  int32_t next_point = (int32_t) cpf_record[i].channel[chan][ret].interest_point + 1;

                  wave[i].cy = wave[i].data[prev_point] + ((wave[i].data[next_point] - wave[i].data[prev_point]) / 2);

                  wave[i].cx = cpf_record[i].channel[chan][ret].interest_point;
                }


              //  Set the min and max values (always use the CZMIL_DEEP_CHANNEL size so that SHALLOW and DEEP will match).

              wave_bounds[i].min_y = -10;
              wave_bounds[i].max_y = 1100;
              wave_bounds[i].min_x = 0;
              wave_bounds[i].max_x = (high_czmil - low_czmil + 1) * 64;


              //  Add 5% to the X axis.

              wave_bounds[i].range_x = wave_bounds[i].max_x - wave_bounds[i].min_x;
              wave_bounds[i].min_x = wave_bounds[i].min_x - NINT (((float) wave_bounds[i].range_x * 0.05 + 1));
              wave_bounds[i].max_x = wave_bounds[i].max_x + NINT (((float) wave_bounds[i].range_x * 0.05 + 1));
              wave_bounds[i].range_x = wave_bounds[i].max_x - wave_bounds[i].min_x;
              wave_bounds[i].range_y = wave_bounds[i].max_y - wave_bounds[i].min_y;
            }
        }


      //  Done reading the information.

      l_share.key = 0;
      abe_share->key = 0;
      abe_share->modcode = return_type;


      map->redrawMapArea (NVTrue);
    }
}



//  A bunch of slots.

void 
waveWaterfall::slotQuit ()
{
  //  Let the parent program know that we have died from something other than the kill switch from the parent.

  if (abe_share->key != kill_switch) abe_share->killed = kill_switch;


  envout ();


  //  Let go of the shared memory.

  abeShare->detach ();


  exit (0);
}



void 
waveWaterfall::slotMode (bool on)
{
  wave_line_mode = on;

  if (on)
    {
      bMode->setIcon (QIcon (":/icons/mode_line.png"));
    }
  else
    {
      bMode->setIcon (QIcon (":/icons/mode_dot.png"));
    }

  force_redraw = NVTrue;
}



void 
waveWaterfall::setFields ()
{
  if (pos_format == "hdms") bGrp->button (0)->setChecked (true);
  if (pos_format == "hdm") bGrp->button (1)->setChecked (true);
  if (pos_format == "hd") bGrp->button (2)->setChecked (true);
  if (pos_format == "dms") bGrp->button (3)->setChecked (true);
  if (pos_format == "dm") bGrp->button (4)->setChecked (true);
  if (pos_format == "d") bGrp->button (5)->setChecked (true);


  int32_t hue, sat, val;

  primaryColor.getHsv (&hue, &sat, &val);
  if (val < 128)
    {
      bPrimaryPalette.setColor (QPalette::Normal, QPalette::ButtonText, Qt::white);
      bPrimaryPalette.setColor (QPalette::Inactive, QPalette::ButtonText, Qt::white);
    }
  else
    {
      bPrimaryPalette.setColor (QPalette::Normal, QPalette::ButtonText, Qt::black);
      bPrimaryPalette.setColor (QPalette::Inactive, QPalette::ButtonText, Qt::black);
    }
  bPrimaryPalette.setColor (QPalette::Normal, QPalette::Button, primaryColor);
  bPrimaryPalette.setColor (QPalette::Inactive, QPalette::Button, primaryColor);
  bPrimaryColor->setPalette (bPrimaryPalette);


  transPrimaryColor = primaryColor;
  transPrimaryColor.setAlpha (WAVE_ALPHA);


  secondaryColor.getHsv (&hue, &sat, &val);
  if (val < 128)
    {
      bSecondaryPalette.setColor (QPalette::Normal, QPalette::ButtonText, Qt::white);
      bSecondaryPalette.setColor (QPalette::Inactive, QPalette::ButtonText, Qt::white);
    }
  else
    {
      bSecondaryPalette.setColor (QPalette::Normal, QPalette::ButtonText, Qt::black);
      bSecondaryPalette.setColor (QPalette::Inactive, QPalette::ButtonText, Qt::black);
    }
  bSecondaryPalette.setColor (QPalette::Normal, QPalette::Button, secondaryColor);
  bSecondaryPalette.setColor (QPalette::Inactive, QPalette::Button, secondaryColor);
  bSecondaryColor->setPalette (bSecondaryPalette);


  transSecondaryColor = secondaryColor;
  transSecondaryColor.setAlpha (WAVE_ALPHA);


  backgroundColor.getHsv (&hue, &sat, &val);
  if (val < 128)
    {
      bBackgroundPalette.setColor (QPalette::Normal, QPalette::ButtonText, Qt::white);
      bBackgroundPalette.setColor (QPalette::Inactive, QPalette::ButtonText, Qt::white);
    }
  else
    {
      bBackgroundPalette.setColor (QPalette::Normal, QPalette::ButtonText, Qt::black);
      bBackgroundPalette.setColor (QPalette::Inactive, QPalette::ButtonText, Qt::black);
    }
  bBackgroundPalette.setColor (QPalette::Normal, QPalette::Button, backgroundColor);
  bBackgroundPalette.setColor (QPalette::Inactive, QPalette::Button, backgroundColor);
  bBackgroundColor->setPalette (bBackgroundPalette);
}



void
waveWaterfall::slotPrefs ()
{
  prefsD = new QDialog (this, (Qt::WindowFlags) Qt::WA_DeleteOnClose);
  prefsD->setWindowTitle (tr ("waveWaterfall Preferences"));
  prefsD->setModal (true);

  QVBoxLayout *vbox = new QVBoxLayout (prefsD);
  vbox->setMargin (5);
  vbox->setSpacing (5);

  QGroupBox *fbox = new QGroupBox (tr ("Position Format"), prefsD);
  fbox->setWhatsThis (bGrpText);

  QRadioButton *hdms = new QRadioButton (tr ("Hemisphere Degrees Minutes Seconds.decimal"));
  QRadioButton *hdm_ = new QRadioButton (tr ("Hemisphere Degrees Minutes.decimal"));
  QRadioButton *hd__ = new QRadioButton (tr ("Hemisphere Degrees.decimal"));
  QRadioButton *sdms = new QRadioButton (tr ("+/-Degrees Minutes Seconds.decimal"));
  QRadioButton *sdm_ = new QRadioButton (tr ("+/-Degrees Minutes.decimal"));
  QRadioButton *sd__ = new QRadioButton (tr ("+/-Degrees.decimal"));

  bGrp = new QButtonGroup (prefsD);
  bGrp->setExclusive (true);
  connect (bGrp, SIGNAL (buttonClicked (int)), this, SLOT (slotPosClicked (int)));

  bGrp->addButton (hdms, 0);
  bGrp->addButton (hdm_, 1);
  bGrp->addButton (hd__, 2);
  bGrp->addButton (sdms, 3);
  bGrp->addButton (sdm_, 4);
  bGrp->addButton (sd__, 5);

  QHBoxLayout *fboxSplit = new QHBoxLayout;
  QVBoxLayout *fboxLeft = new QVBoxLayout;
  QVBoxLayout *fboxRight = new QVBoxLayout;
  fboxSplit->addLayout (fboxLeft);
  fboxSplit->addLayout (fboxRight);
  fboxLeft->addWidget (hdms);
  fboxLeft->addWidget (hdm_);
  fboxLeft->addWidget (hd__);
  fboxRight->addWidget (sdms);
  fboxRight->addWidget (sdm_);
  fboxRight->addWidget (sd__);
  fbox->setLayout (fboxSplit);

  vbox->addWidget (fbox, 1);

  if (pos_format == "hdms") bGrp->button (0)->setChecked (true);
  if (pos_format == "hdm") bGrp->button (1)->setChecked (true);
  if (pos_format == "hd") bGrp->button (2)->setChecked (true);
  if (pos_format == "dms") bGrp->button (3)->setChecked (true);
  if (pos_format == "dm") bGrp->button (4)->setChecked (true);
  if (pos_format == "d") bGrp->button (5)->setChecked (true);


  QGroupBox *cbox = new QGroupBox (tr ("Colors"), this);
  QVBoxLayout *cboxLayout = new QVBoxLayout;
  cbox->setLayout (cboxLayout);
  QHBoxLayout *cboxTopLayout = new QHBoxLayout;
  QHBoxLayout *cboxBottomLayout = new QHBoxLayout;
  cboxLayout->addLayout (cboxTopLayout);
  cboxLayout->addLayout (cboxBottomLayout);


  bPrimaryColor = new QPushButton (tr ("Primary"), this);
  bPrimaryColor->setToolTip (tr ("Change primary return marker color"));
  bPrimaryColor->setWhatsThis (bPrimaryColor->toolTip ());
  bPrimaryPalette = bPrimaryColor->palette ();
  connect (bPrimaryColor, SIGNAL (clicked ()), this, SLOT (slotPrimaryColor ()));
  cboxBottomLayout->addWidget (bPrimaryColor);


  bSecondaryColor = new QPushButton (tr ("Secondary"), this);
  bSecondaryColor->setToolTip (tr ("Change secondary return marker color"));
  bSecondaryColor->setWhatsThis (bSecondaryColor->toolTip ());
  bSecondaryPalette = bSecondaryColor->palette ();
  connect (bSecondaryColor, SIGNAL (clicked ()), this, SLOT (slotSecondaryColor ()));
  cboxBottomLayout->addWidget (bSecondaryColor);


  bBackgroundColor = new QPushButton (tr ("Background"), this);
  bBackgroundColor->setToolTip (tr ("Change display background color"));
  bBackgroundColor->setWhatsThis (bBackgroundColor->toolTip ());
  bBackgroundPalette = bBackgroundColor->palette ();
  connect (bBackgroundColor, SIGNAL (clicked ()), this, SLOT (slotBackgroundColor ()));
  cboxBottomLayout->addWidget (bBackgroundColor);


  vbox->addWidget (cbox, 1);


  QHBoxLayout *actions = new QHBoxLayout (0);
  vbox->addLayout (actions);

  QPushButton *bHelp = new QPushButton (prefsD);
  bHelp->setIcon (QIcon (":/icons/contextHelp.png"));
  bHelp->setToolTip (tr ("Enter What's This mode for help"));
  connect (bHelp, SIGNAL (clicked ()), this, SLOT (slotHelp ()));
  actions->addWidget (bHelp);

  actions->addStretch (10);

  bRestoreDefaults = new QPushButton (tr ("Restore Defaults"), this);
  bRestoreDefaults->setToolTip (tr ("Restore all preferences to the default state"));
  bRestoreDefaults->setWhatsThis (restoreDefaultsText);
  connect (bRestoreDefaults, SIGNAL (clicked ()), this, SLOT (slotRestoreDefaults ()));
  actions->addWidget (bRestoreDefaults);

  QPushButton *closeButton = new QPushButton (tr ("Close"), prefsD);
  closeButton->setToolTip (tr ("Close the preferences dialog"));
  connect (closeButton, SIGNAL (clicked ()), this, SLOT (slotClosePrefs ()));
  actions->addWidget (closeButton);


  setFields ();


  prefsD->show ();
}



void
waveWaterfall::slotPosClicked (int id)
{
  switch (id)
    {
    case 0:
    default:
      pos_format = "hdms";
      break;

    case 1:
      pos_format = "hdm";
      break;

    case 2:
      pos_format = "hd";
      break;

    case 3:
      pos_format = "dms";
      break;

    case 4:
      pos_format = "dm";
      break;

    case 5:
      pos_format = "d";
      break;
    }
}



void
waveWaterfall::slotClosePrefs ()
{
  prefsD->close ();
}



void
waveWaterfall::slotPrimaryColor ()
{
  QColor clr;

  clr = QColorDialog::getColor (primaryColor, this, tr ("waveWaterfall Primary Marker Color"), QColorDialog::ShowAlphaChannel);

  if (clr.isValid ()) primaryColor = clr;

  setFields ();

  force_redraw = NVTrue;
}



void
waveWaterfall::slotSecondaryColor ()
{
  QColor clr;

  clr = QColorDialog::getColor (secondaryColor, this, tr ("waveWaterfall Secondary Marker Color"), QColorDialog::ShowAlphaChannel);

  if (clr.isValid ()) secondaryColor = clr;

  setFields ();

  force_redraw = NVTrue;
}



void
waveWaterfall::slotBackgroundColor ()
{
  QColor clr;

  clr = QColorDialog::getColor (backgroundColor, this, tr ("waveWaterfall Background Color"));

  if (clr.isValid ()) backgroundColor = clr;

  setFields ();

  force_redraw = NVTrue;
}



void 
waveWaterfall::scaleWave (int32_t x, int32_t y, int32_t *new_x, int32_t *new_y, NVMAP_DEF l_mapdef, int32_t wave_num)
{
  *new_y = NINT (((float) (x - wave_bounds[0].min_x) / (float) wave_bounds[0].range_x) * (float) l_mapdef.draw_height);


  //  Plot against adjusted and then shift based on wave number

  *new_x = NINT (((float) (y - wave_bounds[0].min_y) / (float) wave_bounds[0].range_y) * (float) adjusted_width) + WAVE_OFFSET * wave_num;
}



void 
waveWaterfall::slotPlotWaves (NVMAP_DEF l_mapdef)
{
  static int32_t          save_rec[MAX_STACK_POINTS];
  static WAVE             save_wave[MAX_STACK_POINTS];
  static HYDRO_OUTPUT_T   save_hof[MAX_STACK_POINTS];
  static CZMIL_CPF_Data   save_cpf[MAX_STACK_POINTS];
  static QString          save_line[MAX_STACK_POINTS];
  int32_t                 pix_x[2], pix_y[2];
  double                  dist;
  QColor                  curWaveColor, curPrimaryColor, curSecondaryColor, saveWaveColor;


  /*  If we haven't yet loaded a point or the center point isn't the right type we need to GTHOOD.  */

  if (!wave[0].lidar) return;


  adjusted_width = l_mapdef.draw_width - MAX_STACK_POINTS * WAVE_OFFSET;


  lock_track = NVTrue;
  for (int32_t i = 0 ; i < MAX_STACK_POINTS ; i++)
    {
      //  Because the trackCursor function may be changing the data while we're still plotting it we save it
      //  to this static structure.  lock_track stops trackCursor from updating while we're trying to get an
      //  atomic snapshot of the data for the latest point.

      save_wave[i] = wave[i];
      save_hof[i] = hof_record[i];
      save_cpf[i] = cpf_record[i];
      save_rec[i] = recnum[i];
      save_line[i] = line_name[i];
    }
  lock_track = NVFalse;


  for (int32_t i = 0 ; i < MAX_STACK_POINTS ; i++)
    {
      if (save_rec[i] != -1)
        {
          curWaveColor = waveColor[i];
          curPrimaryColor = primaryColor;
          curSecondaryColor = secondaryColor;

          if (l_share.mwShare.multiLine[i] == l_share.mwShare.multiLine[0]) curWaveColor = waveColor[0];

          if (wave[i].lidar)
            {
              //  Draw the waveforms.

              scaleWave (1, save_wave[i].data[0], &pix_x[0], &pix_y[0], l_mapdef, i);

              for (int32_t j = 1 ; j < wave[i].length ; j++)
                {
                  scaleWave (j, save_wave[i].data[j], &pix_x[1], &pix_y[1], l_mapdef, i);

                  if (wave_line_mode)
                    {
                      if (save_wave[i].data[j - 1] >= 0) map->drawLine (pix_x[0], pix_y[0], pix_x[1], pix_y[1], curWaveColor, 2, NVFalse, Qt::SolidLine);
                    }
                  else
                    {
                      map->fillRectangle (pix_x[0], pix_y[0], SPOT_SIZE, SPOT_SIZE, curWaveColor, NVFalse);
                    }
                  pix_x[0] = pix_x[1];
                  pix_y[0] = pix_y[1];
                }


              //  Add marks

              switch (wave[i].rec_type)
                {
                case PFM_SHOALS_1K_DATA:
                case PFM_CHARTS_HOF_DATA:

                  if (secondary[i])
                    {
                      if (save_hof[i].sec_bot_chan == wave_type)
                        {
                          scaleWave (save_hof[i].bot_bin_second, save_wave[i].data[save_hof[i].bot_bin_second], &pix_x[0], &pix_y[0], l_mapdef, i);
                          drawX (pix_x[0], pix_y[0], 10, 2, curSecondaryColor);
                        }
                    }

                  if (save_hof[i].bot_channel == wave_type  && save_hof[i].abdc != 74)
                    {
                      scaleWave (save_hof[i].bot_bin_first, save_wave[i].data[save_hof[i].bot_bin_first], &pix_x[0], &pix_y[0], l_mapdef, i);
                      drawX (pix_x[0], pix_y[0], 10, 2, curPrimaryColor);
                    }
                  break;


                case PFM_CZMIL_DATA:

                  if (save_wave[i].cx != 0.0)
                    {
                      scaleWave (save_wave[i].cx, save_wave[i].cy, &pix_x[0], &pix_y[0], l_mapdef, i);
                      drawX (pix_x[0], pix_y[0], 10, 2, curPrimaryColor);
                    }

                  break;
                }


              //  Set the status bar labels

              dateLabel[i]->setText (date_time[i]);
              lineLabel[i]->setText (save_line[i]);

              if (!pfm_geo_distance (pfm_handle[l_share.mwShare.multiPfm[0]], l_share.mwShare.multiPoint[0].y, l_share.mwShare.multiPoint[0].x,
                                     l_share.mwShare.multiPoint[i].y, l_share.mwShare.multiPoint[i].x, &dist))
                {
                  QString dist_text;
                  dist_text = QString ("%L1").arg (dist, 0, 'f', 2);
                  //dist_text = QString ("%1,%L2").arg (save_wave[i].channel_ndx).arg (save_wave[i].range, 0, 'f', 2);
                  distLabel[i]->setText (dist_text);
                }
              else
                {
                  distLabel[i]->setText ("");
                }

            }
        }
    }


  int32_t w = l_mapdef.draw_width, h = l_mapdef.draw_height;

  if (wave_type == APD)
    {
      map->drawText ("SHAL", w - 80, h - 10, waveColor[0], NVTrue);
    }
  else
    {
      map->drawText ("DEEP", w - 80, h - 10, waveColor[0], NVTrue);
    }


  //  Color code the line labels.

  for (int32_t i = 0 ; i < MAX_STACK_POINTS ; i++)
    {
      int32_t hue, sat, val;

      curWaveColor = waveColor[i];
      if (l_share.mwShare.multiLine[i] == l_share.mwShare.multiLine[0]) curWaveColor = waveColor[0];

      curWaveColor.getHsv (&hue, &sat, &val);

      if (val < 192 || hue > 200)
        {
          dateLabelPalette[i].setColor (QPalette::Normal, QPalette::WindowText, Qt::white);
          dateLabelPalette[i].setColor (QPalette::Inactive, QPalette::WindowText, Qt::white);
          lineLabelPalette[i].setColor (QPalette::Normal, QPalette::WindowText, Qt::white);
          lineLabelPalette[i].setColor (QPalette::Inactive, QPalette::WindowText, Qt::white);
          distLabelPalette[i].setColor (QPalette::Normal, QPalette::WindowText, Qt::white);
          distLabelPalette[i].setColor (QPalette::Inactive, QPalette::WindowText, Qt::white);
        }
      else
        {
          dateLabelPalette[i].setColor (QPalette::Normal, QPalette::WindowText, Qt::black);
          dateLabelPalette[i].setColor (QPalette::Inactive, QPalette::WindowText, Qt::black);
          lineLabelPalette[i].setColor (QPalette::Normal, QPalette::WindowText, Qt::black);
          lineLabelPalette[i].setColor (QPalette::Inactive, QPalette::WindowText, Qt::black);
          distLabelPalette[i].setColor (QPalette::Normal, QPalette::WindowText, Qt::black);
          distLabelPalette[i].setColor (QPalette::Inactive, QPalette::WindowText, Qt::black);
        }
      dateLabelPalette[i].setColor (QPalette::Normal, QPalette::Window, curWaveColor);
      dateLabelPalette[i].setColor (QPalette::Inactive, QPalette::Window, curWaveColor);
      dateLabel[i]->setPalette (dateLabelPalette[i]);
      lineLabelPalette[i].setColor (QPalette::Normal, QPalette::Window, curWaveColor);
      lineLabelPalette[i].setColor (QPalette::Inactive, QPalette::Window, curWaveColor);
      lineLabel[i]->setPalette (lineLabelPalette[i]);
      distLabelPalette[i].setColor (QPalette::Normal, QPalette::Window, curWaveColor);
      distLabelPalette[i].setColor (QPalette::Inactive, QPalette::Window, curWaveColor);
      distLabel[i]->setPalette (distLabelPalette[i]);
    }
}



void 
waveWaterfall::drawX (int32_t x, int32_t y, int32_t size, int32_t width, QColor color)
{
  int32_t hs = size / 2;

  map->drawLine (x - hs, y + hs, x + hs, y - hs, color, width, NVTrue, Qt::SolidLine);
  map->drawLine (x + hs, y + hs, x - hs, y - hs, color, width, NVTrue, Qt::SolidLine);
}



void
waveWaterfall::slotRestoreDefaults ()
{
  static uint8_t first = NVTrue;

  pos_format = "hdms";
  apd_width = WAVE_X_SIZE;
  apd_height = WAVE_Y_SIZE;
  apd_window_x = 0;
  apd_window_y = 0;
  pmt_width = WAVE_X_SIZE;
  pmt_height = WAVE_Y_SIZE;
  pmt_window_x = 0;
  pmt_window_y = 0;
  primaryColor = Qt::green;
  secondaryColor = Qt::red;
  backgroundColor = Qt::black;

  for (int32_t i = 0 ; i < MAX_STACK_POINTS ; i++)
    {
      waveColor[i].setRgb (abe_share->mwShare.multiColors[i].r,
                           abe_share->mwShare.multiColors[i].g,
                           abe_share->mwShare.multiColors[i].b);
      waveColor[i].setAlpha (abe_share->mwShare.multiColors[i].a);
    }


  //  Set the window size and location from the defaults

  if (wave_type == APD)
    {
      this->resize (apd_width, apd_height);
      width = apd_width;
      height = apd_height;
    }
  else
    {
      this->resize (pmt_width, pmt_height);
      width = pmt_width;
      height = pmt_height;
    }


  //  The first time will be called from envin and the prefs dialog won't exist yet.

  if (!first) setFields ();
  first = NVFalse;

  force_redraw = NVTrue;
}



void
waveWaterfall::about ()
{
  QMessageBox::about (this, VERSION,
                      "waveWaterfall - CHARTS waveform waterfall display."
                      "\n\nAuthor : Jan C. Depner (area.based.editor@gmail.com)");
}


void
waveWaterfall::slotAcknowledgments ()
{
  QMessageBox::about (this, VERSION, acknowledgmentsText);
}



void
waveWaterfall::aboutQt ()
{
  QMessageBox::aboutQt (this, VERSION);
}



//  Get the users defaults.

void
waveWaterfall::envin ()
{
  //  We need to get the font from the global settings.

#ifdef NVWIN3X
  QString ini_file2 = QString (getenv ("USERPROFILE")) + "/ABE.config/" + "globalABE.ini";
#else
  QString ini_file2 = QString (getenv ("HOME")) + "/ABE.config/" + "globalABE.ini";
#endif

  font = QApplication::font ();

  QSettings settings2 (ini_file2, QSettings::IniFormat);
  settings2.beginGroup ("globalABE");


  QString defaultFont = font.toString ();
  QString fontString = settings2.value (QString ("ABE map GUI font"), defaultFont).toString ();
  font.fromString (fontString);


  settings2.endGroup ();


  double saved_version = 0.0;


  // Set Defaults so the if keys don't exist the parms are defined

  slotRestoreDefaults ();
  force_redraw = NVFalse;


  //  Get the INI file name

#ifdef NVWIN3X
  QString ini_file = QString (getenv ("USERPROFILE")) + "/ABE.config/waveWaterfall.ini";
#else
  QString ini_file = QString (getenv ("HOME")) + "/ABE.config/waveWaterfall.ini";
#endif

  QSettings settings (ini_file, QSettings::IniFormat);
  settings.beginGroup ("waveWaterfall");


  saved_version = settings.value (QString ("settings version"), saved_version).toDouble ();


  //  If the settings version has changed we need to leave the values at the new defaults since they may have changed.

  if (settings_version != saved_version) return;


  pos_format = settings.value (QString ("position form"), pos_format).toString ();

  wave_line_mode = settings.value (QString ("Wave line mode flag"), wave_line_mode).toBool ();


  int32_t red, green, blue, alpha;


  red = settings.value (QString ("First color/red"), primaryColor.red ()).toInt ();
  green = settings.value (QString ("First color/green"), primaryColor.green ()).toInt ();
  blue = settings.value (QString ("First color/blue"), primaryColor.blue ()).toInt ();
  alpha = settings.value (QString ("First color/alpha"), primaryColor.alpha ()).toInt ();
  primaryColor.setRgb (red, green, blue, alpha);

  red = settings.value (QString ("Second color/red"), secondaryColor.red ()).toInt ();
  green = settings.value (QString ("Second color/green"), secondaryColor.green ()).toInt ();
  blue = settings.value (QString ("Second color/blue"), secondaryColor.blue ()).toInt ();
  alpha = settings.value (QString ("Second color/alpha"), secondaryColor.alpha ()).toInt ();
  secondaryColor.setRgb (red, green, blue, alpha);

  red = settings.value (QString ("Background color/red"), backgroundColor.red ()).toInt ();
  green = settings.value (QString ("Background color/green"), backgroundColor.green ()).toInt ();
  blue = settings.value (QString ("Background color/blue"), backgroundColor.blue ()).toInt ();
  alpha = settings.value (QString ("Background color/alpha"), backgroundColor.alpha ()).toInt ();
  backgroundColor.setRgb (red, green, blue, alpha);


  transPrimaryColor = primaryColor;
  transPrimaryColor.setAlpha (WAVE_ALPHA);
  transSecondaryColor = secondaryColor;
  transSecondaryColor.setAlpha (WAVE_ALPHA);



  this->restoreState (settings.value (QString ("main window state")).toByteArray (), NINT (settings_version * 100.0));


  settings.endGroup ();


  //  Special location settings so APD doesn't step on PMT and vice versa.

  if (wave_type == APD)
    {
#ifdef NVWIN3X
      QString ini_file2 = QString (getenv ("USERPROFILE")) + "/ABE.config/waveWaterfall_shallow.ini";
#else
      QString ini_file2 = QString (getenv ("HOME")) + "/ABE.config/waveWaterfall_shallow.ini";
#endif

      QSettings geo_settings (ini_file2, QSettings::IniFormat);
      geo_settings.beginGroup (QString ("waveWaterfall_shallow"));

      apd_width = geo_settings.value (QString ("width"), apd_width).toInt ();
      apd_height = geo_settings.value (QString ("height"), apd_height).toInt ();
      apd_window_x = geo_settings.value (QString ("window x"), apd_window_x).toInt ();
      apd_window_y = geo_settings.value (QString ("window y"), apd_window_y).toInt ();

      geo_settings.endGroup ();
    }
  else
    {
#ifdef NVWIN3X
      QString ini_file2 = QString (getenv ("USERPROFILE")) + "/ABE.config/waveWaterfall_deep.ini";
#else
      QString ini_file2 = QString (getenv ("HOME")) + "/ABE.config/waveWaterfall_deep.ini";
#endif

      QSettings geo_settings (ini_file2, QSettings::IniFormat);
      geo_settings.beginGroup (QString ("waveWaterfall_deep"));

      pmt_width = geo_settings.value (QString ("width"), pmt_width).toInt ();
      pmt_height = geo_settings.value (QString ("height"), pmt_height).toInt ();
      pmt_window_x = geo_settings.value (QString ("window x"), pmt_window_x).toInt ();
      pmt_window_y = geo_settings.value (QString ("window y"), pmt_window_y).toInt ();

      geo_settings.endGroup ();
    }
}




//  Save the users defaults.

void
waveWaterfall::envout ()
{
  //  Get the INI file name

#ifdef NVWIN3X
  QString ini_file = QString (getenv ("USERPROFILE")) + "/ABE.config/waveWaterfall.ini";
#else
  QString ini_file = QString (getenv ("HOME")) + "/ABE.config/waveWaterfall.ini";
#endif

  QSettings settings (ini_file, QSettings::IniFormat);
  settings.beginGroup ("waveWaterfall");


  settings.setValue (QString ("settings version"), settings_version);

  settings.setValue (QString ("position form"), pos_format);

  settings.setValue (QString ("apd width"), apd_width);
  settings.setValue (QString ("apd height"), apd_height);
  settings.setValue (QString ("apd window x"), apd_window_x);
  settings.setValue (QString ("apd window y"), apd_window_y);

  settings.setValue (QString ("pmt width"), pmt_width);
  settings.setValue (QString ("pmt height"), pmt_height);
  settings.setValue (QString ("pmt window x"), pmt_window_x);
  settings.setValue (QString ("pmt window y"), pmt_window_y);


  settings.setValue (QString ("Wave line mode flag"), wave_line_mode);


  settings.setValue (QString ("First color/red"), primaryColor.red ());
  settings.setValue (QString ("First color/green"), primaryColor.green ());
  settings.setValue (QString ("First color/blue"), primaryColor.blue ());
  settings.setValue (QString ("First color/alpha"), primaryColor.alpha ());

  settings.setValue (QString ("Second color/red"), secondaryColor.red ());
  settings.setValue (QString ("Second color/green"), secondaryColor.green ());
  settings.setValue (QString ("Second color/blue"), secondaryColor.blue ());
  settings.setValue (QString ("Second color/alpha"), secondaryColor.alpha ());

  settings.setValue (QString ("Background color/red"), backgroundColor.red ());
  settings.setValue (QString ("Background color/green"), backgroundColor.green ());
  settings.setValue (QString ("Background color/blue"), backgroundColor.blue ());
  settings.setValue (QString ("Background color/alpha"), backgroundColor.alpha ());


  settings.setValue (QString ("main window state"), this->saveState (NINT (settings_version * 100.0)));

  settings.endGroup ();


  //  Special location settings so APD doesn't step on PMT and veice versa.

  if (wave_type == APD)
    {
      //  Use frame geometry to get the absolute x and y.

      QRect tmp = this->frameGeometry ();

      apd_window_x = tmp.x ();
      apd_window_y = tmp.y ();


      //  Use geometry to get the width and height.

      tmp = this->geometry ();

      apd_width = tmp.width ();
      apd_height = tmp.height ();


      QSettings geo_settings (QString ("navo.navy.mil"), QString ("waveWaterfall_shallow"));
      geo_settings.beginGroup (QString ("waveWaterfall_shallow"));

      geo_settings.setValue (QString ("width"), apd_width);
      geo_settings.setValue (QString ("height"), apd_height);
      geo_settings.setValue (QString ("window x"), apd_window_x);
      geo_settings.setValue (QString ("window y"), apd_window_y);

      geo_settings.endGroup ();
    }
  else
    {
      //  Use frame geometry to get the absolute x and y.

      QRect tmp = this->frameGeometry ();

      pmt_window_x = tmp.x ();
      pmt_window_y = tmp.y ();


      //  Use geometry to get the width and height.

      tmp = this->geometry ();

      pmt_width = tmp.width ();
      pmt_height = tmp.height ();


      QSettings geo_settings (QString ("navo.navy.mil"), QString ("waveWaterfall_deep"));
      geo_settings.beginGroup (QString ("waveWaterfall_deep"));

      geo_settings.setValue (QString ("width"), pmt_width);
      geo_settings.setValue (QString ("height"), pmt_height);
      geo_settings.setValue (QString ("window x"), pmt_window_x);
      geo_settings.setValue (QString ("window y"), pmt_window_y);

      geo_settings.endGroup ();
    }
}
