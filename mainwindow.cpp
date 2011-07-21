#include "mainwindow.h"
#include "ui_mainwindow.h"

#include "cdockwidgetregistry.h"
#include "cpluginmanager.h"

#include "testsuiteexecutivedialog.h"

#include "main.h"

#include "compilerthread.h"
#include "ccc65interface.h"

#include <QApplication>
#include <QStringList>
#include <QMessageBox>
#include <QSettings>

OutputPaneDockWidget* output = NULL;
ProjectBrowserDockWidget* projectBrowser = NULL;

MainWindow::MainWindow(QWidget* parent) :
   QMainWindow(parent),
   ui(new Ui::MainWindow)
{
   QSettings settings;


   // Initialize preferences dialogs.
   EmulatorPrefsDialog::readSettings();

   QObject::connect(compiler, SIGNAL(compileStarted()), this, SLOT(compiler_compileStarted()));
   QObject::connect(compiler, SIGNAL(compileDone(bool)), this, SLOT(compiler_compileDone(bool)));

   generalTextLogger = new CTextLogger();
   buildTextLogger = new CTextLogger();
   debugTextLogger = new CTextLogger();
   searchTextLogger = new CTextLogger();

   nesicideProject = new CNesicideProject();

   ui->setupUi(this);

   QObject::connect(ui->tabWidget,SIGNAL(tabModified(int,bool)),this,SLOT(tabWidget_tabModified(int,bool)));
   QObject::connect(ui->tabWidget,SIGNAL(tabAdded(int)),this,SLOT(tabWidget_tabAdded(int)));

   ui->menuWindow->setEnabled(false);

   m_pEmulator = new NESEmulatorDockWidget();
   addDockWidget(Qt::RightDockWidgetArea, m_pEmulator );
   m_pEmulator->hide();
   QObject::connect(m_pEmulator, SIGNAL(visibilityChanged(bool)), this, SLOT(reflectedEmulator_close(bool)));
   CDockWidgetRegistry::addWidget ( "Emulator", m_pEmulator );

   m_pSourceNavigator = new SourceNavigator(ui->tabWidget);
   ui->compilerToolbar->addWidget(m_pSourceNavigator);
   CDockWidgetRegistry::addWidget ( "Source Navigator", m_pSourceNavigator );

   m_pSearchBar = new SearchBar("SearchBar");
   ui->searchToolBar->addWidget(m_pSearchBar);
   CDockWidgetRegistry::addWidget ( "Search Bar", m_pSearchBar );

   m_pEmulatorControl = new EmulatorControl();
   ui->debuggerToolbar->addWidget(m_pEmulatorControl);

   // Add menu for emulator control.  The emulator control provides menu for itself!  =]
   QAction* firstEmuMenuAction = ui->menuEmulator->actions().at(0);
   ui->menuEmulator->insertActions(firstEmuMenuAction,m_pEmulatorControl->menu());
   ui->menuEmulator->insertSeparator(firstEmuMenuAction);

   m_pSearch = new SearchDockWidget();
   addDockWidget(Qt::LeftDockWidgetArea, m_pSearch );
   m_pSearch->hide();
   QObject::connect(m_pSearch, SIGNAL(visibilityChanged(bool)), this, SLOT(reflectedSearch_close(bool)));
   CDockWidgetRegistry::addWidget ( "Search", m_pSearch );

   projectBrowser = new ProjectBrowserDockWidget(ui->tabWidget);
   addDockWidget(Qt::LeftDockWidgetArea, projectBrowser );
   projectBrowser->hide();
   QObject::connect(projectBrowser, SIGNAL(visibilityChanged(bool)), this, SLOT(reflectedProjectBrowser_close(bool)));
   CDockWidgetRegistry::addWidget ( "Project", projectBrowser );

   output = new OutputPaneDockWidget(ui->tabWidget);
   addDockWidget(Qt::BottomDockWidgetArea, output );
   output->hide();
   QObject::connect(output, SIGNAL(visibilityChanged(bool)), this, SLOT(reflectedOutput_Window_close(bool)));
   QObject::connect(generalTextLogger,SIGNAL(updateText(QString)),output,SLOT(updateGeneralPane(QString)));
   QObject::connect(buildTextLogger,SIGNAL(updateText(QString)),output,SLOT(updateBuildPane(QString)));
   QObject::connect(debugTextLogger,SIGNAL(updateText(QString)),output,SLOT(updateDebugPane(QString)));
   QObject::connect(searchTextLogger,SIGNAL(updateText(QString)),output,SLOT(updateSearchPane(QString)));
   QObject::connect(generalTextLogger,SIGNAL(eraseText()),output,SLOT(eraseGeneralPane()));
   QObject::connect(buildTextLogger,SIGNAL(eraseText()),output,SLOT(eraseBuildPane()));
   QObject::connect(debugTextLogger,SIGNAL(eraseText()),output,SLOT(eraseDebugPane()));
   QObject::connect(searchTextLogger,SIGNAL(eraseText()),output,SLOT(eraseSearchPane()));
   QObject::connect(breakpointWatcher,SIGNAL(showPane(int)),output,SLOT(showPane(int)));
   CDockWidgetRegistry::addWidget ( "Output", output );

   generalTextLogger->write("<strong>NESICIDE</strong> Alpha Release");
   generalTextLogger->write("<strong>Plugin Scripting Subsystem:</strong> " + pluginManager->getVersionInfo());

   m_pBreakpointInspector = new BreakpointDockWidget ();
   addDockWidget(Qt::BottomDockWidgetArea, m_pBreakpointInspector );
   m_pBreakpointInspector->hide();
   QObject::connect(m_pBreakpointInspector, SIGNAL(visibilityChanged(bool)), this, SLOT(reflectedBreakpointInspector_close(bool)));
   QObject::connect(m_pBreakpointInspector,SIGNAL(markProjectDirty(bool)),this,SLOT(markProjectDirty(bool)));
   CDockWidgetRegistry::addWidget ( "Breakpoints", m_pBreakpointInspector );

   m_pGfxCHRMemoryInspector = new CHRMEMInspector ();
   m_pGfxCHRMemoryInspector->setFeatures(QDockWidget::DockWidgetClosable|QDockWidget::DockWidgetFloatable|QDockWidget::DockWidgetMovable);
   m_pGfxCHRMemoryInspector->setWindowTitle("CHR Memory Visualizer");
   m_pGfxCHRMemoryInspector->setObjectName("chrMemoryVisualizer");
   m_pGfxCHRMemoryInspector->setAllowedAreas(Qt::AllDockWidgetAreas);
   addDockWidget(Qt::BottomDockWidgetArea, m_pGfxCHRMemoryInspector );
   m_pGfxCHRMemoryInspector->hide();
   QObject::connect(m_pGfxCHRMemoryInspector, SIGNAL(visibilityChanged(bool)), this, SLOT(reflectedGfxCHRMemoryInspector_close(bool)));
   QObject::connect(m_pGfxCHRMemoryInspector,SIGNAL(markProjectDirty(bool)),this,SLOT(markProjectDirty(bool)));
   CDockWidgetRegistry::addWidget ( "CHR Memory Visualizer", m_pGfxCHRMemoryInspector );

   m_pGfxOAMMemoryInspector = new OAMVisualizerDockWidget ();
   addDockWidget(Qt::BottomDockWidgetArea, m_pGfxOAMMemoryInspector );
   m_pGfxOAMMemoryInspector->hide();
   QObject::connect(m_pGfxOAMMemoryInspector, SIGNAL(visibilityChanged(bool)), this, SLOT(reflectedGfxOAMMemoryInspector_close(bool)));
   QObject::connect(m_pGfxOAMMemoryInspector,SIGNAL(markProjectDirty(bool)),this,SLOT(markProjectDirty(bool)));
   CDockWidgetRegistry::addWidget ( "OAM Memory Visualizer", m_pGfxOAMMemoryInspector );

   m_pGfxNameTableMemoryInspector = new NameTableVisualizerDockWidget ();
   addDockWidget(Qt::RightDockWidgetArea, m_pGfxNameTableMemoryInspector );
   m_pGfxNameTableMemoryInspector->hide();
   QObject::connect(m_pGfxNameTableMemoryInspector, SIGNAL(visibilityChanged(bool)), this, SLOT(reflectedGfxNameTableMemoryInspector_close(bool)));
   QObject::connect(m_pGfxNameTableMemoryInspector,SIGNAL(markProjectDirty(bool)),this,SLOT(markProjectDirty(bool)));
   CDockWidgetRegistry::addWidget ( "Name Table Visualizer", m_pGfxNameTableMemoryInspector );

   m_pExecutionInspector = new ExecutionInspectorDockWidget();
   addDockWidget(Qt::BottomDockWidgetArea, m_pExecutionInspector );
   m_pExecutionInspector->hide();
   QObject::connect(m_pExecutionInspector, SIGNAL(visibilityChanged(bool)), this, SLOT(reflectedExecutionInspector_close(bool)));
   QObject::connect(m_pExecutionInspector,SIGNAL(markProjectDirty(bool)),this,SLOT(markProjectDirty(bool)));
   CDockWidgetRegistry::addWidget ( "Execution Inspector", m_pExecutionInspector );

   m_pExecutionVisualizer = new ExecutionVisualizerDockWidget();
   addDockWidget(Qt::BottomDockWidgetArea, m_pExecutionVisualizer );
   m_pExecutionVisualizer->hide();
   QObject::connect(m_pExecutionVisualizer, SIGNAL(visibilityChanged(bool)), this, SLOT(reflectedExecutionVisualizer_Inspector_close(bool)));
   QObject::connect(m_pExecutionVisualizer,SIGNAL(markProjectDirty(bool)),this,SLOT(markProjectDirty(bool)));
   CDockWidgetRegistry::addWidget ( "Execution Visualizer", m_pExecutionVisualizer );

   m_pAssemblyInspector = new CodeBrowserDockWidget();
   addDockWidget(Qt::RightDockWidgetArea, m_pAssemblyInspector );
   m_pAssemblyInspector->hide();
   QObject::connect(m_pAssemblyInspector, SIGNAL(visibilityChanged(bool)), this, SLOT(reflectedAssemblyInspector_close(bool)));
   QObject::connect(m_pAssemblyInspector,SIGNAL(markProjectDirty(bool)),this,SLOT(markProjectDirty(bool)));
   CDockWidgetRegistry::addWidget ( "Assembly Browser", m_pAssemblyInspector );

   m_pCodeDataLoggerInspector = new CodeDataLoggerDockWidget();
   addDockWidget(Qt::RightDockWidgetArea, m_pCodeDataLoggerInspector );
   m_pCodeDataLoggerInspector->hide();
   QObject::connect(m_pCodeDataLoggerInspector, SIGNAL(visibilityChanged(bool)), this, SLOT(reflectedCodeDataLoggerInspector_close(bool)));
   QObject::connect(m_pCodeDataLoggerInspector,SIGNAL(markProjectDirty(bool)),this,SLOT(markProjectDirty(bool)));
   CDockWidgetRegistry::addWidget ( "Code/Data Logger Inspector", m_pCodeDataLoggerInspector );

   m_pBinCPURegisterInspector = new RegisterInspectorDockWidget(eMemory_CPUregs);
   m_pBinCPURegisterInspector->setObjectName("cpuRegisterInspector");
   m_pBinCPURegisterInspector->setWindowTitle("CPU Register Inspector");
   addDockWidget(Qt::BottomDockWidgetArea, m_pBinCPURegisterInspector );
   m_pBinCPURegisterInspector->hide();
   QObject::connect(m_pBinCPURegisterInspector, SIGNAL(visibilityChanged(bool)), this, SLOT(reflectedBinCPURegisterInspector_close(bool)));
   QObject::connect(m_pBinCPURegisterInspector,SIGNAL(markProjectDirty(bool)),this,SLOT(markProjectDirty(bool)));
   CDockWidgetRegistry::addWidget ( "CPU Register Inspector", m_pBinCPURegisterInspector );

   m_pBinCPURAMInspector = new MemoryInspectorDockWidget(eMemory_CPU);
   m_pBinCPURAMInspector->setObjectName("cpuMemoryInspector");
   m_pBinCPURAMInspector->setWindowTitle("CPU RAM Inspector");
   addDockWidget(Qt::BottomDockWidgetArea, m_pBinCPURAMInspector );
   m_pBinCPURAMInspector->hide();
   QObject::connect(m_pBinCPURAMInspector, SIGNAL(visibilityChanged(bool)), this, SLOT(reflectedBinCPURAMInspector_close(bool)));
   QObject::connect(m_pBinCPURAMInspector,SIGNAL(markProjectDirty(bool)),this,SLOT(markProjectDirty(bool)));
   CDockWidgetRegistry::addWidget ( "CPU RAM Inspector", m_pBinCPURAMInspector );

   m_pBinROMInspector = new MemoryInspectorDockWidget(eMemory_cartROM);
   m_pBinROMInspector->setObjectName("cartPRGROMMemoryInspector");
   m_pBinROMInspector->setWindowTitle("PRG-ROM Inspector");
   addDockWidget(Qt::BottomDockWidgetArea, m_pBinROMInspector );
   m_pBinROMInspector->hide();
   QObject::connect(m_pBinROMInspector, SIGNAL(visibilityChanged(bool)), this, SLOT(reflectedBinROMInspector_close(bool)));
   QObject::connect(m_pBinROMInspector,SIGNAL(markProjectDirty(bool)),this,SLOT(markProjectDirty(bool)));
   CDockWidgetRegistry::addWidget ( "PRG-ROM Inspector", m_pBinROMInspector );

   m_pBinNameTableMemoryInspector = new MemoryInspectorDockWidget(eMemory_PPU);
   m_pBinNameTableMemoryInspector->setObjectName("ppuNameTableMemoryInspector");
   m_pBinNameTableMemoryInspector->setWindowTitle("NameTable Inspector");
   addDockWidget(Qt::BottomDockWidgetArea, m_pBinNameTableMemoryInspector );
   m_pBinNameTableMemoryInspector->hide();
   QObject::connect(m_pBinNameTableMemoryInspector, SIGNAL(visibilityChanged(bool)), this, SLOT(reflectedBinNameTableMemoryInspector_close(bool)));
   QObject::connect(m_pBinNameTableMemoryInspector,SIGNAL(markProjectDirty(bool)),this,SLOT(markProjectDirty(bool)));
   CDockWidgetRegistry::addWidget ( "NameTable Inspector", m_pBinNameTableMemoryInspector );

   m_pBinPPURegisterInspector = new RegisterInspectorDockWidget(eMemory_PPUregs);
   m_pBinPPURegisterInspector->setObjectName("ppuRegisterInspector");
   m_pBinPPURegisterInspector->setWindowTitle("PPU Register Inspector");
   addDockWidget(Qt::BottomDockWidgetArea, m_pBinPPURegisterInspector );
   m_pBinPPURegisterInspector->hide();
   QObject::connect(m_pBinPPURegisterInspector, SIGNAL(visibilityChanged(bool)), this, SLOT(reflectedBinPPURegisterInspector_close(bool)));
   QObject::connect(m_pBinPPURegisterInspector,SIGNAL(markProjectDirty(bool)),this,SLOT(markProjectDirty(bool)));
   CDockWidgetRegistry::addWidget ( "PPU Register Inspector", m_pBinPPURegisterInspector );

   m_pPPUInformationInspector = new PPUInformationDockWidget();
   addDockWidget(Qt::BottomDockWidgetArea, m_pPPUInformationInspector );
   m_pPPUInformationInspector->hide();
   QObject::connect(m_pPPUInformationInspector, SIGNAL(visibilityChanged(bool)), this, SLOT(reflectedPPUInformationInspector_close(bool)));
   QObject::connect(m_pPPUInformationInspector,SIGNAL(markProjectDirty(bool)),this,SLOT(markProjectDirty(bool)));
   CDockWidgetRegistry::addWidget ( "PPU Information", m_pPPUInformationInspector );

   m_pBinAPURegisterInspector = new RegisterInspectorDockWidget(eMemory_IOregs);
   m_pBinAPURegisterInspector->setObjectName("apuRegisterInspector");
   m_pBinAPURegisterInspector->setWindowTitle("APU Register Inspector");
   addDockWidget(Qt::BottomDockWidgetArea, m_pBinAPURegisterInspector );
   m_pBinAPURegisterInspector->hide();
   QObject::connect(m_pBinAPURegisterInspector, SIGNAL(visibilityChanged(bool)), this, SLOT(reflectedBinAPURegisterInspector_close(bool)));
   QObject::connect(m_pBinAPURegisterInspector,SIGNAL(markProjectDirty(bool)),this,SLOT(markProjectDirty(bool)));
   CDockWidgetRegistry::addWidget ( "APU Register Inspector", m_pBinAPURegisterInspector );

   m_pAPUInformationInspector = new APUInformationDockWidget();
   addDockWidget(Qt::BottomDockWidgetArea, m_pAPUInformationInspector );
   m_pAPUInformationInspector->hide();
   QObject::connect(m_pAPUInformationInspector, SIGNAL(visibilityChanged(bool)), this, SLOT(reflectedAPUInformationInspector_close(bool)));
   QObject::connect(m_pAPUInformationInspector,SIGNAL(markProjectDirty(bool)),this,SLOT(markProjectDirty(bool)));
   CDockWidgetRegistry::addWidget ( "APU Information", m_pAPUInformationInspector );

   m_pBinCHRMemoryInspector = new MemoryInspectorDockWidget(eMemory_cartCHRMEM);
   m_pBinCHRMemoryInspector->setObjectName("chrMemoryInspector");
   m_pBinCHRMemoryInspector->setWindowTitle("CHR Memory Inspector");
   addDockWidget(Qt::BottomDockWidgetArea, m_pBinCHRMemoryInspector );
   m_pBinCHRMemoryInspector->hide();
   QObject::connect(m_pBinCHRMemoryInspector, SIGNAL(visibilityChanged(bool)), this, SLOT(reflectedBinCHRMemoryInspector_close(bool)));
   QObject::connect(m_pBinCHRMemoryInspector,SIGNAL(markProjectDirty(bool)),this,SLOT(markProjectDirty(bool)));
   CDockWidgetRegistry::addWidget ( "CHR Memory Inspector", m_pBinCHRMemoryInspector );

   m_pBinOAMMemoryInspector = new RegisterInspectorDockWidget(eMemory_PPUoam);
   m_pBinOAMMemoryInspector->setObjectName("oamMemoryInspector");
   m_pBinOAMMemoryInspector->setWindowTitle("OAM Memory Inspector");
   addDockWidget(Qt::BottomDockWidgetArea, m_pBinOAMMemoryInspector );
   m_pBinOAMMemoryInspector->hide();
   QObject::connect(m_pBinOAMMemoryInspector, SIGNAL(visibilityChanged(bool)), this, SLOT(reflectedBinOAMMemoryInspector_close(bool)));
   QObject::connect(m_pBinOAMMemoryInspector,SIGNAL(markProjectDirty(bool)),this,SLOT(markProjectDirty(bool)));
   CDockWidgetRegistry::addWidget ( "OAM Memory Inspector", m_pBinOAMMemoryInspector );

   m_pBinPaletteMemoryInspector = new MemoryInspectorDockWidget(eMemory_PPUpalette);
   m_pBinPaletteMemoryInspector->setObjectName("ppuPaletteMemoryInspector");
   m_pBinPaletteMemoryInspector->setWindowTitle("Palette Memory Inspector");
   addDockWidget(Qt::BottomDockWidgetArea, m_pBinPaletteMemoryInspector );
   m_pBinPaletteMemoryInspector->hide();
   QObject::connect(m_pBinPaletteMemoryInspector, SIGNAL(visibilityChanged(bool)), this, SLOT(reflectedBinPaletteMemoryInspector_close(bool)));
   QObject::connect(m_pBinPaletteMemoryInspector,SIGNAL(markProjectDirty(bool)),this,SLOT(markProjectDirty(bool)));
   CDockWidgetRegistry::addWidget ( "Palette Memory Inspector", m_pBinPaletteMemoryInspector );

   m_pBinSRAMMemoryInspector = new MemoryInspectorDockWidget(eMemory_cartSRAM);
   m_pBinSRAMMemoryInspector->setObjectName("cartSRAMMemoryInspector");
   m_pBinSRAMMemoryInspector->setWindowTitle("Cartridge SRAM Memory Inspector");
   addDockWidget(Qt::BottomDockWidgetArea, m_pBinSRAMMemoryInspector );
   m_pBinSRAMMemoryInspector->hide();
   QObject::connect(m_pBinSRAMMemoryInspector, SIGNAL(visibilityChanged(bool)), this, SLOT(reflectedBinSRAMMemoryInspector_close(bool)));
   QObject::connect(m_pBinSRAMMemoryInspector,SIGNAL(markProjectDirty(bool)),this,SLOT(markProjectDirty(bool)));
   CDockWidgetRegistry::addWidget ( "Cartridge SRAM Memory Inspector", m_pBinSRAMMemoryInspector );

   m_pBinEXRAMMemoryInspector = new MemoryInspectorDockWidget(eMemory_cartEXRAM);
   m_pBinEXRAMMemoryInspector->setObjectName("cartEXRAMMemoryInspector");
   m_pBinEXRAMMemoryInspector->setWindowTitle("Cartridge EXRAM Memory Inspector");
   addDockWidget(Qt::BottomDockWidgetArea, m_pBinEXRAMMemoryInspector );
   m_pBinEXRAMMemoryInspector->hide();
   QObject::connect(m_pBinEXRAMMemoryInspector, SIGNAL(visibilityChanged(bool)), this, SLOT(reflectedBinEXRAMMemoryInspector_close(bool)));
   QObject::connect(m_pBinEXRAMMemoryInspector,SIGNAL(markProjectDirty(bool)),this,SLOT(markProjectDirty(bool)));
   CDockWidgetRegistry::addWidget ( "Cartridge EXRAM Memory Inspector", m_pBinEXRAMMemoryInspector );

   m_pMapperInformationInspector = new MapperInformationDockWidget();
   addDockWidget(Qt::BottomDockWidgetArea, m_pMapperInformationInspector );
   m_pMapperInformationInspector->hide();
   QObject::connect(m_pMapperInformationInspector, SIGNAL(visibilityChanged(bool)), this, SLOT(reflectedMapperInformationInspector_close(bool)));
   QObject::connect(m_pMapperInformationInspector,SIGNAL(markProjectDirty(bool)),this,SLOT(markProjectDirty(bool)));
   CDockWidgetRegistry::addWidget ( "Cartridge Mapper Information", m_pMapperInformationInspector );

   m_pBinMapperMemoryInspector = new RegisterInspectorDockWidget(eMemory_cartMapper);
   m_pBinMapperMemoryInspector->setObjectName("cartMapperRegisterInspector");
   m_pBinMapperMemoryInspector->setWindowTitle("Cartridge Mapper Register Inspector");
   addDockWidget(Qt::BottomDockWidgetArea, m_pBinMapperMemoryInspector );
   m_pBinMapperMemoryInspector->hide();
   QObject::connect(m_pBinMapperMemoryInspector, SIGNAL(visibilityChanged(bool)), this, SLOT(reflectedBinMapperMemoryInspector_close(bool)));
   QObject::connect(m_pBinMapperMemoryInspector,SIGNAL(markProjectDirty(bool)),this,SLOT(markProjectDirty(bool)));
   CDockWidgetRegistry::addWidget ( "Cartridge Mapper Register Inspector", m_pBinMapperMemoryInspector );

   m_pSymbolInspector = new SymbolWatchDockWidget();
   addDockWidget(Qt::BottomDockWidgetArea, m_pSymbolInspector );
   m_pSymbolInspector->hide();
   QObject::connect(m_pSymbolInspector, SIGNAL(visibilityChanged(bool)), this, SLOT(reflectedSymbol_Watch_close(bool)));
   QObject::connect(m_pSymbolInspector,SIGNAL(markProjectDirty(bool)),this,SLOT(markProjectDirty(bool)));
   CDockWidgetRegistry::addWidget ( "Symbol Inspector", m_pSymbolInspector );

   // Set TV standard to use.
   int systemMode = EmulatorPrefsDialog::getTVStandard();
   ui->actionNTSC->setChecked(systemMode==MODE_NTSC);
   ui->actionPAL->setChecked(systemMode==MODE_PAL);
   nesSetSystemMode(systemMode);

   // Set up controllers.
   nesSetControllerType(0,EmulatorPrefsDialog::getControllerType(0));
   nesSetControllerSpecial(0,EmulatorPrefsDialog::getControllerSpecial(0));
   nesSetControllerType(1,EmulatorPrefsDialog::getControllerType(1));
   nesSetControllerSpecial(1,EmulatorPrefsDialog::getControllerSpecial(1));

   // Set sound channel enables.
   bool square1 = EmulatorPrefsDialog::getSquare1Enabled();
   bool square2 = EmulatorPrefsDialog::getSquare2Enabled();
   bool triangle = EmulatorPrefsDialog::getTriangleEnabled();
   bool noise = EmulatorPrefsDialog::getNoiseEnabled();
   bool dmc = EmulatorPrefsDialog::getDMCEnabled();
   int mask = ((square1<<0)|(square2<<1)|(triangle<<2)|(noise<<3)|(dmc<<4));

   ui->actionSquare_1->setChecked(square1);
   ui->actionSquare_2->setChecked(square2);
   ui->actionTriangle->setChecked(triangle);
   ui->actionNoise->setChecked(noise);
   ui->actionDelta_Modulation->setChecked(dmc);
   ui->actionMute_All->setChecked(!mask);

   if ( EnvironmentSettingsDialog::showWelcomeOnStart() )
   {
      ui->tabWidget->addTab(ui->tab,"Welcome Page");
      ui->webView->setUrl(QUrl( "http://wiki.nesicide.com/doku.php?id=nesicide_user_manual"));
   }
   else
   {
      ui->tabWidget->removeTab(ui->tabWidget->indexOf(ui->tab));
   }

   QStringList argv = QApplication::arguments();
   QStringList argv_nes = argv.filter ( QRegExp("*.nes",Qt::CaseInsensitive,QRegExp::Wildcard) );
   QStringList argv_nesproject = argv.filter ( QRegExp("*.nesproject",Qt::CaseInsensitive,QRegExp::Wildcard) );

   if ( (argv_nes.count() >= 1) &&
        (argv_nesproject.count() >= 1) )
   {
      QMessageBox::information ( 0, "Command Line Error", "Cannot specify a .nes and a .nesproject file to open.\n" );
   }

   if ( argv_nes.count() >= 1 )
   {
      openROM(argv_nes.at(0));

      if ( argv_nes.count() > 1 )
      {
         QMessageBox::information ( 0, "Command Line Error", "Too many NES ROM files were specified on the command\n"
                                    "line.  Only the first NES ROM was opened, all others\n"
                                    "were ignored." );
      }
   }
   else if ( argv_nesproject.count() >= 1 )
   {
      openProject(argv_nesproject.at(0));

      if ( argv_nesproject.count() > 1 )
      {
         QMessageBox::information ( 0, "Command Line Error", "Too many NESICIDE project files were specified on the command\n"
                                    "line.  Only the first NESICIDE project was opened, all others\n"
                                    "were ignored." );
      }
   }
   else if ( EnvironmentSettingsDialog::trackRecentProjects() )
   {
      argv.clear();
      argv.append(settings.value("LastProject").toString());
      if ( !(argv.at(0).isEmpty()) )
      {
         if ( argv.at(0).contains(".nesproject") )
         {
            openProject(argv.at(0));
         }
         else
         {
            openROM(argv.at(0));
         }
      }
   }

   projectDataChangesEvent();

   // For now don't use the value from the settings, because nesGetAudioSamples()
   // always returns APU_SAMPLES samples
   //emulator->adjustAudio(EnvironmentSettingsDialog::soundBufferDepth());
   emulator->adjustAudio( APU_SAMPLES );
   emulator->resetEmulator();

   // Always call this last CPTODO: WHY?
   pluginManager->doInitScript();
   pluginManager->loadPlugins();

   if ( EnvironmentSettingsDialog::rememberWindowSettings() )
   {
      restoreGeometry(settings.value("IDEGeometry").toByteArray());
      restoreState(settings.value("IDEState").toByteArray());
   }
}

