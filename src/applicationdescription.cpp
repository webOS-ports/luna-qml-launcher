/*
 * Copyright (C) 2014 Christophe Chapuis <chris.chapuis@gmail.com>
 * Copyright (C) 2013 Simon Busch <morphis@gravedo.de>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>
 */

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFile>
#include <QDebug>

#include "applicationdescription.h"

namespace luna
{

ApplicationDescription::ApplicationDescription() :
    mHeadless(false),
    mTrustScope(ApplicationDescription::TrustScopeSystem)
{
}

ApplicationDescription::ApplicationDescription(const ApplicationDescription& other) :
    mId(other.id()),
    mTitle(other.title()),
    mIcon(other.icon()),
    mEntryPoint(other.entryPoint()),
    mHeadless(other.headless()),
    mApplicationBasePath(other.basePath()),
    mTrustScope(other.trustScope()),
    mPluginName(other.pluginName()),
    mFlickable(other.flickable()),
    mInternetConnectivityRequired(other.internetConnectivityRequired()),
    mUrlsAllowed(other.urlsAllowed()),
    mUserAgent(other.userAgent()),
    mUseLuneOSStyle(other.useLuneOSStyle())
{
}

ApplicationDescription::ApplicationDescription(const QString &data, const QString &applicationBasePath) :
    mHeadless(false),
    mApplicationBasePath(applicationBasePath),
    mFlickable(false),
    mInternetConnectivityRequired(false),
    mTrustScope(ApplicationDescription::TrustScopeSystem),
    mUserAgent(""),
    mUseLuneOSStyle(false)
{
    initializeFromData(data);
}

ApplicationDescription::~ApplicationDescription()
{
}

void ApplicationDescription::initializeFromData(const QString &data)
{
    QJsonDocument document = QJsonDocument::fromJson(data.toUtf8());

    if (!document.isObject()) {
        qWarning() << "Failed to parse application description";
        return;
    }

    QJsonObject rootObject = document.object();

    if (rootObject.contains("id") && rootObject.value("id").isString())
        mId = rootObject.value("id").toString();

    if (rootObject.contains("main") && rootObject.value("main").isString())
        mEntryPoint = locateEntryPoint(rootObject.value("main").toString());

    if (rootObject.contains("noWindow") && rootObject.value("noWindow").isBool())
        mHeadless = rootObject.value("noWindow").toBool();

    if (rootObject.contains("title") && rootObject.value("title").isString())
        mTitle = rootObject.value("title").toString();

    if (rootObject.contains("icon") && rootObject.value("icon").isString()) {
        QString iconPath = rootObject.value("icon").toString();

        // we're only allow locally stored icons so we must prefix them with file:// to
        // store it in a QUrl object
        if (!iconPath.startsWith("file://"))
            iconPath.prepend("file://");

        mIcon = iconPath;
    }

    if (rootObject.contains("flickable") && rootObject.value("flickable").isBool())
        mFlickable = rootObject.value("flickable").toBool();

    if (rootObject.contains("internetConnectivityRequired") && rootObject.value("internetConnectivityRequired").isBool())
        mInternetConnectivityRequired = rootObject.value("internetConnectivityRequired").toBool();

    if (mIcon.isEmpty() || !mIcon.isLocalFile() || !QFile::exists(mIcon.toLocalFile()))
        mIcon = QUrl("qrc:///qml/images/default-app-icon.png");

    if (rootObject.contains("urlsAllowed") && rootObject.value("urlsAllowed").isArray()) {
        QJsonArray urlsAllowed = rootObject.value("urlsAllowed").toArray();
        for (int n = 0; n < urlsAllowed.size(); n++) {
            if (!urlsAllowed[n].isString())
                continue;

            mUrlsAllowed.append(urlsAllowed[n].toString());
        }
    }

    if (rootObject.contains("plugin") && rootObject.value("plugin").isString())
        mPluginName = rootObject.value("plugin").toString();

    if (rootObject.contains("userAgent") && rootObject.value("userAgent").isString())
        mUserAgent = rootObject.value("userAgent").toString();

    if (rootObject.contains("useLuneOSStyle") && rootObject.value("useLuneOSStyle").isBool())
        mUseLuneOSStyle = rootObject.value("useLuneOSStyle").toBool();
}

QUrl ApplicationDescription::locateEntryPoint(const QString &entryPoint)
{
    QUrl entryPointAsUrl(entryPoint);

    if (entryPointAsUrl.scheme() == "file" ||
        entryPointAsUrl.scheme() == "http" ||
        entryPointAsUrl.scheme() == "https")
        return entryPointAsUrl;

    if (entryPointAsUrl.scheme() != "") {
        qWarning("Entry point %s for application %s is invalid",
                 entryPoint.toUtf8().constData(),
                 mId.toUtf8().constData());
        return QUrl("");
    }

    return QUrl(QString("file://%1/%2").arg(mApplicationBasePath).arg(entryPoint));
}

bool ApplicationDescription::hasRemoteEntryPoint() const
{
    return mEntryPoint.scheme() == "http" ||
           mEntryPoint.scheme() == "https";
}

QString ApplicationDescription::id() const
{
    return mId;
}

QString ApplicationDescription::title() const
{
    return mTitle;
}

QUrl ApplicationDescription::icon() const
{
    return mIcon;
}

QUrl ApplicationDescription::entryPoint() const
{
    return mEntryPoint;
}

bool ApplicationDescription::headless() const
{
    return mHeadless;
}

ApplicationDescription::TrustScope ApplicationDescription::trustScope() const
{
    return mTrustScope;
}

QString ApplicationDescription::basePath() const
{
    return mApplicationBasePath;
}

QString ApplicationDescription::pluginName() const
{
    return mPluginName;
}

bool ApplicationDescription::flickable() const
{
    return mFlickable;
}

bool ApplicationDescription::internetConnectivityRequired() const
{
    return mInternetConnectivityRequired;
}

QStringList ApplicationDescription::urlsAllowed() const
{
    return mUrlsAllowed;
}

QString ApplicationDescription::userAgent() const
{
    return mUserAgent;
}

bool ApplicationDescription::useLuneOSStyle() const
{
    return mUseLuneOSStyle;
}

}
