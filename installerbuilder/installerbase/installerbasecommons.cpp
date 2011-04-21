/**************************************************************************
**
** This file is part of Qt SDK**
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).*
**
** Contact:  Nokia Corporation qt-info@nokia.com**
**
** No Commercial Usage
**
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception version
** 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you are unsure which license is appropriate for your use, please contact
** (qt-info@nokia.com).
**
**************************************************************************/
#include "installerbasecommons.h"

#include <common/installersettings.h>
#include <messageboxhandler.h>
#include <qinstaller.h>
#include <qinstallercomponent.h>

#include <QtCore/QDir>
#include <QtCore/QFileInfo>
#include <QtCore/QTimer>

#include <QtGui/QLabel>
#include <QtGui/QProgressBar>
#include <QtGui/QRadioButton>
#include <QtGui/QStackedWidget>
#include <QtGui/QVBoxLayout>

using namespace QInstaller;


// -- IntroductionPageImpl

IntroductionPageImpl::IntroductionPageImpl(QInstaller::Installer *installer)
    : QInstaller::IntroductionPage(installer)
{
    QWidget *widget = new QWidget(this);
    QVBoxLayout *layout = new QVBoxLayout(widget);

    m_packageManager = new QRadioButton(tr("Package manager"), this);
    layout->addWidget(m_packageManager);
    m_packageManager->setChecked(installer->isPackageManager());
    connect(m_packageManager, SIGNAL(toggled(bool)), this, SLOT(setPackageManager(bool)));

    m_updateComponents = new QRadioButton(tr("Update components"), this);
    layout->addWidget(m_updateComponents);
    m_updateComponents->setChecked(installer->isUpdater());
    connect(m_updateComponents, SIGNAL(toggled(bool)), this, SLOT(setUpdater(bool)));

    m_removeAllComponents = new QRadioButton(tr("Remove all components"), this);
    layout->addWidget(m_removeAllComponents);
    m_removeAllComponents->setChecked(installer->isUninstaller());
    connect(m_removeAllComponents, SIGNAL(toggled(bool)), this, SLOT(setUninstaller(bool)));
    connect(m_removeAllComponents, SIGNAL(toggled(bool)), installer, SLOT(setCompleteUninstallation(bool)));

    layout->addItem(new QSpacerItem(1, 1, QSizePolicy::Minimum, QSizePolicy::Expanding));

    m_label = new QLabel(this);
    m_label->setWordWrap(true);
    m_label->setText(tr("Retrieving information from remote installation sources..."));
    layout->addWidget(m_label);

    m_progressBar = new QProgressBar(this);
    m_progressBar->setRange(0, 0);
    layout->addWidget(m_progressBar);

    layout->addItem(new QSpacerItem(1, 1, QSizePolicy::Minimum, QSizePolicy::Expanding));
    widget->setLayout(layout);
    setWidget(widget);

    installer->setCompleteUninstallation(installer->isUninstaller());
}

int IntroductionPageImpl::nextId() const
{
    if (installer()->isUninstaller())
        return Installer::ReadyForInstallation;

    if (installer()->isUpdater() || installer()->isPackageManager())
        return Installer::ComponentSelection;

    return QInstaller::IntroductionPage::nextId();
}

void IntroductionPageImpl::showAll()
{
    showWidgets(true);
}

void IntroductionPageImpl::hideAll()
{
    showWidgets(false);
}

void IntroductionPageImpl::showMetaInfoUdate()
{
    showWidgets(false);
    m_label->setVisible(true);
    m_progressBar->setVisible(true);
}

void IntroductionPageImpl::showMaintenanceTools()
{
    showWidgets(true);
    m_label->setVisible(false);
    m_progressBar->setVisible(false);
}

void IntroductionPageImpl::setMaintenanceToolsEnabled(bool enable)
{
    m_packageManager->setEnabled(enable);
    m_updateComponents->setEnabled(enable);
    m_removeAllComponents->setEnabled(enable);
}

void IntroductionPageImpl::message(KDJob *job, const QString &msg)
{
    Q_UNUSED(job)
    m_label->setText(msg);
}

void IntroductionPageImpl::setUpdater(bool value)
{
    if (value) {
        installer()->setUpdater();
        emit initUpdater();
    }
}