MainWindow::~MainWindow()
{
   ui->tabWidget->clear();

   delete generalTextLogger;
   delete buildTextLogger;
   delete debugTextLogger;
   delete searchTextLogger;

   delete nesicideProject;
   delete pluginManager;
   delete ui;

   delete m_pBreakpointInspector;
   delete m_pGfxCHRMemoryInspector;
   delete m_pGfxOAMMemoryInspector;
   delete m_pGfxNameTableMemoryInspector;
   delete m_pExecutionInspector;
   delete m_pExecutionVisualizer;
   delete m_pAssemblyInspector;
   delete m_pCodeDataLoggerInspector;
   delete m_pBinCPURegisterInspector;
   delete m_pBinCPURAMInspector;
   delete m_pBinROMInspector;
   delete m_pBinNameTableMemoryInspector;
   delete m_pBinPPURegisterInspector;
   delete m_pPPUInformationInspector;
   delete m_pBinAPURegisterInspector;
   delete m_pAPUInformationInspector;
   delete m_pBinCHRMemoryInspector;
   delete m_pBinOAMMemoryInspector;
   delete m_pBinPaletteMemoryInspector;
   delete m_pBinSRAMMemoryInspector;
   delete m_pBinEXRAMMemoryInspector;
   delete m_pMapperInformationInspector;
   delete m_pBinMapperMemoryInspector;
   delete m_pSourceNavigator;
   delete m_pSymbolInspector;
   delete m_pSearch;
}

