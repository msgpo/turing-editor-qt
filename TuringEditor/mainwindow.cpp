/****************************************************************************
**
** Copyright (C) 2004-2006 Trolltech ASA. All rights reserved.
**
** This file is part of the example classes of the Qt Toolkit.
**
** Licensees holding a valid Qt License Agreement may use this file in
** accordance with the rights, responsibilities and obligations
** contained therein.  Please consult your licensing agreement or
** contact sales@trolltech.com if any conditions of this licensing
** agreement are not clear to you.
**
** Further information about Qt licensing is available at:
** http://www.trolltech.com/products/qt/licensing.html or by
** contacting info@trolltech.com.
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
****************************************************************************/

#include <QtGui>
#include <QtDebug>
#include <QPalette>

#include <Qsci/qsciscintilla.h>
#include <Qsci/qscistyle.h>

#include "mainwindow.h"
#include "Qsci/qscilexer.h"
#include "turinglexer.h"
#include "findreplacedialog.h"

MainWindow::MainWindow()
{
    textEdit = new QsciScintilla;

    lex = new TuringLexer(this);
    textEdit->setLexer(lex);

    textEdit->setFolding(QsciScintilla::CircledFoldStyle);
    textEdit->setAutoIndent(true);
    textEdit->setTabWidth(4);
    
    textEdit->setCallTipsStyle(QsciScintilla::CallTipsNoContext);
    textEdit->setAutoCompletionCaseSensitivity(false);
    textEdit->setAutoCompletionSource(QsciScintilla::AcsAll);
    textEdit->setAutoCompletionThreshold(5);

    textEdit->markerDefine(QsciScintilla::RightArrow,1);
    textEdit->setAnnotationDisplay(QsciScintilla::AnnotationBoxed);
    textEdit->indicatorDefine(QsciScintilla::SquiggleIndicator,2);
    textEdit->indicatorDefine(QsciScintilla::BoxIndicator,3);
    textEdit->setIndicatorForegroundColor(QColor(50,250,50),3);

    darkErrMsgStyle = new QsciStyle(-1,"dark error style",QColor(255,220,220),QColor(184,50,50),QFont());
    lightErrMsgStyle = new QsciStyle(-1,"light error style",Qt::black,QColor(230,150,150),QFont());

    findDialog = new FindReplaceDialog();
    connect(findDialog,SIGNAL(findAll(QString)),this,SLOT(findAll(QString)));
    connect(findDialog,SIGNAL(find(QString,bool,bool,bool)),this,SLOT(find(QString,bool,bool,bool)));
    connect(findDialog,SIGNAL(findNext()),this,SLOT(findNext()));
    connect(findDialog,SIGNAL(replace(QString)),this,SLOT(replace(QString)));
    connect(findDialog,SIGNAL(replaceAll(QString,QString,bool,bool)),this,SLOT(replaceAll(QString,QString,bool,bool)));

    setCentralWidget(textEdit);

    createActions();
    createMenus();

    createToolBars();
    createStatusBar();

    readSettings();

    connect(textEdit, SIGNAL(textChanged()),
            this, SLOT(documentWasModified()));

    setCurrentFile("");
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    if (maybeSave()) {
        writeSettings();
        event->accept();
    } else {
        event->ignore();
    }
}
//! Switch to the dark coding theme. Based on the common "Twilight" theme
void MainWindow::darkTheme() {
    qDebug() << "Switching to dark theme";
    lex->setTheme("Dark");
    //textEdit->setLexer(lex);
    //textEdit->setWhitespaceBackgroundColor(lex->defaultPaper(0));
    textEdit->setCaretForegroundColor(QColor(167,167,167));
    textEdit->setSelectionBackgroundColor(QColor(221,240,255,45));
    clearErrors();
}
//! Switch to the dark coding theme. Based on the theme used in the original Turing editor.
void MainWindow::lightTheme() {
    qDebug() << "Switching to light theme";
    lex->setTheme("Default");
    //textEdit->setLexer(lex);
    textEdit->setCaretForegroundColor(QColor(0,0,0));
    textEdit->setSelectionBackgroundColor(palette().color(QPalette::Highlight));
    //textEdit->setSelectionBackgroundColor(QColor(60,150,200,40));
    clearErrors();
}
//! Uses a scintilla annotation to display an error box below a certain line.
//! if from and to are provided a squiggly underline is used. 
//! From and to are character indexes into the line.
//! If to is greater than the line length it will wrap.
void MainWindow::showError(int line,QString errMsg,int from, int to)
{
    textEdit->markerAdd(line,1);
    QsciStyle *errStyle = lex->getTheme() == "Dark" ? darkErrMsgStyle : lightErrMsgStyle;
    textEdit->annotate(line,"^ " + errMsg,*errStyle);

    if(from >= 0 && to >= 0) {
        textEdit->fillIndicatorRange(line,from,line,to,2);
    }
}
//! removes all error annotations from display.
void MainWindow::clearErrors() {
    textEdit->clearAnnotations();
    int lines = textEdit->lines();
    textEdit->clearIndicatorRange(0,0,lines,textEdit->text(lines).length(),2);
    textEdit->markerDeleteAll(1);
}

