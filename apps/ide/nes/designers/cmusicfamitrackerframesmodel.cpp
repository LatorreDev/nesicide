#include "cmusicfamitrackerframesmodel.h"

#include "stdio.h"

static char modelStringBuffer [ 16 ];

CMusicFamiTrackerFramesModel::CMusicFamiTrackerFramesModel(CFamiTrackerDoc* pDoc,QObject*)
{
   m_pDocument = pDoc;
}

CMusicFamiTrackerFramesModel::~CMusicFamiTrackerFramesModel()
{
}

QVariant CMusicFamiTrackerFramesModel::data(const QModelIndex& index, int role) const
{
   if (!index.isValid())
   {
      return QVariant();
   }

   if (role != Qt::DisplayRole)
   {
      return QVariant();
   }

   sprintf(modelStringBuffer,"%02X",m_pDocument->GetPatternAtFrame(index.row(),index.column()));

   return QVariant(modelStringBuffer);
}

Qt::ItemFlags CMusicFamiTrackerFramesModel::flags(const QModelIndex& /*index*/) const
{
   Qt::ItemFlags flags = Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsEditable;

   return flags;
}

QVariant CMusicFamiTrackerFramesModel::headerData(int section, Qt::Orientation orientation, int role) const
{
   if (role != Qt::DisplayRole)
   {
      return QVariant();
   }

   if ( orientation == Qt::Vertical )
   {
      sprintf ( modelStringBuffer, "%02X", section );
   }
   else
   {
      return QVariant();
   }

   return QVariant(modelStringBuffer);
}

bool CMusicFamiTrackerFramesModel::setData ( const QModelIndex& index, const QVariant& value, int )
{
   unsigned int data;
   bool ok = false;

   data = value.toString().toInt(&ok,16);

   if ( ok )
   {
      m_pDocument->SetPatternAtFrame(index.row(),index.column(),data);
      emit dataChanged(index,index);
   }

   return ok;
}

QModelIndex CMusicFamiTrackerFramesModel::index(int row, int column, const QModelIndex&) const
{
   if ( (row >= 0) && (column >= 0) )
   {
      // CPTODO: return orders row data here when music data is ported.
      return createIndex(row,column,row);
   }

   return QModelIndex();
}

int CMusicFamiTrackerFramesModel::rowCount(const QModelIndex&) const
{
   return m_pDocument->GetFrameCount();
}

int CMusicFamiTrackerFramesModel::columnCount(const QModelIndex& /*parent*/) const
{
   return m_pDocument->GetChannelCount();
}

void CMusicFamiTrackerFramesModel::update()
{
   emit layoutChanged();
//   emit dataChanged(QModelIndex(),QModelIndex());
}
