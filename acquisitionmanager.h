#ifndef ACQUISITIONMANAGER_H
#define ACQUISITIONMANAGER_H

#include <QObject>
#include <QDateTime>
#include <QTime>
#include <QTimer>
#include <QThread>

#include "datastructs.h"
#include "experiment.h"

#ifdef BC_CUDA
#include "gpuaverager.h"
#endif

class AcquisitionManager : public QObject
{
    Q_OBJECT
public:
    explicit AcquisitionManager(QObject *parent = 0);
    ~AcquisitionManager();

    enum AcquisitionState
    {
        Idle,
        Acquiring,
        Paused
    };

signals:
    void logMessage(const QString,const BlackChirp::LogMessageCode = BlackChirp::LogNormal);
    void statusMessage(const QString);
    void experimentInitialized(const Experiment);
    void experimentComplete(const Experiment);
    void ftmwUpdateProgress(qint64);
    void ftmwNumShots(qint64);
    void lifPointUpdate(QPair<QPoint,BlackChirp::LifPoint>);
    void nextLifPoint(double delay, double frequency);
    void lifShotAcquired(int);
    void beginAcquisition();
    void endAcquisition();
    void timeDataSignal();
    void timeData(const QList<QPair<QString,QVariant>>, bool plot=true, QDateTime t = QDateTime::currentDateTime());

    void newFidList(QList<Fid>);
    void takeSnapshot(const Experiment);
    void doFinalSave(const Experiment);
    void snapshotComplete();

public slots:
    void beginExperiment(Experiment exp);
    void processFtmwScopeShot(const QByteArray b);
    void processLifScopeShot(const LifTrace t);
    void lifHardwareReady(bool success);
    void changeRollingAverageShots(int newShots);
    void resetRollingAverage();
    void getTimeData();
    void processTimeData(const QList<QPair<QString,QVariant>> timeDataList, bool plot);
    void exportAsciiFid(const QString s);
    void pause();
    void resume();
    void abort();

private:
    Experiment d_currentExperiment;
    AcquisitionState d_state;
    QTimer *d_timeDataTimer = nullptr;
    QThread *p_saveThread;
    int d_currentShift;
    float d_lastFom;

    void checkComplete();
    void finishAcquisition();
    bool calculateShift(const QByteArray b);
    float calculateFom(const QVector<qint64> vec, const Fid fid, QPair<int,int> range, int trialShift);

#ifdef BC_CUDA
    GpuAverager gpuAvg;
#endif


};

#endif // ACQUISITIONMANAGER_H
