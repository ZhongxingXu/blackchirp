#include "experiment.h"

#include <QSettings>
#include <QApplication>
#include <QDir>
#include <QFile>
#include <QTextStream>

Experiment::Experiment() : data(new ExperimentData)
{

}

Experiment::Experiment(const Experiment &rhs) : data(rhs.data)
{

}

Experiment &Experiment::operator=(const Experiment &rhs)
{
    if (this != &rhs)
        data.operator=(rhs.data);
    return *this;
}

Experiment::~Experiment()
{

}

int Experiment::number() const
{
    return data->number;
}

QDateTime Experiment::startTime() const
{
    return data->startTime;
}

int Experiment::timeDataInterval() const
{
    return data->timeDataInterval;
}

int Experiment::autoSaveShots() const
{
    return data->autoSaveShotsInterval;
}

bool Experiment::isInitialized() const
{
    return data->isInitialized;
}

bool Experiment::isAborted() const
{
    return data->isAborted;
}

bool Experiment::isDummy() const
{
    return data->isDummy;
}

bool Experiment::isLifWaiting() const
{
    return data->waitForLifSet;
}

FtmwConfig Experiment::ftmwConfig() const
{
    return data->ftmwCfg;
}

PulseGenConfig Experiment::pGenConfig() const
{
    return data->pGenCfg;
}

FlowConfig Experiment::flowConfig() const
{
    return data->flowCfg;
}

LifConfig Experiment::lifConfig() const
{
    return data->lifCfg;
}

IOBoardConfig Experiment::iobConfig() const
{
    return data->iobCfg;
}

bool Experiment::isComplete() const
{
    //check each sub expriment!
    return (data->ftmwCfg.isComplete() && data->lifCfg.isComplete());
}

bool Experiment::hardwareSuccess() const
{
    return data->hardwareSuccess;
}

QString Experiment::errorString() const
{
    return data->errorString;
}

QMap<QString, QList<QPair<QVariant, bool> > > Experiment::timeDataMap() const
{
    return data->timeDataMap;
}

QString Experiment::startLogMessage() const
{
    return data->startLogMessage;
}

QString Experiment::endLogMessage() const
{
    return data->endLogMessage;
}

BlackChirp::LogMessageCode Experiment::endLogMessageCode() const
{
    return data->endLogMessageCode;
}

QMap<QString, QPair<QVariant, QString> > Experiment::headerMap() const
{
    QMap<QString, QPair<QVariant, QString> > out;

    out.insert(QString("AuxDataInterval"),qMakePair(data->timeDataInterval,QString("s")));
    out.insert(QString("AutosaveInterval"),qMakePair(data->autoSaveShotsInterval,QString("shots")));

    auto it = data->validationConditions.constBegin();
    QString prefix("Validation.");
    QString empty("");
    for(;it != data->validationConditions.constEnd(); it++)
    {
        out.insert(prefix+it.key()+QString(".Min"),qMakePair(it.value().min,empty));
        out.insert(prefix+it.key()+QString(".Max"),qMakePair(it.value().max,empty));
    }

    out.unite(data->ftmwCfg.headerMap());
    out.unite(data->lifCfg.headerMap());
    out.unite(data->pGenCfg.headerMap());
    out.unite(data->flowCfg.headerMap());
    out.unite(data->iobCfg.headerMap());

    return out;
}

bool Experiment::snapshotReady()
{
    if(isComplete())
        return false;

    if(ftmwConfig().isEnabled())
    {
        if(ftmwConfig().completedShots() > 0)
        {
            qint64 d = ftmwConfig().completedShots() - data->lastSnapshot;
            if(d > 0)
            {
                bool out = !(d % static_cast<qint64>(data->autoSaveShotsInterval));
                if(out)
                    data->lastSnapshot = ftmwConfig().completedShots();
                return out;
            }
            else
                return false;
        }
    }
    else if(lifConfig().isEnabled())
    {
        if(lifConfig().completedShots() > 0)
        {
            qint64 d = lifConfig().completedShots() - data->lastSnapshot;
            if(d > 0)
            {
                bool out = !(d % static_cast<qint64>(data->autoSaveShotsInterval));
                if(out)
                    data->lastSnapshot = lifConfig().completedShots();
                return out;
            }
            else
                return false;
        }
    }

    return false;
}

void Experiment::setTimeDataInterval(const int t)
{
    data->timeDataInterval = t;
}

void Experiment::setAutoSaveShotsInterval(const int s)
{
    data->autoSaveShotsInterval = s;
}