//! clears all annotations, markers and indicators
void MainWindow::clearEverything() {
    textEdit->clearAnnotations();
    int lines = textEdit->lines();
    textEdit->clearIndicatorRange(0,0,lines,textEdit->text(lines).length(),-1);
    textEdit->markerDeleteAll(-1);
}

void MainWindow::newFile()
{
    if (maybeSave()) {
        textEdit->clear();
        setCurrentFile("");
    }
}

void MainWindow::replace(QString repText) {
    textEdit->replace(repText);
}
//! Selects the first appearance of a string and sets it up for the find next command.
void MainWindow::find(QString findText,bool caseSensitive,bool regex,bool wholeWord) {
    textEdit->findFirst(findText,regex,caseSensitive,wholeWord,true);
}
//! Only call after find. Selects the next occurence
void MainWindow::findNext() {
    textEdit->findNext();
}
//! Replaces all occurences of a string in the current document.
//! the regex and greedyRegex switches are to allow regex matching.
void MainWindow::replaceAll(QString findText,QString repText,bool regex,bool greedyRegex)
{
    QString docText = textEdit->text();
    if(regex){
        QRegExp findRE(findText);
        findRE.setMinimal(!greedyRegex);
        docText.replace(findRE, repText);
    } else {
        docText.replace(findText,repText);
    }
    textEdit->setText(docText);
}
//! Places a green box indicator around all occurences of a string.
void MainWindow::findAll(QString findText)
{
    QList<int> found;
    QString docText = textEdit->text();
    int end = docText.lastIndexOf(findText);
    int cur = -1; // so when it does the first +1 it starts at the beginning

    textEdit->SendScintilla(QsciScintillaBase::SCI_INDICATORCLEARRANGE,0,textEdit->length());
    textEdit->SendScintilla(QsciScintillaBase::SCI_SETINDICATORCURRENT,3);

    if(end != -1){ // end is -1 if the text is not found
        while(cur != end) {
            cur = docText.indexOf(findText,cur+1);

            //textEdit->SendScintilla(QsciScintillaBase::SCI_SETINDICATORCURRENT,QsciScintillaBase::INDIC_ROUNDBOX);
            textEdit->SendScintilla(QsciScintillaBase::SCI_INDICATORFILLRANGE,cur,findText.length());
        }
    }
}

void MainWindow::open()
{
    if (maybeSave()) {
        QString fileName = QFileDialog::getOpenFileName(this);
        if (!fileName.isEmpty())
            loadFile(fileName);
    }
}

bool MainWindow::save()
{
    if (curFile.isEmpty()) {
        return saveAs();
    } else {
        return saveFile(curFile);
    }
}

bool MainWindow::saveAs()
{
    QString fileName = QFileDialog::getSaveFileName(this);
    if (fileName.isEmpty())
        return false;

    return saveFile(fileName);
}

void MainWindow::about()
{
   QMessageBox::about(this, tr("About Open Turing"),
            tr("The new Open Turing Editor. "
               "Written by Tristan Hume using "
               "the Qscintilla2 editor component."));
}

void MainWindow::documentWasModified()
{
    setWindowModified(textEdit->isModified());
}

