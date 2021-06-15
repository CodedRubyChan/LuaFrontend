#include <MainWindow.hpp>
#include <ui_MainWindow.h>

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent), ui(new Ui::MainWindow)
{
    // UI INITIALIZATION

    ui->setupUi(this);

    QStringList _labelList = { "Title", "Author(s)", "Description" };
    ui->scriptWidget->setHeaderLabels(_labelList);
    ui->scriptWidget->setColumnWidth(0, 128);

    // VARIABLE INITIALIZATION

    _autoBool = false;
    _darkPalBool = false;
    _consoleBool = false;

    _console = new Console(this);
    _waitWindow = new WaitDialog(this);

    _runTimer = new QTimer(this);
    latchTimer = new QTimer(this);

    _runTimer->moveToThread(this->thread());
    latchTimer->moveToThread(this->thread());

    _basePath = QCoreApplication::applicationDirPath();

    // SIGNAL CONSTRUCTOR

    connect(_runTimer, SIGNAL(timeout()), this, SLOT(runEvent()));
    connect(latchTimer, SIGNAL(timeout()), this, SLOT(latchEvent()));

    connect(ui->actionDark, SIGNAL(triggered()), this, SLOT(darkToggle()));
    connect(ui->actionAutoReload, SIGNAL(triggered()), this, SLOT(autoToggle()));
    connect(ui->actionConsole, SIGNAL(triggered()), this, SLOT(consoleToggle()));

    connect(ui->actionStop, SIGNAL(triggered()), this, SLOT(stopEvent()));
    connect(ui->actionStart, SIGNAL(triggered()), this, SLOT(startEvent()));
    connect(ui->actionReload, SIGNAL(triggered()), this, SLOT(reloadEvent()));

    connect(ui->gameWidget, SIGNAL(currentRowChanged(int)), this, SLOT(gameClickEvent(int)));
    connect(ui->scriptWidget, SIGNAL(itemDoubleClicked(QTreeWidgetItem*, int)), this, SLOT(scriptClickEvent(QTreeWidgetItem*, int)));

    // CONTEXT MENU CONSTRUCTOR

    connect(ui->gameWidget, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(gameContextEvent(QPoint)));
    connect(ui->scriptWidget, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(scriptContextEvent(QPoint)));

    // PALETTE INITIALIZATION

    _darkPal.setColor(QPalette::Window, QColor(53,53,53));
    _darkPal.setColor(QPalette::WindowText, Qt::white);
    _darkPal.setColor(QPalette::Base, QColor(42,42,42));
    _darkPal.setColor(QPalette::AlternateBase, QColor(66,66,66));
    _darkPal.setColor(QPalette::ToolTipBase, Qt::white);
    _darkPal.setColor(QPalette::ToolTipText, Qt::white);
    _darkPal.setColor(QPalette::Text, Qt::white);
    _darkPal.setColor(QPalette::Dark, QColor(35,35,35));
    _darkPal.setColor(QPalette::Shadow, QColor(20,20,20));
    _darkPal.setColor(QPalette::Button, QColor(53,53,53));
    _darkPal.setColor(QPalette::ButtonText, Qt::white);
    _darkPal.setColor(QPalette::BrightText, Qt::red);
    _darkPal.setColor(QPalette::Link, QColor(42,130,218));
    _darkPal.setColor(QPalette::Highlight, QColor(42,130,218));
    _darkPal.setColor(QPalette::HighlightedText, Qt::white);
    _darkPal.setColor(QPalette::Disabled, QPalette::Text, QColor(127, 127, 127));
    _darkPal.setColor(QPalette::Disabled, QPalette::Highlight, QColor(80,80,80));
    _darkPal.setColor(QPalette::Disabled, QPalette::WindowText, QColor(127,127,127));
    _darkPal.setColor(QPalette::Disabled, QPalette::ButtonText, QColor(127,127,127));
    _darkPal.setColor(QPalette::Disabled, QPalette::HighlightedText, QColor(127,127,127));

    _lightPal = this->style()->standardPalette();

    // EXECUTION

    parseGame();
    parseScript();
}

