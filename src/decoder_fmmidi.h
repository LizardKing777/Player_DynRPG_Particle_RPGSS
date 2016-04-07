/*
 * This file is part of EasyRPG Player.
 *
 * EasyRPG Player is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * EasyRPG Player is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with EasyRPG Player. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _EASYRPG_AUDIO_DECODER_FMMIDI_H_
#define _EASYRPG_AUDIO_DECODER_FMMIDI_H_

#ifdef HAVE_FMMIDI

// Headers
#include <string>
#include <functional>

#include <vector>
#include <memory>
#include "audio_decoder.h"
#include "midisequencer.h"
#include "midisynth.h"

class FmMidiDecoder : public AudioDecoder, midisequencer::output {
public:
	FmMidiDecoder();

	~FmMidiDecoder();

	// Audio Decoder interface
	bool Open(FILE* file) override;

	const std::vector<char>& Decode(int length) override;

	bool IsFinished() const override;

	std::string GetError() const override;

	void GetFormat(int& frequency, AudioDecoder::Format& format, AudioDecoder::Channel& channels) const override;

	bool SetFormat(int frequency, AudioDecoder::Format format, AudioDecoder::Channel channels) override;

	// midisequencer::output interface
protected:
	FILE* file;
	double mtime = 0.0;

	int synthesize(int_least16_t* output, std::size_t samples, float rate);
	void midi_message(int, uint_least32_t message) override;
	void sysex_message(int, const void* data, std::size_t size) override;
	void meta_event(int, const void*, std::size_t) override;
	void reset() override;

	std::unique_ptr<midisequencer::sequencer> seq;
	std::unique_ptr<midisynth::synthesizer> synth;
	std::unique_ptr<midisynth::fm_note_factory> note_factory;
	midisynth::DRUMPARAMETER p;
	void load_programs();
};

#endif

#endif
