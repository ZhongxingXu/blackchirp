#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QList>
#include <QPair>

#include "experiment.h"

class QThread;
class QCloseEvent;
class LogHandler;
class AcquisitionManager;
class HardwareManager;
class Led;
class BatchManager;
class QLabel;
class QLineEdit;
class QDoubleSpinBox;

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

    enum ProgramState
    {
        Idle,
        Acquiring,
        Paused,
        Disconnected,
        Asleep
    };

    struct FlowWidgets {
        QLineEdit *nameEdit;
        QDoubleSpinBox *controlBox;
        Led *led;
        QLabel *nameLabel;
        QDoubleSpinBox *displayBox;
    };

signals:
    void statusMessage(const QString);

public slots:
    void startExperiment();
    void batchComplete(bool aborted);
    void experimentInitialized(Experiment exp);
    void hardwareInitialized(bool success);
    void pauseUi();
    void resumeUi();
    void launchCommunicationDialog();
    void launchRfConfigDialog();
    void updatePulseLeds(const PulseGenConfig cc);
    void updatePulseLed(int index,PulseGenConfig::Setting s, QVariant val);
    void updateFlow(int ch, double val);
    void updateFlowName(int ch, QString name);
    void updateFlowSetpoint(int ch, double val);
    void updatePressureSetpoint(double val);
    void updatePressureControl(bool en);

private:
    Ui::MainWindow *ui;
    QList<QPair<QThread*,QObject*> > d_threadObjectList;
    QList<QPair<QLabel*,Led*>> d_ledList;
    QList<FlowWidgets> d_flowWidgets;
    LogHandler *p_lh;
    HardwareManager *p_hwm;
    AcquisitionManager *p_am;

    bool d_hardwareConnected;
    QThread *d_batchThread;

    void configureUi(ProgramState s);
    void startBatch(BatchManager *bm, bool sleepWhenDone = false);

    ProgramState d_state;

protected:
    void closeEvent(QCloseEvent *ev);

};

#endif // MAINWINDOW_H
