#include "codebrowserdockwidget.h"
#include "ui_codebrowserdockwidget.h"

#include "emulator_core.h"

#include "cmarker.h"
#include "cdockwidgetregistry.h"

#include "main.h"

#include <QMessageBox>

CodeBrowserDockWidget::CodeBrowserDockWidget(QWidget *parent) :
    CDebuggerBase(parent),
    ui(new Ui::CodeBrowserDockWidget)
{
   ui->setupUi(this);
   assemblyViewModel = new CCodeBrowserDisplayModel(this);
   ui->tableView->setModel(assemblyViewModel);

   // Connect signals to the models to have the model update.
   QObject::connect ( emulator, SIGNAL(cartridgeLoaded()), assemblyViewModel, SLOT(update()));
   QObject::connect ( emulator, SIGNAL(emulatorReset()), assemblyViewModel, SLOT(update()) );
   QObject::connect ( emulator, SIGNAL(emulatorPaused(bool)), assemblyViewModel, SLOT(update()) );
   QObject::connect ( breakpointWatcher, SIGNAL(breakpointHit()), assemblyViewModel, SLOT(update()) );
   QObject::connect ( this, SIGNAL(breakpointsChanged()), assemblyViewModel, SLOT(update()) );

   // Connect signals to the UI to have the UI update.
   QObject::connect ( emulator, SIGNAL(cartridgeLoaded()), this, SLOT(cartridgeLoaded()) );
   QObject::connect ( emulator, SIGNAL(emulatorReset()), this, SLOT(cartridgeLoaded()) );
   QObject::connect ( breakpointWatcher, SIGNAL(breakpointHit()), this, SLOT(breakpointHit()) );
}

CodeBrowserDockWidget::~CodeBrowserDockWidget()
{
   delete ui;
   delete assemblyViewModel;
}

void CodeBrowserDockWidget::showEvent(QShowEvent* e)
{
   QDockWidget* breakpointInspector = dynamic_cast<QDockWidget*>(CDockWidgetRegistry::getWidget("Breakpoints"));
   QDockWidget* executionVisualizer = dynamic_cast<QDockWidget*>(CDockWidgetRegistry::getWidget("Execution Visualizer"));

   // Specifically not connecting to updateDebuggers signal here since it doesn't make much sense to
   // update the code position until a pause/breakpoint.
   QObject::connect ( emulator, SIGNAL(emulatorPaused(bool)), this, SLOT(emulatorPaused(bool)) );

   QObject::connect ( breakpointInspector, SIGNAL(breakpointsChanged()), assemblyViewModel, SLOT(update()) );
   QObject::connect ( executionVisualizer, SIGNAL(snapTo(QString)), this, SLOT(snapTo(QString)) );

   ui->tableView->setCurrentIndex(assemblyViewModel->index(nesGetSLOCFromAddress(nesGetCPUProgramCounterOfLastSync()),0));
   ui->tableView->scrollTo(ui->tableView->currentIndex());
}

void CodeBrowserDockWidget::hideEvent(QHideEvent* e)
{
   QObject::disconnect ( emulator, SIGNAL(updateDebuggers()), assemblyViewModel, SLOT(update()));
   QObject::disconnect ( emulator, SIGNAL(emulatorPaused(bool)), this, SLOT(emulatorPaused(bool)) );
}

void CodeBrowserDockWidget::contextMenuEvent(QContextMenuEvent* e)
{
   CBreakpointInfo* pBreakpoints = nesGetBreakpointDatabase();
   QMenu menu;
   int bp;
   int addr = 0;
   int absAddr = 0;
   QModelIndex index = ui->tableView->currentIndex();

   addr = nesGetAddressFromSLOC(index.row());

   absAddr = nesGetAbsoluteAddressFromAddress(addr);

   if ( addr != -1 )
   {
      bp = pBreakpoints->FindExactMatch ( eBreakOnCPUExecution,
                                          eBreakpointItemAddress,
                                          0,
                                          addr,
                                          absAddr,
                                          addr,
                                          eBreakpointConditionNone,
                                          0,
                                          eBreakpointDataNone,
                                          0 );

      // Build context menu...
      menu.addAction(ui->actionRun_to_here);
      menu.addSeparator();

      // If breakpoint isn't set here, give menu options to set one...
      if ( bp < 0 )
      {
         menu.addAction(ui->actionBreak_on_CPU_execution_here);
      }
      else
      {
         if ( pBreakpoints->GetStatus(bp) == Breakpoint_Disabled )
         {
            menu.addAction(ui->actionEnable_breakpoint);
            menu.addAction(ui->actionRemove_breakpoint);
         }
         else
         {
            menu.addAction(ui->actionDisable_breakpoint);
            menu.addAction(ui->actionRemove_breakpoint);
         }
      }

      menu.addSeparator();
      menu.addAction(ui->actionClear_marker);
      menu.addSeparator();

      menu.addAction(ui->actionStart_marker_here);
      menu.addAction(ui->actionEnd_marker_here);

      // Run the context menu...
      // CPTODO: Hokey trick to provide the breakpoint-of-interest to action handlers...
      m_breakpointIndex = bp;

      menu.exec(e->globalPos());

      emit breakpointsChanged();
      emit markProjectDirty(true);
   }
}