MainWindow::~MainWindow() { delete ui; }

// GENERAL FUNCTIONS

int MainWindow::parseGame()
{
    // Spawn the error box.
    // And also set all the variables commonly used.

    QMessageBox _errorBox;

    _errorBox.setMinimumWidth(256);
    _errorBox.setIcon(QMessageBox::Critical);
    _errorBox.setStandardButtons(QMessageBox::Ok);

    // Fetch the game configuation database.

    auto _tomlDir = QString(_basePath + "\\configs\\gameConfig.toml");

    // Try to read it. If the file cannot be accessed for... whatever,
    // act accordingly and cancel the operations.

    try { _gameToml = toml::parse(_tomlDir.toStdString()); }
    catch (std::runtime_error _ex) {

        _errorBox.setDetailedText(_ex.what());
        _errorBox.setWindowTitle("Error #403 - Cannot access file.");

        _errorBox.setText("The file \"gameConfig.toml\" cannot be found or accessed.\n"
                          "Game Configuration could not be loaded!\n"
                          "LuaFrontend will now be terminated.");

        _errorBox.exec();

        exit(403);
    }

    // Go through all of the elements and parse them.
    // Make the first index as the current table, and
    // select it in the box.

    for (std::size_t i = 0; i < _gameToml.size(); i++)
    {
        auto _tblStr = QString("GameEntry%1").arg(i, 2, 10, QLatin1Char('0'));
        auto _table = toml::find(_gameToml, _tblStr.toStdString());

        if (i == 0)
        {
            auto _tmpOffset = QString::fromStdString(toml::find(_table, "Offset").as_string().str);
            auto _tmpAddress = QString::fromStdString(toml::find(_table, "Address").as_string().str);

            _currGame.exeName = QString::fromStdString(toml::find(_table, "Executable").as_string().str);
            _currGame.gameName = QString::fromStdString(toml::find(_table, "Title").as_string().str);
            _currGame.scriptPath = QString::fromStdString(toml::find(_table, "Path").as_string().str);

            _currGame.isBigEndian = toml::find(_table, "BigEndian").as_boolean();

            _currGame.offset = _tmpOffset.toULong(nullptr, 16);
            _currGame.baseAddress = _tmpAddress.toULong(nullptr, 16);
        }

        auto _name = toml::find(_table, "Title").as_string().str;
        auto _nameStr = QString::fromStdString(_name);

        ui->gameWidget->addItem(_nameStr);
        ui->gameWidget->setCurrentRow(0);
    }

    return 0;
}