void IntroductionPageImpl::setUninstaller(bool value)
{
    if (value) {
        installer()->setUninstaller();
        emit initUninstaller();
    }
}

void IntroductionPageImpl::setPackageManager(bool value)
{
    if (value) {
        installer()->setPackageManager();
        emit initPackageManager();
    }
}

void IntroductionPageImpl::showWidgets(bool show)
{
    m_label->setVisible(show);
    m_progressBar->setVisible(show);
    m_packageManager->setVisible(show);
    m_updateComponents->setVisible(show);
    m_removeAllComponents->setVisible(show);
}


// -- TargetDirectoryPageImpl

/*!
    A custom target directory selection based due to the no-space restriction...
*/
TargetDirectoryPageImpl::TargetDirectoryPageImpl(Installer *installer)
  : TargetDirectoryPage(installer)
{
    QPalette palette;
    palette.setColor(QPalette::WindowText, Qt::red);

    m_warningLabel = new QLabel(this);
    m_warningLabel->setPalette(palette);

    insertWidget(m_warningLabel, QLatin1String("MessageLabel"), 2);
}

QString TargetDirectoryPageImpl::targetDirWarning() const
{
    if (targetDir().contains(QLatin1Char(' ')))
        return TargetDirectoryPageImpl::tr("The installation path must not contain any space.");
    return QString();
}

bool TargetDirectoryPageImpl::isComplete() const
{
    const QString warning = targetDirWarning();
    m_warningLabel->setText(warning);

    return warning.isEmpty();
}

bool TargetDirectoryPageImpl::askQuestion(const QString &identifier, const QString &message)
{
    QMessageBox::StandardButton bt =
        MessageBoxHandler::warning(MessageBoxHandler::currentBestSuitParent(), identifier,
        TargetDirectoryPageImpl::tr("Warning"), message, QMessageBox::Yes | QMessageBox::No);
    QTimer::singleShot(100, wizard()->page(nextId()), SLOT(repaint()));
    return bt == QMessageBox::Yes;
}

bool TargetDirectoryPageImpl::failWithWarning(const QString &identifier, const QString &message)
{
    MessageBoxHandler::critical(MessageBoxHandler::currentBestSuitParent(), identifier,
        TargetDirectoryPageImpl::tr("Warning"), message);
    QTimer::singleShot(100, wizard()->page(nextId()), SLOT(repaint()));
    return false;
}

bool TargetDirectoryPageImpl::validatePage()
{
    if (!isVisible())
        return true;

    if (targetDir().isEmpty()) {
        MessageBoxHandler::critical(MessageBoxHandler::currentBestSuitParent(),
            QLatin1String("forbiddenTargetDirectory"), tr("Error"),
            tr( "The install directory cannot be empty, please specify a valid folder"), QMessageBox::Ok);
        return false;
    }

    QString remove = installer()->value(QLatin1String("RemoveTargetDir"));
    if (!QVariant(remove).toBool())
        return true;

    const QDir dir(targetDir());
    if (dir.exists() && dir.entryList(QDir::AllEntries | QDir::NoDotAndDotDot).isEmpty())
        return true;

    QFileInfo fi(targetDir());
    if (fi.isDir()) {
        if (dir == QDir::root()) {
            MessageBoxHandler::critical(MessageBoxHandler::currentBestSuitParent(),
                QLatin1String("forbiddenTargetDirectory"), tr("Error"),
                tr("As the install directory is completely deleted installing in %1 is forbidden")
                .arg(QDir::rootPath()), QMessageBox::Ok);
            return false;
        }

        QString fileName = installer()->settings().uninstallerName();
#if defined(Q_WS_MAC)
        if (QFileInfo(QCoreApplication::applicationDirPath() + QLatin1String("/../..")).isBundle())
            fileName += QLatin1String(".app/Contents/MacOS/") + fileName;
#elif defined(Q_OS_WIN)
        fileName += QLatin1String(".exe");
#endif

        QFileInfo fi2(targetDir() + QDir::separator() + fileName);
        if (fi2.exists()) {
            return askQuestion(QLatin1String("overwriteTargetDirectory"),
                TargetDirectoryPageImpl::tr("The folder you selected exists already and "
                "contains an installation.\nDo you want to overwrite it?"));
        }

        return askQuestion(QLatin1String("overwriteTargetDirectory"),
            tr("You have selected an existing, non-empty folder for installation.\n"
            "Note that it will be completely wiped on uninstallation of this application.\n"
            "It is not advisable to install into this folder as installation might fail.\n"
            "Do you want to continue?"));
    } else if (fi.isFile() || fi.isSymLink()) {
        MessageBoxHandler::critical(MessageBoxHandler::currentBestSuitParent(),
            QLatin1String("WrongTargetDirectory"), tr("Error"),
            tr("You have selected an existing file or symlink, please choose a different target for "
            "installation."), QMessageBox::Ok);
        return false;
    }
    return true;
}


