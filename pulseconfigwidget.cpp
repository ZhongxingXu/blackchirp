#include "pulseconfigwidget.h"
#include "ui_pulseconfigwidget.h"
#include <QSettings>
#include <QPushButton>
#include <QToolButton>
#include <QLineEdit>
#include <QDialog>
#include <QFormLayout>
#include <QDialogButtonBox>

PulseConfigWidget::PulseConfigWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::PulseConfigWidget)
{
    ui->setupUi(this);

    QSettings s(QSettings::SystemScope, QApplication::organizationName(), QApplication::applicationName());
    s.beginGroup(QString("pGen"));
    int numChannels = s.value(QString("numChannels"),8).toInt();


    auto vc = static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged);
    s.beginReadArray(QString("channels"));
    for(int i=0; i<numChannels; i++)
    {
        s.setArrayIndex(i);
        ChWidgets ch;
        int col = 0;

        ch.label = new QLabel(s.value(QString("name"),QString("Ch%1").arg(i)).toString(),this);
        ch.label->setAlignment(Qt::AlignRight);
        ui->pulseConfigBoxLayout->addWidget(ch.label,i+1,col,1,1);
        col++;

        ch.delayBox = new QDoubleSpinBox(this);
        ch.delayBox->setKeyboardTracking(false);
        ch.delayBox->setRange(0.0,100000.0);
        ch.delayBox->setDecimals(3);
        ch.delayBox->setSuffix(QString::fromUtf16(u" µs"));
        ch.delayBox->setValue(s.value(QString("defaultDelay"),0.0).toDouble());
        ch.delayBox->setSingleStep(s.value(QString("delayStep"),1.0).toDouble());
        ui->pulseConfigBoxLayout->addWidget(ch.delayBox,i+1,col,1,1);
        connect(ch.delayBox,vc,[=](double val){ emit settingChanged(i,PulseGenConfig::Delay,val); } );
        col++;

        ch.widthBox = new QDoubleSpinBox(this);
        ch.widthBox->setKeyboardTracking(false);
        ch.widthBox->setRange(0.010,100000.0);
        ch.widthBox->setDecimals(3);
        ch.widthBox->setSuffix(QString::fromUtf16(u" µs"));
        ch.widthBox->setValue(s.value(QString("defaultWidth"),0.050).toDouble());
        ch.widthBox->setSingleStep(s.value(QString("widthStep"),1.0).toDouble());
        ui->pulseConfigBoxLayout->addWidget(ch.widthBox,i+1,col,1,1);
        connect(ch.widthBox,vc,[=](double val){ emit settingChanged(i,PulseGenConfig::Width,val); } );
        col++;

        ch.onButton = new QPushButton(this);
        ch.onButton->setCheckable(true);
        ch.onButton->setChecked(s.value(QString("defaultEnabled"),false).toBool());
        if(ch.onButton->isChecked())
            ch.onButton->setText(QString("On"));
        else
            ch.onButton->setText(QString("Off"));
        ui->pulseConfigBoxLayout->addWidget(ch.onButton,i+1,col,1,1);
        connect(ch.onButton,&QPushButton::toggled,[=](bool en){ emit settingChanged(i,PulseGenConfig::Enabled,en); } );
        connect(ch.onButton,&QPushButton::toggled,[=](bool en){
            if(en)
                ch.onButton->setText(QString("On"));
            else
                ch.onButton->setText(QString("Off")); } );
        col++;

        ch.cfgButton = new QToolButton(this);
        ch.cfgButton->setIcon(QIcon(":/icons/configure.png"));
        ui->pulseConfigBoxLayout->addWidget(ch.cfgButton,i+1,col,1,1);
        connect(ch.cfgButton,&QToolButton::clicked,[=](){ launchChannelConfig(i); } );
        col++;

        ch.nameEdit = new QLineEdit(ch.label->text(),this);
        ch.nameEdit->setMaxLength(8);
        ch.nameEdit->hide();

        ch.levelButton = new QPushButton(this);
        ch.levelButton->setCheckable(true);
        if(s.value(QString("level"),PulseGenConfig::ActiveHigh) == QVariant(PulseGenConfig::ActiveHigh))
        {
            ch.levelButton->setChecked(true);
            ch.levelButton->setText(QString("Active High"));
        }
        else
        {
            ch.levelButton->setChecked(false);
            ch.levelButton->setText(QString("Active Low"));
        }
        connect(ch.levelButton,&QPushButton::toggled,[=](bool en){
            if(en)
                ch.levelButton->setText(QString("Active High"));
            else
                ch.levelButton->setText(QString("Active Low"));
        });
        ch.levelButton->hide();

        ch.delayStepBox = new QDoubleSpinBox(this);
        ch.delayStepBox->setDecimals(3);
        ch.delayStepBox->setRange(0.001,1000.0);
        ch.delayStepBox->setSuffix(QString::fromUtf16(u" µs"));
        ch.delayStepBox->setValue(s.value(QString("delayStep"),1.0).toDouble());
        ch.delayStepBox->hide();

        ch.widthStepBox = new QDoubleSpinBox(this);
        ch.widthStepBox->setDecimals(3);
        ch.widthStepBox->setRange(0.001,1000.0);
        ch.widthStepBox->setSuffix(QString::fromUtf16(u" µs"));
        ch.widthStepBox->setValue(s.value(QString("widthStep"),1.0).toDouble());
        ch.widthStepBox->hide();

        d_widgetList.append(ch);
    }

    ui->pulseConfigBoxLayout->setColumnStretch(0,0);
    ui->pulseConfigBoxLayout->setColumnStretch(1,1);
    ui->pulseConfigBoxLayout->setColumnStretch(2,1);
    ui->pulseConfigBoxLayout->setColumnStretch(3,0);
    ui->pulseConfigBoxLayout->setColumnStretch(4,0);

    s.endArray();
    s.endGroup();
}