int MainWindow::parseScript()
{
    // Clear the script widget first.

    ui->scriptWidget->clear();

    // Figure out what path we are working with.
    // Is it absolute or relative? If it has a slash
    // as it's first character, it's relative to us.

    auto _path = _currGame.scriptPath;

    if (_path.at(0) == '/' || _path.at(0) == '\\')
        _path = _basePath + _path;

    // Check if the script folder actually
    // exists. If it does not, error out.

    QDir _scriptDir(_path);
    auto _existBool = _scriptDir.exists();

    if (!_existBool)
        return 404;

    // Feed the information to a fake backend.
    // This backend runs nothing. It's just there
    // to check the scripts.

    backendFake = new LuaBackend(_path.toStdString().c_str(), 0x00000000, _console);

    // Fun. Do the below for every script:

    for (size_t i = 0; i < backendFake->loadedScripts.size(); i++)
    {
        // Make a tree item for them. Regardless if
        // they are valid or not, we are adding 'em in.

        auto _item = new QTreeWidgetItem();
        auto _script = backendFake->loadedScripts[i];

        // Make the tree item have the default
        // values for the script.

        string _luaName = _script->luaState["LUA_NAME"];

        _item->setData(0, 1392, QVariant(0));
        _item->setData(0, 1807, QVariant(_script->scriptPath));

        _item->setText(0, QString::fromStdString(_luaName));
        _item->setText(1, "Unknown");
        _item->setText(2, "Not Available.");

        // If the script is valid, do the following:

        if (_script->parseResult.valid())
        {
            // We check if either the init or the frame
            // function is absent. If so, this script will
            // throw a warning stating as such. But it will
            // still be runnable.

            auto _initCheck = _script->initFunction;
            auto _frameCheck = _script->frameFunction;

            if ((!_initCheck || !_frameCheck) && (!_initCheck && !_frameCheck) == false)
            {
                QString _message = "";

                if (!_initCheck)
                    _message = "Warning #1: Initialization function cannot be found or is invalid.";

                if (!_frameCheck)
                    _message = "Warning #2: Frame function cannot be found or is invalid.";

                // Has a warning? Uncheck it.

                _item->setIcon(0, QIcon(":/resources/warning.ico"));
                _item->setCheckState(0, Qt::CheckState::Unchecked);
                _item->setToolTip(0, _message);
            }

            else if (!_initCheck && !_frameCheck)
            {
                _item->setIcon(0, QIcon(":/resources/error.ico"));
                _item->setToolTip(0, "Error #199: Not a valid LuaBackend script.\n"
                                     "Cannot be executed.");
            }

            else
            {
                // All OK scripts are checked off by default.

                _item->setCheckState(0, Qt::CheckState::Checked);
                _item->setIcon(0, QIcon(":/resources/good.ico"));
            }
        }

        // If the script is invalid, state as such and
        // make it non-checkable.

        else
        {
            LuaError _err = _script->parseResult;

            _item->setData(0, 1392, QVariant(_err.what()));
            _item->setIcon(0, QIcon(":/resources/error.ico"));
            _item->setToolTip(0, "Error #200: Not a valid Lua script.\n"
                                 "Fatal errors were encountered.");
        }

        // Try and parse the LUAGUI elements.
        // Verify if they exist, and if they do,
        // use them in the GUI.

        LuaObject _nameScr = _script->luaState["LUAGUI_NAME"];
        LuaObject _authScr = _script->luaState["LUAGUI_AUTH"];
        LuaObject _descScr = _script->luaState["LUAGUI_DESC"];

        if (_nameScr.valid() && _nameScr.as<string>().length() > 0)
            _item->setText(0, QString::fromStdString(_nameScr.as<string>()));

        if (_authScr.valid() && _authScr.as<string>().length() > 0)
            _item->setText(1, QString::fromStdString(_authScr.as<string>()));

        if (_descScr.valid() && _descScr.as<string>().length() > 0)
            _item->setText(2, QString::fromStdString(_descScr.as<string>()));

        // Add the dang item.

        ui->scriptWidget->addTopLevelItem(_item);
    }

    return 0;
}

// TOGGLE EVENTS

void MainWindow::darkToggle()
{
    QString _titleTxt = "Switch to %1 Mode";

    _darkPalBool ? qApp->setPalette(_lightPal) : qApp->setPalette(_darkPal);
    _darkPalBool ? _titleTxt = _titleTxt.arg("Dark") : _titleTxt = _titleTxt.arg("Light");

    ui->actionDark->setText(_titleTxt);

    _darkPalBool = !_darkPalBool;
}

void MainWindow::autoToggle()
{
    QString _titleTxt = "%1 Auto-Reload";

    _autoBool ? _titleTxt = _titleTxt.arg("Enable") : _titleTxt = _titleTxt.arg("Disable");
    ui->actionAutoReload->setText(_titleTxt);

    _autoBool = !_autoBool;
}

void MainWindow::consoleToggle()
{
    QString _titleTxt = "%1 Console...";

    _consoleBool ? _console->hide() : _console->show();
    _consoleBool ? _titleTxt = _titleTxt.arg("Show") : _titleTxt = _titleTxt.arg("Hide");

    ui->actionConsole->setText(_titleTxt);

    _consoleBool = !_consoleBool;
}

