#include <fstream>
#include <vector>
#include <cstdint>

using namespace std;

const int TPQN = 480;

// -------------------------
// write big-endian
// -------------------------
void writeBE(ofstream& f, uint32_t value, int bytes) {
    for (int i = bytes - 1; i >= 0; --i)
        f.put((value >> (i * 8)) & 0xFF);
}

// -------------------------
// MIDI event builder
// -------------------------
void writeVarLen(vector<uint8_t>& track, uint32_t value) {
    uint32_t buffer = value & 0x7F;

    while ((value >>= 7)) {
        buffer <<= 8;
        buffer |= ((value & 0x7F) | 0x80);
    }

    while (true) {
        track.push_back(buffer & 0xFF);
        if (buffer & 0x80)
            buffer >>= 8;
        else
            break;
    }
}

int main() {

    vector<uint8_t> track;

    // -------------------------
    // D7 chord notes (MIDI)
    // -------------------------
    vector<uint8_t> chord = {
        62, // D
        66, // F#
        69, // A
        72  // C
    };

    uint32_t startTick = 0;
    uint32_t duration  = TPQN; // 1 beat

    // tempo 120 BPM
    track.insert(track.end(),
        {0x00,0xFF,0x51,0x03,0x07,0xA1,0x20});

    // NOTE ON (chord)
    for (auto n : chord) {
        writeVarLen(track, startTick);
        track.push_back(0x90); // channel 1
        track.push_back(n);
        track.push_back(100);
        startTick = 0;
    }

    // NOTE OFF (after 1 beat)
    for (auto n : chord) {
        writeVarLen(track, duration);
        track.push_back(0x80);
        track.push_back(n);
        track.push_back(0);
        duration = 0;
    }

    // end of track
    track.insert(track.end(),
        {0x00,0xFF,0x2F,0x00});

    // -------------------------
    // write file
    // -------------------------
    ofstream midi("d7.mid", ios::binary);

    midi.write("MThd", 4);
    writeBE(midi, 6, 4);
    writeBE(midi, 0, 2);
    writeBE(midi, 1, 2);
    writeBE(midi, TPQN, 2);

    midi.write("MTrk", 4);
    writeBE(midi, (uint32_t)track.size(), 4);

    midi.write((char*)track.data(), track.size());

    midi.close();

    return 0;
}
