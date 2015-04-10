#ifndef CHIRPCONFIG_H
#define CHIRPCONFIG_H

#include <QSharedDataPointer>
#include <QPointF>
#include <QVector>
#include <QMap>
#include <QVariant>
#include <QPair>

class ChirpConfigData;

//note: all time units are microseconds; all frequency units are MHz
class ChirpConfig
{
public:
    ChirpConfig();
    ChirpConfig(const ChirpConfig &);
    ChirpConfig &operator=(const ChirpConfig &);
    ~ChirpConfig();

    struct ChirpSegment {
        double startFreqMHz;
        double endFreqMHz;
        double durationUs;
        double alphaUs;
    };

    bool isValid() const;
    double preChirpProtection() const;
    double preChirpDelay() const;
    double postChirpProtection() const;
    int numChirps() const;
    double chirpInterval() const;

    QList<ChirpConfig::ChirpSegment> segmentList() const;

    double chirpDuration() const;
    double totalDuration() const;
    QVector<QPointF> getChirpMicroseconds() const;
    QVector<QPointF> getChirpSegmentMicroSeconds(double t1, double t2) const;
    QMap<QString,QPair<QVariant,QString>> headerMap() const;
    QString toString() const;

    void setPreChirpProtection(const double d);
    void setPreChirpDelay(const double d);
    void setPostChirpProtection(const double d);
    void setNumChirps(const int n);
    void setChirpInterval(const double i);
    void setSegmentList(const QList<ChirpConfig::ChirpSegment> l);

private:
    QSharedDataPointer<ChirpConfigData> data;

    int getFirstSample(double time) const;
    int getLastSample(double time) const;
    double getSampleTime(const int sample) const;
    double calculateChirp(const ChirpSegment segment, const double t, const double phase) const;
    double calculateEndingPhaseRadians(const ChirpSegment segment, const double endingTime, const double startingPhase) const;
    double realToAwgFreq(const double realFreq) const;
    double awgToRealFreq(const double awgFreq) const;

    bool validate();
};

class ChirpConfigData : public QSharedData
{
public:
    ChirpConfigData() : preChirpProtection(-1.0), preChirpDelay(-1.0), postChirpProtection(-1.0), numChirps(0), chirpInterval(-1.0),
        valonTxFreq(-1.0), valonTxMult(-1.0), awgMult(-1.0), mixerSideband(0.0), totalMult(0.0), isValid(false) {}

    double preChirpProtection;
    double preChirpDelay;
    double postChirpProtection;
    int numChirps;
    double chirpInterval;

    double valonTxFreq;
    double valonTxMult;
    double awgMult;
    double mixerSideband;
    double totalMult;


    //working data to improve efficiency; do not record to disk!
    double sampleRateSperS; //awg rate, samples per second
    double sampleRateSperUS; //awg rate, samples per microecond
    double sampleIntervalS; //awg sample interval in seconds
    double sampleIntervalUS; //awg sample interval is microseconds
    bool isValid;

    QList<ChirpConfig::ChirpSegment> segments;

};

#endif // CHIRPCONFIG_H