void MainWindow::changeEvent(QEvent* e)
{
   QMainWindow::changeEvent(e);

   switch (e->type())
   {
      case QEvent::LanguageChange:
         ui->retranslateUi(this);
         break;
      default:
         break;
   }
}

void MainWindow::dragEnterEvent(QDragEnterEvent* event)
{
   if ( event->mimeData()->hasUrls() )
   {
      event->acceptProposedAction();
   }
}

void MainWindow::dragMoveEvent(QDragMoveEvent* event)
{
   if ( event->mimeData()->hasUrls() )
   {
      event->acceptProposedAction();
   }
}

void MainWindow::dropEvent(QDropEvent* event)
{
   QList<QUrl> fileUrls;
   QString     fileName;
   QFileInfo   fileInfo;
   QStringList extensions;

   if ( event->mimeData()->hasUrls() )
   {
      output->showPane(OutputPaneDockWidget::Output_General);

      fileUrls = event->mimeData()->urls();

      foreach ( QUrl fileUrl, fileUrls )
      {
         fileName = fileUrl.toLocalFile();

         fileInfo.setFile(fileName);

         if ( !fileInfo.suffix().compare("nesproject",Qt::CaseInsensitive) )
         {
            if ( nesicideProject->isInitialized() )
            {
               closeProject();
            }
            openProject(fileName);

            event->acceptProposedAction();
         }
         else if ( !fileInfo.suffix().compare("nes",Qt::CaseInsensitive) )
         {
            if ( nesicideProject->isInitialized() )
            {
               closeProject();
            }
            openROM(fileName);

            event->acceptProposedAction();
         }
         else
         {
            openFile(fileName);

            event->acceptProposedAction();
         }
      }
   }
}

