#pragma once

#include <QLabel>

/** 与底栏 pbLocalBadge 一致的「本地」标识 */
void styleLocalMusicBadge(QLabel *badge, bool dark);
void updateLocalMusicBadge(QLabel *badge, bool isLocal, bool dark);
