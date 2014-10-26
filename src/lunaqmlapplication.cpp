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
#include <QtGui/qpa/qplatformnativeinterface.h>

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
    mLaunchParameters("{}"),
    mWindow(0),
    mPrivileged(false)
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
    ApplicationDescription desc(manifestData, applicationBasePath);

    if (!validateApplication(desc)) {
        qWarning("Got invalid application description for app %s",
                 desc.id().toUtf8().constData());
        return 2;
    }

    if (desc.id().startsWith("org.webosports."))
        mPrivileged = true;

    // We set the application id as application name so that locally stored things for
    // each application are separated and remain after the application was stopped.
    QCoreApplication::setApplicationName(desc.id());

    if (!setupLs2Configuration(desc.id(), applicationBasePath)) {
        qWarning("Failed to configure ls2 access correctly");
        return -1;
    }

    webos_application_init(desc.id().toUtf8().constData(), &event_handlers, this);
    webos_application_attach(g_main_loop_new(g_main_context_default(), TRUE));

    if (!setup(applicationBasePath, desc.entryPoint()))
        return -1;

    return this->exec();
}

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
    if (desc.id().length() == 0)
        return false;

    if (desc.entryPoint().isLocalFile() && !QFile::exists(desc.entryPoint().toLocalFile()))
        return false;

    return true;
}

LunaQmlApplication::~LunaQmlApplication()
{
}

QString LunaQmlApplication::launchParameters() const
{
    return mLaunchParameters;
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

    mWindow = static_cast<QQuickWindow*>(rootItem);
    if (!mWindow) {
        qWarning() << "Application root item is not a window!";
        return false;
    }

    QPlatformNativeInterface *nativeInterface = QGuiApplication::platformNativeInterface();
    if (nativeInterface) {
        // Make sure the window is fully created when we want to deal with it
        mWindow->create();

        // set different information bits for our window
        nativeInterface->setWindowProperty(mWindow->handle(), QString("appId"), QVariant(QCoreApplication::applicationName()));
        nativeInterface->setWindowProperty(mWindow->handle(), QString("type"), QVariant("card"));
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
