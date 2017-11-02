/*
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

#ifndef LUNAQMLAPPLICATION_H
#define LUNAQMLAPPLICATION_H

#include <QObject>
#include <QQmlEngine>
#include <QGuiApplication>
#include <QQuickWindow>

namespace luna {
    class ApplicationDescription;
}

class LunaQmlApplication : public QGuiApplication
{
    Q_OBJECT
    Q_PROPERTY(QObject *appInfo READ appDescription CONSTANT)
    Q_PROPERTY(QString launchParameters READ launchParameters)

public:
    explicit LunaQmlApplication(int& argc, char **argv);
    virtual ~LunaQmlApplication();

    int launchApp();

    QString launchParameters() const;
    QObject *appDescription() const;

    static void onRelaunch(const char *parameters, void *user_data);

signals:
    void relaunched(const QString& parameters);

private:
    bool setup(const QString& applicationBasePath, const QUrl& path);
    void relaunch(const char *parameters);
    bool validateApplication(const luna::ApplicationDescription& desc);

    bool setupLs2Configuration(const QString& appId, const QString& applicationBasePath);
    bool pushLs2Role(const QString& rolePath, bool publicBus);

private:
    QQmlEngine mEngine;
    QString mManifestPath;
    luna::ApplicationDescription *mAppDescription;
    QString mLaunchParameters;
    QQuickWindow *mWindow;
    bool mPrivileged;
    bool mHeadless;
};

#endif // PHONEAPPLICATION_H
