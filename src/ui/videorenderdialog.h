#pragma once

#include <QDialog>

class QCheckBox;
class QLabel;
class QPushButton;
class QSlider;

class ApiClient;

/**
 * 生成横屏分享视频：起始时间、水印选项（与 Web / Android 一致）。
 */
class VideoRenderDialog : public QDialog
{
    Q_OBJECT

public:
    VideoRenderDialog(ApiClient *api,
                      int musicId,
                      const QString &title,
                      const QString &artist,
                      int trackDurationSec,
                      QWidget *parent = nullptr);

    double startSec() const;
    bool watermarked() const;

private:
    void setupUi();
    void refreshRangeLabel();
    void syncVipAndApply();

    ApiClient *m_api = nullptr;
    int m_musicId = 0;
    QString m_songTitle;
    QString m_songArtist;
    int m_trackDurationSec = 0;
    bool m_isVip = false;
    int m_clipSec = 15;

    QLabel *m_hintLbl = nullptr;
    QLabel *m_rangeLbl = nullptr;
    QSlider *m_startSlider = nullptr;
    QCheckBox *m_watermarkCheck = nullptr;
    QPushButton *m_submitBtn = nullptr;
};