void Experiment::setInitialized()
{
    bool initSuccess = true;
    data->startTime = QDateTime::currentDateTime();

    if(data->ftmwCfg.isEnabled())
    {
        if(!data->ftmwCfg.prepareForAcquisition())
        {
            setErrorString(data->ftmwCfg.errorString());
            data->isInitialized = false;
            return;
        }
    }

    QSettings s(QSettings::SystemScope,QApplication::organizationName(),QApplication::applicationName());
    int num = s.value(QString("exptNum"),0).toInt()+1;
    data->number = num;

    if(ftmwConfig().isEnabled() && ftmwConfig().type() == BlackChirp::FtmwPeakUp)
    {
        data->startLogMessage = QString("Peak up mode started.");
        data->endLogMessage = QString("Peak up mode ended.");
    }
    else
    {
        data->startLogMessage = QString("Starting experiment %1.").arg(num);
        data->endLogMessage = QString("Experiment %1 complete.").arg(num);
    }

    QDir d(BlackChirp::getExptDir(num));
    if(!d.exists())
    {
        initSuccess = d.mkpath(d.absolutePath());
        if(!initSuccess)
        {
            data->isInitialized = false;
            data->errorString = QString("Could not create the directory %1 for saving.").arg(d.absolutePath());
            return;
        }
    }

    //write headers; chirps, etc
    //scan header
    if(!saveHeader())
    {
        data->isInitialized = false;
        data->errorString = QString("Could not open the file %1 for writing.")
                .arg(BlackChirp::getExptFile(data->number,BlackChirp::HeaderFile));
        return;
    }

    //chirp file
    if(data->ftmwCfg.isEnabled())
    {
        if(!saveChirpFile())
        {
            data->isInitialized = false;
            data->errorString = QString("Could not open the file %1 for writing.")
                    .arg(BlackChirp::getExptFile(num,BlackChirp::ChirpFile));
            return;
        }
    }


    data->isInitialized = initSuccess;

    if(initSuccess)
        s.setValue(QString("exptNum"),0); //FIXME

}

void Experiment::setAborted()
{
    data->isAborted = true;
    if(ftmwConfig().isEnabled() && (ftmwConfig().type() == BlackChirp::FtmwTargetShots || ftmwConfig().type() == BlackChirp::FtmwTargetTime ))
    {
        data->endLogMessage = QString("Experiment %1 aborted.").arg(number());
        data->endLogMessageCode = BlackChirp::LogError;
    }
    else if(ftmwConfig().isEnabled() && lifConfig().isEnabled() && !lifConfig().isComplete())
    {
        data->endLogMessage = QString("Experiment %1 aborted.").arg(number());
        data->endLogMessageCode = BlackChirp::LogError;
    }
}

void Experiment::setDummy()
{
    data->isDummy = true;
}

void Experiment::setLifWaiting(bool wait)
{
    data->waitForLifSet = wait;
}

void Experiment::setFtmwConfig(const FtmwConfig cfg)
{
    data->ftmwCfg = cfg;
}

void Experiment::setScopeConfig(const BlackChirp::FtmwScopeConfig &cfg)
{
    data->ftmwCfg.setScopeConfig(cfg);
}

void Experiment::setLifConfig(const LifConfig cfg)
{
    data->lifCfg = cfg;
}

void Experiment::setIOBoardConfig(const IOBoardConfig cfg)
{
    data->iobCfg = cfg;
}

bool Experiment::setFidsData(const QList<QVector<qint64> > l)
{
    if(!data->ftmwCfg.setFidsData(l))
    {
        setErrorString(ftmwConfig().errorString());
        return false;
    }

    return true;
}

bool Experiment::addFids(const QByteArray newData, int shift)
{
    if(!data->ftmwCfg.addFids(newData,shift))
    {
        setErrorString(ftmwConfig().errorString());
        return false;
    }

    return true;
}

bool Experiment::addLifWaveform(const LifTrace t)
{
    return data->lifCfg.addWaveform(t);
}

void Experiment::overrideTargetShots(const int target)
{
    data->ftmwCfg.setTargetShots(target);
}

void Experiment::resetFids()
{
    data->ftmwCfg.resetFids();
}

void Experiment::setPulseGenConfig(const PulseGenConfig c)
{
    data->pGenCfg = c;
}

void Experiment::setFlowConfig(const FlowConfig c)
{
    data->flowCfg = c;
}

void Experiment::setErrorString(const QString str)
{
    data->errorString = str;
}