// TRIGGER EVENTS

void MainWindow::startEvent()
{
    // Activate the timer which will
    // take care of everything necessary.

    _waitWindow->show();
    latchTimer->start(100);
}

void MainWindow::stopEvent()
{
    // Reset the console.

    _console->close();
    _console = new Console(this);

    // Stop the run thread.

    _runTimer->stop();

    // Restore the buttons.

    ui->actionStart->setEnabled(true);
    ui->actionReload->setEnabled(false);
    ui->actionStop->setEnabled(false);
}

void MainWindow::reloadEvent()
{
    // Reset the console.

    _console->close();
    _console = new Console(this);

    // Stop the run thread.

    _runTimer->stop();

    // Run the latch thread with the window.

    _waitWindow->show();
    latchTimer->start();

    // Restore the buttons
    // Just in case the reload fails.

    ui->actionStart->setEnabled(true);
    ui->actionReload->setEnabled(false);
    ui->actionStop->setEnabled(false);
}

void MainWindow::gameClickEvent(int currentRow)
{
    auto _tblStr = QString("GameEntry%1").arg(currentRow, 2, 10, QLatin1Char('0'));
    auto _table = toml::find(_gameToml, _tblStr.toStdString());

    auto _tmpOffset = QString::fromStdString(toml::find(_table, "Offset").as_string().str);
    auto _tmpAddress = QString::fromStdString(toml::find(_table, "Address").as_string().str);

    _currGame.exeName = QString::fromStdString(toml::find(_table, "Executable").as_string().str);
    _currGame.gameName = QString::fromStdString(toml::find(_table, "Title").as_string().str);
    _currGame.scriptPath = QString::fromStdString(toml::find(_table, "Path").as_string().str);

    _currGame.isBigEndian = toml::find(_table, "BigEndian").as_boolean();

    _currGame.offset = _tmpOffset.toULong(nullptr, 16);
    _currGame.baseAddress = _tmpAddress.toULong(nullptr, 16);

    if (parseScript() == 404)
    {
        QMessageBox _errorBox;

        _errorBox.setMinimumWidth(512);
        _errorBox.setIcon(QMessageBox::Critical);
        _errorBox.setStandardButtons(QMessageBox::Ok);

        _errorBox.setWindowTitle("Error #404: Folder not found!");
        _errorBox.setText("The script folder for this game does not exist.\n"
                          "Please edit the game information or create the folder.");

        _errorBox.exec();
    }
}

void MainWindow::scriptClickEvent(QTreeWidgetItem *item, int)
{
    // Capture the data.

    auto _errData = item->data(0, 1392);

    // If the data is non-existent,
    // do absolutely nothing.

    if (_errData.toString() == "0")
        return;

    // If there IS data, bind the data
    // do the following;

    else
    {
        // Construct the box.

        QMessageBox _errorBox;

        _errorBox.setMinimumWidth(512);
        _errorBox.setIcon(QMessageBox::Critical);
        _errorBox.setStandardButtons(QMessageBox::Ok);

        // This is only for Error #200. Set the
        // title and the text accordingly.

        _errorBox.setWindowTitle("Error #200: Invalid Script");
        _errorBox.setText("This script is not a valid Lua script.\n"
                          "Fatal errors were encountered.");

        // Set the detailed text to the error
        // given out by sol3, then execute the
        // box.

        _errorBox.setDetailedText(_errData.toString());
        _errorBox.exec();
    }
}

// OFF-TRIGGER EVENTS