void CodeBrowserDockWidget::changeEvent(QEvent* e)
{
   CDebuggerBase::changeEvent(e);

   switch (e->type())
   {
      case QEvent::LanguageChange:
         ui->retranslateUi(this);
         break;
      default:
         break;
   }
}

void CodeBrowserDockWidget::snapTo(QString item)
{
   uint32_t addr;

   // Make sure item is something we care about
   if ( item.startsWith("Address:") )
   {
      QStringList splits;
      splits = item.split(QRegExp("[:()]"));
      if ( splits.count() == 5 )
      {
         addr = splits.at(3).toInt(NULL,16);

         ui->tableView->setCurrentIndex(assemblyViewModel->index(nesGetSLOCFromAddress(addr),0));
         ui->tableView->scrollTo(ui->tableView->currentIndex());
         ui->tableView->resizeColumnsToContents();
      }
   }
}

void CodeBrowserDockWidget::breakpointHit()
{
   show();
   ui->tableView->setCurrentIndex(assemblyViewModel->index(nesGetSLOCFromAddress(nesGetCPUProgramCounterOfLastSync()),0));
   ui->tableView->scrollTo(ui->tableView->currentIndex());
   ui->tableView->resizeColumnsToContents();
}

void CodeBrowserDockWidget::emulatorPaused(bool showMe)
{
   if ( showMe )
   {
      show();
   }
   ui->tableView->setCurrentIndex(assemblyViewModel->index(nesGetSLOCFromAddress(nesGetCPUProgramCounterOfLastSync()),0));
   ui->tableView->scrollTo(ui->tableView->currentIndex());
   ui->tableView->resizeColumnsToContents();
}

void CodeBrowserDockWidget::cartridgeLoaded()
{
   if ( nesROMIsLoaded() )
   {
      show();
   }
   ui->tableView->setCurrentIndex(assemblyViewModel->index(nesGetSLOCFromAddress(nesGetCPUProgramCounterOfLastSync()),0));
   ui->tableView->scrollTo(ui->tableView->currentIndex());
   ui->tableView->resizeColumnsToContents();
}

void CodeBrowserDockWidget::on_actionBreak_on_CPU_execution_here_triggered()
{
   CBreakpointInfo* pBreakpoints = nesGetBreakpointDatabase();
   QModelIndex index = ui->tableView->currentIndex();
   int bpIdx;
   int addr = 0;
   int absAddr = 0;

   addr = nesGetAddressFromSLOC(index.row());

   absAddr = nesGetAbsoluteAddressFromAddress(addr);

   if ( addr != -1 )
   {
      bpIdx = pBreakpoints->AddBreakpoint ( eBreakOnCPUExecution,
                                            eBreakpointItemAddress,
                                            0,
                                            addr,
                                            absAddr,
                                            addr,
                                            eBreakpointConditionNone,
                                            0,
                                            eBreakpointDataNone,
                                            0,
                                            true );

      if ( bpIdx < 0 )
      {
         QString str;
         str.sprintf("Cannot add breakpoint, maximum of %d already used.", NUM_BREAKPOINTS);
         QMessageBox::information(0, "Error", str);
      }
      else
      {
         emit breakpointsChanged();
         emit markProjectDirty(true);
      }
   }
}

