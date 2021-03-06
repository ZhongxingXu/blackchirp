#include "chirptablemodel.h"

#include <QDoubleSpinBox>
#include <QSettings>
#include <QApplication>

ChirpTableModel::ChirpTableModel(QObject *parent)
    : QAbstractTableModel(parent)
{

}

ChirpTableModel::~ChirpTableModel()
{

}



int ChirpTableModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)
    return d_segmentList.size();
}

int ChirpTableModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)
    return 3;
}

QVariant ChirpTableModel::data(const QModelIndex &index, int role) const
{
    if(index.row() >= d_segmentList.size())
        return QVariant();

    if(role == Qt::TextAlignmentRole)
        return Qt::AlignCenter;

    if(role == Qt::DisplayRole)
    {
        switch(index.column()) {
        case 0:
            return QString::number(d_segmentList.at(index.row()).startFreqMHz,'f',3);
            break;
        case 1:
            return QString::number(d_segmentList.at(index.row()).endFreqMHz,'f',3);
            break;
        case 2:
            return QString::number(d_segmentList.at(index.row()).durationUs*1e3,'f',1);
            break;
        default:
            return QVariant();
            break;
        }
    }
    else if(role == Qt::EditRole)
    {
        switch(index.column()) {
        case 0:
            return d_segmentList.at(index.row()).startFreqMHz;
            break;
        case 1:
            return d_segmentList.at(index.row()).endFreqMHz;
            break;
        case 2:
            return d_segmentList.at(index.row()).durationUs*1e3;
            break;
        default:
            return QVariant();
            break;
        }
    }

    return QVariant();
}

QVariant ChirpTableModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if(role == Qt::DisplayRole)
    {
        if(orientation == Qt::Vertical)
            return section + 1;
        else
        {
            switch(section) {
            case 0:
                return QString("f Start (MHz)");
                break;
            case 1:
                return QString("f End (MHz)");
                break;
            case 2:
                return QString("Duration (ns)");
                break;
            default:
                return QVariant();
                break;
            }
        }
    }
    else if(role == Qt::ToolTipRole)
    {
        if(orientation == Qt::Vertical)
            return QVariant();

        switch(section) {
        case 0:
            return QString("Starting frequency for the chirp segment (in MHz).");
            break;
        case 1:
            return QString("Ending frequency for the chirp segment (in MHz).");
            break;
        case 2:
            return QString("Duration of the chirp (in ns)");
            break;
        default:
            return QVariant();
            break;
        }

    }

    return QVariant();
}

bool ChirpTableModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if(role != Qt::EditRole)
        return false;

    if(index.row() >= d_segmentList.size() || index.column() > 3)
        return false;

    bool ok = false;
    double newVal = value.toDouble(&ok);

    if(!ok)
        return false;

    switch (index.column()) {
    case 0:
        d_segmentList[index.row()].startFreqMHz = newVal;
        break;
    case 1:
        d_segmentList[index.row()].endFreqMHz = newVal;
        break;
    case 2:
        d_segmentList[index.row()].durationUs = newVal/1e3;
        break;
    default:
        return false;
        break;
    }

    d_segmentList[index.row()].alphaUs = (d_segmentList.at(index.row()).endFreqMHz - d_segmentList.at(index.row()).startFreqMHz)/d_segmentList.at(index.row()).durationUs;
    emit dataChanged(index,index);
    emit modelChanged();
    return true;
}

bool ChirpTableModel::removeRows(int row, int count, const QModelIndex &parent)
{
    if(row < 0 || row+count > d_segmentList.size() || d_segmentList.isEmpty())
        return false;

    beginRemoveRows(parent,row,row+count-1);
    for(int i=0; i<count; i++)
        d_segmentList.removeAt(row);
    endRemoveRows();

    emit modelChanged();
    return true;
}