void MainWindow::keyPressEvent(QKeyEvent *event)
{
   if ( (event->key() == Qt::Key_F) &&
        (event->modifiers() == Qt::ControlModifier) )
   {
      m_pSearchBar->setFocus();
   }
}

void MainWindow::hideEvent(QHideEvent *event)
{
   CDockWidgetRegistry::saveVisibility();
}

void MainWindow::showEvent(QShowEvent *event)
{
   CDockWidgetRegistry::restoreVisibility();
}

void MainWindow::projectDataChangesEvent()
{
   projectBrowser->layoutChangedEvent();
   projectBrowser->setVisible(nesicideProject->isInitialized());
   output->setVisible(nesicideProject->isInitialized());

   // Enabled/Disable actions based on if we have a project loaded or not
   ui->actionNew_Project->setEnabled(!nesicideProject->isInitialized());
   ui->actionCreate_Project_from_ROM->setEnabled(!nesicideProject->isInitialized());
   ui->actionOpen_Project->setEnabled(!nesicideProject->isInitialized());
   ui->actionProject_Properties->setEnabled(nesicideProject->isInitialized());
   ui->action_Project_Browser->setEnabled(nesicideProject->isInitialized());
   ui->action_Close_Project->setEnabled(nesicideProject->isInitialized());
   ui->actionCompile_Project->setEnabled(nesicideProject->isInitialized());
   ui->actionSave_Project->setEnabled(nesicideProject->isInitialized());
   ui->actionSave_Project_As->setEnabled(nesicideProject->isInitialized());
   ui->actionClean_Project->setEnabled(nesicideProject->isInitialized());

   // Enable/Disable actions based on if we have a project loaded or not and a good compile
   ui->actionLoad_In_Emulator->setEnabled ( nesicideProject->isInitialized() && compiler->assembledOk() );

   if (ui->tabWidget->currentIndex() >= 0)
   {
      ICenterWidgetItem* projectItem = dynamic_cast<ICenterWidgetItem*>(ui->tabWidget->currentWidget());

      if ( projectItem && projectItem->isModified() )
      {
         ui->actionSave_Active_Document->setEnabled(projectItem->isModified());
      }
      else
      {
         ui->actionSave_Active_Document->setEnabled(false);
      }
   }

   setWindowTitle(nesicideProject->getProjectTitle().prepend("NESICIDE - "));
}

