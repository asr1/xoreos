/* xoreos - A reimplementation of BioWare's Aurora engine
 *
 * xoreos is the legal property of its developers, whose names can be
 * found in the AUTHORS file distributed with this source
 * distribution.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 *
 * The Infinity, Aurora, Odyssey and Eclipse engines, Copyright (c) BioWare corp.
 * The Electron engine, Copyright (c) Obsidian Entertainment and BioWare corp.
 */

/** @file engines/kotor/gui/options/gameplay.cpp
 *  The gameplay menu.
 */

#include "engines/aurora/widget.h"

#include "engines/kotor/gui/options/gameplay.h"
#include "engines/kotor/gui/options/mousesettings.h"
#include "engines/kotor/gui/options/keyboardconfig.h"

namespace Engines {

namespace KotOR {

OptionsGameplayMenu::OptionsGameplayMenu() {
	load("optgameplay");

	_mousesettings = new OptionsMouseSettingsMenu();
	_keyboardconfiguration = new OptionsKeyboardConfigurationMenu();

}
OptionsGameplayMenu::~OptionsGameplayMenu() {
	delete _mousesettings;
	delete _keyboardconfiguration;
}



void OptionsGameplayMenu::callbackActive(Widget &widget) {

	if (widget.getTag() == "BTN_MOUSE") {
		sub(*_mousesettings);
		return;
	}

	if (widget.getTag() == "BTN_KEYMAP") {
		sub(*_keyboardconfiguration);
		return;
	}

	if (widget.getTag() == "BTN_DEFAULT") {

	}

	if (widget.getTag() == "BTN_BACK") {
		_returnCode = 1;
		return;
	}
}

} // End of namespace KotOR

} // End of namespace Engines