PulseConfigWidget::~PulseConfigWidget()
{
    delete ui;
}

void PulseConfigWidget::launchChannelConfig(int ch)
{
    if(ch < 0 || ch >= d_widgetList.size())
        return;

    QDialog d(this);
    d.setWindowTitle(QString("Configure Pulse Channel %1").arg(ch));

    QFormLayout *fl = new QFormLayout();
    QVBoxLayout *vbl = new QVBoxLayout();
    QDialogButtonBox *bb = new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel);
    connect(bb->button(QDialogButtonBox::Ok),&QPushButton::clicked,&d,&QDialog::accept);
    connect(bb->button(QDialogButtonBox::Cancel),&QPushButton::clicked,&d,&QDialog::reject);

    ChWidgets chw = d_widgetList.at(ch);

    fl->addRow(QString("Channel Name"),chw.nameEdit);
    fl->addRow(QString("Active Level"),chw.levelButton);
    fl->addRow(QString("Delay Step Size"),chw.delayStepBox);
    fl->addRow(QString("Width Step Size"),chw.widthStepBox);

    chw.nameEdit->show();
    chw.levelButton->show();
    chw.delayStepBox->show();
    chw.widthStepBox->show();

    vbl->addLayout(fl,1);
    vbl->addWidget(bb);

    d.setLayout(vbl);
    if(d.exec() == QDialog::Accepted)
    {
        QSettings s(QSettings::SystemScope, QApplication::organizationName(), QApplication::applicationName());
        s.beginGroup(QString("pGen"));;
        s.beginWriteArray(QString("channels"));
        s.setArrayIndex(ch);

        chw.label->setText(chw.nameEdit->text());
        s.setValue(QString("name"),chw.nameEdit->text());
        emit settingChanged(ch,PulseGenConfig::Name,chw.nameEdit->text());

        chw.delayBox->setSingleStep(chw.delayStepBox->value());
        s.setValue(QString("delayStep"),chw.delayStepBox->value());

        chw.widthBox->setSingleStep(chw.widthStepBox->value());
        s.setValue(QString("widthStep"),chw.widthStepBox->value());

        if(chw.levelButton->isChecked())
        {
            s.setValue(QString("level"),PulseGenConfig::ActiveHigh);
            emit settingChanged(ch,PulseGenConfig::Level,PulseGenConfig::ActiveHigh);
        }
        else
        {
            s.setValue(QString("level"),PulseGenConfig::ActiveLow);
            emit settingChanged(ch,PulseGenConfig::Level,PulseGenConfig::ActiveLow);
        }

        s.endArray();
        s.endGroup();
        s.sync();
    }

    chw.nameEdit->setParent(this);
    chw.nameEdit->hide();
    chw.levelButton->setParent(this);
    chw.levelButton->hide();
    chw.delayStepBox->setParent(this);
    chw.delayStepBox->hide();
    chw.widthStepBox->setParent(this);
    chw.widthStepBox->hide();

}
