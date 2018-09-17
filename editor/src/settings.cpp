/* -*- Mode: C++; indent-tabs-mode: nil; tab-width: 4 -*-
 * -*- coding: utf-8 -*-
 *
 * Copyright (C) 2011 ~ 2018 Deepin, Inc.
 *               2011 ~ 2018 Wang Yong
 *
 * Author:     Wang Yong <wangyong@deepin.com>
 * Maintainer: Wang Yong <wangyong@deepin.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "settings.h"

#include "dthememanager.h"
#include <DSettings>
#include <DSettingsGroup>
#include <DSettingsOption>
#include <QStyleFactory>
#include <QFontDatabase>
#include <QApplication>
#include <QDebug>
#include <QDir>
#include <QStandardPaths>

Settings::Settings(QWidget *parent)
    : QObject(parent)
{
    m_configPath = QString("%1/%2/%3/config.conf")
        .arg(QStandardPaths::writableLocation(QStandardPaths::ConfigLocation))
        .arg(qApp->organizationName())
        .arg(qApp->applicationName());

    m_backend = new QSettingBackend(m_configPath);

    settings = DSettings::fromJsonFile(":/resource/settings.json");
    settings->setBackend(m_backend);

    auto fontSize = settings->option("base.font.size");
    connect(fontSize, &Dtk::Core::DSettingsOption::valueChanged, this, [=] (QVariant value) {
        emit adjustFontSize(value.toInt());
    });

    auto tabSpaceNumber = settings->option("advance.editor.tabspacenumber");
    connect(tabSpaceNumber, &Dtk::Core::DSettingsOption::valueChanged, this, [=](QVariant value) {
        emit adjustTabSpaceNumber(value.toInt());
    });

    QFontDatabase fontDatabase;
    auto fontFamliy = settings->option("base.font.family");
    QMap<QString, QVariant> fontDatas;

    QStringList values = fontDatabase.families();
    QStringList keys = values;
    fontDatas.insert("keys", keys);
    fontDatas.insert("values", values);
    fontFamliy->setData("items", fontDatas);

    if (fontFamliy->value().toString().isEmpty()) {
        fontFamliy->setValue(QFontDatabase::systemFont(QFontDatabase::FixedFont).family());
    }

    connect(fontFamliy, &Dtk::Core::DSettingsOption::valueChanged, this, [=] (QVariant value) {
        adjustFont(value.toString());
    });

    auto keymap = settings->option("shortcuts.keymap.keymap");
    QMap<QString, QVariant> keymapMap;
    keymapMap.insert("keys", QStringList() << "standard" << "emacs" << "customize");
    keymapMap.insert("values", QStringList() << tr("Standard") << "Emacs" << tr("Customize"));
    keymap->setData("items", keymapMap);

    connect(keymap, &Dtk::Core::DSettingsOption::valueChanged, this, [=] (QVariant value) {
        // Update all key's display value with user select keymap.
        updateAllKeysWithKeymap(value.toString());
    });

    auto windowState = settings->option("advance.window.windowstate");
    QMap<QString, QVariant> windowStateMap;
    windowStateMap.insert("keys", QStringList() << "window_normal" << "window_maximum" << "fullscreen");
    windowStateMap.insert("values", QStringList() << tr("Normal") << tr("Maximum") << tr("Fullscreen"));
    windowState->setData("items", windowStateMap);

    connect(settings, &Dtk::Core::DSettings::valueChanged, this, [=] (const QString &key, const QVariant &value) {
        // Change keymap to customize once user change any keyshortcut.
        if (!m_userChangeKey && key.startsWith("shortcuts.") && key != "shortcuts.keymap.keymap" && !key.contains("_keymap_")) {
            m_userChangeKey = true;

            QString currentKeymap = settings->option("shortcuts.keymap.keymap")->value().toString();

            QStringList keySplitList = key.split(".");
            keySplitList[1] = QString("%1_keymap_customize").arg(keySplitList[1]);
            QString customizeKey = keySplitList.join(".");

            // Just update customize key user input, don't change keymap.
            if (currentKeymap == "customize") {
                settings->option(customizeKey)->setValue(value);
            }
            // If current kemap is not "customize".
            // Copy all customize keys from current keymap, and then update customize key just user input.
            // Then change keymap name.
            else {
                copyCustomizeKeysFromKeymap(currentKeymap);
                settings->option(customizeKey)->setValue(value);
                keymap->setValue("customize");
            }

            m_userChangeKey = false;
        }
    });
}

Settings::~Settings()
{
}

// This function is workaround, it will remove after DTK fixed SettingDialog theme bug.
void Settings::dtkThemeWorkaround(QWidget *parent, const QString &theme)
{
    parent->setStyle(QStyleFactory::create(theme));

    for (auto obj : parent->children()) {
        auto w = qobject_cast<QWidget *>(obj);
        if (!w) {
            continue;
        }

        dtkThemeWorkaround(w, theme);
    }
}

void Settings::updateAllKeysWithKeymap(QString keymap)
{
    m_userChangeKey = true;

    for (auto option : settings->group("shortcuts.window")->options()) {
        QStringList keySplitList = option->key().split(".");
        keySplitList[1] = QString("%1_keymap_%2").arg(keySplitList[1]).arg(keymap);
        option->setValue(settings->option(keySplitList.join("."))->value().toString());
    }

    for (auto option : settings->group("shortcuts.editor")->options()) {
        QStringList keySplitList = option->key().split(".");
        keySplitList[1] = QString("%1_keymap_%2").arg(keySplitList[1]).arg(keymap);
        option->setValue(settings->option(keySplitList.join("."))->value().toString());
    }

    m_userChangeKey = false;
}

void Settings::copyCustomizeKeysFromKeymap(QString keymap)
{
    m_userChangeKey = true;

    for (auto option : settings->group("shortcuts.window_keymap_customize")->options()) {
        QStringList keySplitList = option->key().split(".");
        keySplitList[1] = QString("window_keymap_%1").arg(keymap);
        option->setValue(settings->option(keySplitList.join("."))->value().toString());
    }

    for (auto option : settings->group("shortcuts.editor_keymap_customize")->options()) {
        QStringList keySplitList = option->key().split(".");
        keySplitList[1] = QString("editor_keymap_%1").arg(keymap);
        option->setValue(settings->option(keySplitList.join("."))->value().toString());
    }

    m_userChangeKey = false;
}