void MainWindow::connectEvent()
{
    // Calculate the path as we do in
    // parseScript().

    auto _path = _currGame.scriptPath;

    if (_path.at(0) == '/' || _path.at(0) == '\\')
        _path = _basePath + _path;

    // Calculate the base address that we
    // need to feed into the backend.

    auto _baseAddress = _currGame.baseAddress;
    auto _exeAddress = (uint64_t)MemoryLib::FindBaseAddr(MemoryLib::PHandle, _currGame.exeName.toStdString());

    // If we want the base address of the Executable,
    // calculate accordingly. Otherwise, add the offset
    // to the given address.

    if (_baseAddress == 0)
        _baseAddress = _exeAddress + _currGame.offset;

    else
        _baseAddress += _currGame.offset;

    MemoryLib::SetBaseAddr(_baseAddress);

    // Feed the information to a read backend.
    // This backend actually runs everything needed.

    backend = new LuaBackend(_path.toStdString().c_str(), _baseAddress, _console);

    // Since there is a two-way sorting, this is easy.
    // Check the state of the script widget. Make a new
    // script list with the ticked scripts.

    vector<LuaBackend::LuaScript*> _scriptList;

    for (int i = 0; i < ui->scriptWidget->topLevelItemCount(); i++)
    {
        auto _item = ui->scriptWidget->topLevelItem(i);

        if (_item->checkState(0) == Qt::CheckState::Checked)
            _scriptList.push_back(backend->loadedScripts[i]);
    }

    // Feed the newly made list to the backend.

    backend->loadedScripts.clear();
    backend->loadedScripts = _scriptList;

    // Execute the initialization functions.

    for (auto _script : backend->loadedScripts)
    {
        if (_script->initFunction)
        {
            auto _result = _script->initFunction();

            if (!_result.valid())
            {
                sol::error _err = _result;

                auto _errStr = QString(_err.what());
                _console->printMessage(_errStr + "<br>", 3);
            }
        }
    }

    // Set the buttons, show the console,
    // and run the main loop thread.

    ui->actionStart->setEnabled(false);
    ui->actionReload->setEnabled(true);
    ui->actionStop->setEnabled(true);

    if (!_consoleBool)
        consoleToggle();

    _runTimer->start(backend->frameLimit);
}

void MainWindow::latchEvent()
{
    // Try to latch into the process, if successful;

    if (MemoryLib::LatchProcess(_currGame.exeName.toStdString(), _currGame.baseAddress, _currGame.isBigEndian))
    {
        // Hide the wait dialog, stop the timer.

        _waitWindow->hide();
        latchTimer->stop();

        // Continue with Backend Execution.

        connectEvent();
    }
}

void MainWindow::runEvent()
{
    // The main script thread.

    for (int i = 0; i < backend->loadedScripts.size(); i++)
    {
        auto _script = backend->loadedScripts[i];

        if (_script->frameFunction)
        {
            auto _result = _script->frameFunction();

            if (!_result.valid())
            {
                sol::error _err = _result;

                auto _errStr = QString(_err.what());
                _console->printMessage(_errStr + "<br>", 3);

                backend->loadedScripts.erase(backend->loadedScripts.begin() + i);
            }
        }
    }

    // If the interval changes, apply this change.

    if (_runTimer->interval() != backend->frameLimit)
        _runTimer->setInterval(backend->frameLimit);

    // If auto-reload is enabled and the process
    // is lost, activate the reload event.

    // This is OS-specific and only works in Windows
    // at this moment. This will be made Linux-compliant.

    if (_autoBool)
    {
        #if defined(_WIN32) || defined(_WIN64)
            DWORD _retCode = 0;
            GetExitCodeProcess(MemoryLib::PHandle, &_retCode);

            if (_retCode != STILL_ACTIVE)
                reloadEvent();
        #endif
    }
}

// CONTEXT CONSTRUCTOR EVENTS

void MainWindow::gameContextEvent(QPoint point)
{
    /*

    // This is dumb.
    // Another quirk of QT I do not like.
    // ...still better than VC++ tho.

    QMenu _contextGame;

    QAction _addAction("Add Game...");
    QAction _editAction("Edit Game...");
    QAction _removeAction("Remove Game...");

    connect(&_addAction, SIGNAL(triggered()), this, SLOT(addGameEvent()));
    connect(&_editAction, SIGNAL(triggered()), this, SLOT(editGameEvent()));
    connect(&_removeAction, SIGNAL(triggered()), this, SLOT(removeGameEvent()));

    _contextGame.addAction(&_addAction);
    _contextGame.addAction(&_editAction);
    _contextGame.addAction(&_removeAction);

    _contextGame.exec(ui->gameWidget->mapToGlobal(point));

    */
}