void MainWindow::markProjectDirty(bool dirty)
{
   nesicideProject->setDirty(dirty);
}

void MainWindow::on_actionSave_Project_triggered()
{
   QSettings settings;

   if (projectFileName.isEmpty())
   {
      projectFileName = QFileDialog::getSaveFileName(this, "Save Project", QDir::currentPath(),
                                                     "NESICIDE Project (*.nesproject)");
   }

   if (!projectFileName.isEmpty())
   {
      saveProject();
   }

   settings.setValue("LastProject",projectFileName);
}

void MainWindow::saveEmulatorState(QString fileName)
{
   QFile file(fileName);

   if ( !file.open( QFile::WriteOnly) )
   {
      QMessageBox::critical(this, "Error", "An error occured while trying to open the save state file for writing.");
      return;
   }

   QDomDocument doc;
   QDomProcessingInstruction instr = doc.createProcessingInstruction("xml", "version='1.0' encoding='UTF-8'");
   doc.appendChild(instr);

   if (!emulator->serialize(doc, doc))
   {
      QMessageBox::critical(this, "Error", "An error occured while trying to serialize the save state data.");
      file.close();
   }

   // Create a text stream so we can stream the XML data to the file easily.
   QTextStream ts( &file );

   // Use the standard C++ stream function for streaming the string representation of our XML to
   // our file stream.
   ts << doc.toString();

   // And finally close the file.
   file.close();
}

void MainWindow::saveProject()
{
   QFile file(projectFileName);

   if ( !file.open( QFile::WriteOnly) )
   {
      QMessageBox::critical(this, "Error", "An error occured while trying to open the project file for writing.");
      projectFileName.clear();
      return;
   }

   QDomDocument doc;
   QDomProcessingInstruction instr = doc.createProcessingInstruction("xml", "version='1.0' encoding='UTF-8'");
   doc.appendChild(instr);

   if (!nesicideProject->serialize(doc, doc))
   {
      QMessageBox::critical(this, "Error", "An error occured while trying to serialize the project data.");
      file.close();
      projectFileName.clear();
   }

   // Create a text stream so we can stream the XML data to the file easily.
   QTextStream ts( &file );

   // Use the standard C++ stream function for streaming the string representation of our XML to
   // our file stream.
   ts << doc.toString();

   // And finally close the file.
   file.close();

   // Now save the emulator state if a save state file is specified.
   if ( !nesicideProject->getProjectCartridgeSaveStateName().isEmpty() )
   {
      saveEmulatorState(nesicideProject->getProjectCartridgeSaveStateName());
   }
}

void MainWindow::on_actionSave_Project_As_triggered()
{
   // Allow the user to select a file name. Note that using the static function produces a native
   // file dialog, while creating an instance of QFileDialog results in a non-native file dialog..
   projectFileName = QFileDialog::getSaveFileName(this, "Save Project", QDir::currentPath(),
                                                  "NESICIDE Project (*.nesproject)");

   if (!projectFileName.isEmpty())
   {
      saveProject();
   }
}

void MainWindow::on_actionProject_Properties_triggered()
{
   ProjectPropertiesDialog dlg;

   if (dlg.exec() == QDialog::Accepted)
   {
      nesicideProject->setDirty(true);
   }
}

void MainWindow::on_actionNew_Project_triggered()
{
   NewProjectDialog dlg(this,"New Project","Untitled");

   if (dlg.exec() == QDialog::Accepted)
   {
      projectFileName.clear();

      projectBrowser->disableNavigation();

      QDir::setCurrent(dlg.getPath());

      nesicideProject->initializeProject();
      nesicideProject->setDirty(true);
      nesicideProject->setProjectTitle(dlg.getName());

      projectBrowser->enableNavigation();
      projectDataChangesEvent();
   }
}

void MainWindow::openROM(QString fileName)
{
   QSettings settings;

   output->showPane(OutputPaneDockWidget::Output_General);

   emulator->pauseEmulation(false);

   // Remove any lingering project content
   projectBrowser->disableNavigation();
   nesicideProject->terminateProject();

   // Clear output
   output->clearAllPanes();
   output->show();

   // Create new project from ROM
   nesicideProject->initializeProject();
   nesicideProject->createProjectFromRom(fileName);

   // Set up some default stuff guessing from the path...
   QFileInfo fileInfo(fileName);
   QDir::setCurrent(fileInfo.path());
   nesicideProject->setProjectLinkerOutputName(fileInfo.completeBaseName()+".prg");
   nesicideProject->setProjectCHRROMOutputName(fileInfo.completeBaseName()+".chr");
   nesicideProject->setProjectCartridgeOutputName(fileInfo.fileName());
   nesicideProject->setProjectCartridgeSaveStateName(fileInfo.completeBaseName()+".sav");
   nesicideProject->setProjectDebugInfoName(fileInfo.completeBaseName()+".dbg");

   // Load debugger info if we can find it.
   CCC65Interface::captureDebugInfo();

   projectBrowser->enableNavigation();

   if ( !nesicideProject->getProjectCartridgeSaveStateName().isEmpty() )
   {
      QDomDocument saveDoc;
      QFile saveFile( nesicideProject->getProjectCartridgeSaveStateName() );

      if (saveFile.open(QFile::ReadOnly))
      {
         saveDoc.setContent(saveFile.readAll());
         nesicideProject->setSaveStateDoc(saveDoc);
      }
      saveFile.close();
   }

   emulator->resetEmulator();
   emulator->primeEmulator();

   if ( EnvironmentSettingsDialog::runRomOnLoad() )
   {
      emulator->startEmulation();
   }

   projectDataChangesEvent();

   ui->actionEmulation_Window->setChecked(true);
   on_actionEmulation_Window_toggled(true);

   settings.setValue("LastProject",fileName);
}

void MainWindow::on_actionCreate_Project_from_ROM_triggered()
{
   QString romPath = EnvironmentSettingsDialog::romPath();
   QString fileName = QFileDialog::getOpenFileName(this, "Open ROM", romPath, "iNES ROM (*.nes)");

   if (fileName.isEmpty())
   {
      return;
   }

   openROM(fileName);
}

void MainWindow::tabWidget_tabModified(int tab, bool modified)
{
   QList<QAction*> actions = ui->menuWindow->actions();
   QString match;

   match = ui->tabWidget->tabText(tab);
   if ( modified )
   {
      match = match.left(match.length()-1);
   }
   else
   {
      match = match + "*";
   }
   foreach ( QAction* action, actions )
   {
      if ( match == action->text() )
      {
         if ( modified )
         {
            match = match + "*";
         }
         else
         {
            match = match.left(match.length()-1);
         }
         action->setText(match);
      }
   }
   ui->actionSave_Active_Document->setEnabled(modified);
}

void MainWindow::windowMenu_triggered()
{
   QAction* action = dynamic_cast<QAction*>(sender());
   int tab;

   if ( action )
   {
      for ( tab = 0; tab < ui->tabWidget->count(); tab++ )
      {
         if ( ui->tabWidget->tabText(tab) == action->text() )
         {
            ui->tabWidget->setCurrentIndex(tab);
         }
      }
   }
}

void MainWindow::tabWidget_tabAdded(int tab)
{
   QList<QAction*> actions = ui->menuWindow->actions();
   QString label = ui->tabWidget->tabText(tab);
   QAction* action;
   bool found = false;

   foreach ( action, actions )
   {
      if ( label == action->text() )
      {
         found = true;
      }
   }
   if ( !found )
   {
      action = ui->menuWindow->addAction(label);
      QObject::connect(action,SIGNAL(triggered()),this,SLOT(windowMenu_triggered()));
   }
   ui->menuWindow->setEnabled(ui->menuWindow->actions().count()>0);
}

