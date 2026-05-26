#include "localmusicbadgelabel.h"

#include "core/i18n.h"

void styleLocalMusicBadge(QLabel *badge, bool dark)
{
    if (!badge)
        return;
    badge->setObjectName(QStringLiteral("localMusicBadge"));
    if (dark) {
        badge->setStyleSheet(QStringLiteral(
            "QLabel#localMusicBadge {"
            "  font-size: 11px;"
            "  font-weight: 800;"
            "  color: #100818;"
            "  padding: 2px 8px;"
            "  border-radius: 8px;"
            "  background: qlineargradient(x1:0,y1:0,x2:1,y2:1, stop:0 #7EE8C8, stop:0.42 #C8FFD8, stop:1 #ECC8FF);"
            "  border: 1px solid rgba(255,255,255,0.78);"
            "}"));
    } else {
        badge->setStyleSheet(QStringLiteral(
            "QLabel#localMusicBadge {"
            "  font-size: 11px;"
            "  font-weight: 800;"
            "  color: #160f22;"
            "  padding: 2px 8px;"
            "  border-radius: 8px;"
            "  background: qlineargradient(x1:0,y1:0,x2:1,y2:1, stop:0 #42C4C4, stop:0.45 #A8F0D8, stop:1 #D0B0FF);"
            "  border: 1px solid rgba(70,40,120,0.45);"
            "}"));
    }
}

void updateLocalMusicBadge(QLabel *badge, bool isLocal, bool dark)
{
    if (!badge)
        return;
    badge->setVisible(isLocal);
    if (!isLocal)
        return;
    badge->setText(I18n::instance().tr(QStringLiteral("localMusicBadge")));
    styleLocalMusicBadge(badge, dark);
}
