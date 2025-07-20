#pragma once

#ifndef GAMERUNNERTOOLBAR_H
#define GAMERUNNERTOOLBAR_H

#include <QToolBar>
#include <QAction>

class GameRunnerToolbar : public QToolBar
{
    Q_OBJECT
public:
    explicit GameRunnerToolbar(QWidget *parent = nullptr);

    void setPlayingState(bool isPlaying);

signals:
    void playSceneRequested();
    void playGameRequested();
    void pauseRequested();
    void stopRequested();

private:
    QAction* m_playSceneAction;
    QAction* m_playGameAction;
    QAction* m_pauseAction;
    QAction* m_stopAction;
};

#endif // GAMERUNNERTOOLBAR_H