void MainWindow::on_tabWidget_tabCloseRequested(int index)
{
   ICenterWidgetItem* projectItem = dynamic_cast<ICenterWidgetItem*>(ui->tabWidget->widget(index));
   QList<QAction*> actions = ui->menuWindow->actions();
   QString label = ui->tabWidget->tabText(index);
   QAction* action;

   foreach ( action, actions )
   {
      if ( label == action->text() )
      {
         QObject::disconnect(action,SIGNAL(triggered()),this,SLOT(windowMenu_triggered()));
         ui->menuWindow->removeAction(action);
      }
   }
   ui->menuWindow->setEnabled(ui->menuWindow->actions().count()>0);

   if (projectItem)
   {
      if (projectItem->onCloseQuery())
      {
         ui->tabWidget->removeTab(index);
         projectItem->onClose();
      }
   }
   else
   {
      ui->tabWidget->removeTab(index);
   }
}

void MainWindow::on_action_Project_Browser_toggled(bool visible)
{
   if ( visible )
   {
      projectBrowser->show();
   }
   else
   {
      projectBrowser->hide();
   }
}

void MainWindow::on_actionEmulation_Window_toggled(bool value)
{
   if (value)
   {
      m_pEmulator->show();
   }
   else
   {
      m_pEmulator->hide();
   }
}

void MainWindow::closeEvent ( QCloseEvent* event )
{
   QSettings settings;

   settings.setValue("IDEGeometry",saveGeometry());
   settings.setValue("IDEState",saveState());

   emulator->pauseEmulation(false);

   if (nesicideProject->isInitialized())
   {
      closeProject();
   }

   QMainWindow::closeEvent(event);
}

void MainWindow::openProject(QString fileName)
{
   QSettings settings;
   QString errors;
   bool    ok;

   if (QFile::exists(fileName))
   {
      QDomDocument doc;
      QFile file( fileName );

      if (!file.open(QFile::ReadOnly))
      {
         QMessageBox::critical(this, "Error", "Failed to open the project file.");
         return;
      }

      if (!doc.setContent(file.readAll()))
      {
         QMessageBox::critical(this, "Error", "Failed to parse the project xml data.");
         file.close();
         return;
      }

      file.close();

      projectBrowser->disableNavigation();

      nesicideProject->initializeProject();

      // Clear output
      output->clearAllPanes();
      output->show();

      // Set up some default stuff guessing from the path...
      QFileInfo fileInfo(fileName);
      QDir::setCurrent(fileInfo.path());

      // Load new project content
      ok = nesicideProject->deserialize(doc,doc,errors);
      if ( !ok )
      {
         QMessageBox::warning(this,"Project Load Error", "The project failed to load.\n\n"+errors);

         nesicideProject->terminateProject();
      }

      // Load ROM if it exists.
      if ( !nesicideProject->getProjectCartridgeOutputName().isEmpty() )
      {
         QDir dir(QDir::currentPath());
         QString romName;
         romName = dir.fromNativeSeparators(dir.relativeFilePath(nesicideProject->getProjectCartridgeOutputName()));

         nesicideProject->createProjectFromRom(romName,true);

         // Load debugger info if we can find it.
         CCC65Interface::captureDebugInfo();

         if ( !nesicideProject->getProjectCartridgeSaveStateName().isEmpty() )
         {
            QDomDocument saveDoc;
            QFile saveFile( nesicideProject->getProjectCartridgeSaveStateName() );

            if (saveFile.open(QFile::ReadOnly))
            {
               saveDoc.setContent(saveFile.readAll());
               nesicideProject->setSaveStateDoc(saveDoc);
            }
            saveFile.close();
         }

         emulator->primeEmulator();
//         emulator->resetEmulator();

         if ( EnvironmentSettingsDialog::runRomOnLoad() )
         {
            emulator->startEmulation();
         }

         ui->actionEmulation_Window->setChecked(true);
         on_actionEmulation_Window_toggled(true);
      }

      projectBrowser->enableNavigation();

      settings.setValue("LastProject",fileName);

      projectDataChangesEvent();

      projectFileName = fileName;
   }
}

void MainWindow::on_actionOpen_Project_triggered()
{
   QString fileName = QFileDialog::getOpenFileName(this, "Open Project", "", "NESICIDE Project (*.nesproject)");

   if (fileName.isEmpty())
   {
      return;
   }

   openProject(fileName);
}

void MainWindow::on_tabWidget_currentChanged(int index)
{
   ICenterWidgetItem* projectItem = dynamic_cast<ICenterWidgetItem*>(ui->tabWidget->widget(index));

   if ( projectItem && projectItem->isModified() )
   {
      ui->actionSave_Active_Document->setEnabled(projectItem->isModified());
   }
   else
   {
      ui->actionSave_Active_Document->setEnabled(false);
   }
}

void MainWindow::on_actionSave_Active_Document_triggered()
{
   ICenterWidgetItem* projectItem = dynamic_cast<ICenterWidgetItem*>(ui->tabWidget->currentWidget());

   if (projectItem)
   {
      projectItem->onSave();
   }
}

void MainWindow::reflectedOutput_Window_close(bool toplevel)
{
   ui->actionOutput_Window->setChecked(toplevel);
}

void MainWindow::on_actionOutput_Window_toggled(bool value)
{
   output->setVisible(value);
}

void MainWindow::reflectedProjectBrowser_close(bool toplevel)
{
   ui->action_Project_Browser->setChecked(toplevel);
}

void MainWindow::on_actionCompile_Project_triggered()
{
   int tab;

   output->showPane(OutputPaneDockWidget::Output_Build);
   emulator->pauseEmulation(false);

   if ( EnvironmentSettingsDialog::saveAllOnCompile() )
   {
      on_actionSave_Project_triggered();

      // Try to close all opened editors
      for ( tab = 0; tab < ui->tabWidget->count(); tab++ )
      {
         ICenterWidgetItem* item = dynamic_cast<ICenterWidgetItem*>(ui->tabWidget->widget(tab));
         if ( item )
         {
            if ( item->isModified() )
            {
               item->onSave();
            }
         }
      }
   }
   compiler->assemble();
}

void MainWindow::compiler_compileStarted()
{
   ui->actionCompile_Project->setEnabled(false);
   ui->actionLoad_In_Emulator->setEnabled(false);
}

void MainWindow::compiler_compileDone(bool bOk)
{
   ui->actionCompile_Project->setEnabled(true);

   projectDataChangesEvent();
}

void MainWindow::on_actionExecution_Inspector_toggled(bool value)
{
   m_pExecutionInspector->setVisible(value);
}

void MainWindow::reflectedExecutionInspector_close (bool toplevel)
{
   ui->actionExecution_Inspector->setChecked(toplevel);
}

void MainWindow::on_actionExecution_Visualizer_Inspector_toggled(bool value)
{
   m_pExecutionVisualizer->setVisible(value);
}

void MainWindow::reflectedExecutionVisualizer_Inspector_close (bool toplevel)
{
   ui->actionExecution_Visualizer_Inspector->setChecked(toplevel);
}

void MainWindow::on_actionBreakpoint_Inspector_toggled(bool value)
{
   m_pBreakpointInspector->setVisible(value);
}

void MainWindow::reflectedBreakpointInspector_close (bool toplevel)
{
   ui->actionBreakpoint_Inspector->setChecked(toplevel);
}

void MainWindow::on_actionGfxCHRMemory_Inspector_toggled(bool value)
{
   m_pGfxCHRMemoryInspector->setVisible(value);
}

void MainWindow::reflectedGfxCHRMemoryInspector_close (bool toplevel)
{
   ui->actionGfxCHRMemory_Inspector->setChecked(toplevel);
}

void MainWindow::on_actionGfxOAMMemory_Inspector_toggled(bool value)
{
   m_pGfxOAMMemoryInspector->setVisible(value);
}

void MainWindow::reflectedGfxOAMMemoryInspector_close (bool toplevel)
{
   ui->actionGfxOAMMemory_Inspector->setChecked(toplevel);
}

void MainWindow::on_actionGfxNameTableMemory_Inspector_toggled(bool value)
{
   m_pGfxNameTableMemoryInspector->setVisible(value);
}

void MainWindow::reflectedGfxNameTableMemoryInspector_close (bool toplevel)
{
   ui->actionGfxNameTableMemory_Inspector->setChecked(toplevel);
}

void MainWindow::on_actionBinOAMMemory_Inspector_toggled(bool value)
{
   m_pBinOAMMemoryInspector->setVisible(value);
}

void MainWindow::reflectedBinOAMMemoryInspector_close (bool toplevel)
{
   ui->actionBinOAMMemory_Inspector->setChecked(toplevel);
}