Qt::ItemFlags ChirpTableModel::flags(const QModelIndex &index) const
{
    if(index.row() < d_segmentList.size())
        return Qt::ItemIsEnabled|Qt::ItemIsSelectable|Qt::ItemIsEditable;

    return Qt::ItemIsEnabled|Qt::ItemIsSelectable;
}

void ChirpTableModel::addSegment(double start, double end, double dur, int pos)
{
    BlackChirp::ChirpSegment cs;
    cs.startFreqMHz = start;
    cs.endFreqMHz = end;
    cs.durationUs = dur;
    cs.alphaUs = (end-start)/dur;

    if(pos < 0 || pos >= d_segmentList.size())
    {
        beginInsertRows(QModelIndex(),d_segmentList.size(),d_segmentList.size());
        d_segmentList.append(cs);
    }
    else
    {
        beginInsertRows(QModelIndex(),pos,pos);
        d_segmentList.insert(pos,cs);
    }
    endInsertRows();
    emit modelChanged();
}

void ChirpTableModel::moveSegments(int first, int last, int delta)
{
    //make sure all movement is within valid ranges
    if(first + delta < 0 || last + delta >= d_segmentList.size())
        return;

    //this bit of code is not intuitive! read docs on QAbstractItemModel::beginMoveRows() carefully!
    if(delta>0)
    {
        if(!beginMoveRows(QModelIndex(),first,last,QModelIndex(),last+2))
            return;
    }
    else
    {
        if(!beginMoveRows(QModelIndex(),first,last,QModelIndex(),first-1))
            return;
    }

    QList<BlackChirp::ChirpSegment> chunk = d_segmentList.mid(first,last-first+1);

    //remove selected rows
    for(int i=0; i<last-first+1; i++)
        d_segmentList.removeAt(first);

    //insert rows at their new location
    for(int i = chunk.size(); i>0; i--)
    {
        if(delta>0)
            d_segmentList.insert(first+1,chunk.at(i-1));
        else
            d_segmentList.insert(first-1,chunk.at(i-1));
    }
    endMoveRows();

    emit modelChanged();
}

void ChirpTableModel::removeSegments(QList<int> rows)
{
    qSort(rows);
    for(int i=rows.size(); i>0; i--)
        removeRows(rows.at(i-1),1,QModelIndex());
}

QList<BlackChirp::ChirpSegment> ChirpTableModel::segmentList() const
{
    return d_segmentList;
}


ChirpDoubleSpinBoxDelegate::ChirpDoubleSpinBoxDelegate(QObject *parent) : QStyledItemDelegate(parent)
{
}

QWidget *ChirpDoubleSpinBoxDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    Q_UNUSED(option)

    QDoubleSpinBox *editor = new QDoubleSpinBox(parent);

    QSettings s(QSettings::SystemScope,QApplication::organizationName(),QApplication::applicationName());
    double chirpMin = s.value(QString("rfConfig/chirpMin"),26500.0).toDouble();
    double chirpMax = s.value(QString("rfConfig/chirpMax"),40000.0).toDouble();

    switch(index.column())
    {
    case 0:
    case 1:
        editor->setMinimum(chirpMin);
        editor->setMaximum(chirpMax);
        editor->setDecimals(3);
        break;
    case 2:
        editor->setMinimum(0.1);
        editor->setMaximum(100000.0);
        editor->setSingleStep(10.0);
        editor->setDecimals(1);
        break;
    default:
        break;
    }

    return editor;
}

void ChirpDoubleSpinBoxDelegate::setEditorData(QWidget *editor, const QModelIndex &index) const
{
    double val = index.model()->data(index, Qt::EditRole).toDouble();

    static_cast<QDoubleSpinBox*>(editor)->setValue(val);
}

void ChirpDoubleSpinBoxDelegate::setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const
{
    QDoubleSpinBox *sb = static_cast<QDoubleSpinBox*>(editor);
    sb->interpretText();
    model->setData(index,sb->value(),Qt::EditRole);
}

void ChirpDoubleSpinBoxDelegate::updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    Q_UNUSED(index)
    editor->setGeometry(option.rect);
}
