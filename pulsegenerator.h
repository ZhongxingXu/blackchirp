#ifndef PULSEGENERATOR_H
#define PULSEGENERATOR_H

#include "hardwareobject.h"

#include "pulsegenconfig.h"

class PulseGenerator : public HardwareObject
{
    Q_OBJECT
public:
    PulseGenerator(QObject *parent = nullptr);
    ~PulseGenerator();

public slots:
    PulseGenConfig config() const { return d_config; }
    virtual QVariant read(const int index, const BlackChirp::PulseSetting s) =0;
    virtual double readRepRate() =0;

    virtual BlackChirp::PulseChannelConfig read(const int index);

    virtual bool set(const int index, const BlackChirp::PulseSetting s, const QVariant val) =0;
    virtual bool setChannel(const int index, const BlackChirp::PulseChannelConfig cc);
    virtual bool setAll(const PulseGenConfig cc);

    virtual bool setRepRate(double d) =0;
    virtual bool setLifDelay(double d);

signals:
    void settingUpdate(int,BlackChirp::PulseSetting,QVariant);
    void configUpdate(const PulseGenConfig);
    void repRateUpdate(double);

protected:
    PulseGenConfig d_config;
    virtual void readAll();
};

#ifdef BC_PGEN
#if BC_PGEN==1
#include "qc9528.h"
class Qc9528;
typedef Qc9528 PulseGeneratorHardware;

//NOTE: Gas, AWG, excimer, and LIF channels are hardcoded
#define BC_PGEN_GASCHANNEL 0
#define BC_PGEN_AWGCHANNEL 1
#define BC_PGEN_XMERCHANNEL 2
#define BC_PGEN_LIFCHANNEL 3
#define BC_PGEN_NUMCHANNELS 8

#else
#include "virtualpulsegenerator.h"
class VirtualPulseGenerator;
typedef VirtualPulseGenerator PulseGeneratorHardware;

//NOTE: Gas, AWG, excimer, and LIF channels are hardcoded
#define BC_PGEN_GASCHANNEL 0
#define BC_PGEN_AWGCHANNEL 1
#define BC_PGEN_XMERCHANNEL 2
#define BC_PGEN_LIFCHANNEL 3
#define BC_PGEN_NUMCHANNELS 8

#endif
#else
#define BC_PGEN_GASCHANNEL 0
#define BC_PGEN_AWGCHANNEL 0
#define BC_PGEN_XMERCHANNEL 0
#define BC_PGEN_LIFCHANNEL 0
#define BC_PGEN_NUMCHANNELS 0
#endif

#endif // PULSEGENERATOR_H
