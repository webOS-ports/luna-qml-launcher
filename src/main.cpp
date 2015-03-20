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

#include "lunaqmlapplication.h"

int main(int argc, char **argv)
{
    /* Usage: luna-qml-launcher manifestPath parameters */

    QString appDataPath = "/media/cryptofs/app-data/";
    setenv("XDG_DATA_HOME", appDataPath.toUtf8().constData(), 1);

    QString appCachePath = "/media/cryptofs/app-cache/";
    setenv("XDG_CACHE_HOME", appCachePath.toUtf8().constData(), 1);

    LunaQmlApplication application(argc, argv);
    return application.launchApp();
}