void CodeBrowserDockWidget::on_actionRun_to_here_triggered()
{
   QModelIndex index = ui->tableView->currentIndex();
   int addr = 0;
   int absAddr = 0;

   addr = nesGetAddressFromSLOC(index.row());

   absAddr = nesGetAbsoluteAddressFromAddress(addr);

   if ( addr != -1 )
   {
      nesSetGotoAddress(addr);
   }// CPTODO: fix the goto for absolute
}

void CodeBrowserDockWidget::on_actionDisable_breakpoint_triggered()
{
   CBreakpointInfo* pBreakpoints = nesGetBreakpointDatabase();

   if ( m_breakpointIndex >= 0 )
   {
      pBreakpoints->ToggleEnabled(m_breakpointIndex);

      emit breakpointsChanged();
      emit markProjectDirty(true);
   }
}

void CodeBrowserDockWidget::on_actionRemove_breakpoint_triggered()
{
   CBreakpointInfo* pBreakpoints = nesGetBreakpointDatabase();

   if ( m_breakpointIndex >= 0 )
   {
      pBreakpoints->RemoveBreakpoint(m_breakpointIndex);

      emit breakpointsChanged();
      emit markProjectDirty(true);
   }
}

void CodeBrowserDockWidget::on_actionEnable_breakpoint_triggered()
{
   CBreakpointInfo* pBreakpoints = nesGetBreakpointDatabase();

   if ( m_breakpointIndex >= 0 )
   {
      pBreakpoints->ToggleEnabled(m_breakpointIndex);

      emit breakpointsChanged();
      emit markProjectDirty(true);
   }
}

void CodeBrowserDockWidget::on_actionStart_marker_here_triggered()
{
   CMarker* markers = nesGetExecutionMarkerDatabase();
   int marker;
   int addr = 0;
   QModelIndex index = ui->tableView->currentIndex();

   if ( index.isValid() )
   {
      addr = nesGetAddressFromSLOC(index.row());

      if ( addr != -1 )
      {
         // Find unused Marker entry...
         marker = markers->AddMarker(addr,nesGetAbsoluteAddressFromAddress(addr));

         emit breakpointsChanged();
         emit markProjectDirty(true);
      }
   }
}

void CodeBrowserDockWidget::on_actionEnd_marker_here_triggered()
{
   CMarker* markers = nesGetExecutionMarkerDatabase();
   int marker = markers->FindInProgressMarker();
   int addr = 0;
   QModelIndex index = ui->tableView->currentIndex();

   if ( marker >= 0 )
   {
      addr = nesGetAddressFromSLOC(index.row());

      if ( addr != -1 )
      {
         markers->CompleteMarker(marker,addr,nesGetAbsoluteAddressFromAddress(addr));

         emit breakpointsChanged();
         emit markProjectDirty(true);
      }
   }
}

void CodeBrowserDockWidget::on_actionClear_marker_triggered()
{
   CMarker* markers = nesGetExecutionMarkerDatabase();
   markers->ClearAllMarkers();

   emit breakpointsChanged();
   emit markProjectDirty(true);
}

void CodeBrowserDockWidget::on_tableView_pressed(QModelIndex index)
{
   CBreakpointInfo* pBreakpoints = nesGetBreakpointDatabase();
   int bp;
   int addr = 0;
   int absAddr = 0;

   if ( QApplication::mouseButtons() == Qt::LeftButton )
   {
      if ( index.isValid() && index.column() == 0 )
      {
         addr = nesGetAddressFromSLOC(index.row());

         absAddr = nesGetAbsoluteAddressFromAddress(addr);

         if ( addr != -1 )
         {
            bp = pBreakpoints->FindExactMatch ( eBreakOnCPUExecution,
                                                eBreakpointItemAddress,
                                                0,
                                                addr,
                                                absAddr,
                                                addr,
                                                eBreakpointConditionNone,
                                                0,
                                                eBreakpointDataNone,
                                                0 );

            if ( bp < 0 )
            {
               on_actionBreak_on_CPU_execution_here_triggered();
            }
            else
            {
               if ( pBreakpoints->GetStatus(bp) == Breakpoint_Disabled )
               {
                  pBreakpoints->RemoveBreakpoint(bp);
               }
               else
               {
                  pBreakpoints->SetEnabled(bp,false);
               }
            }

            emit breakpointsChanged();
            emit markProjectDirty(true);
         }
      }
   }
}