bool Experiment::addTimeData(const QList<QPair<QString, QVariant> > dataList, bool plot)
{
    //return false if scan should be aborted
    bool out = true;
    for(int i=0; i<dataList.size(); i++)
    {
        QString key = dataList.at(i).first;
        QVariant value = dataList.at(i).second;

        if(data->timeDataMap.contains(key))
            data->timeDataMap[key].append(qMakePair(value,plot));
        else
        {
            QList<QPair<QVariant,bool>> newList;
            newList.append(qMakePair(value,plot));
            data->timeDataMap.insert(key,newList);
        }

        if(data->validationConditions.contains(key))
        {
            //convert key if needed
            QString name = BlackChirp::channelNameLookup(key);
            if(name.isEmpty())
                name = key;

            bool ok = false;
            double d = value.toDouble(&ok);

            if(!ok)
            {
                out = false;
                data->errorString = QString("Aborting because the item \"%1\" (value = %2) cannot be converted to a double.").arg(name).arg(value.toString());
                break;
            }
            else
            {
                const BlackChirp::ValidationItem &vi = data->validationConditions.value(key);
                if(d < vi.min || d > vi.max)
                {
                    out = false;
                    data->errorString = QString("Aborting because %1 is outside specified range (Value = %2, Min = %3, Max = %4).")
                            .arg(name).arg(d,0,'g').arg(vi.min,0,'g').arg(vi.max,0,'g');
                    break;
                }
            }
        }
    }

    return out;
}

void Experiment::addTimeStamp()
{
    QString key("exptTimeStamp");
    if(data->timeDataMap.contains(key))
        data->timeDataMap[key].append(qMakePair(QDateTime::currentDateTime(),false));
    else
    {
        QList<QPair<QVariant,bool>> newList;
        newList.append(qMakePair(QDateTime::currentDateTime(),false));
        data->timeDataMap.insert(key,newList);
    }
}

void Experiment::setValidationItems(const QMap<QString, BlackChirp::ValidationItem> m)
{
    data->validationConditions = m;
}

void Experiment::setHardwareFailed()
{
    data->hardwareSuccess = false;
}

void Experiment::incrementFtmw()
{
    data->ftmwCfg.increment();
}

void Experiment::finalSave() const
{
    //record validation keys
    QSettings s(QSettings::SystemScope,QApplication::organizationName(),QApplication::applicationName());
    QString keys = s.value(QString("knownValidationKeys"),QString("")).toString();
    QStringList knownKeyList = keys.split(QChar(';'),QString::SkipEmptyParts);

    auto it = data->timeDataMap.constBegin();
    while(it != data->timeDataMap.constEnd())
    {
        QString key = it.key();
        if(!knownKeyList.contains(key))
            knownKeyList.append(key);
        it++;
    }

    keys.clear();
    if(knownKeyList.size() > 0)
    {
        keys = knownKeyList.at(0);
        for(int i=1; i<knownKeyList.size();i++)
            keys.append(QString(";%1").arg(knownKeyList.at(i)));

        s.setValue(QString("knownValidationKeys"),keys);
    }

    if(ftmwConfig().isEnabled())
        ftmwConfig().writeFidFile(data->number);

    if(lifConfig().isEnabled())
            lifConfig().writeLifFile(data->number);
}

bool Experiment::saveHeader() const
{

    QFile hdr(BlackChirp::getExptFile(data->number,BlackChirp::HeaderFile));
    if(hdr.open(QIODevice::WriteOnly))
    {
        QTextStream t(&hdr);
        t << BlackChirp::headerMapToString(headerMap());
        t.flush();
        hdr.close();
        return true;
    }
    else
        return false;
}

bool Experiment::saveChirpFile() const
{
    QFile chp(BlackChirp::getExptFile(data->number,BlackChirp::ChirpFile));
    if(chp.open(QIODevice::WriteOnly))
    {
        QTextStream t(&chp);
        t << data->ftmwCfg.chirpConfig().toString();
        t.flush();
        chp.close();
        return true;
    }
    else
        return false;
}

void Experiment::snapshot(int snapNum, const Experiment other) const
{
    if(ftmwConfig().isEnabled())
    {
        FtmwConfig cf = ftmwConfig();
        if(other.number() == data->number && other.isInitialized())
        {
            if(cf.subtractFids(other.ftmwConfig().fidList()))
                cf.writeFidFile(data->number,snapNum);
        }
        else
            cf.writeFidFile(data->number,snapNum);
    }

    if(lifConfig().isEnabled())
        lifConfig().writeLifFile(data->number);
}

