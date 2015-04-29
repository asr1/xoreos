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

/** @file
 *  Handling BioWare's TLK talk tables.
 */

#include "src/common/util.h"
#include "src/common/stream.h"
#include "src/common/file.h"
#include "src/common/encoding.h"
#include "src/common/error.h"

#include "src/aurora/talktable_tlk.h"
#include "src/aurora/talkman.h"

static const uint32 kTLKID    = MKTAG('T', 'L', 'K', ' ');
static const uint32 kVersion3 = MKTAG('V', '3', '.', '0');
static const uint32 kVersion4 = MKTAG('V', '4', '.', '0');

namespace Aurora {

TalkTable_TLK::TalkTable_TLK(Common::SeekableReadStream *tlk, uint32 languageID) :
	_tlk(tlk), _stringsOffset(0) {

	load();

	if (languageID != 0xFFFFFFFF)
		_languageID = languageID;
}

TalkTable_TLK::~TalkTable_TLK() {
	delete _tlk;
}

void TalkTable_TLK::load() {
	assert(_tlk);

	try {
		readHeader(*_tlk);

		if (_id != kTLKID)
			throw Common::Exception("Not a TLK file");

		if (_version != kVersion3 && _version != kVersion4)
			throw Common::Exception("Unsupported TLK file version %08X", _version);

		_languageID = _tlk->readUint32LE();

		uint32 stringCount = _tlk->readUint32LE();
		_entries.resize(stringCount);

		// V4 added this field; it's right after the header in V3
		uint32 tableOffset = 20;
		if (_version == kVersion4)
			tableOffset = _tlk->readUint32LE();

		_stringsOffset = _tlk->readUint32LE();

		// Go to the table
		_tlk->seek(tableOffset);

		// Read in all the table data
		if (_version == kVersion3)
			readEntryTableV3();
		else
			readEntryTableV4();

		if (_tlk->err())
			throw Common::Exception(Common::kReadError);

	} catch (Common::Exception &e) {
		delete _tlk;

		e.add("Failed reading TLK file");
		throw;
	}
}

void TalkTable_TLK::readEntryTableV3() {
	for (Entries::iterator entry = _entries.begin(); entry != _entries.end(); ++entry) {
		entry->flags          = _tlk->readUint32LE();
		entry->soundResRef    = Common::readStringFixed(*_tlk, Common::kEncodingASCII, 16);
		entry->volumeVariance = _tlk->readUint32LE();
		entry->pitchVariance  = _tlk->readUint32LE();
		entry->offset         = _tlk->readUint32LE() + _stringsOffset;
		entry->length         = _tlk->readUint32LE();
		entry->soundLength    = _tlk->readIEEEFloatLE();
	}
}

void TalkTable_TLK::readEntryTableV4() {
	for (Entries::iterator entry = _entries.begin(); entry != _entries.end(); ++entry) {
		entry->soundID = _tlk->readUint32LE();
		entry->offset  = _tlk->readUint32LE();
		entry->length  = _tlk->readUint16LE();
		entry->flags   = kFlagTextPresent;
	}
}

void TalkTable_TLK::readString(Entry &entry) const {
	if (!entry.text.empty() || (entry.length == 0) || !(entry.flags & kFlagTextPresent))
		// We already have the string
		return;

	assert(_tlk);

	_tlk->seek(entry.offset);

	uint32 length = MIN<uint32>(entry.length, _tlk->size() - _tlk->pos());
	if (length == 0)
		return;

	Common::MemoryReadStream *data   = _tlk->readStream(length);
	Common::MemoryReadStream *parsed = preParseColorCodes(*data);

	Common::Encoding encoding = TalkMan.getEncoding(_languageID);
	if (encoding != Common::kEncodingInvalid)
		entry.text = Common::readString(*parsed, encoding);
	else
		entry.text = "[???]";

	delete parsed;
	delete data;
}

uint32 TalkTable_TLK::getLanguageID() const {
	return _languageID;
}

bool TalkTable_TLK::hasEntry(uint32 strRef) const {
	return strRef < _entries.size();
}

static const Common::UString kEmptyString = "";
const Common::UString &TalkTable_TLK::getString(uint32 strRef) const {
	if (strRef >= _entries.size())
		return kEmptyString;

	readString(_entries[strRef]);

	return _entries[strRef].text;
}

const Common::UString &TalkTable_TLK::getSoundResRef(uint32 strRef) const {
	if (strRef >= _entries.size())
		return kEmptyString;

	return _entries[strRef].soundResRef;
}

uint32 TalkTable_TLK::getLanguageID(Common::SeekableReadStream &tlk) {
	uint32 id, version;
	bool utf16le;

	AuroraBase::readHeader(tlk, id, version, utf16le);

	if ((id != kTLKID) || ((version != kVersion3) && (version != kVersion4)))
		return 0xFFFFFFFF;

	return tlk.readUint32LE();
}

uint32 TalkTable_TLK::getLanguageID(const Common::UString &file) {
	Common::File tlk;
	if (!tlk.open(file))
		return 0xFFFFFFFF;

	return getLanguageID(tlk);
}

} // End of namespace Aurora