// -- QtInstallerGui

QtInstallerGui::QtInstallerGui(Installer *installer)
    : Gui(installer, 0)
{
    setPage(Installer::Introduction, new IntroductionPageImpl(installer));
    setPage(Installer::TargetDirectory, new TargetDirectoryPageImpl(installer));
    setPage(Installer::ComponentSelection, new ComponentSelectionPage(m_installer));
    setPage(Installer::LicenseCheck, new LicenseAgreementPage(installer));
#ifdef Q_OS_WIN
    setPage(Installer::StartMenuSelection, new StartMenuDirectoryPage(installer));
#endif
    setPage(Installer::ReadyForInstallation, new ReadyForInstallationPage(installer));
    setPage(Installer::PerformInstallation, new PerformInstallationPage(installer));
    setPage(Installer::InstallationFinished, new FinishedPage(installer));

    bool ok = false;
    const int startPage = installer->value(QLatin1String("GuiStartPage")).toInt(&ok);
    if(ok)
        setStartId(startPage);
}

void QtInstallerGui::init()
{
    if (m_installer->components(true, m_installer->runMode()).count() == 1)
        wizardPageVisibilityChangeRequested(false, Installer::ComponentSelection);
}


// -- QtUninstallerGui

QtUninstallerGui::QtUninstallerGui(Installer *installer)
    : Gui(installer, 0)
{
    IntroductionPageImpl *intro = new IntroductionPageImpl(installer);
    connect(intro, SIGNAL(initUpdater()), this, SLOT(updateRestartPage()));
    connect(intro, SIGNAL(initUninstaller()), this, SLOT(updateRestartPage()));
    connect(intro, SIGNAL(initPackageManager()), this, SLOT(updateRestartPage()));

    setPage(Installer::Introduction, intro);
    setPage(Installer::ComponentSelection, new ComponentSelectionPage(m_installer));
    setPage(Installer::LicenseCheck, new LicenseAgreementPage(installer));
    setPage(Installer::ReadyForInstallation, new ReadyForInstallationPage(installer));
    setPage(Installer::PerformInstallation, new PerformInstallationPage(installer));
    setPage(Installer::InstallationFinished, new FinishedPage(installer));

    RestartPage *p = new RestartPage(installer);
    connect(p, SIGNAL(restart()), this, SIGNAL(gotRestarted()));
    setPage(Installer::InstallationFinished + 1, p);

    if (installer->isUninstaller())
        wizardPageVisibilityChangeRequested(false, Installer::InstallationFinished + 1);
}

void QtUninstallerGui::init()
{
    const bool visible = !m_installer->components(false, m_installer->runMode()).isEmpty();

    wizardPageVisibilityChangeRequested(visible, Installer::ComponentSelection);
    wizardPageVisibilityChangeRequested(visible, Installer::LicenseCheck);
}

int QtUninstallerGui::nextId() const
{
    const int next = QWizard::nextId();
    if (next == Installer::LicenseCheck) {
        const int nextNextId = pageIds().value(pageIds().indexOf(next)+ 1, -1);
        if (!m_installer->isPackageManager() && !m_installer->isUpdater())
            return nextNextId;

        QList<Component*> components = m_installer->componentsToInstall(m_installer->runMode());
        if (components.isEmpty())
            return nextNextId;

        bool foundLicense = false;
        foreach (Component* component, components) {
            if (component->isInstalled())
                continue;
            foundLicense |= !component->licenses().isEmpty();
        }
        return foundLicense ? next : nextNextId;
    }
    return next;
}

void QtUninstallerGui::updateRestartPage()
{
    wizardPageVisibilityChangeRequested((m_installer->isUninstaller() ? false : true),
        Installer::InstallationFinished + 1);
}