void MainWindow::createActions()
{
    lightThemeAct = new QAction(tr("&Light theme"), this);
    lightThemeAct->setStatusTip(tr("Change to a light theme."));
    connect(lightThemeAct, SIGNAL(triggered()), this, SLOT(lightTheme()));

    darkThemeAct = new QAction(tr("&Dark theme"), this);
    darkThemeAct->setStatusTip(tr("Change to a dark theme."));
    connect(darkThemeAct, SIGNAL(triggered()), this, SLOT(darkTheme()));

    findAct = new QAction(tr("&Find"), this);
    findAct->setShortcut(tr("Ctrl+F"));
    findAct->setStatusTip(tr("Find text in file."));
    connect(findAct, SIGNAL(triggered()), findDialog, SLOT(activate()));

    clearAct = new QAction(tr("&Clear Errors + Info"), this);
    clearAct->setStatusTip(tr("Clear all error messages, boxes, lines, etc. in the document."));
    connect(clearAct, SIGNAL(triggered()), this, SLOT(clearEverything()));

    newAct = new QAction(QIcon(":/images/new.png"), tr("&New"), this);
    newAct->setShortcut(tr("Ctrl+N"));
    newAct->setStatusTip(tr("Create a new file"));
    connect(newAct, SIGNAL(triggered()), this, SLOT(newFile()));

    openAct = new QAction(QIcon(":/images/open.png"), tr("&Open..."), this);
    openAct->setShortcut(tr("Ctrl+O"));
    openAct->setStatusTip(tr("Open an existing file"));
    connect(openAct, SIGNAL(triggered()), this, SLOT(open()));

    saveAct = new QAction(QIcon(":/images/save.png"), tr("&Save"), this);
    saveAct->setShortcut(tr("Ctrl+S"));
    saveAct->setStatusTip(tr("Save the document to disk"));
    connect(saveAct, SIGNAL(triggered()), this, SLOT(save()));

    saveAsAct = new QAction(tr("Save &As..."), this);
    saveAsAct->setStatusTip(tr("Save the document under a new name"));
    connect(saveAsAct, SIGNAL(triggered()), this, SLOT(saveAs()));

    exitAct = new QAction(tr("E&xit"), this);
    exitAct->setShortcut(tr("Ctrl+Q"));
    exitAct->setStatusTip(tr("Exit the application"));
    connect(exitAct, SIGNAL(triggered()), this, SLOT(close()));

    cutAct = new QAction(QIcon(":/images/cut.png"), tr("Cu&t"), this);
    cutAct->setShortcut(tr("Ctrl+X"));
    cutAct->setStatusTip(tr("Cut the current selection's contents to the "
                            "clipboard"));
    connect(cutAct, SIGNAL(triggered()), textEdit, SLOT(cut()));

    copyAct = new QAction(QIcon(":/images/copy.png"), tr("&Copy"), this);
    copyAct->setShortcut(tr("Ctrl+C"));
    copyAct->setStatusTip(tr("Copy the current selection's contents to the "
                             "clipboard"));
    connect(copyAct, SIGNAL(triggered()), textEdit, SLOT(copy()));

    pasteAct = new QAction(QIcon(":/images/paste.png"), tr("&Paste"), this);
    pasteAct->setShortcut(tr("Ctrl+V"));
    pasteAct->setStatusTip(tr("Paste the clipboard's contents into the current "
                              "selection"));
    connect(pasteAct, SIGNAL(triggered()), textEdit, SLOT(paste()));

    aboutAct = new QAction(tr("&About"), this);
    aboutAct->setStatusTip(tr("Show the application's About box"));
    connect(aboutAct, SIGNAL(triggered()), this, SLOT(about()));

    aboutQtAct = new QAction(tr("About &Qt"), this);
    aboutQtAct->setStatusTip(tr("Show the Qt library's About box"));
    connect(aboutQtAct, SIGNAL(triggered()), qApp, SLOT(aboutQt()));

    cutAct->setEnabled(false);
    copyAct->setEnabled(false);
    connect(textEdit, SIGNAL(copyAvailable(bool)),
            cutAct, SLOT(setEnabled(bool)));
    connect(textEdit, SIGNAL(copyAvailable(bool)),
            copyAct, SLOT(setEnabled(bool)));
}

