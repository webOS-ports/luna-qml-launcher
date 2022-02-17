/*
 * Copyright (C) 2014 Christophe Chapuis <chris.chapuis@gmail.com>
 * Copyright (C) 2014 Simon Busch <morphis@gravedo.de>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License version 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <QDebug>

#include <QStringList>
#include <QFileInfo>
#include <QDir>
#include <QTemporaryFile>
#include <QFont>

#include <QQmlEngine>
#include <QQmlContext>
#include <QQuickItem>

#include <QtGui/QGuiApplication>

#include <glib.h>
#include <webos_application.h>

#include <lunaservice.h>

#include <sys/types.h>
#include <unistd.h>

#include "lunaqmlapplication.h"
#include "applicationdescription.h"

struct webos_application_event_handlers event_handlers = {
    .activate = NULL,
    .deactivate = NULL,
    .suspend = NULL,
    .relaunch = LunaQmlApplication::onRelaunch,
    .lowmemory = NULL
};

using namespace luna;

LunaQmlApplication::LunaQmlApplication(int& argc, char **argv) :
    QGuiApplication(argc, argv),
    mAppDescription(nullptr),
    mLaunchParameters("{}"),
    mWindow(nullptr),
    mPrivileged(false),
    mHeadless(false)
{
    if (arguments().size() >= 2) {
        mManifestPath = arguments().at(1);
        qDebug() << "Launching app: " << mManifestPath;
    }
    if (arguments().size() >= 3) {
        mLaunchParameters = arguments().at(2);
        qDebug() << "Launched with parameters: " << mLaunchParameters;
    }

    connect(&mEngine, SIGNAL(quit()), this, SLOT(quit()));

    setFont(QFont("Prelude"));
    // We can safely set this to false here as the compositor will make sure
    // that unless we're allowed by having multiple windows or those which
    // are marked with keepAlive that we get killed.
    setQuitOnLastWindowClosed(false);
}

int LunaQmlApplication::launchApp()
{
    QFile manifestFile(mManifestPath);
    if (!manifestFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning("Failed to read application manifest %s",
                 mManifestPath.toUtf8().constData());
        return 1;
    }

    QString manifestData = QTextStream(&manifestFile).readAll();
    manifestFile.close();

    QString applicationBasePath = QFileInfo(mManifestPath).absoluteDir().path();
    qDebug() << "applicationBasePath" << applicationBasePath;
    mAppDescription = new ApplicationDescription(manifestData, applicationBasePath);

    if (!validateApplication(*mAppDescription)) {
        qWarning("Got invalid application description for app %s",
                 mAppDescription->getId().toUtf8().constData());
        return 2;
    }

    if (mAppDescription->getId().startsWith("org.webosports.") || mAppDescription->getId().startsWith("com.palm.") || mAppDescription->getId().startsWith("com.webos."))
        mPrivileged = true;

    mHeadless = mAppDescription->isHeadLess();

    // We want to make sure we can still use XHR for loading local files at our end in our QML apps.
    qputenv("QML_XHR_ALLOW_FILE_READ", QByteArray("1"));
    
    if (mAppDescription->useLuneOSStyle())
        setenv("QT_QUICK_CONTROLS_STYLE", "LuneOS", 1);

    // We set the application id as application name so that locally stored things for
    // each application are separated and remain after the application was stopped.
    QCoreApplication::setApplicationName(mAppDescription->getId());

    if (!setupLs2Configuration(mAppDescription->getId(), applicationBasePath)) {
        qWarning("Failed to configure ls2 access correctly");
        return -1;
    }

    // The main service handle is the one for luna-qml-launcher, not the one of the QML app.
    // The service handle of the app will be managed afterwards by LunaService QML plugin.
    QString qmlLauncherAppId = "org.webosports.luna-qml-launcher-" + QString::number(QCoreApplication::applicationPid());

    webos_application_init(mAppDescription->getId().toUtf8().constData(), qmlLauncherAppId.toUtf8().constData(), &event_handlers, this);
    webos_application_attach(g_main_loop_new(g_main_context_default(), TRUE));

    // Now load the QML app
    if (!setup(applicationBasePath, mAppDescription->getEntryPoint()))
        return -1;

    return this->exec();
}

//FIXME This will need replacing once we move to latest OSE LS2 commit where pub/prv is gone. 
//Maybe we can use something like https://github.com/webosose/appinstalld2/commit/202f87e25a8a46a988979f173b7a3d36644a9046

bool LunaQmlApplication::setupLs2Configuration(const QString& appId, const QString& applicationBasePath)
{
    QString privRolePath = QString("/usr/share/ls2/roles/prv/%1.json").arg(appId);
    if (QFile::exists(privRolePath))
        pushLs2Role(privRolePath, false);

    QString pubRolePath = QString("/usr/share/ls2/roles/pub/%1.json").arg(appId);
    if (QFile::exists(pubRolePath))
        pushLs2Role(pubRolePath, true);

    return true;
}

bool LunaQmlApplication::pushLs2Role(const QString &rolePath, bool publicBus)
{
    LSHandle *handle = 0;
    LSError lserror;

    LSErrorInit(&lserror);

    qWarning("Pushing role file %s ...", rolePath.toUtf8().constData());

    if (!LSRegisterPubPriv(NULL, &handle, publicBus, &lserror)) {
        qWarning("Failed to register handle while push %s role: %s",
                 publicBus ? "public" : "private", lserror.message);
        LSErrorFree(&lserror);
        return false;
    }

    if (!LSPushRole(handle, rolePath.toUtf8().constData(), &lserror)) {
        qWarning("Failed to push %s role: %s", publicBus ? "public" : "private",
                 lserror.message);
        LSErrorFree(&lserror);
        LSUnregister(handle, NULL);
        return false;
    }

    LSUnregister(handle, NULL);

    return true;
}

bool LunaQmlApplication::validateApplication(const luna::ApplicationDescription& desc)
{
    if (desc.getId().length() == 0)
        return false;

    if (desc.getEntryPoint().isLocalFile() && !QFile::exists(desc.getEntryPoint().toLocalFile()))
        return false;

    return true;
}

LunaQmlApplication::~LunaQmlApplication()
{
    if(mAppDescription) delete mAppDescription; mAppDescription = nullptr;
}

QString LunaQmlApplication::launchParameters() const
{
    return mLaunchParameters;
}

QObject* LunaQmlApplication::appDescription() const
{
    return mAppDescription;
}

bool LunaQmlApplication::setup(const QString& applicationBasePath, const QUrl& path)
{
    if (path.isEmpty()) {
        qWarning() << "Invalid app path:" << path;
        return false;
    }

    mEngine.addImportPath(applicationBasePath);

    mEngine.rootContext()->setContextProperty("application", this);

    QQmlComponent appComponent(&mEngine, path);
    if (appComponent.isError()) {
        qWarning() << "Errors while loading app from" << path;
        qWarning() << appComponent.errors();
        return false;
    }

    QObject *rootItem = appComponent.beginCreate(mEngine.rootContext());
    if (!rootItem) {
        qWarning() << "Error creating app from" << path;
        qWarning() << appComponent.errors();
        return false;
    }

    appComponent.completeCreate();

    if (!mHeadless) {
        mWindow = static_cast<QQuickWindow*>(rootItem);
        if (!mWindow) {
            qWarning() << "Application root item is not a window!";
            return false;
        }
    }

    return true;
}

void LunaQmlApplication::onRelaunch(const char *parameters, void *user_data)
{
    LunaQmlApplication *app = static_cast<LunaQmlApplication*>(user_data);
    app->relaunch(parameters);
}

void LunaQmlApplication::relaunch(const char *parameters)
{
    emit relaunched(QString(parameters));
}
