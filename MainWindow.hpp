#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QUrl>
#include <QTimer>
#include <QDateTime>
#include <QMainWindow>
#include <QMessageBox>
#include <QTreeWidget>
#include <QTextStream>
#include <QDesktopServices>

#include <toml.hpp>
#include <Console.hpp>
#include <WaitDialog.hpp>
#include <LuaBackend.hpp>
#include <AboutFrontend.hpp>

QT_BEGIN_NAMESPACE
    namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    struct QGameInfo
    {
        QString gameName;
        QString exeName;
        QString scriptPath;
        uint64_t baseAddress;
        uint64_t offset;
        bool isBigEndian;
    };

    Q_OBJECT

    public:
        QTimer* latchTimer;

        LuaBackend* backend;
        LuaBackend* backendFake;

        MainWindow(QWidget *parent = nullptr);
        ~MainWindow();

    public slots:
        void consoleToggle();

    private slots:
        void runEvent();
        void stopEvent();
        void autoToggle();
        void latchEvent();
        void startEvent();
        void darkToggle();
        void reloadEvent();
        void connectEvent();
        void gameClickEvent(int);
        void scriptClickEvent(QTreeWidgetItem*, int);

        void gameContextEvent(QPoint);
        void scriptContextEvent(QPoint);

        void newScriptEvent();
        void editScriptEvent();
        void deleteScriptEvent();
        void refreshScriptEvent();
        void folderOpenEvent();

        void showAbout();

    private:
        toml::value _gameToml;

        QPalette _darkPal;
        QPalette _lightPal;

        bool _autoBool;
        bool _consoleBool;
        bool _darkPalBool;

        Console* _console;
        WaitDialog* _waitWindow;
        AboutFrontend* _aboutDiag;

        QTimer* _runTimer;

        QString _basePath;
        QGameInfo _currGame;

        int parseGame();
        int parseScript();

        Ui::MainWindow *ui;

};

#endif // MAINWINDOW_H
