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

#include <QQmlEngine>
#include <QQmlContext>
#include <QQuickItem>

#include <QtGui/QGuiApplication>
#include <QtGui/qpa/qplatformnativeinterface.h>

#include <glib.h>
#include <webos_application.h>

#include <luna-service.h>

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
    mLaunchParameters("{}")
{
    if (arguments().size() >= 2) {
        mManifestPath = arguments().at(1);
        qDebug() << "Launching app: " << mManifestPath;
    }
    if (arguments().size() >= 3) {
        mLaunchParameters = arguments().at(2);
        qDebug() << "Launched with parameters: " << mLaunchParameters;
    }
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

    // We set the application id as application name so that locally stored things for
    // each application are separated and remain after the application was stopped.
    QCoreApplication::setApplicationName(desc.id());

    setupLs2Configuration(applicationBasePath);

    webos_application_init(desc.id().toUtf8().constData(), &event_handlers, this);
    webos_application_attach(g_main_loop_new(g_main_context_default(), TRUE));

    this->setup(desc.entryPoint());

    return this->exec();
}

void LunaQmlApplication::setupLs2Configuration(const QString& appId, const QString& applicationBasePath)
{
    QString pubRolePath = QString("%1/roles/pub/%2.json")
            .arg(applicationBasePath)
            .arg(appId);

    if (QFile::exists(pubRolePath))
        pushLs2Role(pubRolePath, true);

    QString prvRolePath = QString("%1/roles/prv/%2.json")
            .arg(applicationBasePath)
            .arg(appId);

    if (QFile::exists(prvRolePath))
        pushLs2Role(prvRolePath, false);
}

void LunaQmlApplication::pushLs2Role(const QString &rolePath, bool publicBus)
{
    LSHandle *handle = 0;
    LSError lserror;

    LSErrorInit(&lserror);

    if (!LSRegister(NULL, &handle, publicBus, &lserror)) {
        qWarning("Failed to register handle while push %s role: %s",
                 publicBus ? "public" : "private", lserror.message);
        LSErrorFree(&lserror);
        return;
    }

    if (!LSPushRole(handle, rolePath.toUtf8().constData(), &lserror)) {
        qWarning("Failed to push %s role: %s", publicBus ? "public" : "private",
                 lserror.message);
        LSErrorFree(&lserror);
    }

    LSUnregister(handle, NULL);
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

bool LunaQmlApplication::setup(const QUrl& path)
{
    if (path.isEmpty()) {
        qWarning() << "Invalid app path:" << path;
        return false;
    }

    mEngine.rootContext()->setContextProperty("application", this);

    QQmlComponent appComponent(&mEngine, path);
    if (appComponent.isError()) {
        qWarning() << "Errors while loading app from" << path;
        qWarning() << appComponent.errors();
        return false;
    }

    QObject *app = appComponent.beginCreate(mEngine.rootContext());
    if (!app) {
        qWarning() << "Error creating app from" << path;
        qWarning() << appComponent.errors();
        return false;
    }

    QPlatformNativeInterface *nativeInterface = QGuiApplication::platformNativeInterface();
    if( nativeInterface ) {
        // set different information bits for our window
        nativeInterface->setWindowProperty(QString("appId"), QVariant(QCoreApplication::applicationName()));
        nativeInterface->setWindowProperty(QString("type"), QVariant("card"));
    }

    appComponent.completeCreate();

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