void MainWindow::on_actionBinCPURegister_Inspector_toggled(bool value)
{
   m_pBinCPURegisterInspector->setVisible(value);
}

void MainWindow::reflectedBinCPURegisterInspector_close ( bool toplevel )
{
   ui->actionBinCPURegister_Inspector->setChecked(toplevel);
}

void MainWindow::on_actionBinCPURAM_Inspector_toggled(bool value)
{
   m_pBinCPURAMInspector->setVisible(value);
}

void MainWindow::reflectedBinCPURAMInspector_close (bool toplevel)
{
   ui->actionBinCPURAM_Inspector->setChecked(toplevel);
}

void MainWindow::on_actionBinROM_Inspector_toggled(bool value)
{
   m_pBinROMInspector->setVisible(value);
}

void MainWindow::reflectedBinROMInspector_close (bool toplevel)
{
   ui->actionBinROM_Inspector->setChecked(toplevel);
}

void MainWindow::on_actionBinNameTableMemory_Inspector_toggled(bool value)
{
   m_pBinNameTableMemoryInspector->setVisible(value);
}

void MainWindow::reflectedBinNameTableMemoryInspector_close (bool toplevel)
{
   ui->actionBinNameTableMemory_Inspector->setChecked(toplevel);
}

void MainWindow::on_actionBinPaletteMemory_Inspector_toggled(bool value)
{
   m_pBinPaletteMemoryInspector->setVisible(value);
}

void MainWindow::reflectedBinPaletteMemoryInspector_close (bool toplevel)
{
   ui->actionBinPaletteMemory_Inspector->setChecked(toplevel);
}

void MainWindow::on_actionBinPPURegister_Inspector_toggled(bool value)
{
   m_pBinPPURegisterInspector->setVisible(value);
}

void MainWindow::reflectedBinPPURegisterInspector_close ( bool toplevel )
{
   ui->actionBinPPURegister_Inspector->setChecked(toplevel);
}

void MainWindow::on_actionBinAPURegister_Inspector_toggled(bool value)
{
   m_pBinAPURegisterInspector->setVisible(value);
}

void MainWindow::reflectedBinAPURegisterInspector_close ( bool toplevel )
{
   ui->actionBinAPURegister_Inspector->setChecked(toplevel);
}

void MainWindow::on_actionBinCHRMemory_Inspector_toggled(bool value)
{
   m_pBinCHRMemoryInspector->setVisible(value);
}

void MainWindow::reflectedBinCHRMemoryInspector_close ( bool toplevel )
{
   ui->actionBinCHRMemory_Inspector->setChecked(toplevel);
}

void MainWindow::on_actionBinSRAMMemory_Inspector_toggled(bool value)
{
   m_pBinSRAMMemoryInspector->setVisible(value);
}

void MainWindow::reflectedBinSRAMMemoryInspector_close ( bool toplevel )
{
   ui->actionBinSRAMMemory_Inspector->setChecked(toplevel);
}

void MainWindow::on_actionBinEXRAMMemory_Inspector_toggled(bool value)
{
   m_pBinEXRAMMemoryInspector->setVisible(value);
}

void MainWindow::reflectedBinEXRAMMemoryInspector_close ( bool toplevel )
{
   ui->actionBinEXRAMMemory_Inspector->setChecked(toplevel);
}

void MainWindow::on_actionBinMapperMemory_Inspector_toggled(bool value)
{
   m_pBinMapperMemoryInspector->setVisible(value);
}

void MainWindow::reflectedBinMapperMemoryInspector_close ( bool toplevel )
{
   ui->actionBinMapperMemory_Inspector->setChecked(toplevel);
}

void MainWindow::on_actionAssembly_Inspector_toggled(bool value)
{
   m_pAssemblyInspector->setVisible(value);
}

void MainWindow::reflectedEmulator_close ( bool toplevel )
{
   ui->actionEmulation_Window->setChecked(toplevel);
}

void MainWindow::reflectedAssemblyInspector_close ( bool toplevel )
{
   ui->actionAssembly_Inspector->setChecked(toplevel);
}

void MainWindow::on_actionCodeDataLogger_Inspector_toggled(bool value)
{
   m_pCodeDataLoggerInspector->setVisible(value);
}

void MainWindow::reflectedCodeDataLoggerInspector_close ( bool toplevel )
{
   ui->actionCodeDataLogger_Inspector->setChecked(toplevel);
}

void MainWindow::on_actionPPUInformation_Inspector_toggled(bool value)
{
   m_pPPUInformationInspector->setVisible(value);
}

void MainWindow::reflectedPPUInformationInspector_close ( bool toplevel )
{
   ui->actionPPUInformation_Inspector->setChecked(toplevel);
}

void MainWindow::on_actionAPUInformation_Inspector_toggled(bool value)
{
   m_pAPUInformationInspector->setVisible(value);
}

void MainWindow::reflectedAPUInformationInspector_close ( bool toplevel )
{
   ui->actionAPUInformation_Inspector->setChecked(toplevel);
}

void MainWindow::on_actionMapperInformation_Inspector_toggled(bool value)
{
   m_pMapperInformationInspector->setVisible(value);
}

void MainWindow::reflectedMapperInformationInspector_close ( bool toplevel )
{
   ui->actionMapperInformation_Inspector->setChecked(toplevel);
}

void MainWindow::on_actionSymbol_Watch_toggled(bool value)
{
   m_pSymbolInspector->setVisible(value);
}

void MainWindow::reflectedSymbol_Watch_close ( bool toplevel )
{
   ui->actionSymbol_Watch->setChecked(toplevel);
}

void MainWindow::on_actionSearch_toggled(bool value)
{
   output->showPane(OutputPaneDockWidget::Output_Search);
   m_pSearch->setVisible(value);
}

void MainWindow::reflectedSearch_close ( bool toplevel )
{
   ui->actionSearch->setChecked(toplevel);
}

void MainWindow::on_action_About_Nesicide_triggered()
{
   AboutDialog* dlg = new AboutDialog(this);
   dlg->exec();
   delete dlg;
}

void MainWindow::closeProject()
{
   int tab;

   if (nesicideProject->isDirty())
   {
      int result = QMessageBox::question(this,"Save Project?","Your project settings, project content, or debugger content has changed, would you like to save the project?",QMessageBox::Yes,QMessageBox::No);
      if ( result == QMessageBox::Yes )
      {
         on_actionSave_Project_triggered();
      }
   }

   // Close all inspectors
   CDockWidgetRegistry::hideAll();

   m_pSourceNavigator->shutdown();

   // Stop the emulator if it is running
   emulator->pauseEmulation(false);

   // Now save the emulator state if a save state file is specified.
   if ( !nesicideProject->getProjectCartridgeSaveStateName().isEmpty() )
   {
      saveEmulatorState(nesicideProject->getProjectCartridgeSaveStateName());
   }

   // Terminate the project and let the IDE know
   projectBrowser->disableNavigation();

   // Try to close all opened editors
   for ( tab = 0; tab < ui->tabWidget->count(); tab++ )
   {
      ICenterWidgetItem* item = dynamic_cast<ICenterWidgetItem*>(ui->tabWidget->widget(tab));
      if ( item )
      {
         ui->tabWidget->setCurrentWidget(ui->tabWidget->widget(tab));
         if ( item->onSaveQuery() )
         {
            item->onSave();
         }
         ui->tabWidget->removeTab(tab);
      }
   }

   CCC65Interface::clear();

   nesicideProject->terminateProject();

   emulator->primeEmulator();
   emulator->resetEmulator();

   if ( EnvironmentSettingsDialog::showWelcomeOnStart() )
   {
      ui->tabWidget->addTab(ui->tab,"Welcome Page");
      ui->webView->setUrl(QUrl( "http://wiki.nesicide.com/doku.php?id=nesicide_user_manual"));
   }

   // Clear output
   output->clearAllPanes();
   output->hide();

   projectFileName = "";

   // Let the UI know what's up
   projectDataChangesEvent();
}

void MainWindow::on_action_Close_Project_triggered()
{
   QSettings settings;

   settings.setValue("LastProject","");

   closeProject();
}

void MainWindow::on_actionNTSC_triggered()
{
   EmulatorPrefsDialog::setTVStandard(MODE_NTSC);
   ui->actionNTSC->setChecked(true);
   ui->actionPAL->setChecked(false);
   nesSetSystemMode(MODE_NTSC);

   emulator->resetEmulator();
   emulator->startEmulation();
}

