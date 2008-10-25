/***************************************************************************
 *   Copyright (C) 2005-08 by the Quassel IRC Team                         *
 *   devel@quassel-irc.org                                                 *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#include <QVBoxLayout>

#include "notificationssettingspage.h"

#include "qtui.h"

NotificationsSettingsPage::NotificationsSettingsPage(QWidget *parent)
  : SettingsPage(tr("Behaviour"), tr("Notifications"), parent) {

  QVBoxLayout *layout = new QVBoxLayout(this);
  foreach(AbstractNotificationBackend *backend, QtUi::notificationBackends()) {
    SettingsPage *cw = backend->configWidget();
    if(cw) {
      _configWidgets.append(cw);
      layout->addWidget(cw);
      connect(cw, SIGNAL(changed(bool)), SLOT(widgetHasChanged()));
    }
  }
  layout->addStretch(1);
}

bool NotificationsSettingsPage::hasDefaults() const {
  return true;
}

void NotificationsSettingsPage::defaults() {
  foreach(SettingsPage *cw, _configWidgets)
    cw->defaults();
  widgetHasChanged();
}

void NotificationsSettingsPage::load() {
  foreach(SettingsPage *cw, _configWidgets)
    cw->load();
  setChangedState(false);
}

void NotificationsSettingsPage::save() {
  foreach(SettingsPage *cw, _configWidgets)
    cw->save();
  setChangedState(false);
}

void NotificationsSettingsPage::widgetHasChanged() {
  bool changed = false;
  foreach(SettingsPage *cw, _configWidgets) {
    if(cw->hasChanged()) {
      changed = true;
      break;
    }
  }
  if(changed != hasChanged()) setChangedState(changed);
}