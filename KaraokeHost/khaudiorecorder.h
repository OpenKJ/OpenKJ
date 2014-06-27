#ifndef KHAUDIORECORDER_H
#define KHAUDIORECORDER_H

#include <QObject>
#include <QAudioRecorder>


class KhAudioRecorder : public QObject
{
    Q_OBJECT
private:
    QAudioRecorder *audioRecorder;
    //QAudioEncoderSettings audioSettings;

public:
    explicit KhAudioRecorder(QObject *parent = 0);
    void record(QString filename);
    void stop();
    void pause();
    void unpause();

signals:

private slots:
    void audioRecorderError(QMediaRecorder::Error error);

public slots:

};

#endif // KHAUDIORECORDER_H
