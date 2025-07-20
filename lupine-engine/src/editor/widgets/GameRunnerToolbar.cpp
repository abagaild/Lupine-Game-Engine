#include "GameRunnerToolbar.h"

GameRunnerToolbar::GameRunnerToolbar(QWidget *parent) : QToolBar(parent)
{
    setWindowTitle("Game Runner");

    m_playSceneAction = addAction("▶ Play Scene");
    m_playGameAction = addAction("🎮 Play Game");
    addSeparator();
    m_pauseAction = addAction("⏸ Pause");
    m_stopAction = addAction("⏹ Stop");

    m_playSceneAction->setToolTip("Play the current scene");
    m_playGameAction->setToolTip("Play the project's main scene");
    m_pauseAction->setToolTip("Pause the running game");
    m_stopAction->setToolTip("Stop the running game");

    m_pauseAction->setEnabled(false);
    m_stopAction->setEnabled(false);

    connect(m_playSceneAction, &QAction::triggered, this, &GameRunnerToolbar::playSceneRequested);
    connect(m_playGameAction, &QAction::triggered, this, &GameRunnerToolbar::playGameRequested);
    connect(m_pauseAction, &QAction::triggered, this, &GameRunnerToolbar::pauseRequested);
    connect(m_stopAction, &QAction::triggered, this, &GameRunnerToolbar::stopRequested);
}

void GameRunnerToolbar::setPlayingState(bool isPlaying)
{
    m_playSceneAction->setEnabled(!isPlaying);
    m_playGameAction->setEnabled(!isPlaying);
    m_pauseAction->setEnabled(isPlaying);
    m_stopAction->setEnabled(isPlaying);
}