void MainWindow::createMenus()
{
    fileMenu = menuBar()->addMenu(tr("&File"));
    fileMenu->addAction(newAct);
    fileMenu->addAction(openAct);
    fileMenu->addAction(saveAct);
    fileMenu->addAction(saveAsAct);
    fileMenu->addSeparator();
    fileMenu->addAction(exitAct);

    editMenu = menuBar()->addMenu(tr("&Edit"));
    editMenu->addAction(cutAct);
    editMenu->addAction(copyAct);
    editMenu->addAction(pasteAct);
    editMenu->addAction(findAct);

    viewMenu = menuBar()->addMenu(tr("&View"));
    viewMenu->addAction(clearAct);
    viewMenu->addSeparator();
    viewMenu->addAction(lightThemeAct);
    viewMenu->addAction(darkThemeAct);

    menuBar()->addSeparator();

    helpMenu = menuBar()->addMenu(tr("&Help"));
    helpMenu->addAction(aboutAct);
    helpMenu->addAction(aboutQtAct);
}

void MainWindow::createToolBars()
{
    fileToolBar = addToolBar(tr("File"));
    fileToolBar->addAction(newAct);
    fileToolBar->addAction(openAct);
    fileToolBar->addAction(saveAct);

    editToolBar = addToolBar(tr("Edit"));
    editToolBar->addAction(cutAct);
    editToolBar->addAction(copyAct);
    editToolBar->addAction(pasteAct);
}

void MainWindow::createStatusBar()
{
    statusBar()->showMessage(tr("Ready"));
}

void MainWindow::readSettings()
{
    QSettings settings("The Open Turing Project", "Open Turing Editor");
    QPoint pos = settings.value("pos", QPoint(200, 200)).toPoint();
    QSize size = settings.value("size", QSize(400, 400)).toSize();
    QString theme = settings.value("theme", "Default").toString();
    resize(size);
    move(pos);
    qDebug() << "Loading settings. Theme: " << theme;
    if(theme == "Dark") {
        darkTheme();
    } else {
        lightTheme();
    }
}

void MainWindow::writeSettings()
{
    qDebug() << "Saving settings. Theme: " << lex->getTheme();
    QSettings settings("The Open Turing Project", "Open Turing Editor");
    settings.setValue("theme", lex->getTheme());
    settings.setValue("pos", pos());
    settings.setValue("size", size());
}

bool MainWindow::maybeSave()
{
    if (textEdit->isModified()) {
        int ret = QMessageBox::warning(this, tr("Open Turing Editor"),
                     tr("The document has been modified.\n"
                        "Do you want to save your changes?"),
                     QMessageBox::Yes | QMessageBox::Default,
                     QMessageBox::No,
                     QMessageBox::Cancel | QMessageBox::Escape);
        if (ret == QMessageBox::Yes)
            return save();
        else if (ret == QMessageBox::Cancel)
            return false;
    }
    return true;
}

void MainWindow::loadFile(const QString &fileName)
{
    QFile file(fileName);
    if (!file.open(QFile::ReadOnly)) {
        QMessageBox::warning(this, tr("Open Turing Editor"),
                             tr("Cannot read file %1:\n%2.")
                             .arg(fileName)
                             .arg(file.errorString()));
        return;
    }

    QTextStream in(&file);
    QApplication::setOverrideCursor(Qt::WaitCursor);
    textEdit->setText(in.readAll());
    QApplication::restoreOverrideCursor();

    setCurrentFile(fileName);
    statusBar()->showMessage(tr("File loaded"), 2000);

    showError(5,"Error: bad stuff!");
}

bool MainWindow::saveFile(const QString &fileName)
{
    QFile file(fileName);
    if (!file.open(QFile::WriteOnly)) {
        QMessageBox::warning(this, tr("Open Turing Editor"),
                             tr("Cannot write file %1:\n%2.")
                             .arg(fileName)
                             .arg(file.errorString()));
        return false;
    }

    QTextStream out(&file);
    QApplication::setOverrideCursor(Qt::WaitCursor);
    out << textEdit->text();
    QApplication::restoreOverrideCursor();

    setCurrentFile(fileName);
    statusBar()->showMessage(tr("File saved"), 2000);
    return true;
}

void MainWindow::setCurrentFile(const QString &fileName)
{
    curFile = fileName;
    textEdit->setModified(false);
    setWindowModified(false);

    QString shownName;
    if (curFile.isEmpty())
        shownName = "untitled.txt";
    else
        shownName = strippedName(curFile);

    setWindowTitle(tr("%1[*] - %2").arg(shownName).arg(tr("Open Turing Editor")));
}

QString MainWindow::strippedName(const QString &fullFileName)
{
    return QFileInfo(fullFileName).fileName();
}