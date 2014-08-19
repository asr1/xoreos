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
 */

/** @file engines/kotor2/gui/main/main.h
 *  The KotOR 2 main menu.
 */

#ifndef ENGINES_KOTOR2_GUI_MAIN_MAIN_H
#define ENGINES_KOTOR2_GUI_MAIN_MAIN_H

#include "engines/kotor/gui/gui.h"

namespace Engines {

namespace KotOR2 {

class Module;

class MainMenu : public ::Engines::KotOR::GUI {
public:
	MainMenu(Module &module);
	~MainMenu();

protected:
	void initWidget(Widget &widget);

	void callbackActive(Widget &widget);

private:
	Module *_module;
};

} // End of namespace KotOR2

} // End of namespace Engines

#endif // ENGINES_KOTOR2_GUI_MAIN_MAIN_H
