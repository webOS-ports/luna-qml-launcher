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

#include <json-c/json.h>

#include "applicationdescription.h"

namespace luna
{

ApplicationDescription::ApplicationDescription() : ApplicationDescriptionBase(),
    mTrustScope(ApplicationDescription::TrustScopeSystem)
{
}

ApplicationDescription::ApplicationDescription(const ApplicationDescription& other) :
    ApplicationDescriptionBase(other),
    mApplicationBasePath(other.basePath()),
    mTrustScope(other.trustScope()),
    mUseLuneOSStyle(other.useLuneOSStyle()),
    mUseWebEngine(other.useWebEngine())
{
}

ApplicationDescription::ApplicationDescription(const QString &data, const QString &applicationBasePath) :
    ApplicationDescriptionBase(),
    mApplicationBasePath(applicationBasePath),
    mTrustScope(ApplicationDescription::TrustScopeSystem),
    mUseLuneOSStyle(false),
    mUseWebEngine(false)
{
    initializeFromData(data);
}

ApplicationDescription::~ApplicationDescription()
{
}

void ApplicationDescription::initializeFromData(const QString &data)
{
    struct json_object* root = json_tokener_parse( data.toUtf8().constData() );
    if(!root)
    {
        qWarning() << "Failed to parse application description";
        return;
    }

    fromJsonObject(root);

	struct json_object* label = json_object_object_get(root,"useLuneOSStyle");
	if (label) {
		mUseLuneOSStyle = json_object_get_boolean(label);
	}

        label = json_object_object_get(root,"useWebEngine");
        if (label) {
                mUseWebEngine = json_object_get_boolean(label);
        }

    if(root)
        json_object_put(root);
}

QUrl ApplicationDescription::locateEntryPoint(const QString &entryPoint) const
{
    QUrl entryPointAsUrl(entryPoint);

    if (entryPointAsUrl.scheme() == "file" ||
        entryPointAsUrl.scheme() == "http" ||
        entryPointAsUrl.scheme() == "https")
        return entryPointAsUrl;

    if (entryPointAsUrl.scheme() != "") {
        qWarning("Entry point %s for application %s is invalid",
                 entryPoint.toUtf8().constData(),
                 getId().toUtf8().constData());
        return QUrl("");
    }

    return QUrl(QString("file://%1/%2").arg(mApplicationBasePath).arg(entryPoint));
}

bool ApplicationDescription::hasRemoteEntryPoint() const
{
    return getEntryPoint().scheme() == "http" ||
           getEntryPoint().scheme() == "https";
}

QString ApplicationDescription::getId() const
{
    return QString::fromStdString(id());
}

QString ApplicationDescription::getTitle() const
{
    return QString::fromStdString(title());
}

QUrl ApplicationDescription::getIcon() const
{
    QString iconPath = 	QString::fromStdString(icon());

    // we're only allow locally stored icons so we must prefix them with file:// to
    // store it in a QUrl object
    if (!iconPath.startsWith("file://"))
        iconPath.prepend("file://");

    QUrl lIcon(iconPath);
    if (lIcon.isEmpty() || !lIcon.isLocalFile() || !QFile::exists(lIcon.toLocalFile()))
        lIcon = QUrl("qrc:///qml/images/default-app-icon.png");

    return lIcon;
}

QUrl ApplicationDescription::getEntryPoint() const
{
    return locateEntryPoint(QString::fromStdString(entryPoint()));
}

ApplicationDescription::TrustScope ApplicationDescription::trustScope() const
{
    return mTrustScope;
}

QString ApplicationDescription::basePath() const
{
    return mApplicationBasePath;
}

QString ApplicationDescription::getPluginName() const
{
    return QString::fromStdString(pluginName());
}

QStringList ApplicationDescription::getUrlsAllowed() const
{
    QStringList mQStringList;

    std::list<std::string>::const_iterator constIterator;
    for (constIterator = urlsAllowed().begin(); constIterator != urlsAllowed().end(); ++constIterator) {
        mQStringList << QString::fromStdString(*constIterator);
    }

    return mQStringList;
}

QString ApplicationDescription::getUserAgent() const
{
    return QString::fromStdString(userAgent());
}

bool ApplicationDescription::useLuneOSStyle() const
{
    return mUseLuneOSStyle;
}

bool ApplicationDescription::useWebEngine() const
{
    return mUseWebEngine;
}

}
