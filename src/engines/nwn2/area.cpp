/* xoreos - A reimplementation of BioWare's Aurora engine
 *
 * xoreos is the legal property of its developers, whose names
 * can be found in the AUTHORS file distributed with this source
 * distribution.
 *
 * xoreos is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
 * of the License, or (at your option) any later version.
 *
 * xoreos is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with xoreos. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file engines/nwn2/area.cpp
 *  NWN2 area.
 */

#include "common/util.h"
#include "common/error.h"

#include "aurora/locstring.h"
#include "aurora/gfffile.h"
#include "aurora/2dafile.h"
#include "aurora/2dareg.h"

#include "sound/sound.h"

#include "engines/aurora/util.h"

#include "engines/nwn2/area.h"
#include "engines/nwn2/module.h"

namespace Engines {

namespace NWN2 {

Area::Area(Module &module, const Common::UString &resRef) : _module(&module), _loaded(false),
	_resRef(resRef), _visible(false) {

	// Load ARE and GIT

	Aurora::GFFFile are(_resRef, Aurora::kFileTypeARE, MKTAG('A', 'R', 'E', ' '));
	loadARE(are.getTopLevel());

	Aurora::GFFFile git(_resRef, Aurora::kFileTypeGIT, MKTAG('G', 'I', 'T', ' '));
	loadGIT(git.getTopLevel());

	_loaded = true;
}

Area::~Area() {
	hide();
}

Common::UString Area::getName(const Common::UString &resRef) {
	try {
		Aurora::GFFFile are(resRef, Aurora::kFileTypeARE, MKTAG('A', 'R', 'E', ' '));

		Aurora::LocString name;
		are.getTopLevel().getLocString("Name", name);

		Common::UString str = name.getString();
		if (!str.empty() && (*--str.end() == '\n'))
			str.erase(--str.end());

		return str;

	} catch (...) {
	}

	return "";
}

const Common::UString &Area::getResRef() {
	return _resRef;
}

const Common::UString &Area::getName() {
	return _name;
}

const Common::UString &Area::getDisplayName() {
	return _displayName;
}

uint32 Area::getMusicDayTrack() const {
	return _musicDayTrack;
}

uint32 Area::getMusicNightTrack() const {
	return _musicNightTrack;
}

uint32 Area::getMusicBattleTrack() const {
	return _musicBattleTrack;
}

void Area::setMusicDayTrack(uint32 track) {
	_musicDayTrack = track;
	_musicDay      = TwoDAReg.get("ambientmusic").getRow(track).getString("Resource");
}

void Area::setMusicNightTrack(uint32 track) {
	_musicNightTrack = track;
	_musicNight      = TwoDAReg.get("ambientmusic").getRow(track).getString("Resource");
}

void Area::setMusicBattleTrack(uint32 track) {
	_musicBattleTrack = track;

	if (_musicBattleTrack != Aurora::kStrRefInvalid) {
		const Aurora::TwoDAFile &ambientMusic = TwoDAReg.get("ambientmusic");

		// Normal battle music
		_musicBattle = ambientMusic.getRow(_musicBattleTrack).getString("Resource");

		// Battle stingers
		Common::UString stinger[3];
		stinger[0] = ambientMusic.getRow(_musicBattleTrack).getString("Stinger1");
		stinger[1] = ambientMusic.getRow(_musicBattleTrack).getString("Stinger2");
		stinger[2] = ambientMusic.getRow(_musicBattleTrack).getString("Stinger3");

		for (int i = 0; i < 3; i++)
			if (!stinger[i].empty())
				_musicBattleStinger.push_back(stinger[i]);
	}
}

void Area::stopSound() {
	stopAmbientMusic();
	stopAmbientSound();
}

void Area::stopAmbientMusic() {
	SoundMan.stopChannel(_ambientMusic);
}

void Area::stopAmbientSound() {
	SoundMan.stopChannel(_ambientSound);
}

void Area::playAmbientMusic(Common::UString music) {
	stopAmbientMusic();

	// TODO: Area::playAmbientMusic(): Day/Night
	if (music.empty())
		music = _musicDay;

	if (music.empty())
		return;

	_ambientMusic = playSound(music, Sound::kSoundTypeMusic, true);
}

void Area::playAmbientSound(Common::UString sound) {
	stopAmbientSound();

	// TODO: Area::playAmbientSound():  Day/Night
	if (sound.empty())
		sound = _ambientDay;

	if (sound.empty())
		return;

	_ambientSound = playSound(sound, Sound::kSoundTypeSFX, true, _ambientDayVol);
}

void Area::show() {
	if (_visible)
		return;

	// Play music and sound
	playAmbientSound();
	playAmbientMusic();

	_visible = true;
}

void Area::hide() {
	if (!_visible)
		return;

	stopSound();

	_visible = false;
}

void Area::loadARE(const Aurora::GFFStruct &are) {
	// Name

	Aurora::LocString name;
	are.getLocString("Name", name);

	_name = name.getString();
	if (!_name.empty() && (*--_name.end() == '\n'))
		_name.erase(--_name.end());

	_displayName = createDisplayName(_name);

	// Tiles

	_width  = are.getUint("Width");
	_height = are.getUint("Height");
}

void Area::loadGIT(const Aurora::GFFStruct &git) {
	// Generic properties
	if (git.hasField("AreaProperties"))
		loadProperties(git.getStruct("AreaProperties"));
}

void Area::loadProperties(const Aurora::GFFStruct &props) {
	// Ambient sound

	const Aurora::TwoDAFile &ambientSound = TwoDAReg.get("ambientsound");

	uint32 ambientDay   = props.getUint("AmbientSndDay"  , Aurora::kStrRefInvalid);
	uint32 ambientNight = props.getUint("AmbientSndNight", Aurora::kStrRefInvalid);

	_ambientDay   = ambientSound.getRow(ambientDay  ).getString("Resource");
	_ambientNight = ambientSound.getRow(ambientNight).getString("Resource");

	uint32 ambientDayVol   = CLIP<uint32>(props.getUint("AmbientSndDayVol"  , 127), 0, 127);
	uint32 ambientNightVol = CLIP<uint32>(props.getUint("AmbientSndNitVol", 127), 0, 127);

	_ambientDayVol   = 1.25 * (1.0 - (1.0 / powf(5.0, ambientDayVol   / 127.0)));
	_ambientNightVol = 1.25 * (1.0 - (1.0 / powf(5.0, ambientNightVol / 127.0)));

	// TODO: PresetInstance0 - PresetInstance7


	// Ambient music

	setMusicDayTrack  (props.getUint("MusicDay"   , Aurora::kStrRefInvalid));
	setMusicNightTrack(props.getUint("MusicNight" , Aurora::kStrRefInvalid));

	// Battle music

	setMusicBattleTrack(props.getUint("MusicBattle", Aurora::kStrRefInvalid));
}

// "{0011}Farlong Downstairs" -> "Farlong Downstairs"
Common::UString Area::createDisplayName(const Common::UString &name) {
	bool inBrace = false;

	Common::UString displayName;
	for (Common::UString::iterator it = name.begin(); it != name.end(); ++it) {
		if (*it == '{') {
			inBrace = true;
			continue;
		}

		if (*it == '}') {
			inBrace = false;
			continue;
		}

		if (!inBrace)
			displayName += *it;
	}

	return displayName;
}

} // End of namespace NWN2

} // End of namespace Engines