void MainWindow::scriptContextEvent(QPoint point)
{
    // This is also dumb.

    QMenu _contextScript;

    QAction _reloadAction("Refresh List");
    QAction _folderAction("Open Script Folder...");

    QAction _addAction("New Script...");
    QAction _editAction("Edit Script...");
    QAction _deleteAction("Delete Script...");

    connect(&_addAction, SIGNAL(triggered()), this, SLOT(newScriptEvent()));
    connect(&_editAction, SIGNAL(triggered()), this, SLOT(editScriptEvent()));
    connect(&_deleteAction, SIGNAL(triggered()), this, SLOT(deleteScriptEvent()));
    connect(&_reloadAction, SIGNAL(triggered()), this, SLOT(refreshScriptEvent()));
    connect(&_folderAction, SIGNAL(triggered()), this, SLOT(folderOpenEvent()));

    _contextScript.addAction(&_reloadAction);
    _contextScript.addAction(&_folderAction);
    _contextScript.addSeparator();

    if (ui->scriptWidget->currentItem() == nullptr)
    {
        _editAction.setEnabled(false);
        _deleteAction.setEnabled(false);
    }

    _contextScript.addAction(&_addAction);
    _contextScript.addAction(&_editAction);
    _contextScript.addAction(&_deleteAction);

    _contextScript.exec(ui->scriptWidget->mapToGlobal(point));
}

// CONTEXT TRIGGER EVENTS

void MainWindow::newScriptEvent()
{
    // Figure out what path we are working with.
    // The same shit as parseScript().

    auto _path = _currGame.scriptPath;

    if (_path.at(0) == '/' || _path.at(0) == '\\')
        _path = _basePath + _path;

    // Construct the new Lua filename.

    auto _name = _path + "\\LuaFile-" + QDateTime::currentDateTime().toString("yyyy-MM-dd_hh-mm-ss") + ".lua";

    // Open the file.

    QFile _luaFile(_name);

    // Total abomination.
    // But eh, as long as it works,
    // I do not care.

    // Construct the template.

    QString _luaTemplate =

    "LUAGUI_NAME = \"\"\n"
    "LUAGUI_AUTH = \"\"\n"
    "LUAGUI_DESC = \"\"\n"
    "\n"
    "function _OnInit()\n"
    "end\n"
    "\n"
    "function _OnFrame()\n"
    "end\n";

    // Write the template to the new file and refresh the list.

    if (_luaFile.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        QTextStream _txtStream(&_luaFile);

        _txtStream << _luaTemplate;
        _luaFile.close();
    }

    parseScript();

    // Open the file for editing.

    QDesktopServices::openUrl(QUrl::fromLocalFile(_name));
}

void MainWindow::editScriptEvent()
{
    auto _data = ui->scriptWidget->currentItem()->data(0, 1807);
    QDesktopServices::openUrl(QUrl::fromLocalFile(_data.toString()));
}

void MainWindow::deleteScriptEvent()
{
    auto _data = ui->scriptWidget->currentItem()->data(0, 1807);
    QFile _tmpFile(_data.toString());

    _tmpFile.remove();
    parseScript();
}

void MainWindow::refreshScriptEvent() { parseScript(); }

void MainWindow::folderOpenEvent()
{
    // Figure out what path we are working with.
    // The same shit as parseScript().

    auto _path = _currGame.scriptPath;

    if (_path.at(0) == '/' || _path.at(0) == '\\')
        _path = _basePath + _path;

    // Open it.

    QDesktopServices::openUrl(QUrl::fromLocalFile(_path));
}
