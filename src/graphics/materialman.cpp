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
 * The Infinity, Aurora, Odyssey, Eclipse and Lycium engines, Copyright (c) BioWare corp.
 * The Electron engine, Copyright (c) Obsidian Entertainment and BioWare corp.
 */

/** @file graphics/materialman.cpp
 *  A material manager.
 */


#include <OgreMaterialManager.h>
#include <OgreTechnique.h>
#include <OgrePass.h>

#include "common/ustring.h"
#include "common/error.h"
#include "common/threads.h"

#include "graphics/textureman.h"
#include "graphics/materialman.h"

#include "events/requests.h"

DECLARE_SINGLETON(Graphics::MaterialManager)

namespace Graphics {

MaterialDeclaration::MaterialDeclaration() {
	reset();
}

void MaterialDeclaration::reset() {
	ambient  [0] = 1.0; ambient  [1] = 1.0; ambient  [2] = 1.0;
	diffuse  [0] = 1.0; diffuse  [1] = 1.0; diffuse  [2] = 1.0; diffuse [3] = 1.0;
	specular [0] = 1.0; specular [1] = 1.0; specular [2] = 1.0; specular[3] = 1.0;
	selfIllum[0] = 0.0; selfIllum[1] = 0.0; selfIllum[2] = 0.0;

	shininess = 0.0;

	receiveShadows = true;
	writeColor     = true;
	writeDepth     = true;

	transparency = kTransparencyHintUnknown;

	textures.clear();
}

void MaterialDeclaration::trimTextures() {
	while (!textures.empty() && textures.back().empty())
		textures.pop_back();
}


MaterialManager::MaterialManager() {
}

MaterialManager::~MaterialManager() {
}

Ogre::MaterialPtr MaterialManager::get(const Common::UString &texture) {
	MaterialDeclaration decl;

	decl.textures.push_back(texture);

	return get(decl);
}

Ogre::MaterialPtr MaterialManager::get(const MaterialDeclaration &decl) {
	if (!Common::isMainThread()) {
		Ogre::MaterialPtr (MaterialManager::*f)(const MaterialDeclaration &) = &MaterialManager::get;

		Events::MainThreadFunctor<Ogre::MaterialPtr> functor(boost::bind(f, this, decl));

		return RequestMan.callInMainThread(functor);
	}

	Common::UString name = canonicalName(decl);

	Ogre::MaterialPtr material = Ogre::MaterialManager::getSingleton().getByName(name.c_str());
	if (!material.isNull())
		return material;

	material = Ogre::MaterialManager::getSingleton().create(name.c_str(), "General");

	create(decl, material);

	return material;
}

Ogre::MaterialPtr MaterialManager::getInvisible() {
	MaterialDeclaration decl;

	decl.receiveShadows = false;
	decl.writeColor     = false;
	decl.writeDepth     = false;

	return get(decl);
}

Ogre::MaterialPtr MaterialManager::getBlack() {
	MaterialDeclaration decl;

	decl.ambient  [0] = 0.0; decl.ambient  [1] = 0.0; decl.ambient  [2] = 0.0;
	decl.diffuse  [0] = 0.0; decl.diffuse  [1] = 0.0; decl.diffuse  [2] = 0.0; decl.diffuse [3] = 1.0;
	decl.specular [0] = 0.0; decl.specular [1] = 0.0; decl.specular [2] = 0.0; decl.specular[3] = 1.0;
	decl.selfIllum[0] = 0.0; decl.selfIllum[1] = 0.0; decl.selfIllum[2] = 0.0;

	decl.shininess = 0.0;

	decl.receiveShadows = false;

	return get(decl);
}

void MaterialManager::create(const MaterialDeclaration &decl, Ogre::MaterialPtr material) {
	material->getTechnique(0)->getPass(0)->setAmbient(decl.ambient[0], decl.ambient[1], decl.ambient[2]);
	material->getTechnique(0)->getPass(0)->setDiffuse(decl.diffuse[0], decl.diffuse[1], decl.diffuse[2], decl.diffuse[3]);
	material->getTechnique(0)->getPass(0)->setSpecular(decl.specular[0], decl.specular[1], decl.specular[2], decl.specular[3]);
	material->getTechnique(0)->getPass(0)->setShininess(decl.shininess);
	material->getTechnique(0)->getPass(0)->setSelfIllumination(decl.selfIllum[0], decl.selfIllum[1], decl.selfIllum[2]);

	bool transparent = true;
	for (uint t = 0; t < decl.textures.size(); t++) {
		Ogre::TexturePtr texture((Ogre::Texture *) 0);

		try {
			if (!decl.textures[t].empty())
				texture = TextureMan.get(decl.textures[t]);
		} catch (Common::Exception &e) {
			Common::printException(e, "WARNING: ");
		}

		if (texture.isNull())
			continue;

		Ogre::TextureUnitState *texState = material->getTechnique(0)->getPass(0)->createTextureUnitState();
		texState->setTextureCoordSet(t);

		texState->setTexture(texture);
		texState->setTextureAddressingMode(Ogre::TextureUnitState::TAM_WRAP);

		// DXT1 textures used in Aurora games are always opaque
		if (!texture->hasAlpha() || ((PixelFormat)texture->getFormat() == kPixelFormatDXT1))
			transparent = false;
	}

	// Even if the textures themselves aren't tranparent, the color might still be
	if (decl.diffuse[3] != 1.0)
		transparent = true;

	// Figure out whether this material is transparent.
	// If we don't get a hint from the declaration, try to infer
	// it from the texture and color information gathered above.
	TransparencyHint transparency = decl.transparency;
	if (transparency == kTransparencyHintUnknown)
		transparency = transparent ? kTransparencyHintTransparent : kTransparencyHintOpaque;

	if (transparency == kTransparencyHintTransparent) {
		material->getTechnique(0)->getPass(0)->setSceneBlending(Ogre::SBT_TRANSPARENT_ALPHA);
		material->getTechnique(0)->getPass(0)->setDepthWriteEnabled(false);
	} else {
		material->getTechnique(0)->getPass(0)->setSceneBlending(Ogre::SBT_REPLACE);
		material->getTechnique(0)->getPass(0)->setDepthWriteEnabled(decl.writeDepth);
	}

	material->getTechnique(0)->getPass(0)->setColourWriteEnabled(decl.writeColor);

	material->setReceiveShadows(decl.receiveShadows);
}

static Common::UString concat(const std::vector<Common::UString> &str) {
	Common::UString c;

	for (std::vector<Common::UString>::const_iterator t = str.begin(); t != str.end(); ++t)
		c += *t + "#";

	c += "@";

	return c;
}

static Common::UString concat(const float *f, uint n) {
	Common::UString c;

	for (uint i = 0; i < n; i++)
		c += Common::UString::sprintf("%6.3f#", f[i]);

	c += "@";

	return c;
}

static Common::UString concat(const bool *b, uint n) {
	Common::UString c;

	for (uint i = 0; i < n; i++)
		c += Common::UString::sprintf("%d#", (int) b[i]);

	c += "@";

	return c;
}

Common::UString MaterialManager::canonicalName(const MaterialDeclaration &decl) {
	return concat(decl.textures) +
	       concat(decl.ambient, 3) + concat(decl.diffuse, 4) + concat(decl.specular, 4) +
	       concat(decl.selfIllum, 3) + concat(&decl.shininess, 1) +
	       concat(&decl.receiveShadows, 1) + concat(&decl.writeColor, 1) + concat(&decl.writeDepth, 1);
}

} // End of namespace Graphics