void MainWindow::on_actionPAL_triggered()
{
   EmulatorPrefsDialog::setTVStandard(MODE_PAL);
   ui->actionNTSC->setChecked(false);
   ui->actionPAL->setChecked(true);
   nesSetSystemMode(MODE_PAL);

   emulator->resetEmulator();
   emulator->startEmulation();
}

void MainWindow::on_actionDelta_Modulation_toggled(bool value)
{
   EmulatorPrefsDialog::setDMCEnabled(value);
   if ( value )
   {
      ui->actionMute_All->setChecked(false);
      nesSetAudioChannelMask(nesGetAudioChannelMask()|0x10);
   }
   else
   {
      nesSetAudioChannelMask(nesGetAudioChannelMask()&(~0x10));
   }
}

void MainWindow::on_actionNoise_toggled(bool value)
{
   EmulatorPrefsDialog::setNoiseEnabled(value);
   if ( value )
   {
      ui->actionMute_All->setChecked(false);
      nesSetAudioChannelMask(nesGetAudioChannelMask()|0x08);
   }
   else
   {
      nesSetAudioChannelMask(nesGetAudioChannelMask()&(~0x08));
   }
}

void MainWindow::on_actionTriangle_toggled(bool value)
{
   EmulatorPrefsDialog::setTriangleEnabled(value);
   if ( value )
   {
      ui->actionMute_All->setChecked(false);
      nesSetAudioChannelMask(nesGetAudioChannelMask()|0x04);
   }
   else
   {
      nesSetAudioChannelMask(nesGetAudioChannelMask()&(~0x04));
   }
}

void MainWindow::on_actionSquare_2_toggled(bool value)
{
   EmulatorPrefsDialog::setSquare2Enabled(value);
   if ( value )
   {
      ui->actionMute_All->setChecked(false);
      nesSetAudioChannelMask(nesGetAudioChannelMask()|0x02);
   }
   else
   {
      nesSetAudioChannelMask(nesGetAudioChannelMask()&(~0x02));
   }
}

void MainWindow::on_actionSquare_1_toggled(bool value)
{
   EmulatorPrefsDialog::setSquare1Enabled(value);
   if ( value )
   {
      ui->actionMute_All->setChecked(false);
      nesSetAudioChannelMask(nesGetAudioChannelMask()|0x01);
   }
   else
   {
      nesSetAudioChannelMask(nesGetAudioChannelMask()&(~0x01));
   }
}

void MainWindow::on_actionMute_All_toggled(bool value)
{
   EmulatorPrefsDialog::setSquare1Enabled(!value);
   EmulatorPrefsDialog::setSquare2Enabled(!value);
   EmulatorPrefsDialog::setTriangleEnabled(!value);
   EmulatorPrefsDialog::setNoiseEnabled(!value);
   EmulatorPrefsDialog::setDMCEnabled(!value);
   ui->actionSquare_1->setChecked(!value);
   ui->actionSquare_2->setChecked(!value);
   ui->actionTriangle->setChecked(!value);
   ui->actionNoise->setChecked(!value);
   ui->actionDelta_Modulation->setChecked(!value);

   if ( value )
   {
      nesSetAudioChannelMask(nesGetAudioChannelMask()&(~0x1F));
   }
   else
   {
      nesSetAudioChannelMask(nesGetAudioChannelMask()|0x1F);
   }
}

void MainWindow::on_actionEnvironment_Settings_triggered()
{
   EnvironmentSettingsDialog dlg;
   int tab;

   dlg.exec();

   // Update any open document windows.
   for ( tab = 0; tab < ui->tabWidget->count(); tab++ )
   {
      CodeEditorForm* codeEditor = dynamic_cast<CodeEditorForm*>(ui->tabWidget->widget(tab));
      if ( codeEditor )
      {
         codeEditor->restyleText();
      }
   }
}

void MainWindow::on_actionPreferences_triggered()
{
   EmulatorPrefsDialog dlg;

   dlg.exec();

   // Synchronize UI elements with changes.
   // Set TV standard to use.
   int systemMode = EmulatorPrefsDialog::getTVStandard();
   ui->actionNTSC->setChecked(systemMode==MODE_NTSC);
   ui->actionPAL->setChecked(systemMode==MODE_PAL);
   nesSetSystemMode(systemMode);

   bool square1 = EmulatorPrefsDialog::getSquare1Enabled();
   bool square2 = EmulatorPrefsDialog::getSquare2Enabled();
   bool triangle = EmulatorPrefsDialog::getTriangleEnabled();
   bool noise = EmulatorPrefsDialog::getNoiseEnabled();
   bool dmc = EmulatorPrefsDialog::getDMCEnabled();
   int mask = ((square1<<0)|(square2<<1)|(triangle<<2)|(noise<<3)|(dmc<<4));

   if ( !(square1|square2|triangle|noise|dmc) )
   {
      ui->actionMute_All->setChecked(true);
      nesSetAudioChannelMask(nesGetAudioChannelMask()&(~0x1F));
   }
   else
   {
      ui->actionMute_All->setChecked(false);
      nesSetAudioChannelMask(nesGetAudioChannelMask()|mask);
   }
   ui->actionSquare_1->setChecked(square1);
   ui->actionSquare_2->setChecked(square2);
   ui->actionTriangle->setChecked(triangle);
   ui->actionNoise->setChecked(noise);
   ui->actionDelta_Modulation->setChecked(dmc);

   if ( EmulatorPrefsDialog::videoSettingsChanged() )
   {
      if ( EmulatorPrefsDialog::getScalingFactor() > 1 )
      {
         m_pEmulator->setFloating(true);
      }
      m_pEmulator->resize((EmulatorPrefsDialog::getScalingFactor()*256)+2,(EmulatorPrefsDialog::getScalingFactor()*240)+2);
   }

   if ( EmulatorPrefsDialog::controllerSettingsChanged() )
   {
      // Set up controllers.
      nesSetControllerType(0,EmulatorPrefsDialog::getControllerType(0));
      nesSetControllerSpecial(0,EmulatorPrefsDialog::getControllerSpecial(0));
      nesSetControllerType(1,EmulatorPrefsDialog::getControllerType(1));
      nesSetControllerSpecial(1,EmulatorPrefsDialog::getControllerSpecial(1));
   }
}

void MainWindow::on_actionOnline_Help_triggered()
{
   ui->tabWidget->addTab(ui->tab,"Welcome Page");
   ui->webView->setUrl(QUrl( "http://wiki.nesicide.com/doku.php?id=nesicide_user_manual"));
}

void MainWindow::on_actionLoad_In_Emulator_triggered()
{
   if ( compiler->assembledOk() )
   {
      buildTextLogger->write("<b>Loading ROM...</b>");

      if ( !(CCC65Interface::captureINESImage() && CCC65Interface::captureDebugInfo()) )
      {
         buildTextLogger->write("<font color='red'><b>Load failed.</b></font>");
         return;
      }

      emulator->primeEmulator();
      emulator->resetEmulator();
      emulator->pauseEmulation(true);

      buildTextLogger->write("<b>Load complete.</b>");

      ui->actionEmulation_Window->setChecked(true);
      on_actionEmulation_Window_toggled(true);
   }
}

void MainWindow::on_actionRun_Test_Suite_triggered()
{
   testSuiteExecutive->show();
}

void MainWindow::on_actionE_xit_triggered()
{
}

void MainWindow::on_actionClean_Project_triggered()
{
   output->showPane(OutputPaneDockWidget::Output_Build);
   compiler->clean();
}

void MainWindow::openFile(QString file)
{
   QDir dir(QDir::currentPath());
   QString fileName = dir.fromNativeSeparators(dir.filePath(file));
   QFile fileIn(fileName);

   if ( fileIn.exists() && fileIn.open(QIODevice::ReadOnly|QIODevice::Text) )
   {
      CodeEditorForm* editor = new CodeEditorForm(fileName,QString(fileIn.readAll()));

      fileIn.close();

      ui->tabWidget->addTab(editor, fileName);
      ui->tabWidget->setCurrentWidget(editor);
   }
   else
   {
      // CPTODO: fail silently?
   }
}

void MainWindow::on_actionFullscreen_toggled(bool value)
{
   if ( value )
   {
      m_bEmulatorFloating = m_pEmulator->isFloating();
      m_pEmulator->setFloating(true);
      m_pEmulator->showFullScreen();
   }
   else
   {
      m_pEmulator->showNormal();
      m_pEmulator->setFloating(m_bEmulatorFloating);
   }
}
